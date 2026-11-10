#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include <libraries/xadmaster.h>
#include "VMARC.h"

/* ISO-8859-1 to IBM-037/1047 table */
static const unsigned char a2e_tab[256] = {
    0x00,0x01,0x02,0x03,0x37,0x2d,0x2e,0x2f,0x16,0x05,0x15,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x3c,0x3d,0x32,0x26,0x18,0x19,0x3f,0x27,0x1c,0x1d,0x1e,0x1f,
    0x40,0x5a,0x7f,0x7b,0x5b,0x6c,0x50,0x7d,0x4d,0x5d,0x5c,0x4e,0x6b,0x60,0x4b,0x61,
    0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0x7a,0x5e,0x4c,0x7e,0x6e,0x6f,
    0x7c,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,
    0xd7,0xd8,0xd9,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xad,0xe0,0xbd,0x5f,0x6d,
    0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
    0x97,0x98,0x99,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xc0,0x4f,0xd0,0xa1,0x07,
    0x20,0x21,0x22,0x23,0x24,0x25,0x06,0x17,0x28,0x29,0x2a,0x2b,0x2c,0x09,0x0a,0x1b,
    0x30,0x31,0x1a,0x33,0x34,0x35,0x36,0x08,0x38,0x39,0x3a,0x3b,0x04,0x14,0x3e,0xff,
    0x41,0xaa,0x4a,0xb1,0x9f,0xb2,0x6a,0xb5,0xbb,0xb4,0x9a,0x8a,0xb0,0xca,0xaf,0xbc,
    0x90,0x8f,0xea,0xfa,0xbe,0xa0,0xb6,0xb3,0x9d,0xda,0x9b,0x8b,0xb7,0xb8,0xb9,0xab,
    0x64,0x65,0x62,0x66,0x63,0x67,0x9e,0x68,0x74,0x71,0x72,0x73,0x78,0x75,0x76,0x77,
    0xac,0x69,0xed,0xee,0xeb,0xef,0xec,0xbf,0x80,0xfd,0xfe,0xfb,0xfc,0xba,0xae,0x59,
    0x44,0x45,0x42,0x46,0x43,0x47,0x9c,0x48,0x54,0x51,0x52,0x53,0x58,0x55,0x56,0x57,
    0x8c,0x49,0xcd,0xce,0xcb,0xcf,0xcc,0xe1,0x70,0xdd,0xde,0xdb,0xdc,0x8d,0x8e,0xdf
};

/* Convert decimal byte to BCD */
static unsigned char cvd(int bin) {
  bin &= 0xff;
  return ((bin / 10) << 4) | (bin % 10);
}

/* Mock XAD master runtime functions for test harness */
struct MockBuffer {
  unsigned char *data;
  size_t size;
  size_t capacity;
};

static struct MockBuffer out_data = { NULL, 0, 0 };
static unsigned char *in_arc_data = NULL;
static size_t in_arc_size = 0;

xadERROR xadHookAccess(struct xadMasterBase *xadMasterBase, xadINT32 command, xadSize size, xadPTR buffer, struct xadArchiveInfo *ai) {
  (void)xadMasterBase;
  if (command == XADAC_READ) {
    if (ai->xai_InPos + size > ai->xai_InSize) {
      size_t avail = (ai->xai_InSize > ai->xai_InPos) ? (ai->xai_InSize - ai->xai_InPos) : 0;
      if (avail > 0) {
        memcpy(buffer, in_arc_data + ai->xai_InPos, avail);
        ai->xai_InPos += avail;
      }
      return XADERR_INPUT;
    }
    memcpy(buffer, in_arc_data + ai->xai_InPos, size);
    ai->xai_InPos += size;
    return XADERR_OK;
  } else if (command == XADAC_INPUTSEEK) {
    ai->xai_InPos += size;
    return XADERR_OK;
  } else if (command == XADAC_WRITE) {
    if (out_data.size + size > out_data.capacity) {
      size_t new_cap = (out_data.capacity == 0) ? 4096 : out_data.capacity * 2;
      while (new_cap < out_data.size + size) new_cap *= 2;
      out_data.data = (unsigned char *)realloc(out_data.data, new_cap);
      out_data.capacity = new_cap;
    }
    memcpy(out_data.data + out_data.size, buffer, size);
    out_data.size += size;
    return XADERR_OK;
  }
  return XADERR_NOTSUPPORTED;
}

xadPTR xadAllocObject(struct xadMasterBase *xadMasterBase, xadUINT32 type, ...) {
  (void)xadMasterBase;
  if (type == XADOBJ_FILEINFO) {
    va_list ap;
    va_start(ap, type);
    xadTag tag;
    xadSize namesize = 32;
    xadSize privsize = 32;
    while ((tag = va_arg(ap, xadTag)) != TAG_DONE) {
      xadSize val = va_arg(ap, xadSize);
      if (tag == XAD_OBJNAMESIZE) namesize = val;
      if (tag == XAD_OBJPRIVINFOSIZE) privsize = val;
    }
    va_end(ap);

    struct xadFileInfo *fi = (struct xadFileInfo *)calloc(1, sizeof(struct xadFileInfo));
    fi->xfi_FileName = (char *)calloc(1, namesize + 1);
    fi->xfi_PrivateInfo = calloc(1, privsize);
    return fi;
  }
  return NULL;
}

xadPTR xadAllocObjectA(struct xadMasterBase *xadMasterBase, xadUINT32 type, xadTAGPTR tags) {
  (void)xadMasterBase; (void)type; (void)tags;
  return NULL;
}

void xadFreeObjectA(struct xadMasterBase *xadMasterBase, xadPTR object, xadTAGPTR tags) {
  (void)xadMasterBase; (void)tags;
  if (object) free(object);
}

xadPTR xadAllocVec(struct xadMasterBase *xadMasterBase, xadSize size, xadUINT32 flags) {
  (void)xadMasterBase; (void)flags;
  return calloc(1, size);
}

void xadCopyMem(struct xadMasterBase *xadMasterBase, const void *src, void *dest, xadSize size) {
  (void)xadMasterBase;
  memcpy(dest, src, size);
}

/* --------------------------------------------------------------------
 * Helpers to build test VMARC archives
 * -------------------------------------------------------------------- */
static void append_bytes(struct MockBuffer *mb, const void *data, size_t len) {
  if (mb->size + len > mb->capacity) {
    size_t new_cap = (mb->capacity == 0) ? 4096 : mb->capacity * 2;
    while (new_cap < mb->size + len) new_cap *= 2;
    mb->data = (unsigned char *)realloc(mb->data, new_cap);
    mb->capacity = new_cap;
  }
  memcpy(mb->data + mb->size, data, len);
  mb->size += len;
}

static void append_card_pad(struct MockBuffer *mb) {
  size_t rem = mb->size % 80;
  if (rem != 0) {
    size_t pad = 80 - rem;
    unsigned char zeroes[80] = { 0 };
    append_bytes(mb, zeroes, pad);
  }
}

static void pack_12bit_codes(struct MockBuffer *mb, const unsigned short *codes, int count) {
  unsigned int residual = 0xffffffff;
  for (int i = 0; i < count; i++) {
    unsigned short code = codes[i] & 0x0fff;
    if (residual == 0xffffffff) {
      residual = (unsigned int)(code & 0x0f);
      unsigned char b = (unsigned char)(code >> 4);
      append_bytes(mb, &b, 1);
    } else {
      unsigned char b1 = (unsigned char)((residual << 4) | (code >> 8));
      unsigned char b2 = (unsigned char)(code & 0xff);
      append_bytes(mb, &b1, 1);
      append_bytes(mb, &b2, 1);
      residual = 0xffffffff;
    }
  }
  if (residual != 0xffffffff) {
    unsigned char b = (unsigned char)(residual << 4);
    append_bytes(mb, &b, 1);
  }
}

static void create_test_vmarc_header(struct MockBuffer *mb,
                                     const char *fn, const char *ft, const char *fm,
                                     char recfm, int lrecl, unsigned char flags) {
  const unsigned char hid[8] = { 0x7a, 0xc3, 0xc6, 0xc6, 0x40, 0x40, 0x40, 0x40 };
  unsigned char hdr[30];
  memset(hdr, 0x40, sizeof(hdr));

  hdr[0] = 1; /* Ver */
  hdr[1] = 0; /* Rel */

  for (int i = 0; i < 8; i++) {
    hdr[2 + i] = (i < (int)strlen(fn)) ? a2e_tab[(unsigned char)fn[i]] : 0x40;
  }
  for (int i = 0; i < 8; i++) {
    hdr[10 + i] = (i < (int)strlen(ft)) ? a2e_tab[(unsigned char)ft[i]] : 0x40;
  }
  for (int i = 0; i < 2; i++) {
    hdr[18 + i] = (i < (int)strlen(fm)) ? a2e_tab[(unsigned char)fm[i]] : 0x40;
  }

  hdr[20] = (lrecl >> 8) & 0xff;
  hdr[21] = lrecl & 0xff;

  hdr[22] = cvd(26); /* Year 2026 */
  hdr[23] = cvd(11); /* Month 11 */
  hdr[24] = cvd(10); /* Day 10 */
  hdr[25] = cvd(14); /* Hour 14 */
  hdr[26] = cvd(30); /* Minute 30 */
  hdr[27] = cvd(45); /* Second 45 */

  hdr[28] = a2e_tab[(unsigned char)recfm];
  hdr[29] = flags | 0x20; /* HF_Y2K */

  append_bytes(mb, hid, 8);
  append_bytes(mb, hdr, 30);
}

static void create_test_vmarc_asis(struct MockBuffer *mb,
                                   const char *fn, const char *ft, const char *fm,
                                   char recfm, int lrecl,
                                   const char **lines, int num_lines) {
  create_test_vmarc_header(mb, fn, ft, fm, recfm, lrecl, 0x40 /* HF_ASIS */);

  for (int l = 0; l < num_lines; l++) {
    size_t line_len = strlen(lines[l]);
    size_t rec_len = (recfm == 'F') ? (size_t)lrecl : line_len;
    unsigned char len_bytes[2];
    len_bytes[0] = (rec_len >> 8) & 0xff;
    len_bytes[1] = rec_len & 0xff;
    append_bytes(mb, len_bytes, 2);

    for (size_t c = 0; c < rec_len; c++) {
      unsigned char eb = (c < line_len) ? a2e_tab[(unsigned char)lines[l][c]] : 0x40;
      append_bytes(mb, &eb, 1);
    }
  }

  unsigned char eof_len[2] = { 0, 0 };
  append_bytes(mb, eof_len, 2);
  append_card_pad(mb);
}

static void create_test_vmarc_lzw(struct MockBuffer *mb,
                                  const char *fn, const char *ft, const char *fm,
                                  char recfm, int lrecl,
                                  const char *line) {
  create_test_vmarc_header(mb, fn, ft, fm, recfm, lrecl, 0x00 /* LZW */);

  /* Build 12-bit code sequence: char codes, EOR (0), EOF (0) */
  size_t len = strlen(line);
  unsigned short codes[256];
  int count = 0;
  for (size_t i = 0; i < len; i++) {
    unsigned char eb = a2e_tab[(unsigned char)line[i]];
    codes[count++] = (unsigned short)(eb + 1);
  }
  codes[count++] = 0; /* EOR */
  codes[count++] = 0; /* EOF */

  pack_12bit_codes(mb, codes, count);
  append_card_pad(mb);
}

static void create_test_vmarc_s2(struct MockBuffer *mb,
                                 const char *fn, const char *ft, const char *fm,
                                 char recfm, int lrecl,
                                 const char *line) {
  create_test_vmarc_header(mb, fn, ft, fm, recfm, lrecl, 0x80 /* HF_S2 */);

  size_t len = strlen(line);
  unsigned short codes[256];
  int count = 0;
  for (size_t i = 0; i < len; i++) {
    unsigned char eb = a2e_tab[(unsigned char)line[i]];
    codes[count++] = (unsigned short)(eb + 1);
  }
  codes[count++] = 0; /* EOR */
  codes[count++] = 0; /* EOF */

  pack_12bit_codes(mb, codes, count);
  append_card_pad(mb);
}

/* --------------------------------------------------------------------
 * Main Test Routine
 * -------------------------------------------------------------------- */
int main(void) {
  printf("Starting VMARC XAD Client Test Suite...\n");

  /* Test 1: Recognition with valid and invalid data */
  {
    const unsigned char valid_sig[8] = { 0x7a, 0xc3, 0xc6, 0xc6, 0x40, 0x40, 0x40, 0x40 };
    const unsigned char invalid_sig[8] = "PK\x03\x04\x14\x00\x00\x00";

    assert(FirstClient.xc_RecogData(8, valid_sig, NULL) == 1);
    assert(FirstClient.xc_RecogData(8, invalid_sig, NULL) == 0);
    assert(FirstClient.xc_RecogData(4, valid_sig, NULL) == 0);
    printf("Test 1: Recognition test passed!\n");
  }

  /* Test 2: ASIS archives with variable and fixed records + Latin-1 text */
  {
    struct MockBuffer arc = { NULL, 0, 0 };
    const char *lines1[] = {
      "/* REXX SAMPLE SCRIPT */",
      "SAY 'HELLO FROM IBM CMS VMARC!'",
      "EXIT 0"
    };
    const char *lines2[] = {
      "TITLE: SPECIAL CHARACTERS (LATIN-1 TEST)",
      "CAFÉ ET CRÈME BRÛLÉE",
      "DEUTSCHE GRÜSSE ÜBERALL"
    };

    create_test_vmarc_asis(&arc, "test", "exec", "a1", 'V', 80, lines1, 3);
    create_test_vmarc_asis(&arc, "sample", "text", "b2", 'F', 40, lines2, 3);

    in_arc_data = arc.data;
    in_arc_size = arc.size;

    struct xadArchiveInfo ai;
    memset(&ai, 0, sizeof(ai));
    ai.xai_Client = (struct xadClient *)&FirstClient;
    ai.xai_InSize = in_arc_size;

    xadERROR err = FirstClient.xc_GetInfo(&ai, NULL);
    assert(err == XADERR_OK);
    assert(ai.xai_FileInfo != NULL);

    struct xadFileInfo *fi1 = ai.xai_FileInfo;
    assert(fi1->xfi_EntryNumber == 1);
    assert(strcmp(fi1->xfi_FileName, "TEST.EXEC.A1") == 0);
    assert(fi1->xfi_Date.xd_Year == 2026);
    assert(fi1->xfi_Date.xd_Month == 11);
    assert(fi1->xfi_Date.xd_Day == 10);
    assert(fi1->xfi_Date.xd_Hour == 14);
    assert(fi1->xfi_Date.xd_Minute == 30);
    assert(fi1->xfi_Date.xd_Second == 45);

    ai.xai_CurFile = fi1;
    ai.xai_InPos = fi1->xfi_DataPos;
    out_data.size = 0;
    err = FirstClient.xc_UnArchive(&ai, NULL);
    assert(err == XADERR_OK);

    char *unpacked1 = (char *)calloc(1, out_data.size + 1);
    memcpy(unpacked1, out_data.data, out_data.size);
    printf("Unpacked 1 (%zu bytes):\n%s\n", out_data.size, unpacked1);
    assert(strstr(unpacked1, "SAY 'HELLO FROM IBM CMS VMARC!'") != NULL);
    free(unpacked1);

    struct xadFileInfo *fi2 = fi1->xfi_Next;
    assert(fi2 != NULL);
    assert(fi2->xfi_EntryNumber == 2);
    assert(strcmp(fi2->xfi_FileName, "SAMPLE.TEXT.B2") == 0);

    ai.xai_CurFile = fi2;
    ai.xai_InPos = fi2->xfi_DataPos;
    out_data.size = 0;
    err = FirstClient.xc_UnArchive(&ai, NULL);
    assert(err == XADERR_OK);

    char *unpacked2 = (char *)calloc(1, out_data.size + 1);
    memcpy(unpacked2, out_data.data, out_data.size);
    printf("Unpacked 2 Latin-1 (%zu bytes):\n%s\n", out_data.size, unpacked2);
    assert(strstr(unpacked2, "CAFÉ ET CRÈME BRÛLÉE") != NULL);
    assert(strstr(unpacked2, "DEUTSCHE GRÜSSE ÜBERALL") != NULL);
    free(unpacked2);

    FirstClient.xc_Free(&ai, NULL);
    free(arc.data);
    free(out_data.data);
    out_data.data = NULL;
    out_data.size = 0;
    out_data.capacity = 0;
    free(fi1->xfi_FileName);
    free(fi1->xfi_PrivateInfo);
    free(fi1);
    free(fi2->xfi_FileName);
    free(fi2->xfi_PrivateInfo);
    free(fi2);
    printf("Test 2: ASIS archive extraction passed!\n");
  }

  /* Test 3: LZW compressed subfile */
  {
    struct MockBuffer arc = { NULL, 0, 0 };
    create_test_vmarc_lzw(&arc, "lzwtest", "file", "a1", 'V', 80, "HELLO FROM VMARC LZW DECOMPRESSOR!");

    in_arc_data = arc.data;
    in_arc_size = arc.size;

    struct xadArchiveInfo ai;
    memset(&ai, 0, sizeof(ai));
    ai.xai_Client = (struct xadClient *)&FirstClient;
    ai.xai_InSize = in_arc_size;

    xadERROR err = FirstClient.xc_GetInfo(&ai, NULL);
    assert(err == XADERR_OK);
    assert(ai.xai_FileInfo != NULL);
    assert(strcmp(ai.xai_FileInfo->xfi_FileName, "LZWTEST.FILE.A1") == 0);

    ai.xai_CurFile = ai.xai_FileInfo;
    ai.xai_InPos = ai.xai_FileInfo->xfi_DataPos;
    out_data.size = 0;
    err = FirstClient.xc_UnArchive(&ai, NULL);
    assert(err == XADERR_OK);

    char *unpacked_lzw = (char *)calloc(1, out_data.size + 1);
    memcpy(unpacked_lzw, out_data.data, out_data.size);
    printf("Unpacked LZW (%zu bytes): %s\n", out_data.size, unpacked_lzw);
    assert(strstr(unpacked_lzw, "HELLO FROM VMARC LZW DECOMPRESSOR!") != NULL);
    free(unpacked_lzw);

    FirstClient.xc_Free(&ai, NULL);
    free(arc.data);
    free(out_data.data);
    out_data.data = NULL;
    out_data.size = 0;
    out_data.capacity = 0;
    out_data.data = NULL;
    out_data.size = 0;
    out_data.capacity = 0;
    free(ai.xai_FileInfo->xfi_FileName);
    free(ai.xai_FileInfo->xfi_PrivateInfo);
    free(ai.xai_FileInfo);
    printf("Test 3: LZW decompression test passed!\n");
  }

  /* Test 4: S2 compressed subfile */
  {
    struct MockBuffer arc = { NULL, 0, 0 };
    create_test_vmarc_s2(&arc, "s2test", "c", "m1", 'V', 80, "HELLO FROM VMARC S2 DECOMPRESSOR!");

    in_arc_data = arc.data;
    in_arc_size = arc.size;

    struct xadArchiveInfo ai;
    memset(&ai, 0, sizeof(ai));
    ai.xai_Client = (struct xadClient *)&FirstClient;
    ai.xai_InSize = in_arc_size;

    xadERROR err = FirstClient.xc_GetInfo(&ai, NULL);
    assert(err == XADERR_OK);
    assert(ai.xai_FileInfo != NULL);
    assert(strcmp(ai.xai_FileInfo->xfi_FileName, "S2TEST.C.M1") == 0);

    ai.xai_CurFile = ai.xai_FileInfo;
    ai.xai_InPos = ai.xai_FileInfo->xfi_DataPos;
    out_data.size = 0;
    err = FirstClient.xc_UnArchive(&ai, NULL);
    assert(err == XADERR_OK);

    char *unpacked_s2 = (char *)calloc(1, out_data.size + 1);
    memcpy(unpacked_s2, out_data.data, out_data.size);
    printf("Unpacked S2 (%zu bytes): %s\n", out_data.size, unpacked_s2);
    assert(strstr(unpacked_s2, "HELLO FROM VMARC S2 DECOMPRESSOR!") != NULL);
    free(unpacked_s2);

    FirstClient.xc_Free(&ai, NULL);
    free(arc.data);
    free(out_data.data);
    free(ai.xai_FileInfo->xfi_FileName);
    free(ai.xai_FileInfo->xfi_PrivateInfo);
    free(ai.xai_FileInfo);
    printf("Test 4: S2 decompression test passed!\n");
  }

  printf("\nALL VMARC XAD CLIENT TESTS PASSED SUCCESSFULLY!\n");
  return 0;
}

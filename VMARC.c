#ifndef XADMASTER_VMARC_C
#define XADMASTER_VMARC_C

/*
 *  VMARC client for xadmaster.library / libxad
 *  Ready for MorphOS, AmigaOS, and portable environments.
 *  Handles IBM VM/CMS VMARC archives (ASIS, LZW, S2 compression).
 *  Filenames formatted uppercase: FN.FT.FM.
 *  Text translated from EBCDIC to ISO-8859-1 (Latin-1).
 */

#include "xadClient.h"
#include "VMARC.h"
#include <string.h>
#include <ctype.h>
#include <limits.h>

#ifndef XADMASTERVERSION
  #define XADMASTERVERSION 11
#endif

#define VMARC_VERSION   1
#define VMARC_REVISION  0

XADCLIENTVERSTR("VMARC 1.0 (2026)")

/* VMARC Header ID in EBCDIC: ":CFF    " */
static const xadUINT8 vmarc_hid[8] = { 0x7a, 0xc3, 0xc6, 0xc6, 0x40, 0x40, 0x40, 0x40 };

/* EBCDIC (IBM-037 / IBM-1047) to ISO-8859-1 (Latin-1) conversion table */
static const xadUINT8 e2a_tab[256] = {
    0x00, 0x01, 0x02, 0x03, 0x9c, 0x09, 0x86, 0x7f, 0x97, 0x8d, 0x8e, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x9d, 0x85, 0x08, 0x87, 0x18, 0x19, 0x92, 0x8f, 0x1c, 0x1d, 0x1e, 0x1f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x0a, 0x17, 0x1b, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x05, 0x06, 0x07,
    0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04, 0x98, 0x99, 0x9a, 0x9b, 0x14, 0x15, 0x9e, 0x1a,
    0x20, 0xa0, 0xe2, 0xe4, 0xe0, 0xe1, 0xe3, 0xe5, 0xe7, 0xf1, 0xa2, 0x2e, 0x3c, 0x28, 0x2b, 0x7c,
    0x26, 0xe9, 0xea, 0xeb, 0xe8, 0xed, 0xee, 0xef, 0xec, 0xdf, 0x21, 0x24, 0x2a, 0x29, 0x3b, 0x5e,
    0x2d, 0x2f, 0xc2, 0xc4, 0xc0, 0xc1, 0xc3, 0xc5, 0xc7, 0xd1, 0xa6, 0x2c, 0x25, 0x5f, 0x3e, 0x3f,
    0xf8, 0xc9, 0xca, 0xcb, 0xc8, 0xcd, 0xce, 0xcf, 0xcc, 0x60, 0x3a, 0x23, 0x40, 0x27, 0x3d, 0x22,
    0xd8, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0xab, 0xbb, 0xf0, 0xfd, 0xfe, 0xb1,
    0xb0, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0xaa, 0xba, 0xe6, 0xb8, 0xc6, 0xa4,
    0xb5, 0x7e, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0xa1, 0xbf, 0xd0, 0x5b, 0xde, 0xae,
    0xac, 0xa3, 0xa5, 0xb7, 0xa9, 0xa7, 0xb6, 0xbc, 0xbd, 0xbe, 0xdd, 0xa8, 0xaf, 0x5d, 0xb4, 0xd7,
    0x7b, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0xad, 0xf4, 0xf6, 0xf2, 0xf3, 0xf5,
    0x7d, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0xb9, 0xfb, 0xfc, 0xf9, 0xfa, 0xff,
    0x5c, 0xf7, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xb2, 0xd4, 0xd6, 0xd2, 0xd3, 0xd5,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0xb3, 0xdb, 0xdc, 0xd9, 0xda, 0x9f
};

/* VMARC File Header Field Offsets (after 8-byte HID) */
#define H_VER       0   /* 1 byte */
#define H_REL       1   /* 1 byte */
#define H_FN        2   /* 8 bytes */
#define H_FT        10  /* 8 bytes */
#define H_FM        18  /* 2 bytes */
#define H_LRECL     20  /* 2 bytes */
#define H_YEAR      22  /* 1 byte BCD */
#define H_MONTH     23  /* 1 byte BCD */
#define H_DAY       24  /* 1 byte BCD */
#define H_HOUR      25  /* 1 byte BCD */
#define H_MINUTE    26  /* 1 byte BCD */
#define H_SECOND    27  /* 1 byte BCD */
#define H_RECFM     28  /* 1 byte EBCDIC ('F' or 'V') */
#define H_FLAGS     29  /* 1 byte */
#define H_DLEN      30  /* Basic header length */
#define H_XRECL     30  /* 4 bytes (extended header) */
#define H_RSVD      34  /* 8 bytes (extended header) */
#define H_XDLEN     42  /* Total header length with extended header */

#define HF_S2       0x80
#define HF_ASIS     0x40
#define HF_Y2K      0x20
#define HF_EXTH     0x01

/* LZW and S2 Constants */
#define kodendr     0
#define kodebcd     1
#define kodmax      (kodebcd + 255)
#define TABSIZE     4096
#define HASHSIZE    5003

#define ENDCHAIN    ((struct lzwdsect *) -1)

typedef struct lzwdsect {
  struct lzwdsect *right;
  struct lzwdsect *left;
  struct lzwdsect *pred;
  xadUINT16       refcnt;
  xadUINT16       schar;
} LZWSTRING;

typedef struct lzwhash {
  LZWSTRING *head;
  LZWSTRING *tail;
} LZWHASH;

typedef struct strdsect {
  struct strdsect *strleft;
  struct strdsect *strright;
  struct strdsect *strsiblg;
  struct strdsect *stroffsp;
  xadUINT16       strlen;
  xadUINT16       strcount;
  xadUINT16       strchar;
  xadUINT8        visited;
  xadUINT8        align[9];
} STRDSECT;

/* Private data structure associated with xadFileInfo */
struct VMARCPrivFile {
  xadUINT8  vpf_Flags;
  xadUINT8  vpf_Recfm;
  xadUINT16 vpf_Lrecl;
  xadSize   vpf_DataPos;
};

/* State structure for reading/decompressing */
#define BUFLEN 65536
#define OUTBUFLEN 65536

struct VMARCState {
  struct xadArchiveInfo *ai;
  struct xadMasterBase  *xmb;

  xadUINT8  ibuf[BUFLEN];
  xadUINT32 ibufp;
  xadUINT32 icnt;
  xadSize   ipos;
  xadUINT32 residual;

  xadUINT8  recfm;
  xadUINT32 lrecl;
  xadUINT32 recbytes;
  xadUINT32 eor;
  xadUINT8  is_text;

  xadUINT8  obuf[OUTBUFLEN];
  xadUINT32 obufp;

  /* LZW tables */
  LZWSTRING lzwstrtab[TABSIZE];
  LZWHASH   lzwhashtab[HASHSIZE];
  LZWSTRING *lzwtabs;
  LZWSTRING *lzwtabp;
  LZWSTRING *lzwtabl;

  /* S2 tables */
  STRDSECT  s2strtab[TABSIZE + 1];
  STRDSECT  *s2tabs;
  STRDSECT  *s2tabp;
  STRDSECT  *s2tabl;
};

/* Convert BCD byte to integer */
static xadINT32 cvb(xadUINT8 dec) {
  return ((dec >> 4) * 10) + (dec & 0x0f);
}

/* Flush output buffer */
static xadERROR flush_output(struct VMARCState *s) {
  if (s->obufp > 0) {
    struct xadMasterBase *xadMasterBase = s->xmb;
    xadERROR err = xadHookAccess(XADM XADAC_WRITE, s->obufp, s->obuf, s->ai);
    if (err) return err;
    s->obufp = 0;
  }
  return XADERR_OK;
}

/* Put single byte to output stream buffer */
static xadBOOL put_out_byte(struct VMARCState *s, xadUINT8 b) {
  s->obuf[s->obufp++] = b;
  if (s->obufp >= OUTBUFLEN) {
    if (flush_output(s) != XADERR_OK) {
      return XADFALSE;
    }
  }
  return XADTRUE;
}

/* Read single byte from input stream */
static xadINT32 get_input_byte(struct VMARCState *s) {
  if (s->icnt == 0) {
    struct xadMasterBase *xadMasterBase = s->xmb;
    xadSize remain = (s->ai->xai_InSize > s->ai->xai_InPos) ? (s->ai->xai_InSize - s->ai->xai_InPos) : 0;
    xadUINT32 toread = (remain > BUFLEN) ? BUFLEN : (xadUINT32)remain;
    if (toread == 0) {
      return -1;
    }
    s->ipos = s->ai->xai_InPos;
    if (xadHookAccess(XADM XADAC_READ, toread, s->ibuf, s->ai) != XADERR_OK) {
      return -1;
    }
    s->icnt = toread;
    s->ibufp = 0;
  }
  s->icnt--;
  return s->ibuf[s->ibufp++];
}

/* Read 12-bit code from input */
static xadUINT16 get_code12(struct VMARCState *s) {
  xadINT32 b1;
  xadUINT32 code;

  b1 = get_input_byte(s);
  if (b1 < 0) return 0xffff;

  if (s->residual != 0xffffffff) {
    code = (xadUINT32)b1 | (s->residual << 8);
    s->residual = 0xffffffff;
  } else {
    xadINT32 b2 = get_input_byte(s);
    if (b2 < 0) return 0xffff;
    s->residual = (xadUINT32)b2;
    code = (((xadUINT32)b1) << 4) | (s->residual >> 4);
  }
  return (xadUINT16)(code & 0x0fff);
}

/* Process and output one decoded character / EOR symbol */
static xadINT32 put_decoded_byte(struct VMARCState *s, xadUINT16 c) {
  xadINT32 code = (xadINT32)c - kodebcd;

  if (code < 0) {
    /* End of record code */
    s->eor++;
    if (s->eor > 1 || s->recfm == 'F') {
      return 0; /* EOF */
    }
    if (s->is_text) {
      if (!put_out_byte(s, '\n')) return -1;
    }
    s->recbytes = 0;
    return 1;
  }

  s->eor = 0;

  if (s->recfm == 'F') {
    if (s->lrecl > 0 && s->recbytes == s->lrecl) {
      s->recbytes = 0;
      if (s->is_text) {
        if (!put_out_byte(s, '\n')) return -1;
      }
    }
    s->recbytes++;
  }

  if (s->is_text) {
    code = e2a_tab[code & 0xff];
  }

  if (!put_out_byte(s, (xadUINT8)code)) {
    return -1;
  }
  return 1;
}

/* LZW Support Functions */
static void lzw_lookup(struct VMARCState *s, LZWSTRING *lastpred, xadUINT16 nextchar, LZWSTRING **ent) {
  xadUINT32 key = (xadUINT32)(((uintptr_t)lastpred << 6) ^ nextchar) % HASHSIZE;
  LZWHASH *hash = &s->lzwhashtab[key];
  LZWSTRING *origin = (LZWSTRING *)hash;
  LZWSTRING *probe;
  LZWSTRING *left, *right;

  for (probe = hash->head; probe != origin; probe = probe->right) {
    if (probe->pred == lastpred && probe->schar == nextchar) {
      *ent = probe;
      return;
    }
  }

  do {
    s->lzwtabp++;
    if (s->lzwtabp > s->lzwtabl) {
      s->lzwtabp = s->lzwtabs;
    }
  } while (s->lzwtabp->refcnt != 0);

  if (s->lzwtabp->right != 0) {
    right = s->lzwtabp->right;
    left = s->lzwtabp->left;
    right->left = left;
    left->right = right;
    if (s->lzwtabp->pred != 0 && s->lzwtabp->pred != ENDCHAIN) {
      s->lzwtabp->pred->refcnt--;
    }
  }

  s->lzwtabp->pred = lastpred;
  s->lzwtabp->schar = nextchar;
  s->lzwtabp->refcnt = 0;

  right = origin->right;
  s->lzwtabp->right = right;
  s->lzwtabp->left = origin;
  origin->right = s->lzwtabp;
  right->left = s->lzwtabp;

  *ent = s->lzwtabp;
}

static void lzw_init(struct VMARCState *s) {
  xadUINT32 ndx;
  LZWSTRING *ent;

  memset(s->lzwstrtab, 0, sizeof(s->lzwstrtab));
  for (ndx = 0; ndx < HASHSIZE; ndx++) {
    s->lzwhashtab[ndx].head = (LZWSTRING *)&s->lzwhashtab[ndx];
    s->lzwhashtab[ndx].tail = (LZWSTRING *)&s->lzwhashtab[ndx];
  }

  s->lzwtabp = &s->lzwstrtab[0] - 1;
  s->lzwtabl = &s->lzwstrtab[TABSIZE - 1];

  for (ndx = 0; ndx < (xadUINT32)(kodmax + 1); ndx++) {
    lzw_lookup(s, ENDCHAIN, (xadUINT16)ndx, &ent);
    ent->refcnt = 1;
  }
  s->lzwtabs = ent + 1;
}

static xadERROR decompress_lzw(struct VMARCState *s) {
  LZWSTRING *prev;
  LZWSTRING *last;
  LZWSTRING *ent;
  LZWSTRING *tabptr;
  xadINT32 wrapped;
  xadUINT16 code;
  xadINT32 rc;

  lzw_init(s);
  tabptr = s->lzwtabs;
  tabptr->pred = ENDCHAIN;

  while (1) {
    code = get_code12(s);
    if (code == 0xffff) return XADERR_ILLEGALDATA;

    ent = &s->lzwstrtab[code];
    if (ent->pred == NULL) return XADERR_ILLEGALDATA;

    prev = ENDCHAIN;
    do {
      last = ent->pred;
      ent->pred = prev;
      prev = ent;
      ent = last;
    } while (ent != ENDCHAIN);

    tabptr->schar = prev->schar;

    do {
      rc = put_decoded_byte(s, prev->schar);
      if (rc == 0) return XADERR_OK;
      if (rc < 0) return XADERR_OUTPUT;

      last = prev->pred;
      prev->pred = ent;
      ent = prev;
      prev = last;
    } while (prev != ENDCHAIN);

    ent->refcnt++;
    wrapped = 0;
    tabptr = s->lzwtabp;
    do {
      tabptr++;
      if (tabptr > s->lzwtabl) {
        if (wrapped) break;
        wrapped = 1;
        tabptr = s->lzwtabs;
      }
    } while (tabptr->refcnt != 0);

    if (tabptr > s->lzwtabl) {
      ent->refcnt--;
      continue;
    }

    if (tabptr->pred != ENDCHAIN && tabptr->pred != NULL) {
      tabptr->pred->refcnt--;
    }
    tabptr->pred = ent;
    s->lzwtabp = tabptr;
  }
}

/* S2 Support Functions */
static void s2_addnew(struct VMARCState *s, xadUINT16 slastcode, xadUINT16 lastcode) {
  STRDSECT *left = &s->s2strtab[slastcode];
  STRDSECT *right = &s->s2strtab[lastcode];
  STRDSECT *probe;
  STRDSECT *ostr;
  STRDSECT **psib;
  STRDSECT **opsib;

  if ((left->strlen + right->strlen) > (TABSIZE - 2)) return;

  left->strcount++;
  right->strcount++;

  probe = s->s2tabp;
  do {
    probe++;
    if (probe > s->s2tabl) {
      probe = s->s2tabs;
    }
  } while (probe->strcount != 0);

  ostr = probe->strleft;
  if (ostr) {
    ostr->strcount--;
    psib = &ostr->stroffsp;
    while (*psib != probe) {
      psib = &(*psib)->strsiblg;
    }
    *psib = probe->strsiblg;
    ostr = probe->strright;
    ostr->strcount--;
  }

  if (right == s->s2tabp) {
    if (left == right->strleft) {
      ostr = right;
      right = left;
      left = ostr;
    }
  }

  probe->strleft = left;
  probe->strright = right;
  probe->strlen = 2 + left->strlen + right->strlen;

  psib = &left->stroffsp;
  do {
    opsib = psib;
    left = *psib;
    psib = &left->strsiblg;
  } while (left && (left != s->s2tabp));

  probe->strsiblg = left;
  *opsib = probe;
  s->s2tabp = probe;
}

static void s2_init(struct VMARCState *s) {
  xadUINT16 ndx;

  memset(s->s2strtab, 0, sizeof(s->s2strtab));
  s->s2tabs = &s->s2strtab[kodmax + 1];
  s->s2tabl = &s->s2strtab[TABSIZE];
  s->s2tabp = s->s2tabl;

  for (ndx = 0; ndx < (kodmax + 1); ndx++) {
    s->s2strtab[ndx].strchar = ndx;
    s->s2strtab[ndx].strcount = 1;
  }
}

static xadERROR decompress_s2(struct VMARCState *s) {
  STRDSECT *prev;
  STRDSECT *curr;
  STRDSECT *next;
  xadUINT16 slastcode;
  xadUINT16 lastcode;
  xadINT32 rc;

  s2_init(s);

  slastcode = get_code12(s);
  if (slastcode == 0xffff) return XADERR_ILLEGALDATA;

  rc = put_decoded_byte(s, slastcode);
  if (rc < 0) return XADERR_OUTPUT;
  if (rc == 0) return XADERR_OK;

  while (1) {
    lastcode = get_code12(s);
    if (lastcode == 0xffff) return XADERR_ILLEGALDATA;

    curr = &s->s2strtab[lastcode];
    prev = NULL;
    do {
      do {
        next = curr->strleft;
        curr->strleft = prev;
        prev = curr;
        curr = next;
      } while (curr);

      curr = prev;
      prev = curr->strleft;
      curr->strleft = next;
      rc = put_decoded_byte(s, curr->strchar);
      if (rc < 0) return XADERR_OUTPUT;
      if (rc == 0) return XADERR_OK;

      while (1) {
        next = curr;
        curr = prev;
        if (!curr) break;

        if (!curr->visited) {
          prev = curr->strleft;
          curr->strleft = next;
          next = curr->strright;
          curr->strright = prev;
          curr->visited = 1;
          prev = curr;
          curr = next;
          break;
        }

        curr->visited = 0;
        prev = curr->strright;
        curr->strright = next;
      }
    } while (curr);

    s2_addnew(s, slastcode, lastcode);
    slastcode = lastcode;
  }
}

/* ASIS Decompression */
static xadERROR decompress_asis(struct VMARCState *s) {
  xadINT32 h, l;
  xadINT32 lrecl;

  while (1) {
    h = get_input_byte(s);
    if (h < 0) break;
    l = get_input_byte(s);
    if (l < 0) return XADERR_ILLEGALDATA;

    lrecl = (h << 8) | l;
    if (lrecl == 0) break;

    while (lrecl-- > 0) {
      l = get_input_byte(s);
      if (l < 0) return XADERR_ILLEGALDATA;
      if (put_decoded_byte(s, (xadUINT16)(l + kodebcd)) < 0) {
        return XADERR_OUTPUT;
      }
    }

    if (s->recfm == 'V') {
      if (put_decoded_byte(s, 0) < 0) {
        return XADERR_OUTPUT;
      }
    }
  }

  if (put_decoded_byte(s, 0) < 0) {
    return XADERR_OUTPUT;
  }
  return XADERR_OK;
}

/* --------------------------------------------------------------------
 * XAD Client Callbacks
 * -------------------------------------------------------------------- */

XADRECOGDATA(VMARC)
{
  if (size >= 8 && memcmp(data, vmarc_hid, 8) == 0) {
    return 1;
  }
  return 0;
}

XADGETINFO(VMARC)
{
  xadUINT8 buf[80];
  xadERROR err = XADERR_OK;
  xadUINT32 filenum = 1;
  struct xadFileInfo *lastfi = NULL;

  while (ai->xai_InPos < ai->xai_InSize) {
    xadSize startpos = ai->xai_InPos;

    /* Align to 80-byte record if needed */
    if (startpos % 80 != 0) {
      xadSize skip = 80 - (startpos % 80);
      if (xadHookAccess(XADM XADAC_INPUTSEEK, skip, NULL, ai) != XADERR_OK) {
        break;
      }
      startpos = ai->xai_InPos;
    }

    /* Read header identifier (8 bytes) */
    if (xadHookAccess(XADM XADAC_READ, 8, buf, ai) != XADERR_OK) {
      break;
    }

    if (memcmp(buf, vmarc_hid, 8) != 0) {
      /* Search remaining 80-byte boundaries */
      xadINT32 found = 0;
      while (ai->xai_InPos + 8 <= ai->xai_InSize) {
        if (xadHookAccess(XADM XADAC_INPUTSEEK, 72, NULL, ai) != XADERR_OK) break;
        if (xadHookAccess(XADM XADAC_READ, 8, buf, ai) != XADERR_OK) break;
        if (memcmp(buf, vmarc_hid, 8) == 0) {
          found = 1;
          break;
        }
      }
      if (!found) break;
    }

    /* Read base header (30 bytes) */
    if (xadHookAccess(XADM XADAC_READ, H_DLEN, buf, ai) != XADERR_OK) {
      break;
    }

    /* Validate header version */
    if (buf[H_VER] != 1 || buf[H_REL] > 2) {
      break;
    }

    xadUINT8 flags = buf[H_FLAGS];
    xadUINT8 recfm = e2a_tab[buf[H_RECFM]];
    if (recfm != 'F' && recfm != 'V') {
      break;
    }

    xadUINT16 lrecl = ((xadUINT16)buf[H_LRECL] << 8) | buf[H_LRECL + 1];

    /* Extended header check */
    if (flags & HF_EXTH) {
      xadUINT8 exth[12];
      if (xadHookAccess(XADM XADAC_READ, 12, exth, ai) != XADERR_OK) {
        break;
      }
    }

    xadSize data_start = ai->xai_InPos;

    /* Build filename: UPPERCASE FN.FT.FM (e.g. "MYPROG.EXEC.A1") */
    char fn[9], ft[9], fm[3];
    int i, p;

    /* FN */
    p = 0;
    for (i = 0; i < 8; i++) {
      char c = (char)e2a_tab[buf[H_FN + i]];
      if (c && c != ' ') fn[p++] = toupper((unsigned char)c);
    }
    fn[p] = '\0';
    if (p == 0) strcpy(fn, "FILE");

    /* FT */
    p = 0;
    for (i = 0; i < 8; i++) {
      char c = (char)e2a_tab[buf[H_FT + i]];
      if (c && c != ' ') ft[p++] = toupper((unsigned char)c);
    }
    ft[p] = '\0';
    if (p == 0) strcpy(ft, "DATA");

    /* FM */
    p = 0;
    for (i = 0; i < 2; i++) {
      char c = (char)e2a_tab[buf[H_FM + i]];
      if (c && c != ' ') fm[p++] = toupper((unsigned char)c);
    }
    fm[p] = '\0';
    if (p == 0) strcpy(fm, "A1");

    char full_name[32];
    snprintf(full_name, sizeof(full_name), "%s.%s.%s", fn, ft, fm);
    xadUINT32 name_len = (xadUINT32)strlen(full_name) + 1;

    struct xadFileInfo *fi = (struct xadFileInfo *)xadAllocObject(
      XADM XADOBJ_FILEINFO,
      XAD_OBJNAMESIZE, name_len,
      XAD_OBJPRIVINFOSIZE, sizeof(struct VMARCPrivFile),
      TAG_DONE
    );

    if (!fi) {
      err = XADERR_NOMEMORY;
      break;
    }

    strcpy(fi->xfi_FileName, full_name);
    fi->xfi_EntryNumber = filenum++;
    fi->xfi_DataPos = data_start;
    fi->xfi_Flags = XADFIF_SEEKDATAPOS;

    /* Date and time decoding from BCD */
    xadINT32 year = cvb(buf[H_YEAR]) + 1900;
    if (flags & HF_Y2K) year += 100;
    fi->xfi_Date.xd_Year   = year;
    fi->xfi_Date.xd_Month  = cvb(buf[H_MONTH]);
    fi->xfi_Date.xd_Day    = cvb(buf[H_DAY]);
    fi->xfi_Date.xd_Hour   = cvb(buf[H_HOUR]);
    fi->xfi_Date.xd_Minute = cvb(buf[H_MINUTE]);
    fi->xfi_Date.xd_Second = cvb(buf[H_SECOND]);
    fi->xfi_Date.xd_Micros = 0;

    struct VMARCPrivFile *pf = (struct VMARCPrivFile *)fi->xfi_PrivateInfo;
    if (pf) {
      pf->vpf_Flags = flags;
      pf->vpf_Recfm = recfm;
      pf->vpf_Lrecl = lrecl;
      pf->vpf_DataPos = data_start;
    }

    /* Fast-forward to next subfile header:
     * In VMARC archives, every subfile header is aligned on an 80-byte card boundary.
     * We align to the next 80-byte boundary and inspect 8-byte blocks at each 80-byte boundary.
     */
    xadSize next_header_pos = ai->xai_InSize;
    xadUINT8 scan_buf[10];

    xadSize scan_pos = (data_start + 79) / 80 * 80;
    while (scan_pos + 80 <= ai->xai_InSize) {
      ai->xai_InPos = scan_pos;
      if (xadHookAccess(XADM XADAC_READ, 10, scan_buf, ai) != XADERR_OK) {
        break;
      }
      if (memcmp(scan_buf, vmarc_hid, 8) == 0 && scan_buf[8] == 1 && scan_buf[9] <= 2) {
        next_header_pos = scan_pos;
        break;
      }
      scan_pos += 80;
    }

    fi->xfi_CrunchSize = (next_header_pos > data_start) ? (next_header_pos - data_start) : 0;
    fi->xfi_Size = fi->xfi_CrunchSize;

    if (lastfi) {
      lastfi->xfi_Next = fi;
    } else {
      ai->xai_FileInfo = fi;
    }
    lastfi = fi;

    /* Position at next header */
    if (ai->xai_InPos != next_header_pos) {
      ai->xai_InPos = next_header_pos;
    }
  }

  if (err && !ai->xai_FileInfo) {
    return err;
  }
  return (ai->xai_FileInfo ? XADERR_OK : XADERR_EMPTY);
}

XADUNARCHIVE(VMARC)
{
  struct xadFileInfo *fi = ai->xai_CurFile;
  if (!fi || !fi->xfi_PrivateInfo) return XADERR_BADPARAMS;

  struct VMARCPrivFile *pf = (struct VMARCPrivFile *)fi->xfi_PrivateInfo;
  struct VMARCState *s = (struct VMARCState *)xadAllocVec(XADM sizeof(struct VMARCState), XADMEMF_CLEAR);
  if (!s) return XADERR_NOMEMORY;

  s->ai = ai;
  s->xmb = xadMasterBase;
  s->recfm = pf->vpf_Recfm;
  s->lrecl = pf->vpf_Lrecl;
  s->is_text = 1; /* Text converted to ISO-8859-1 (Latin-1) */
  s->residual = 0xffffffff;

  xadERROR err = XADERR_OK;

  if (pf->vpf_Flags & HF_ASIS) {
    err = decompress_asis(s);
  } else if (pf->vpf_Flags & HF_S2) {
    err = decompress_s2(s);
  } else {
    err = decompress_lzw(s);
  }

  if (err == XADERR_OK) {
    err = flush_output(s);
  }

  xadFreeObjectA(XADM s, NULL);
  return err;
}

XADFREE(VMARC)
{
  /* All xadFileInfo and PrivateInfo allocated through XAD are freed by xadmaster */
  ai->xai_PrivateClient = NULL;
}

XADFIRSTCLIENT(VMARC)
{
  XADNEXTCLIENT,
  XADCLIENT_VERSION,
  XADMASTERVERSION,
  VMARC_VERSION,
  VMARC_REVISION,
  8,
  XADCF_FILEARCHIVER | XADCF_FREEFILEINFO,
  0,
  "VMARC",
  XADRECOGDATAP(VMARC),
  XADGETINFOP(VMARC),
  XADUNARCHIVEP(VMARC),
  XADFREEP(VMARC)
};

#undef XADNEXTCLIENT
#define XADNEXTCLIENT XADNEXTCLIENTNAME(VMARC)

#endif /* XADMASTER_VMARC_C */

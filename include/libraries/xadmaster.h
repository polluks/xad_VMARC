#ifndef LIBRARIES_XADMASTER_H
#define LIBRARIES_XADMASTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

typedef uint32_t           xadUINT32;
typedef int32_t            xadINT32;
typedef uint16_t           xadUINT16;
typedef int16_t            xadINT16;
typedef uint8_t            xadUINT8;
typedef int8_t             xadINT8;

#ifdef AMIGA
typedef uint32_t           xadSize;
typedef int32_t            xadSignSize;
#else
typedef uint64_t           xadSize;
typedef int64_t            xadSignSize;
#endif

typedef void *             xadPTR;
typedef char               xadSTRING;
typedef xadSTRING *        xadSTRPTR;
typedef unsigned int       xadUINT;
typedef int                xadINT;
typedef xadINT             xadERROR;
typedef xadINT             xadBOOL;

#define XADFALSE        0
#define XADTRUE         1
#define XADMEMF_ANY     (0)
#define XADMEMF_CLEAR   (1L << 16)
#define XADMEMF_PUBLIC  (1L << 0)

typedef xadUINT32          xadTag;

struct TagItem {
  xadTag  ti_Tag;
  xadSize ti_Data;
};

typedef const struct TagItem * xadTAGPTR;

#define TAG_PTR (0x80000000UL)
#define TAG_INT (0x40000000UL)
#define TAG_SIZ (0x20000000UL)

#define TAG_DONE   (TAG_INT + 0x10000000UL)
#define TAG_IGNORE (TAG_INT + 0x10000001UL)
#define TAG_MORE   (TAG_PTR + 0x10000002UL)
#define TAG_SKIP   (TAG_INT + 0x10000003UL)

struct Hook {
  xadUINT32 (*h_Entry)();
  xadPTR *h_Data;
};

struct xadMasterBase {
  xadPTR dummy;
};

#define XAD_INSIZE              (TAG_SIZ+  1)
#define XAD_INFILENAME          (TAG_PTR+  2)
#define XAD_INFILEHANDLE        (TAG_PTR+  3)
#define XAD_INMEMORY            (TAG_PTR+  4)
#define XAD_INHOOK              (TAG_PTR+  5)

#define XAD_OUTSIZE             (TAG_SIZ+ 10)
#define XAD_OUTFILENAME         (TAG_PTR+ 11)
#define XAD_OUTFILEHANDLE       (TAG_PTR+ 12)
#define XAD_OUTMEMORY           (TAG_PTR+ 13)
#define XAD_OUTHOOK             (TAG_PTR+ 14)

#define XAD_OBJNAMESIZE         (TAG_INT+ 20)
#define XAD_OBJCOMMENTSIZE      (TAG_INT+ 21)
#define XAD_OBJPRIVINFOSIZE     (TAG_INT+ 22)
#define XAD_OBJBLOCKENTRIES     (TAG_INT+ 23)

#define XAD_PASSWORD            (TAG_PTR+ 51)
#define XAD_ENTRYNUMBER         (TAG_INT+ 52)
#define XAD_PROGRESSHOOK        (TAG_PTR+ 53)
#define XAD_OVERWRITE           (TAG_INT+ 54)

#define XAD_DATEUNIX            (TAG_INT+ 70)
#define XAD_DATEAMIGA           (TAG_INT+ 71)
#define XAD_DATEXADDATE         (TAG_PTR+ 73)
#define XAD_DATECURRENTTIME     (TAG_INT+ 75)
#define XAD_DATEMSDOS           (TAG_INT+ 76)

#define XAD_GETDATEUNIX         (TAG_PTR+ 80)
#define XAD_GETDATEAMIGA        (TAG_PTR+ 81)
#define XAD_GETDATEXADDATE      (TAG_PTR+ 83)
#define XAD_GETDATEMSDOS        (TAG_PTR+ 86)

#define XAD_MAKEGMTDATE         (TAG_INT+ 90)
#define XAD_MAKELOCALDATE       (TAG_INT+ 91)

#define XAD_SETINPOS            (TAG_SIZ+240)
#define XAD_EXTENSION           (TAG_PTR+202)

#define XADOBJ_ARCHIVEINFO      0x0001
#define XADOBJ_FILEINFO         0x0002
#define XADOBJ_DISKINFO         0x0003
#define XADOBJ_HOOKPARAM        0x0004
#define XADOBJ_PROGRESSINFO     0x0006
#define XADOBJ_SPECIAL          0x000B

#define XADAC_READ              10
#define XADAC_WRITE             11
#define XADAC_COPY              12
#define XADAC_INPUTSEEK         13
#define XADAC_OUTPUTSEEK        14

struct xadDate {
  xadUINT32 xd_Micros;
  xadINT32  xd_Year;
  xadUINT8  xd_Month;
  xadUINT8  xd_WeekDay;
  xadUINT8  xd_Day;
  xadUINT8  xd_Hour;
  xadUINT8  xd_Minute;
  xadUINT8  xd_Second;
};

struct xadClient;
struct xadFileInfo;
struct xadDiskInfo;

struct xadArchiveInfo {
  struct xadClient *   xai_Client;
  xadPTR               xai_PrivateClient;
  xadSTRPTR            xai_Password;
  xadUINT32            xai_Flags;
  xadUINT32            xai_LowCyl;
  xadUINT32            xai_HighCyl;
  xadSize              xai_InPos;
  xadSize              xai_InSize;
  xadSize              xai_OutPos;
  xadSize              xai_OutSize;
  struct xadFileInfo * xai_FileInfo;
  struct xadDiskInfo * xai_DiskInfo;
  struct xadFileInfo * xai_CurFile;
  struct xadDiskInfo * xai_CurDisk;
  xadERROR             xai_LastError;
  xadSize *            xai_MultiVolume;
  xadPTR               xai_SkipInfo;
  xadPTR               xai_ImageInfo;
  xadSTRPTR            xai_InName;
};

#define XADAIB_CRYPTED           0
#define XADAIB_FILECORRUPT       1
#define XADAIB_FILEARCHIVE       2

#define XADAIF_CRYPTED          (1<<XADAIB_CRYPTED)
#define XADAIF_FILECORRUPT      (1<<XADAIB_FILECORRUPT)
#define XADAIF_FILEARCHIVE      (1<<XADAIB_FILEARCHIVE)

struct xadFileInfo {
  struct xadFileInfo * xfi_Next;
  xadUINT32            xfi_EntryNumber;
  xadSTRPTR            xfi_EntryInfo;
  xadPTR               xfi_PrivateInfo;
  xadUINT32            xfi_Flags;
  xadSTRPTR            xfi_FileName;
  xadSTRPTR            xfi_Comment;
  xadUINT32            xfi_Protection;
  xadUINT32            xfi_OwnerUID;
  xadUINT32            xfi_OwnerGID;
  xadSTRPTR            xfi_UserName;
  xadSTRPTR            xfi_GroupName;
  xadSize              xfi_Size;
  xadSize              xfi_GroupCrSize;
  xadSize              xfi_CrunchSize;
  xadSTRPTR            xfi_LinkName;
  struct xadDate       xfi_Date;
  xadUINT16            xfi_Generation;
  xadSize              xfi_DataPos;
  struct xadFileInfo * xfi_MacFork;
  xadUINT16            xfi_UnixProtect;
  xadUINT8             xfi_DosProtect;
  xadUINT8             xfi_FileType;
  xadPTR               xfi_Special;
};

#define XADFIB_CRYPTED          0
#define XADFIB_DIRECTORY        1
#define XADFIB_LINK             2
#define XADFIB_INFOTEXT         3
#define XADFIB_GROUPED          4
#define XADFIB_ENDOFGROUP       5
#define XADFIB_NODATE           6
#define XADFIB_DELETED          7
#define XADFIB_SEEKDATAPOS      8
#define XADFIB_NOFILENAME       9
#define XADFIB_NOUNCRUNCHSIZE  10
#define XADFIB_PARTIALFILE     11

#define XADFIF_CRYPTED          (1<<XADFIB_CRYPTED)
#define XADFIF_DIRECTORY        (1<<XADFIB_DIRECTORY)
#define XADFIF_LINK             (1<<XADFIB_LINK)
#define XADFIF_INFOTEXT         (1<<XADFIB_INFOTEXT)
#define XADFIF_GROUPED          (1<<XADFIB_GROUPED)
#define XADFIF_ENDOFGROUP       (1<<XADFIB_ENDOFGROUP)
#define XADFIF_NODATE           (1<<XADFIB_NODATE)
#define XADFIF_DELETED          (1<<XADFIB_DELETED)
#define XADFIF_SEEKDATAPOS      (1<<XADFIB_SEEKDATAPOS)
#define XADFIF_NOFILENAME       (1<<XADFIB_NOFILENAME)
#define XADFIF_NOUNCRUNCHSIZE   (1<<XADFIB_NOUNCRUNCHSIZE)
#define XADFIF_PARTIALFILE      (1<<XADFIB_PARTIALFILE)

#define XADERR_OK               0x0000
#define XADERR_UNKNOWN          0x0001
#define XADERR_INPUT            0x0002
#define XADERR_OUTPUT           0x0003
#define XADERR_BADPARAMS        0x0004
#define XADERR_NOMEMORY         0x0005
#define XADERR_ILLEGALDATA      0x0006
#define XADERR_NOTSUPPORTED     0x0007
#define XADERR_RESOURCE         0x0008
#define XADERR_DECRUNCH         0x0009
#define XADERR_FILETYPE         0x000A
#define XADERR_OPENFILE         0x000B
#define XADERR_SKIP             0x000C
#define XADERR_BREAK            0x000D
#define XADERR_FILEEXISTS       0x000E
#define XADERR_PASSWORD         0x000F
#define XADERR_MAKEDIR          0x0010
#define XADERR_CHECKSUM         0x0011
#define XADERR_VERIFY           0x0012
#define XADERR_GEOMETRY         0x0013
#define XADERR_DATAFORMAT       0x0014
#define XADERR_EMPTY            0x0015
#define XADERR_FILESYSTEM       0x0016

#define XADCLIENT_VERSION       1

#define XADCB_FILEARCHIVER       0
#define XADCB_DISKARCHIVER       1
#define XADCB_EXTERN             2
#define XADCB_FILESYSTEM         3
#define XADCB_NOCHECKSIZE        4
#define XADCB_DATACRUNCHER       5
#define XADCB_FREEFILEINFO      30
#define XADCB_FREEDISKINFO      31

#define XADCF_FILEARCHIVER      (1UL<<XADCB_FILEARCHIVER)
#define XADCF_DISKARCHIVER      (1UL<<XADCB_DISKARCHIVER)
#define XADCF_EXTERN            (1UL<<XADCB_EXTERN)
#define XADCF_FILESYSTEM        (1UL<<XADCB_FILESYSTEM)
#define XADCF_NOCHECKSIZE       (1UL<<XADCB_NOCHECKSIZE)
#define XADCF_DATACRUNCHER      (1UL<<XADCB_DATACRUNCHER)
#define XADCF_FREEFILEINFO      (1UL<<XADCB_FREEFILEINFO)
#define XADCF_FREEDISKINFO      (1UL<<XADCB_FREEDISKINFO)

struct xadClient {
  struct xadClient * xc_Next;
  xadUINT16          xc_Version;
  xadUINT16          xc_MasterVersion;
  xadUINT16          xc_ClientVersion;
  xadUINT16          xc_ClientRevision;
  xadSize            xc_RecogSize;
  xadUINT32          xc_Flags;
  xadUINT32          xc_Identifier;
  xadSTRPTR          xc_ArchiverName;
#if defined(AMIGA) && !defined(__MORPHOS__) && !defined(SDI_MORPHOSNOREG)
  xadBOOL         (* xc_RecogData)();
  xadERROR        (* xc_GetInfo)();
  xadERROR        (* xc_UnArchive)();
  void            (* xc_Free)();
#else
  xadBOOL         (* xc_RecogData)(xadSize size, const xadUINT8 *data,
                  struct xadMasterBase *xadMasterBase);
  xadERROR        (* xc_GetInfo)(struct xadArchiveInfo *ai,
                  struct xadMasterBase *xadMasterBase);
  xadERROR        (* xc_UnArchive)(struct xadArchiveInfo *ai,
                  struct xadMasterBase *xadMasterBase);
  void            (* xc_Free)(struct xadArchiveInfo *ai,
                  struct xadMasterBase *xadMasterBase);
#endif
};

struct xadForeman {
  xadUINT32          xfm_Security;
  xadUINT32          xfm_ID;
  xadUINT16          xfm_Version;
  xadUINT16          xfm_Reserved;
  const xadSTRING *  xfm_VersString;
  const struct xadClient * xfm_FirstClient;
};

#define XADFOREMAN_SECURITY     0x70FF4E75
#define XADFOREMAN_ID           0x58414446
#define XADFOREMAN_VERSION      1

#define XADM xadMasterBase,

/* Prototypes for XAD master functions */
xadERROR xadHookAccess(struct xadMasterBase *xadMasterBase, xadINT32 command, xadSize size, xadPTR buffer, struct xadArchiveInfo *ai);
xadPTR xadAllocObject(struct xadMasterBase *xadMasterBase, xadUINT32 type, ...);
xadPTR xadAllocObjectA(struct xadMasterBase *xadMasterBase, xadUINT32 type, xadTAGPTR tags);
void xadFreeObjectA(struct xadMasterBase *xadMasterBase, xadPTR object, xadTAGPTR tags);
xadPTR xadAllocVec(struct xadMasterBase *xadMasterBase, xadSize size, xadUINT32 flags);
void xadCopyMem(struct xadMasterBase *xadMasterBase, const void *src, void *dest, xadSize size);

#ifdef __cplusplus
}
#endif

#endif /* LIBRARIES_XADMASTER_H */

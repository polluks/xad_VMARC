#ifndef XADMASTER_XADCLIENT_H
#define XADMASTER_XADCLIENT_H

#include <libraries/xadmaster.h>
#include "SDI_compiler.h"
#include "ConvertE.c"

#define XADBASE REG(a6, struct xadMasterBase *xadMasterBase)

#if defined(__MORPHOS__) || defined(SDI_MORPHOSNOREG) || !defined(AMIGA)
#define XADRECOGDATA(name) static xadBOOL name##_RecogData( \
          xadSize size, \
          const xadUINT8 *data, \
          struct xadMasterBase *xadMasterBase)

#define XADGETINFO(name)   static xadERROR name##_GetInfo( \
          struct xadArchiveInfo *ai, \
          struct xadMasterBase *xadMasterBase)

#define XADUNARCHIVE(name) static xadERROR name##_UnArchive( \
          struct xadArchiveInfo *ai, \
          struct xadMasterBase *xadMasterBase)

#define XADFREE(name)      static void name##_Free( \
          struct xadArchiveInfo *ai, \
          struct xadMasterBase *xadMasterBase)
#else
#define XADRECOGDATA(name) static ASM(xadBOOL) name##_RecogData( \
          REG(d0, xadSize size), \
          REG(a0, const xadUINT8 *data), \
          XADBASE)

#define XADGETINFO(name)   static ASM(xadERROR) name##_GetInfo( \
          REG(a0, struct xadArchiveInfo *ai), \
          XADBASE)

#define XADUNARCHIVE(name) static ASM(xadERROR) name##_UnArchive( \
          REG(a0, struct xadArchiveInfo *ai), \
          XADBASE)

#define XADFREE(name)      static ASM(void) name##_Free( \
          REG(a0, struct xadArchiveInfo *ai), \
          XADBASE)
#endif

#define XADRECOGDATAP(name) (xadBOOL (*)()) name##_RecogData
#define XADGETINFOP(name)   (xadERROR (*)()) name##_GetInfo
#define XADUNARCHIVEP(name) (xadERROR (*)()) name##_UnArchive
#define XADFREEP(name)      (void (*)()) name##_Free

#ifdef XADMASTERFILE
  #define XADFIRSTCLIENT(name)  static const struct xadClient name##_Client =
  #define XADCLIENTVERSTR(text)
#else
  #define XADFIRSTCLIENT(name)  const struct xadClient FirstClient =
  #define XADCLIENTVERSTR(text) const xadSTRING version[] = "$VER: " text;
#endif

#define XADCLIENT(name)         static const struct xadClient name##_Client =
#define XADNEXTCLIENT           0
#define XADNEXTCLIENTNAME(name) (struct xadClient *) &name##_Client

#endif /* XADMASTER_XADCLIENT_H */

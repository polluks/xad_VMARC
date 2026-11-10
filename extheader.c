/*  extheader.c - C header for XAD external client (MorphOS / AmigaOS)
 *  Provides standard xadForeman structure at the beginning of the binary
 */

#include <libraries/xadmaster.h>
#include "SDI_compiler.h"

extern const xadSTRING version[];
extern const struct xadClient FirstClient;

const struct xadForeman ForeMan USED = {
  XADFOREMAN_SECURITY,
  XADFOREMAN_ID,
  XADFOREMAN_VERSION,
  0,
  version,
  &FirstClient
};

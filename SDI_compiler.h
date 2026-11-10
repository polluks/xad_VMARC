#ifndef SDI_COMPILER_H
#define SDI_COMPILER_H

/* SDI_compiler.h - compatibility defines for various compilers (AmigaOS, MorphOS, GCC) */

#ifdef ASM
#undef ASM
#endif
#ifdef REG
#undef REG
#endif
#ifdef LREG
#undef LREG
#endif
#ifdef CONST
#undef CONST
#endif
#ifdef SAVEDS
#undef SAVEDS
#endif
#ifdef INLINE
#undef INLINE
#endif
#ifdef REGARGS
#undef REGARGS
#endif
#ifdef STDARGS
#undef STDARGS
#endif

#if defined(__GNUC__)
  #if defined(__MORPHOS__)
    #define STDARGS
    #define STACKEXT
    #define REGARGS
    #if defined(SDI_MORPHOSNOREG)
      #define REG(reg,arg) arg
    #else
      #define REG(reg,arg) arg __asm(#reg)
    #endif
    #define ASM(arg) arg
    #define SAVEDS
    #define INLINE inline
  #elif defined(__amigaos4__)
    #define REG(reg,arg) arg
    #define ASM(arg) arg
    #define SAVEDS
    #define INLINE inline
  #elif defined(__m68k__) || defined(AMIGA)
    #define REG(reg,arg) arg __asm(#reg)
    #define LREG(reg,arg) register REG(reg,arg)
    #define ASM(arg) arg
    #define SAVEDS __saveds
    #define INLINE inline
  #else
    #define REG(reg,arg) arg
    #define ASM(arg) arg
    #define SAVEDS
    #define INLINE inline
  #endif
#elif defined(__VBCC__)
  #define STDARGS
  #define STACKEXT
  #define REGARGS
  #define INLINE inline
  #if defined(__MORPHOS__) || defined(SDI_MORPHOSNOREG)
    #define REG(reg,arg) arg
  #else
    #define REG(reg,arg) __reg(#reg) arg
  #endif
  #define ASM(arg) arg
  #define SAVEDS __saveds
#elif defined(__SASC)
  #define ASM(arg) arg __asm
  #define REG(reg,arg) register __##reg arg
  #define SAVEDS __saveds
  #define INLINE __inline
#else
  #define ASM(arg) arg
  #define REG(reg,arg) arg
  #define SAVEDS
  #define INLINE inline
#endif

#if !defined(CONST)
  #define CONST const
#endif
#if !defined(USED)
  #if defined(__GNUC__)
    #define USED __attribute__((used))
  #else
    #define USED
  #endif
#endif

#endif /* SDI_COMPILER_H */

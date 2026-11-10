#ifndef XADMASTER_CONVERTE_C
#define XADMASTER_CONVERTE_C

/* Endian conversion macros */

#define EndGetM32(a)  (((((unsigned char *) (a))[0]) << 24) | \
                       ((((unsigned char *) (a))[1]) << 16) | \
                       ((((unsigned char *) (a))[2]) <<  8) | \
                       ((((unsigned char *) (a))[3])))

#define EndGetM24(a)  (((((unsigned char *) (a))[0]) << 16) | \
                       ((((unsigned char *) (a))[1]) <<  8) | \
                       ((((unsigned char *) (a))[2])))

#define EndGetM16(a)  (((((unsigned char *) (a))[0]) <<  8) | \
                       ((((unsigned char *) (a))[1])))

#define EndGetI32(a)  (((((unsigned char *) (a))[3]) << 24) | \
                       ((((unsigned char *) (a))[2]) << 16) | \
                       ((((unsigned char *) (a))[1]) <<  8) | \
                       ((((unsigned char *) (a))[0])))

#define EndGetI16(a)  (((((unsigned char *) (a))[1]) <<  8) | \
                       ((((unsigned char *) (a))[0])))

#endif /* XADMASTER_CONVERTE_C */

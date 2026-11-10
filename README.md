# XAD Client for IBM VMARC Archives

This repository provides an XAD client for IBM VM/CMS VMARC archives.
It is compatible with `xadmaster.library` / `libxad` across MorphOS, AmigaOS, and portable Unix environments.

## Features
- **VMARC Recognition (`VMARC_RecogData`)**:
  - Validates the standard 8-byte EBCDIC `:CFF    ` signature on 80-byte card record boundaries.
- **Archive Cataloging (`VMARC_GetInfo`)**:
  - Parses VMARC subfile headers (Version 1, Releases 0–2).
  - Normalizes CMS filenames to uppercase `FN.FT.FM` format (e.g., `MYPROG.EXEC.A1`).
  - Decodes BCD timestamps into standard `xadDate` structures (with Y2K century handling).
  - Handles Fixed ('F') and Variable ('V') record format metadata and record lengths (`LRECL`).
- **Decompression (`VMARC_UnArchive`)**:
  - **ASIS**: Uncompressed block/record storage.
  - **LZW**: 12-bit LZW decompression with KwKwK table handling.
  - **S2**: Hierarchical string-table tree decompression.
  - Converts text from EBCDIC (IBM-037 / IBM-1047) to ISO-8859-1 (Latin-1).
- **MorphOS and AmigaOS Integration**:
  - `extheader.c`: Standard `xadForeman` module header.
  - `SDI_compiler.h`: MorphOS PPC (`__MORPHOS__`, `SDI_MORPHOSNOREG`) and Amiga 68k / OS4 register macro support.
  - `ConvertE.c`: Endian conversion helpers.
  - `VMARC.h` / `xadClient.h`: Client prototypes and registration structures.

## Building and Testing

### Running the Test Suite
To compile and execute the test suite:
```sh
make test
```

### Compiling for MorphOS
Using MorphOS GCC:
```sh
gcc -D__MORPHOS__ -DSDI_MORPHOSNOREG -O2 -noixemul -I. -Iinclude -c VMARC.c -o VMARC.o
gcc -D__MORPHOS__ -DSDI_MORPHOSNOREG -O2 -noixemul -I. -Iinclude -c extheader.c -o extheader.o
gcc -nostartfiles -noixemul -o VMARC extheader.o VMARC.o
strip -s -R .comment VMARC
```

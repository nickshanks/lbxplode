#ifndef __LBX_H__
#define __LBX_H__

#ifndef Uint8
  #define Uint8 unsigned char
#endif
#ifndef Uint16
  #define Uint16 unsigned short
#endif
#ifndef Uint32
  #define Uint32 unsigned int
#endif
#ifndef Sint32
  #define Sint32 signed int
#endif

typedef struct LBXheader
{
  Uint16 files;
  Uint16 magic;
  Uint16 unknown1;
  Uint16 version;  /* Version number */
} LBXheader;

#endif/*__LBX_H__*/

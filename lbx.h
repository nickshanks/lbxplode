#ifndef __LBX_H__
#define __LBX_H__

#include <SDL/SDL_types.h>
#include <SDL/begin_code.h>

typedef struct LBXheader
{
  Sint16 files;    /* Stored as little endian in file */
  Uint8  magic[4]; /* Magic 4 bytes  */
  Uint16 version;  /* Version number */
} LBXheader;

#include <SDL/close_code.h>

#endif/*__LBX_H__*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lbx.h"

// http://www.xprt.net/~s8/prog/orion2/lbx/

#define DEBUG 0

const Uint16 LBX_MAGIC = 65197;
const Uint8 RIFFmagic[]={'R','I','F','F'};
const Uint8 WAVEfmt[]={'W','A','V','E','f','m','t'};
const Uint8 MIDImagic1[]={'M','T','h','d'};
const Uint8 MIDImagic2[]={'M','T','r','k'};

char writebuf[0x10000];

#ifndef __inline
  #define __inline
#endif

#ifndef __inline__
  #define __inline__ __inline
#endif

static __inline__ Uint16 SwapLE16(Uint16 x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return x;
#else
  return (x >> 8) | (x << 8);
#endif
}

static __inline__ Uint32 SwapLE32(Uint32 x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return x;
#else
  return (x >> 24) | ((x & 0x00FF0000) >> 8) |
    ((x & 0x0000FF00) << 8) | (x << 24);
#endif
}

static __inline__ Uint64 SwapLE64(Uint64 x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return x;
#else
  return (x >> 56) | ((x & 0x00FF000000000000) >> 40) |
    ((x & 0x0000FF0000000000) >> 24) | ((x & 0x000000FF00000000) >> 8) |
    ((x & 0x00000000FF000000) << 8) | ((x & 0x0000000000FF0000) << 24) |
    ((x & 0x000000000000FF00) << 40) | (x << 56);
#endif
}

int ParseLBX(char *lbxname);
int ParseOldLBX(FILE *fp, long lbxlen, char *lbxname);
int ExtractFile(FILE *fp, Uint32 offset, Sint32 len, int m, char *lbxname, int final);

/* 0 on true, -1 on false or error */
int IsWAVE(FILE *fp, Uint32 offset);
int IsMIDI(FILE *fp, Uint32 offset);


int main(int argc, char *argv[])
{
  int n;

  if(argc<=1)
  {
    fprintf(stderr,"Usage: %s <file1.lbx> <file2.lbx> ...\n", argv[0]);
    return(1);
  }

  for(n=1; n<argc; n++)
    ParseLBX(argv[n]);

  return(0);
}

int ParseLBX(char *lbxname)
{
  LBXheader header;
  int m;
  long lbxlen;
  FILE *fp;
  Uint32 *offset;

  /* suffix "_0000.ext" will be added, and name buffer is 256 bytes long */
  if(strlen(lbxname)>246)
  {
    fprintf(stderr,"Filename %s is too long, skipping.\n",lbxname);
    return(1);
  }

  fp=fopen(lbxname,"rb");
  if(fp==NULL)
  {
    fprintf(stderr,"Could not open %s\n",lbxname);
    return(1);
  }

  fseek(fp,0,SEEK_END);
  lbxlen=ftell(fp);
  if(lbxlen<sizeof(header))
  {
    fprintf(stderr,"File %s is too small to be a modern LBX archive, trying old format.\n",lbxname);
    return ParseOldLBX(fp, lbxlen, lbxname);
  }

  fseek(fp,0,SEEK_SET);
  if(fread(&header,sizeof(header),1,fp)==0)
  {
    fprintf(stderr,"Modern LBX archive header could not be read from %s, trying old format.\n",lbxname);
    return ParseOldLBX(fp, lbxlen, lbxname);
  }

  header.files=SwapLE16(header.files);
  header.magic=SwapLE16(header.magic);

  if(header.magic!=LBX_MAGIC)
  {
    fprintf(stderr,"File %s is not a modern LBX archive, trying old format.\n",lbxname);
    return ParseOldLBX(fp, lbxlen, lbxname);
  }

  printf("%s contains %u files\n",lbxname,header.files);

  offset=(Uint32 *)malloc(sizeof(Uint32)*(header.files+1));
  if(offset==NULL)
  {
    fprintf(stderr,"Could not allocate offsets buffer.\n");
    fclose(fp);
    return(1);
  }

  fread(offset,sizeof(Uint32),header.files+1,fp);

  if(DEBUG)
    printf("Offsets: ");
  for(m=0; m<=header.files; m++)
  {
    offset[m]=SwapLE32(offset[m]);
    if(DEBUG)
      printf("0x%08x ",offset[m]);
  }
  if(DEBUG)
    printf("\n");

  if(offset[header.files]!=lbxlen)
    fprintf(stderr,"Invalid LBX archive: last offset does not match file length, continuing anyway. Some files may be corrupt.\n");

  for(m=0; m<header.files; m++)
    ExtractFile(fp,offset[m],offset[m+1]-offset[m],m,lbxname,m==header.files-1);

  free(offset);
  fclose(fp);
  return(0);
}

int ParseOldLBX(FILE *fp, long lbxlen, char *lbxname)
{
  /* format used in Star Lords */
  Uint64 headlen;
  Uint64 *offset;
  int noffsets,m;

  fseek(fp,0,SEEK_SET);
  if(fread(&headlen,sizeof(headlen),1,fp)==0)
  {
    fprintf(stderr,"LBX archive header length could not be read from %s\n",lbxname);
    fclose(fp);
    return(1);
  }
  headlen=SwapLE64(headlen);
  noffsets=headlen/sizeof(Uint64);

  printf("%s contains %u files\n",lbxname,noffsets-1);

  offset=(Uint64 *)malloc(headlen);
  if(offset==NULL)
  {
    fprintf(stderr,"Could not allocate offsets buffer.\n");
    fclose(fp);
    return(1);
  }

  fseek(fp,0,SEEK_SET);
  fread(offset,sizeof(Uint64),noffsets,fp);

  if(DEBUG)
    printf("Offsets: ");
  for(m=0; m<=noffsets; m++)
  {
    offset[m]=SwapLE64(offset[m]);
    if(DEBUG)
      printf("0x%016llx ",offset[m]);
    if(offset[m]>UINT32_MAX)
    {
      fprintf(stderr,"Offset %d in %s is too large for 32-bit LBX archive, skipping archive.\n",m,lbxname);
      free(offset);
      fclose(fp);
      return(1);
    }
  }
  if(DEBUG)
    printf("\n");

  if(offset[noffsets-1]!=lbxlen)
    fprintf(stderr,"Invalid LBX archive: last offset does not match file length, continuing anyway. Some files may be corrupt.\n");

  for(m=0; m<noffsets-1; m++)
    ExtractFile(fp,(Uint32)offset[m],offset[m+1]-offset[m],m,lbxname,m==noffsets-2);

  free(offset);
  fclose(fp);
  return(0);
}

int ExtractFile(FILE *fp, Uint32 offset, Sint32 len, int m, char *lbxname, int final)
{
  char fname[256];
  FILE *fpout;

  if(len==0)
  {
    printf("Skipping empty file %04u\n",m);
    return(1);
  }
  else if(len<0)
  {
    fprintf(stderr,"Encountered invalid file %04u length of %d.\n",m,len);
    return(1);
  }

  if(IsWAVE(fp,offset)==0)
    sprintf(fname,"%s_%04u.wav",lbxname,m);
  else if(IsMIDI(fp,offset)==0)
    sprintf(fname,"%s_%04u.mid",lbxname,m);
  else
    sprintf(fname,"%s_%04u",lbxname,m);

  fpout=fopen(fname,"w");
  if(fpout==NULL)
  {
    fprintf(stderr,"Could not open output file %s for writing.\n",fname);
    return(1);
  }

  printf("Writing %s of %u bytes\n",fname,len);
  if(fseek(fp,offset,SEEK_SET)!=0)
  {
    fprintf(stderr,"Could not seek to offset %u in input file.\n",offset);
    return(-1);
  }

  if(final) /* Keep going 'till end of file */
  {
    while(!feof(fp))
    {
      int size=fread(writebuf,1,sizeof(writebuf),fp);
      fwrite(writebuf,1,size,fpout);
    }
  }
  else
  {
    while(len>0)
    {
      int size=fread(writebuf,1,MIN(len,sizeof(writebuf)),fp);
      fwrite(writebuf,1,size,fpout);
      len-=size;
    }
  }

  fclose(fpout);
  return(0);
}

int IsWAVE(FILE *fp, Uint32 offset)
{
  Uint8 magic[15];

  if(fseek(fp,offset,SEEK_SET)!=0)
    return(-1);

  fread(magic,1,sizeof(magic),fp);

  if((memcmp(magic,RIFFmagic,4)!=0)||(memcmp(magic+8,WAVEfmt,7)!=0))
    return(-1);

  return(0);
}

int IsMIDI(FILE *fp, Uint32 offset)
{
  Uint8 magic[18];

  if(fseek(fp,offset,SEEK_SET)!=0)
    return(-1);

  fread(magic,1,sizeof(magic),fp);

  if((memcmp(magic,MIDImagic1,4)!=0)||(memcmp(magic+14,MIDImagic2,4)!=0))
    return(-1);

  return(0);
}

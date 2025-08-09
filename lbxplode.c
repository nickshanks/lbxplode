#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lbx.h"

// http://www.xprt.net/~s8/prog/orion2/lbx/

#define DEBUG 0

const Uint16 LBX_MAGIC = 65197;
const Uint8 RIFFmagic[]={'R','I','F','F'};

char writebuf[0x10000];

static inline Uint16 SwapLE16(Uint16 x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return x;
#else
  return (x >> 8) | (x << 8);
#endif
}

static inline Uint32 SwapLE32(Uint32 x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return x;
#else
  return (x >> 24) | ((x & 0x00FF0000) >> 8) |
    ((x & 0x0000FF00) << 8) | (x << 24);
#endif
}

int ParseLBX(char *lbxname);
int ExtractFile(FILE *fp, Uint32 offset, Sint32 len, int m, char *lbxname, int final);

/* 0 on true, -1 on false or error */
int IsRIFF(FILE *fp, Uint32 offset);


int main(int argc, char *argv[])
{
  int n;

  if(argc<=1)
  {
    fprintf(stderr,"Usage: lbxplode <file1.lbx> <file2.lbx> ...\n");
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
    fprintf(stderr,"File %s is too small to be a valid LBX archive.\n",lbxname);
    fclose(fp);
    return(1);
  }

  fseek(fp,0,SEEK_SET);
  if(fread(&header,sizeof(header),1,fp)==0)
  {
    fprintf(stderr,"LBX archive header could not be read from %s\n",lbxname);
    fclose(fp);
    return(1);
  }

  header.files=SwapLE16(header.files);
  header.magic=SwapLE16(header.magic);

  if(header.magic!=LBX_MAGIC)
  {
    fprintf(stderr,"File %s is not an LBX archive, skipping.\n",lbxname);
    fclose(fp);
    return(1);
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

  if(IsRIFF(fp,offset)>=0)
    sprintf(fname,"%s_%04u.wav",lbxname,m);
  else
    sprintf(fname,"%s_%04u",lbxname,m);

  fpout=fopen(fname,"w");
  if(fpout==NULL)
  {
    fprintf(stderr,"Could not open output file %s for writing.\n",fname);
    return(1);
  }

  printf("Writing %s\n",fname);
  if(fseek(fp,offset,SEEK_SET)!=0)
  {
    fprintf(stderr,"Could not seek to offset %u in input file.\n",offset);
    return(-1);
  }

  if(final) /* Keep going 'till end of file */
  {
    while(!feof(fp))
    {
      int size=fread(writebuf,1,0xffff,fp);
      fwrite(writebuf,1,size,fpout);
    }
  }
  else
  {
    while(len>0)
    {
      int size=fread(writebuf,1,len>0xffff?0xffff:len,fp);
      fwrite(writebuf,1,size,fpout);
      len-=size;
    }
  }

  fclose(fpout);
  return(0);
}

int IsRIFF(FILE *fp, Uint32 offset)
{
  Uint8 magic[15];
  Uint8 WAVEfmt[7]={'W','A','V','E','f','m','t'};

  if(fseek(fp,offset,SEEK_SET)<0) return(-1);

  fread(magic,1,15,fp);

  if((memcmp(magic,RIFFmagic,4)==0)&&(memcmp(magic+8,WAVEfmt,7)==0))
    return(0);
  else
    return(-1);
}

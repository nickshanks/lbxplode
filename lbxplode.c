#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lbx.h"

// http://www.xprt.net/~s8/prog/orion2/lbx/

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

int DumpToFile(FILE *fpout, FILE *fpin, Uint32 offset, Sint32 len);

/* 0 on true, -1 on false or error */
int IsRIFF(FILE *fp, Uint32 offset);


int main(int argc, char *argv[])
{

  if(argc>1)
  {
    int n;
    for(n=1; n<argc; n++)
    {
      LBXheader header;
      int m=0;
      FILE *fp=fopen(argv[n],"rb");
      Uint32 *offset=NULL;

      if(fp==NULL)
      {
        fprintf(stderr,"Couldn't open %s\n",argv[n]);
        continue;
      }

      if(fread(&header,1,sizeof(header),fp)==0)
      {
        fprintf(stderr,"No header in %s\n",argv[n]);
        fclose(fp);
        continue;
      }

      header.files=SwapLE16(header.files);
      header.magic=SwapLE16(header.magic);

      if(header.magic!=LBX_MAGIC)
      {
        fprintf(stderr,"Not an LBX archive!\n");
        fclose(fp);
        continue;
      }

      printf("%s contains %u files\n",argv[n],header.files);

      offset=(Uint32 *)malloc(sizeof(Uint32)*(header.files+1));
      if(offset==NULL)
      {
        fprintf(stderr,"Couldn't allocate offset\n");
        fclose(fp);
        return(1);
      }

      printf("Offsets: ");
      for(m=0; m<=header.files; m++)
      {
        fread(offset+m,1,sizeof(Uint32),fp);
        offset[m]=SwapLE32(offset[m]);
        printf("0x%08x ",offset[m]);
      }
      printf("\n");

      for(m=0; m<header.files; m++)
      {
        char fname[256];
        FILE *fpout=NULL;
        if(IsRIFF(fp,offset[m])>=0)
          sprintf(fname,"%s_%04d.wav",argv[n],m);
        else
          sprintf(fname,"%s_%04d",argv[n],m);

        fpout=fopen(fname,"w");
        if(fpout==NULL) continue;

        printf("Dumping %s\n",fname);
        if(m+1<header.files)
          DumpToFile(fpout,fp,offset[m],offset[m+1]-offset[m]);
        else
          DumpToFile(fpout,fp,offset[m],0);

        fclose(fpout);
      }

      if(offset!=NULL) free(offset);
      fclose(fp);
    }
  }
  else
  {
    fprintf(stderr,"No files specified\n");
  }

  return(0);
}

int DumpToFile(FILE *fpout, FILE *fpin, Uint32 offset, Sint32 len)
{
  if(fseek(fpin,offset,SEEK_SET)<0)
  {
    return(-1);
  }

  if(len==0) /* Keep going 'till end of file */
  {
    while(!feof(fpin))
    {
      int len=fread(writebuf,1,0xffff,fpin);
      fwrite(writebuf,1,len,fpout);
    }
  }
  else
  {
    while(len>0)
    {
      int max;
      if(len>0xffff) max=0xffff;
      else           max=len;

      max=fread(writebuf,1,max,fpin);
      fwrite(writebuf,1,max,fpout);
      len-=max;
    }
  }

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

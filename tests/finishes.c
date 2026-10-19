#include "finishes.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

const FinishChoice finish_choices[]={
  {1,"wall",0xc86432,"sample",1,1}, {2,"floor",0x6496c8,"sample",1,1}
};
const int finish_choice_count=2;
static void write_file(const char *path,const u8 *data,size_t n) {
  FILE *f=fopen(path,"wb");assert(f);assert(fwrite(data,1,n,f)==n);assert(!fclose(f));
}
int main(void) {
  const RoomTile tile={"page",0,0,3,2};
  const RoomBackground bg={.model="test",.x=-7,.y=-3,.tile_count=1,.tiles=&tile};
  const u8 mask[]={3,0,2,0,1,0,3,0,1,0,4,0,1,0,1,0,1,0,2,0,1,0,5,0,1,0,0,0};
  const u8 texture[]={255,255,255};
  write_file("test.mask",mask,sizeof mask);write_file("sample.rgb",texture,sizeof texture);
  u8 px[32];memset(px,77,sizeof px);
  assert(finishes_paint(".",&bg,0,"wall","floor",px,3,2,16));
  const u8 expected[]={184,84,34,77,200,100,50,77,100,150,200,77,77,77,77,77,
    67,100,134,77,100,150,200,77,77,77,77,77,77,77,77,77};
  assert(!memcmp(px,expected,sizeof px));
  assert(!finishes_paint(".",&bg,0,"wall","floor",px,2,2,16));
  assert(!finishes_paint(".",&bg,0,"wall","floor",px,3,2,8));
  write_file("test.mask",mask,sizeof mask-1);
  assert(!finishes_paint(".",&bg,0,"wall","floor",px,3,2,16));
  u8 bad[sizeof mask];memcpy(bad,mask,sizeof bad);bad[4]=0;write_file("test.mask",bad,sizeof bad);
  assert(!finishes_paint(".",&bg,0,"wall","floor",px,3,2,16));
  write_file("sample.rgb",texture,2);
  assert(!finishes_paint(".",&bg,0,"wall","floor",px,3,2,16));
  memset(px,77,sizeof px);assert(finishes_paint(".",&bg,0,"","",px,3,2,16));
  for(size_t i=0;i<sizeof px;i++)assert(px[i]==77);
  puts("OK: finish shading, alpha/stride preservation, negative UV wrapping, original restoration and malformed assets.");
}

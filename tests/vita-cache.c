#include <assert.h>
#include <stdio.h>
typedef void vita2d_texture;
#include "../../Tweebbo-Vita/source/vita/furni-cache.h"

int main(void) {
  FurniTex entries[512]={0};
  for(int i=0;i<512;i++) {
    FurniTex *slot=furni_slot(entries,512,(u16)i,0);
    assert(!slot->used);
    *slot=(FurniTex){.used=true,.def=(u16)i,.stamp=(u32)i+1,.tex=(void *)1};
  }
  for(int i=0;i<512;i++)assert(furni_slot(entries,512,(u16)i,0)->def==i);
  furni_slot(entries,512,0,0)->stamp=513;
  assert(furni_slot(entries,512,512,0)->def==1);
  entries[300].used=false;
  assert(furni_slot(entries,512,512,0)==&entries[300]);
  assert(furni_slot(entries,512,0,0)==&entries[0]);
  assert(furni_oldest(entries,512,&entries[1])==&entries[2]);
  entries[1].tex=NULL;
  assert(furni_oldest(entries,512,NULL)==&entries[2]);
  puts("OK: Vita fills all 512 texture slots, preserves hits and evicts the least recently used entry.");
}

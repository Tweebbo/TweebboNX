#include "audio.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "official_music_limits.h"
static void official_segment(const s8 *pcm,size_t skip,bool loop){
  s16 output[4096*2];audio_clear();assert(audio_play(12,pcm,OFFICIAL_MUSIC_BYTES,8,16000,loop,skip));audio_mix(output,4096);unsigned nonzero=0;
  for(size_t frame=0;frame<4096;frame++){
    size_t sample=skip+frame/3;if(loop)sample%=OFFICIAL_MUSIC_BYTES;
    int expected=sample<OFFICIAL_MUSIC_BYTES?(int)pcm[sample]*256/3:0;
    assert(output[frame*2]==expected&&output[frame*2+1]==expected);nonzero+=expected!=0;
  }
  assert(nonzero>0);audio_clear();
}
int main(int argc,char **argv){
  const s16 wave[]={3000,-3000,6000,-6000};s16 out[16];
  assert(audio_play(0,wave,4,16,48000,false,0));audio_mix(out,4);
  assert(out[0]==1000&&out[1]==1000&&out[2]==-1000&&out[4]==2000&&!audio_busy(0));
  audio_mix(out,2);assert(!out[0]&&!out[2]);
  assert(audio_play(0,wave,4,16,24000,true,1));audio_mix(out,8);
  assert(out[0]==-1000&&out[2]==-1000&&out[4]==2000&&out[12]==1000&&audio_busy(0));
  audio_mute(true);audio_mix(out,2);assert(!out[0]&&!out[1]);audio_mute(false);audio_clear();
  const s8 pcm[]={-128,127};assert(audio_play(8,pcm,2,8,48000,false,0));audio_mix(out,2);assert(out[0]==-10922&&out[2]==10837);
  assert(audio_play(12,pcm,2,8,48000,false,0));audio_mix(out,2);assert(out[0]==-10922&&out[2]==10837);
  assert(!audio_play(14,wave,4,16,48000,false,0));assert(!audio_play(0,wave,4,16,48000,false,4));
  audio_clear();
  if(argc==2){
    FILE *source=fopen(argv[1],"rb");assert(source);s8 *pcm=malloc(OFFICIAL_MUSIC_BYTES);assert(pcm);assert(fread(pcm,1,OFFICIAL_MUSIC_BYTES,source)==OFFICIAL_MUSIC_BYTES&&fgetc(source)==EOF);fclose(source);
    assert(!audio_play(0,pcm,OFFICIAL_MUSIC_BYTES,8,16000,false,0));assert(!audio_play(12,pcm,OFFICIAL_MUSIC_BYTES-1,8,16000,false,0));assert(!audio_play(12,pcm,OFFICIAL_MUSIC_BYTES,16,16000,false,0));assert(!audio_play(12,pcm,OFFICIAL_MUSIC_BYTES,8,22050,false,0));
    official_segment(pcm,0,false);official_segment(pcm,12*16000,false);official_segment(pcm,OFFICIAL_MUSIC_BYTES-64,true);free(pcm);
    puts("PASS: verified official PCM full length accepted only for its dedicated PCM8 channel/rate; actual mixed samples match the source at start,12-second seek and loop wrap.");
  }
  puts("OK: PCM8/16 mixing, stereo, resampling, looping, seek, end, mute and channel bounds.");
}

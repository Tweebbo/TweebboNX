#include "audio.h"
#include <assert.h>
#include <stdio.h>
int main(void){
  const s16 wave[]={3000,-3000,6000,-6000};s16 out[16];
  assert(audio_play(0,wave,4,16,48000,false,0));audio_mix(out,4);
  assert(out[0]==1000&&out[1]==1000&&out[2]==-1000&&out[4]==2000&&!audio_busy(0));
  audio_mix(out,2);assert(!out[0]&&!out[2]);
  assert(audio_play(0,wave,4,16,24000,true,1));audio_mix(out,8);
  assert(out[0]==-1000&&out[2]==-1000&&out[4]==2000&&out[12]==1000&&audio_busy(0));
  audio_mute(true);audio_mix(out,2);assert(!out[0]&&!out[1]);audio_mute(false);audio_clear();
  const s8 pcm[]={-128,127};assert(audio_play(8,pcm,2,8,48000,false,0));audio_mix(out,2);assert(out[0]==-10922&&out[2]==10837);
  assert(!audio_play(12,wave,4,16,48000,false,0));assert(!audio_play(0,wave,4,16,48000,false,4));
  audio_clear();puts("OK: PCM8/16 mixing, stereo, resampling, looping, seek, end, mute and channel bounds.");
}

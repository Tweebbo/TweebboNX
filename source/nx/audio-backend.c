#include "audio.h"
#include <SDL.h>
static SDL_AudioDeviceID device;
static void mix(void *unused,Uint8 *out,int bytes){(void)unused;audio_mix((s16 *)out,bytes/4);}
bool audio_init(void){
  if(device)return true;
  if(SDL_InitSubSystem(SDL_INIT_AUDIO)<0)return false;
  SDL_AudioSpec wanted={0},got;
  wanted.freq=48000;wanted.format=AUDIO_S16SYS;wanted.channels=2;wanted.samples=1024;wanted.callback=mix;
  device=SDL_OpenAudioDevice(NULL,0,&wanted,&got,0);
  if(!device)return false;
  SDL_PauseAudioDevice(device,0);return true;
}
void audio_exit(void){if(device)SDL_CloseAudioDevice(device);device=0;audio_snow_free();audio_clear();}

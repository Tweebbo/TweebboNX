/* Drives the shipped music window against the private paid-disc room. */
#include "client_music.h"
#include "official_music.h"
#include <mbedtls/sha256.h>
static bool official_audio_requested,official_cache_played;
static int official_audio_job;
int __real_net_http(const char *method,const char *path,const char *json,bool auth);
int __wrap_net_http(const char *method,const char *path,const char *json,bool auth){
    int job=__real_net_http(method,path,json,auth);
    if(job&&!strcmp(method,"GET")&&!strcmp(path,"/static/trax/3ds/classic_73.pcm")){
        official_audio_requested=true;official_audio_job=job;dbg_log("Official disc PCM requested through shipped net_http: %s",path);
    }
    return job;
}
bool __real_net_http_result(int id,HttpResult *out);
bool __wrap_net_http_result(int id,HttpResult *out){
    bool done=__real_net_http_result(id,out);
    if(done&&id==official_audio_job){
        static const unsigned char expected[32]={0x93,0x83,0x93,0xf4,0x7e,0xe6,0xb6,0x0a,0xfa,0x6c,0x64,0x29,0xef,0xee,0xad,0x5b,0x15,0x5f,0x81,0x03,0xbc,0xdf,0x6c,0x86,0x05,0x70,0xf1,0xf3,0xad,0xe1,0xb8,0x0d};
        unsigned char digest[32];assert(out->status==200&&out->body&&out->len==OFFICIAL_MUSIC_BYTES);
        assert(mbedtls_sha256_ret((const unsigned char*)out->body,out->len,digest,0)==0&&!memcmp(digest,expected,32));
        dbg_log("PASS: actual official HTTP response bytes=%zu and exact source SHA-256.",(size_t)out->len);
    }
    return done;
}
bool __real_audio_play(int channel,const void *data,size_t samples,int bits,int rate,bool loop,size_t skip);
bool __wrap_audio_play(int channel,const void *data,size_t samples,int bits,int rate,bool loop,size_t skip){
    bool played=__real_audio_play(channel,data,samples,bits,rate,loop,skip);
    if(official_audio_requested&&channel==12){assert(samples==OFFICIAL_MUSIC_BYTES&&bits==8&&rate==16000&&!loop);if(played&&!official_cache_played)dbg_log("PASS: full official cached PCM accepted by actual audio mixer, bytes=%zu skip=%zu.",samples,skip);official_cache_played=played;}
    return played;
}
static const char *official_disc_exercise(u64 elapsed){
    static unsigned step;static u64 next;static u32 baseline;static unsigned long audio_before;
    if(!W.in_room||!net_online()||elapsed<1200||elapsed<next)return NULL;
    assert(W.guest_id==19&&!strcmp(W.slug,"official-disc-lab")&&W.room_owner);
    if(elapsed>25000){dbg_log("Official disc exercise timed out at step %u",step);assert(false);}
    next=elapsed+350;
    json_t *queues=json_object_get(host_music,"queues"),*queue=json_object_get(queues,"260");
    switch(step){
        case 0:baseline=W.credits;send_buy(40086);step++;return NULL;
        case 1:if(W.credits!=baseline-1)return NULL;send_use(260);client_ui_music();step++;return NULL;
        case 2:if(!json_is_object(host_music))return NULL;step++;return "official-disc-owned";
        case 3:client_music_action(11009,0);step++;return NULL;
        case 4:if(json_array_size(queue)!=1)return NULL;assert(!strcmp(host_string(json_array_get(queue,0),"provider"),"official"));step++;return "official-disc-loaded";
        case 5:audio_before=atomic_load(&host_audio_frames);client_music_action(11003,0);step++;return NULL;
        case 6:if(!json_is_true(json_object_get(host_music,"playing"))||!official_audio_requested||!official_cache_played||atomic_load(&host_audio_frames)<=audio_before+1000)return NULL;assert(json_integer_value(json_object_get(json_array_get(queue,0),"duration"))==71837);step++;return "official-disc-playing";
        case 7:client_music_action(11010,0);step++;return NULL;
        case 8:if(json_array_size(queue)!=0)return NULL;step++;return "official-disc-returned";
        default:ui_exercise_done=true;dbg_log("PASS: real paid source disc purchase, native owned-disc group/add/play/remove UI, source71837ms packet, classic_73 HTTP request, and nonzero host PCM mix; physical console audio remains unverified.");return NULL;
    }
}

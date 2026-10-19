#include "../source/client_trade.c"
#include <assert.h>
World W;
static bool online=true;
static u64 clock_ms;
static int sends;
static char last[512];
u64 now_ms(void){return clock_ms;}
bool net_online(void){return online;}
Ent *world_ent(u32 id){return id==2?&W.ents[1]:NULL;}
bool client_social_blocked(u32 guest){(void)guest;return false;}
void send_trade(const char *s){sends++;snprintf(last,sizeof last,"%s",s);}
void client_ui_changed(void){}
void show_toast(const char *s,u64 now){(void)s;(void)now;}
void gfx_furni_preload(u16 def){(void)def;}
static void update(const char *stage,int revision,bool ok,bool peer_ok,bool final,const char *mine,const char *theirs){
  static char payload[2048];
  snprintf(payload,sizeof payload,"{\"stage\":\"%s\",\"revision\":%d,\"peer\":2,\"peerName\":\"Other player\",\"okMine\":%s,\"okPeer\":%s,\"confirming\":%s,\"mine\":%s,\"theirs\":%s}",stage,revision,ok?"true":"false",peer_ok?"true":"false",final?"true":"false",mine,theirs);
  W.trade_json=payload;W.trade_rev++;client_trade_update(true);
}
int main(void){
  W.in_room=true;W.me=1;W.guest_id=1;W.ents[1]=(Ent){.id=2,.guest=2,.used=true};
  W.inv[0]=(InvItem){100,1};W.inv[1]=(InvItem){101,2};W.inv_n=2;
  client_trade_reset();client_trade_invite(2);assert(sends==1 && T.outgoing);
  client_trade_invite(2);assert(sends==1);
  update("invite",1,false,false,false,"[]","[]");assert(!T.pending);
  client_trade_action(ACCEPT,0);assert(sends==1);
  update("open",1,false,false,false,"[]","[]");client_trade_action(ADD,100);assert(sends==2);
  client_trade_action(ADD,101);assert(sends==2);
  update("open",2,false,false,false,"[{\"id\":100,\"def\":1}]","[]");assert(!T.pending);
  client_trade_action(READY,0);assert(strstr(last,"\"revision\":2") && strstr(last,"ready"));
  update("open",2,false,true,false,"[{\"id\":100,\"def\":1}]","[]");assert(T.pending);
  update("open",2,true,true,false,"[{\"id\":100,\"def\":1}]","[]");assert(!T.pending);
  update("open",3,false,false,true,"[{\"id\":100,\"def\":1}]","[]");
  client_trade_action(READY,0);assert(strstr(last,"confirm") && strstr(last,"\"revision\":3"));
  int before=sends;client_trade_action(READY,0);assert(sends==before);
  update("open",4,false,false,false,"[{\"id\":100,\"def\":1},{\"id\":101,\"def\":2}]","[]");assert(!T.pending && !T.confirming && !T.ok_mine);
  client_trade_action(READY,0);assert(strstr(last,"ready") && strstr(last,"\"revision\":4"));
  clock_ms=11000;client_trade_update(true);assert(strstr(T.note,"No reply"));before=sends;client_trade_action(READY,0);assert(sends==before);
  client_trade_close();assert(strstr(last,"cancel"));update("cancelled",4,false,false,false,"[]","[]");assert(T.stage==ENDED && !T.pending);
  client_trade_reset();update("invite",1,false,false,false,"[]","[]");client_trade_action(ACCEPT,0);assert(strstr(last,"accept"));
  update("open",1,false,false,false,"[]","[]");online=false;client_trade_update(true);before=sends;client_trade_action(ADD,100);assert(before==sends);
  online=true;client_trade_update(true);assert(strstr(last,"cancel"));
  update("open",2,false,false,false,"[{\"id\":100,\"def\":1}]","[{\"id\":100,\"def\":1}]");assert(T.invalid);
  before=sends;client_trade_action(READY,0);assert(sends==before);
  client_trade_reset();W.trade_json="{\"stage\":\"open\"}";W.trade_rev++;client_trade_update(true);assert(T.invalid);
  client_trade_close();assert(strstr(last,"cancel"));
  client_trade_reset();client_trade_invite(2);client_trade_notice("Welcome to the hotel.");assert(T.pending);
  client_trade_notice("That player needs the latest Tweebbo update on their console to trade.");assert(!T.pending && T.stage==ENDED);
  client_trade_action(DISMISS,0);assert(T.stage==EMPTY);
  puts("OK: trade invitations, two-stage revision-aware acceptance, peer updates, duplicate guards, timeout, reconnect cancellation and malformed snapshots.");
}

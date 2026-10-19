#include "../source/client_social.c"
#include <assert.h>

World W;
static int requests, result_id, response_status=200, controls;
static u64 clock_ms=100;
static const char *response;
static char last_path[800],last_body[800];
static bool online=true;
u64 now_ms(void) {return clock_ms;}
bool net_online(void) {return online;}
const char *net_token(void) {return "test-only";}
int net_http(const char *method,const char *path,const char *body,bool auth) {
  assert(auth);assert(!strcmp(method,body?"POST":"GET"));
  snprintf(last_path,sizeof last_path,"%s",path);snprintf(last_body,sizeof last_body,"%s",body?body:"");return ++requests;
}
bool net_http_result(int job,HttpResult *r) {if(job!=result_id)return false;*r=(HttpResult){.status=response_status,.body=response?strdup(response):NULL};result_id=0;return true;}
void net_goto(const char *slug,const char *arrival) {assert(!strcmp(slug,"park") && !*arrival);}
void client_ui_changed(void) {}
void client_ui_edit(const char *title,const char *initial,bool (*submit)(const char *)) {(void)title;(void)initial;(void)submit;}
void client_ui_edit_draft(const char *title,const char *initial,bool (*submit)(const char *),void (*cancel)(const char *)) {(void)title;(void)initial;(void)submit;(void)cancel;}
void show_toast(const char *s,u64 now) {(void)s;(void)now;}
void client_ui_button(float x,float y,float w,float h,const char *text,int command,int arg) {
  (void)text;(void)arg;assert(x>=0 && y>=0 && w>0 && h>0 && x+w<=480 && y+h<=272);if(command)controls++;
}
void client_ui_label(const char *s,float x,float y,float w,u32 ink) {(void)s;(void)x;(void)y;(void)w;(void)ink;}
void client_ui_field(float x,float y,float w,float h,const char *text,const char *placeholder,int command,int arg) {
  (void)text;(void)placeholder;(void)arg;assert(x>=0 && y>=0 && w>0 && h>0 && x+w<=480 && y+h<=272);if(command)controls++;
}
size_t client_ui_paragraph(const char *s,float x,float y,float w,int lines,u32 ink) {(void)x;(void)y;(void)w;(void)ink;return strlen(s)>(size_t)lines*30?(size_t)lines*30:strlen(s);}
static void complete(const char *json) {response=json;result_id=F.job;client_social_update(true);}
int main(void) {
  W.guest_id=1;client_social_reset();client_social_update(false);assert(!requests);
  client_social_open();client_social_update(true);assert(requests==1 && F.kind==LOAD);
  complete("{\"friends\":[{\"id\":2,\"name\":\"Friend\",\"room\":\"park\"}],\"incoming\":[{\"id\":3,\"name\":\"Guest\"}],\"blocked\":[{\"id\":4,\"name\":\"Blocked\"}]}");
  assert(!F.job && client_social_blocked(4) && !client_social_blocked(2));
  client_social_draw(8,8,310);assert(controls);
  client_social_action(SELECT,2);client_social_action(VISIT,0);client_social_action(CHAT,0);client_social_update(true);
  complete("{\"friends\":[{\"id\":2,\"name\":\"Friend\",\"room\":\"park\"}]}");assert(F.kind==MESSAGES && F.job);
  complete("[{\"id\":10,\"sender\":2,\"text\":\"Hello\"}]");assert(F.kind==READ && F.job);complete("{\"ok\":true}");assert(!F.job);
  assert(message_done("Quoted \"message\" ñ"));assert(F.job && F.kind==ACTION);int before=requests;
  assert(!message_done("Second message"));assert(requests==before);
  json_t *body=json_loads(last_body,0,NULL);assert(body && !strcmp(string(body,"text"),"Quoted \"message\" ñ"));json_decref(body);
  response_status=500;complete("{\"error\":\"Try later\"}");assert(*F.draft && strstr(F.status,"Try later"));response_status=200;
  message_cancelled("Unsent draft");assert(!strcmp(F.draft,"Unsent draft"));
  client_social_open();client_social_action(SELECT,2);assert(!strcmp(F.draft,"Unsent draft"));
  F.next_poll=clock_ms+1000;assert(search_done("niño /?"));assert(strstr(last_path,"%C3%B1") && strstr(last_path,"%2F%3F"));
  complete("[{\"id\":3,\"name\":\"Guest\"}]");client_social_action(SELECT,3);client_social_draw(8,8,310);
  assert(!*F.draft);
  client_social_action(BLOCK,0);assert(F.view==CONFIRM);client_social_action(CONFIRM_ACTION,0);assert(strstr(last_body,"block"));complete("{\"ok\":true}");
  client_social_reset();assert(!F.job && !F.contacts && !F.results && !F.messages && !client_social_blocked(4));
  puts("OK: social lifecycle, private message encoding, duplicate requests, error drafts, search escaping, blocking confirmation and reset.");
}

#include "client_camera.h"
static const char *camera_ui_exercise(u64 elapsed) {
  static int step,job;static u64 at;static u32 saved,baseline;
  const char *shot=NULL;
  if(elapsed<2500 || elapsed<at+700)return NULL;
  assert(W.in_room && net_online());
  switch(step) {
    case 0:for(int i=0;i<W.inv_n;i++)if(W.inv[i].id>baseline)baseline=W.inv[i].id;send_buy(10513);break;
    case 1:if(!client_camera_owned()){assert(elapsed<at+15000);return NULL;}client_ui_camera();break;
    case 2:shot="camera-ready";break;
    case 3:show_tap(80,210);break;
    case 4:assert(gfx_photo_status()==1);shot="camera-captured";break;
    case 5:show_tap(80,210);break;
    case 6:assert(gfx_photo_status()==1);show_tap(268,70);break;
    case 7:shot="camera-zoom";show_tap(210,70);break;
    case 8:show_tap(242,100);break;
    case 9:show_tap(30,96);break;
    case 10:show_tap(292,199);break;
    case 11:show_tap(244,210);break;
    case 12: {
      for(int i=0;i<W.inv_n;i++){const FurniDef *d=def_get(W.inv[i].def);if(d && !strcmp(d->name,"Photo") && W.inv[i].id>baseline)saved=W.inv[i].id;}
      if(!saved){assert(elapsed<at+25000);return NULL;}
      char path[96];snprintf(path,sizeof path,"/api/photo/item/%u/info",saved);job=net_http("GET",path,NULL,true);assert(job);shot="camera-saved";break;
    }
    case 13: {
      HttpResult r;if(!net_http_result(job,&r)){assert(elapsed<at+25000);return NULL;}
      assert(r.status==200);json_t *j=json_loads(r.body,0,NULL);assert(j);
      const char *caption=json_string_value(json_object_get(j,"caption"));assert(caption && *caption);
      json_decref(j);free(r.body);char path[96];snprintf(path,sizeof path,"/api/photo/item/%u",saved);job=net_http("GET",path,NULL,true);assert(job);break;
    }
    case 14: {
      HttpResult r;if(!net_http_result(job,&r)){assert(elapsed<at+25000);return NULL;}
      assert(r.status==200 && r.len>75000 && !memcmp(r.body,"\x89PNG\r\n\x1a\n",8));free(r.body);
      ui_exercise_done=true;dbg_log("OK: native camera capture, preview, zoom, caption, inventory save and authenticated PNG retrieval.");break;
    }
  }
  step++;at=elapsed;return shot;
}

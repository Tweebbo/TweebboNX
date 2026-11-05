/* Link with --wrap=client_ui_button to audit the actual portable form controls. */
#include "furniture.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void __real_client_ui_button(float x,float y,float w,float h,const char *text,int cmd,int arg);
void __wrap_client_ui_button(float x,float y,float w,float h,const char *text,int cmd,int arg){
  static u32 revision;
  if(revision!=furniture_panel.revision){
    revision=furniture_panel.revision;
    fprintf(stderr,"Form trace: revision=%u item=%u kind=%d editable=%d fields=%d definition=%d\n",
      revision,furniture_panel.item,furniture_panel.kind,furniture_panel.editable,furniture_panel.field_count,furniture_panel.variable_definition);
  }
  if(cmd==17001){
    assert(x>=0&&y>=0&&x+w<=SCREEN_W&&y+h<=SCREEN_H);
    static u32 saved_revision;
    if(saved_revision!=revision){saved_revision=revision;fprintf(stderr,"Save trace: revision=%u rect=%.0f,%.0f,%.0f,%.0f\n",revision,x,y,w,h);}
  }
  __real_client_ui_button(x,y,w,h,text,cmd,arg);
}

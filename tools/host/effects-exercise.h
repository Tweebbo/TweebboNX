/* Every effect and direction through the portable renderer, using source frame 3. */
#include "avatar.h"
#include "avatar_effects.h"
static const char *effects_render_frame(bool swimming){
  static unsigned sample;
  static char name[72];
  unsigned total=avatar_fx_animation_count*16;
  if(sample>=total)return NULL;
  const AvatarFxAnimation *effect=&avatar_fx_animations[sample/16];
  int direction=(sample/2)%8;bool moving=sample%2;
  gfx_sprite_transform(0,0,1,1);gfx_filter(false);
  fill_rect(0,0,SCREEN_W,SCREEN_H,0xff15110c);
  if(!swimming)avatar_effect_shadow(effect->id,direction,96,160,1);
  avatar_effect_layers(effect->id,direction,3,moving,96,160,1,true);
  if(swimming)avatar_draw_swim_effected(0,direction,moving?POSE_SWIM:POSE_FLOAT,3,0,direction,96-SWIM_ANCHOR_X,160-ANCHOR_Y,1,effect->id);
  else avatar_draw_effected(0,direction,moving?POSE_WALK:POSE_STD,3,0,direction,0,false,96-ANCHOR_X,160-ANCHOR_Y,1,255,effect->id,3);
  avatar_effect_layers(effect->id,direction,3,moving,96,160,1,false);
  snprintf(name,sizeof name,"%seffect-%u-%d-%s",swimming?"swim-":"",effect->id,direction,moving?"moving":"still");
  if(++sample==total){ui_exercise_done=true;dbg_log("OK: %u effects in all eight directions, standing and moving, rendered and captured.",avatar_fx_animation_count);}
  return name;
}

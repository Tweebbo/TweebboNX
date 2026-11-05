/* Exercises the shipped client against the private wired-live-scene rooms. */
#include "wired_chests.h"
static const char *wired_live_exercise(u64 elapsed){
    static unsigned step,prepared;static u64 next;static int original_x;
    if(!W.in_room||elapsed<1200||elapsed<next)return NULL;
    next=elapsed+500;
    const char *mode=getenv("UI_EXERCISE");
    if(strstr(mode,"services")){
        switch(step++){
            case 0:assert(W.room_owner&&world_item(211));return "live-services-room";
            case 1:send_use(211);return NULL;
            case 2:if(wired_chest_panel.kind!=WC_CHEST){step--;return NULL;}return "live-coin-chest";
            case 3:client_ui_close();send_use(218);return NULL;
            case 4:if(wired_chest_panel.kind!=WC_CONTRACT){step--;return NULL;}return "live-trade-contract";
            case 5:client_ui_close();send_use(219);return NULL;
            case 6:if(wired_chest_panel.kind!=WC_LABEL){step--;return NULL;}return "live-label-editor";
            case 7:client_ui_close();send_use(220);return NULL;
            case 8:if(wired_chest_panel.kind!=WC_LINK){step--;return NULL;}return "live-link-editor";
            default:client_ui_close();ui_exercise_done=true;dbg_log("PASS: actual server C_USE opens coin chest, contract, label and Link through shipped native service routes; no financial confirmations.");return NULL;
        }
    }
    if(strstr(mode,"bots")){
        Ent *bot=NULL;for(int n=0;n<MAX_ENTS;n++)if(W.ents[n].used&&!strcmp(W.ents[n].name,"Lab Robbie")){bot=&W.ents[n];break;}
        if(!prepared){if(!bot)return NULL;send_furniture("{\"op\":\"bot_pickup\",\"bot\":1}");prepared=1;return NULL;}
        if(prepared==1){if(bot)return NULL;send_furniture("{\"op\":\"bot_place\",\"bot\":1,\"x\":4,\"y\":3}");prepared=2;return NULL;}
        if(!bot)return NULL;
        switch(step++){
            case 0:return "live-bots-room";
            case 1:send_chat("hello bot");return NULL;
            case 2:return "live-bot-wired-chat";
            case 3:original_x=bot->x;send_chat("move bot");return NULL;
            case 4:if(bot->x==original_x){step--;return NULL;}return "live-bot-wired-movement";
            case 5:send_chat("follow me");return NULL;
            case 6:return "live-bot-wired-follow";
            default:ui_exercise_done=true;dbg_log("PASS: actual native room bot identity, Wired chat and movement messages, and follow request.");return NULL;
        }
    }
    switch(step++){
        case 0:assert(world_item(248));return "live-effects-room";
        case 1:send_chat("glow");return NULL;
        case 2:return "live-wired-highlight";
        case 3:send_chat("move");next=elapsed+120;return NULL;
        case 4:{const Item *it=world_item(248);if((it->curve_style&7)!=7){step--;next=elapsed+50;return NULL;}assert((it->curve_style&CURVE_FULL)&&item_move_ms(it)==500);return "live-wired-jump";}
        default:ui_exercise_done=true;dbg_log("PASS: actual native highlight and packet157 full-style jump from a saved server Wired stack.");return NULL;
    }
}

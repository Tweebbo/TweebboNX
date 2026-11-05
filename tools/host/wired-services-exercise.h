/* Local UI fixtures exercise the actual shipped panels; requests are tested by the controller harness. */
#include "wired_services.h"
#include "wired_boards.h"
static const char *wired_services_ui_exercise(u64 elapsed){
    static unsigned step;static u64 next;
    if(elapsed<next||!W.in_room||step>=17)return NULL;
    next=elapsed+700;
    switch(step++){
        case 0:if(!W.guest_id)W.guest_id=1;W.owner=W.guest_id;W.room_owner=W.room_rights=true;snprintf(W.model,sizeof W.model,"model_a");snprintf(W.slug,sizeof W.slug,"services-ui-fixture");snprintf(W.title,sizeof W.title,"Local UI fixture room");show_room_changed(now_ms());client_ui_services(WS_BOTS);on_furniture("{\"op\":\"bots\",\"offers\":[{\"id\":11746,\"name\":\"Robbie\",\"price\":25,\"kind\":0},{\"id\":11782,\"name\":\"Skye\",\"price\":40,\"kind\":1}],\"bots\":[{\"id\":900001,\"name\":\"Local UI fixture\",\"room\":null,\"kind\":0}]}");return NULL;
        case 1:return "wired-services-bots-list";
        case 2:show_tap(163,74);assert(wired_service_panel.view==WS_DETAIL);return "wired-services-bot-details";
        case 3:show_tap(80,125);assert(wired_service_panel.view==WS_POSITION);return "wired-services-bot-position";
        case 4:show_tap(237,125);return "wired-services-bot-tile";
        case 5:show_tap(80,177);assert(wired_service_panel.view==WS_CONFIRM);return "wired-services-bot-confirm";
        case 6:show_tap(60,210);assert(wired_service_panel.view==WS_POSITION);client_ui_close();return NULL;
        case 7:client_ui_services(WS_GROUPS);on_furniture("{\"op\":\"groups\",\"canCreate\":true,\"clubActive\":true,\"price\":150,\"groups\":[]}");return NULL;
        case 8:show_tap(70,186);assert(wired_service_panel.view==WS_FORM);assert(wired_services_edit(0,"Local UI fixture"));assert(wired_services_edit(1,"A local interface test. No group is purchased."));return "wired-services-group-form";
        case 9:show_tap(80,175);assert(wired_service_panel.view==WS_CONFIRM);assert(strstr(wired_service_panel.body,"150 credits"));return "wired-services-group-confirm";
        case 10:client_ui_close();client_ui_services(WS_QUESTS);on_furniture("{\"op\":\"quests\",\"quests\":[{\"id\":80,\"name\":\"Place furniture\",\"category\":\"Local UI fixture\",\"target\":5,\"progress\":2,\"reward\":20,\"active\":false,\"available\":true,\"completedSteps\":1,\"totalSteps\":4}]}");return NULL;
        case 11:show_tap(163,74);assert(wired_service_panel.view==WS_DETAIL);return "wired-services-quest-details";
        case 12:client_ui_close();client_ui_boards();on_furniture("{\"type\":\"wired_leaderboards\",\"boards\":[{\"overlay\":90,\"revision\":1,\"title\":\"Local UI fixture board\",\"description\":\"Read-only local rendering test\",\"default\":77,\"choices\":[{\"id\":77,\"name\":\"Score\"},{\"id\":78,\"name\":\"Level\"}]}]}");return NULL;
        case 13:return "wired-services-board-list";
        case 14:show_tap(163,74);on_furniture("{\"type\":\"wired_leaderboard_rows\",\"overlay\":90,\"revision\":1,\"variable\":77,\"chunk\":0,\"total\":100,\"rows\":[{\"rank\":1,\"name\":\"Local avatar\",\"value\":\"9223372036854775807\"},{\"rank\":2,\"name\":\"Another local avatar\",\"value\":\"9007199254740993\"}]}");return NULL;
        case 15:assert(wired_board_panel.rows==2);return "wired-services-board-scores";
        default:ui_exercise_done=true;dbg_log("PASS: shipped native bot, group, quest and leaderboard UI routes, explicit review/cancel, exact score labels and compact layout completed; local fixtures only.");return NULL;
    }
}





#include "../xbg_wtrack_switch.h"
#include <stdio.h>

static int failures;
#define CHECK(expression) do { if(!(expression)){printf("FAIL line %d: %s\n",__LINE__,#expression);++failures;} } while(0)

int main(){
    const int none=XBG_WTRACK_NO_MARKER,empty=-1;


    CHECK(xbg_wtrack_marker(none,false,nullptr)==-1);
    CHECK(xbg_wtrack_enabled(-1,true) && !xbg_wtrack_enabled(-1,false));


    CHECK(xbg_wtrack_marker(none,false,"0")==0 && !xbg_wtrack_enabled(0,true));
    CHECK(xbg_wtrack_marker(none,false,"1")==1 && xbg_wtrack_enabled(1,true) && xbg_wtrack_enabled(1,false));
    CHECK(xbg_wtrack_marker(none,false,"2")==1);
    CHECK(xbg_wtrack_marker(none,false,"")==0 && xbg_wtrack_marker(none,false,"off")==0);


    CHECK(xbg_wtrack_marker('1',false,"0")==1);
    CHECK(xbg_wtrack_marker('0',false,"1")==0);
    CHECK(xbg_wtrack_marker(empty,false,"1")==0);
    CHECK(xbg_wtrack_marker('x',false,nullptr)==0);

    CHECK(xbg_wtrack_marker('1',true,"1")==0);
    CHECK(xbg_wtrack_marker(none,true,"1")==0);
    CHECK(xbg_wtrack_marker(none,true,nullptr)==0);
    if(failures)return 1;
    puts("PASS wtrack switch: marker file before DM_GFX_WTRACK, gfx-no-wtrack overrides, host hooks enable all but 0");
    return 0;
}



#include "../xbg_adreno_policy.h"
#include <stdio.h>

static int failures;
#define CHECK(expression) do { if(!(expression)){printf("FAIL line %d: %s\n",__LINE__,#expression);++failures;} } while(0)

int main(){


    CHECK(xbg_adreno_model(nullptr)==0);
    CHECK(xbg_adreno_model("")==0);
    CHECK(xbg_adreno_model("Adreno")==0);
    CHECK(xbg_adreno_model("Adreno (TM) 730")==730);
    CHECK(xbg_adreno_model("Adreno (TM) 740")==740);
    CHECK(xbg_adreno_model("Adreno 750")==750);
    CHECK(xbg_adreno_model("adreno 740")==740);
    CHECK(xbg_adreno_model("Adreno (TM) 830")==830);
    CHECK(xbg_adreno_model("Adreno 642L")==642);
    CHECK(xbg_adreno_model("Adreno 740 (v2)")==740);
    CHECK(xbg_adreno_model("Adreno X1-85")==0);
    CHECK(xbg_adreno_model("Adreno 7400")==0);
    CHECK(xbg_adreno_model("Mali-G715")==0);



    CHECK(xbg_adreno_rewrites_fault(0x5143,"Adreno (TM) 740"));
    CHECK(xbg_adreno_rewrites_fault(0x5143,"Adreno 750"));
    CHECK(xbg_adreno_rewrites_fault(0x5143,"Adreno (TM) 799"));
    CHECK(!xbg_adreno_rewrites_fault(0x5143,"Adreno (TM) 730"));
    CHECK(!xbg_adreno_rewrites_fault(0x5143,"Adreno (TM) 735"));
    CHECK(!xbg_adreno_rewrites_fault(0x5143,"Adreno (TM) 830"));
    CHECK(!xbg_adreno_rewrites_fault(0x5143,nullptr));
    CHECK(!xbg_adreno_rewrites_fault(0x5143,"Adreno X1-85"));
    CHECK(!xbg_adreno_rewrites_fault(0x10002,"Adreno (TM) 740"));
    CHECK(!xbg_adreno_rewrites_fault(0,"Adreno 750"));



    CHECK(xbg_adreno_compiler_needs_inline(0x5143,"Adreno (TM) 750"));
    CHECK(xbg_adreno_compiler_needs_inline(0x5143,"Adreno 799"));
    CHECK(!xbg_adreno_compiler_needs_inline(0x5143,"Adreno (TM) 740"));
    CHECK(!xbg_adreno_compiler_needs_inline(0x5143,"Adreno (TM) 735"));
    CHECK(!xbg_adreno_compiler_needs_inline(0x5143,"Adreno (TM) 730"));
    CHECK(!xbg_adreno_compiler_needs_inline(0x5143,"Adreno (TM) 830"));
    CHECK(!xbg_adreno_compiler_needs_inline(0x5143,nullptr));
    CHECK(!xbg_adreno_compiler_needs_inline(0x10002,"Adreno 750"));



    CHECK(xbg_adreno_rewrite_option(true,'1',nullptr,false));
    CHECK(xbg_adreno_rewrite_option(true,'1',"0",false));
    CHECK(xbg_adreno_rewrite_option(true,'1',nullptr,true));
    CHECK(xbg_adreno_rewrite_option(true,'1',"0",true));
    CHECK(!xbg_adreno_rewrite_option(true,'0',"1",false));
    CHECK(!xbg_adreno_rewrite_option(true,'0',"1",true));
    CHECK(xbg_adreno_rewrite_option(true,EOF,"0",false));
    CHECK(xbg_adreno_rewrite_option(false,-1,"1",false));
    CHECK(!xbg_adreno_rewrite_option(false,-1,"1",true));
    CHECK(!xbg_adreno_rewrite_option(false,-1,nullptr,true));
    CHECK(!xbg_adreno_rewrite_option(false,-1,"0",false));
    CHECK(!xbg_adreno_rewrite_option(false,-1,nullptr,false));
    CHECK(xbg_adreno_rewrite_option(false,-1,"2",false));



    CHECK(xbg_adreno_dispatch_overruns(0x5143,"Adreno (TM) 830"));
    CHECK(xbg_adreno_dispatch_overruns(0x5143,"Adreno (TM) 800"));
    CHECK(xbg_adreno_dispatch_overruns(0x5143,"Adreno 840"));
    CHECK(xbg_adreno_dispatch_overruns(0x5143,"Adreno (TM) 899"));
    CHECK(!xbg_adreno_dispatch_overruns(0x5143,"Adreno (TM) 730"));
    CHECK(!xbg_adreno_dispatch_overruns(0x5143,"Adreno (TM) 750"));
    CHECK(!xbg_adreno_dispatch_overruns(0x5143,"Adreno (TM) 799"));
    CHECK(!xbg_adreno_dispatch_overruns(0x5143,"Adreno X1-85"));
    CHECK(!xbg_adreno_dispatch_overruns(0x5143,nullptr));
    CHECK(!xbg_adreno_dispatch_overruns(0x10002,"Adreno (TM) 830"));
    CHECK(!xbg_adreno_dispatch_overruns(0x13b5,"Mali-G715"));



    CHECK(xbg_adreno_dispatch_guard_option(true,'1',nullptr,false));
    CHECK(xbg_adreno_dispatch_guard_option(true,'1',"0",false));
    CHECK(xbg_adreno_dispatch_guard_option(true,EOF,nullptr,false));
    CHECK(!xbg_adreno_dispatch_guard_option(true,'0',"1",true));
    CHECK(!xbg_adreno_dispatch_guard_option(true,'0',nullptr,true));
    CHECK(xbg_adreno_dispatch_guard_option(false,-1,nullptr,true));
    CHECK(xbg_adreno_dispatch_guard_option(false,-1,"0",true));
    CHECK(xbg_adreno_dispatch_guard_option(false,-1,"1",false));
    CHECK(!xbg_adreno_dispatch_guard_option(false,-1,"0",false));
    CHECK(!xbg_adreno_dispatch_guard_option(false,-1,nullptr,false));

    if(failures){printf("FAIL %d check(s)\n",failures);return 1;}
    printf("PASS Adreno policy: 740/750 default off, 750-799 inline helpers, 8xx dispatch guard, other vendors unchanged, marker file still wins\n");
    return 0;
}

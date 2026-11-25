

#ifdef NDEBUG
#error Build this regression with assertions enabled (-UNDEBUG).
#endif
#include "../xbg_writeback_policy.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <vector>

static unsigned checks;
#define CHECK(x) do {++checks;assert(x);} while(0)
struct Pending {uint64_t addr,bytes;std::vector<uint8_t> data;};
struct Texture {uint8_t *mem;struct {uint64_t size;} L;uint64_t serial;};
struct Image {Texture texture;unsigned generation;};
static bool superseded(const std::vector<Pending> &p,size_t at,int flags=1,bool alias=false,bool enabled=true) {
    return xbg_wb_superseded(p,at,enabled,[&](uint64_t,uint64_t){return flags;},[&](uint64_t,uint64_t){return alias;});
}
int main() {
    CHECK(xbg_wb_range_valid(100,20));CHECK(!xbg_wb_range_valid(100,0));
    CHECK(xbg_wb_range_valid(UINT64_MAX-1,1));CHECK(!xbg_wb_range_valid(UINT64_MAX,1));
    CHECK(!xbg_wb_range_valid(UINT64_MAX-1,2));
    CHECK(xbg_wb_contains(100,20,100,20));CHECK(xbg_wb_contains(100,20,105,10));
    CHECK(!xbg_wb_contains(100,20,119,2));CHECK(!xbg_wb_contains(100,20,120,1));
    CHECK(!xbg_wb_contains(UINT64_MAX-10,20,UINT64_MAX-5,1));
    CHECK(xbg_wb_overlaps(100,20,119,1));CHECK(!xbg_wb_overlaps(100,20,120,1));
    CHECK(!xbg_wb_overlaps(100,0,100,1));CHECK(!xbg_wb_overlaps(UINT64_MAX-1,4,0,10));
    std::vector<Pending> pending={{100,20,{}},{100,20,{}}};
    CHECK(superseded(pending,0));CHECK(!superseded(pending,1));CHECK(!superseded(pending,2));
    CHECK(!superseded(pending,0,1,false,false));CHECK(!superseded(pending,0,-1));
    CHECK(!superseded(pending,0,4));CHECK(!superseded(pending,0,5));
    CHECK(!superseded(pending,0,0x401));CHECK(!superseded(pending,0,9));CHECK(superseded(pending,0,0x21));
    CHECK(!superseded(pending,0,1,true));
    pending[1]={99,22,{}};CHECK(superseded(pending,0));
    pending[1]={101,20,{}};CHECK(!superseded(pending,0));
    pending[1]={120,20,{}};CHECK(!superseded(pending,0));
    pending={{100,20,{}},{100,10,{}},{110,10,{}}};CHECK(!superseded(pending,0));
    pending={{100,20,{}},{UINT64_MAX-10,30,{}}};CHECK(!superseded(pending,0));
    pending={{UINT64_MAX-1,3,{}},{UINT64_MAX-1,3,{}}};CHECK(!superseded(pending,0));
    unsigned queries=0;
    CHECK(!xbg_wb_superseded(pending,0,false,[&](uint64_t,uint64_t){++queries;return 1;},[](uint64_t,uint64_t){return false;}));CHECK(!queries);



    std::array<uint8_t,64> guest;guest.fill(0x11);uint64_t address=uint64_t(uintptr_t(guest.data()+16));
    pending={{address,16,std::vector<uint8_t>(16,0x22)},{address,16,std::vector<uint8_t>(16,0x33)}};
    CHECK(superseded(pending,0));pending.erase(pending.begin());
    Texture texture{guest.data()+16,{16},7};std::vector<int> order;unsigned resolved=0;
    auto resolve=[&](uint64_t a,uint64_t n){order.push_back(1);CHECK(a==address && n==16);
        for(auto &write:pending)if(xbg_wb_overlaps(a,n,write.addr,write.bytes)){std::memcpy(reinterpret_cast<void*>(uintptr_t(write.addr)),write.data.data(),size_t(write.bytes));++resolved;}
        pending.clear();};
    uint8_t observed=xbg_image_after_cpu_access(texture,true,resolve,[&](const Texture &snapshot){order.push_back(2);CHECK(snapshot.serial==7);return snapshot.mem[0];});
    CHECK(observed==0x33 && resolved==1 && pending.empty());CHECK(order==std::vector<int>({1,2}));
    for(size_t i=0;i<guest.size();++i)CHECK(guest[i]==(i>=16 && i<32?0x33:0x11));



    std::map<uint64_t,std::shared_ptr<Image>> images;
    images[7]=std::make_shared<Image>(Image{texture,1});std::weak_ptr<Image> previous=images[7];
    Image *fresh=xbg_image_after_cpu_access(images[7]->texture,true,
        [&](uint64_t a,uint64_t n){CHECK(a==address && n==16);images.erase(7);CHECK(previous.expired());images[7]=std::make_shared<Image>(Image{texture,2});},
        [&](const Texture &snapshot){CHECK(snapshot.serial==7 && snapshot.mem==guest.data()+16);return images.at(snapshot.serial).get();});
    CHECK(fresh==images.at(7).get() && fresh->generation==2);



    unsigned callbacks=0;
    auto untouched=xbg_image_after_cpu_access(texture,false,[&](uint64_t,uint64_t){++callbacks;},[](const Texture &snapshot){return snapshot.serial;});
    CHECK(!callbacks && untouched==7);
    Texture narrow{guest.data()+32,{4},8};pending={{address,16,std::vector<uint8_t>(16,0x44)}};
    xbg_image_after_cpu_access(narrow,true,[&](uint64_t a,uint64_t n){CHECK(n==4);for(auto &write:pending)if(xbg_wb_overlaps(a,n,write.addr,write.bytes))++callbacks;},[](const Texture &snapshot){return snapshot.serial;});
    CHECK(callbacks==0 && pending.size()==1 && guest[16]==0x33);



    bool resolving=false;unsigned starts=0,lookups=0;
    std::function<void(uint64_t,uint64_t)> reentry;
    reentry=[&](uint64_t,uint64_t){if(resolving)return;resolving=true;++starts;
        xbg_image_after_cpu_access(texture,true,reentry,[&](const Texture &snapshot){++lookups;return images.at(snapshot.serial).get();});resolving=false;};
    xbg_image_after_cpu_access(texture,true,reentry,[&](const Texture &snapshot){++lookups;return images.at(snapshot.serial).get();});
    CHECK(starts==1 && lookups==2 && !resolving);
    std::printf("PASS writeback_skip production_helpers checks=%u; GPU_fences_and_copy_commands=NOT_TESTED\n",checks);return 0;
}

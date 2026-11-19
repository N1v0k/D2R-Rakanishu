

#pragma once
#include <cstddef>
#include <cstdint>

inline bool xbg_wb_range_valid(uint64_t address,uint64_t bytes) {
    return bytes && bytes<=UINT64_MAX-address;
}
inline bool xbg_wb_contains(uint64_t outer,uint64_t outer_bytes,uint64_t inner,uint64_t inner_bytes) {
    return xbg_wb_range_valid(outer,outer_bytes) && xbg_wb_range_valid(inner,inner_bytes) &&
        inner>=outer && inner-outer<=outer_bytes && inner_bytes<=outer_bytes-(inner-outer);
}
inline bool xbg_wb_overlaps(uint64_t a,uint64_t a_bytes,uint64_t b,uint64_t b_bytes) {
    return xbg_wb_range_valid(a,a_bytes) && xbg_wb_range_valid(b,b_bytes) && a<b+b_bytes && b<a+a_bytes;
}
template<class Pending,class Flags,class Aliases>
bool xbg_wb_superseded(const Pending &pending,size_t index,bool enabled,Flags flags,Aliases aliases) {
    if(!enabled || index>=pending.size())return false;
    const auto &old=pending[index];
    if(!xbg_wb_range_valid(old.addr,old.bytes) || old.bytes>SIZE_MAX)return false;
    bool newer=false;
    for(size_t i=index+1;i<pending.size();++i)if(xbg_wb_contains(pending[i].addr,pending[i].bytes,old.addr,old.bytes)) {newer=true;break;}
    if(!newer)return false;
    int pool_flags=flags(old.addr,old.bytes);
    return pool_flags>=0 && !(pool_flags&~0x3ff) && (pool_flags&7)==1 && !(pool_flags&8) && !aliases(old.addr,old.bytes);
}



template<class Texture,class Resolve,class Lookup>
decltype(auto) xbg_image_after_cpu_access(const Texture &texture,bool read,Resolve resolve,Lookup lookup) {
    Texture snapshot=texture;
    if(read && snapshot.mem && snapshot.L.size)resolve(uint64_t(uintptr_t(snapshot.mem)),uint64_t(snapshot.L.size));
    return lookup(snapshot);
}

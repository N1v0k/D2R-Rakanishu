








#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>




inline int xbg_adreno_model(const char *device_name){
    if(!device_name)return 0;
    const char *at=strstr(device_name,"Adreno");
    if(!at)at=strstr(device_name,"adreno");
    if(!at)return 0;
    at+=6;
    while(*at && (*at<'0' || *at>'9'))++at;
    int model=0,digits=0;
    while(*at>='0' && *at<='9' && digits<4){model=model*10+(*at-'0');++at;++digits;}
    return digits==3?model:0;
}






inline int xbg_adreno_model_by_id(uint32_t device_id){
    switch(device_id){
    case 0x06050002u: return 650;
    case 0x06060001u: return 660;
    case 0x07030002u: return 730;
    case 0x43020000u: return 720;
    case 0x43030b00u: return 735;
    case 0x43051401u: return 750;
    case 0x44050001u: return 830;
    default: return 0;
    }
}
inline int xbg_adreno_model_of(const char *device_name,uint32_t device_id){
    const int model=xbg_adreno_model(device_name);
    return model?model:xbg_adreno_model_by_id(device_id);
}









inline bool xbg_adreno_rewrites_fault(uint32_t vendor_id,const char *device_name,uint32_t device_id=0){
    if(vendor_id!=0x5143)return false;
    const int model=xbg_adreno_model_of(device_name,device_id);
    return (model>=740 && model<800) || model==720;
}






inline bool xbg_adreno_rewrite_option(bool marker_present,int marker,const char *environment,bool driver_faults){
    if(marker_present)return marker!='0';
    if(driver_faults)return false;
    return environment && atoi(environment)!=0;
}


















inline bool xbg_adreno_compiler_needs_inline(uint32_t vendor_id,const char *device_name,uint32_t device_id=0){
    if(vendor_id!=0x5143)return false;
    const int model=xbg_adreno_model_of(device_name,device_id);
    return (model>=600 && model<700) || model==735 || (model>=750 && model<800);
}







inline bool xbg_adreno_inline_default(uint32_t vendor_id,const char *device_name,bool fragment_subgroups,uint32_t device_id=0){
    return xbg_adreno_compiler_needs_inline(vendor_id,device_name,device_id) || (vendor_id==0x5143 && !fragment_subgroups);
}






inline bool xbg_adreno_inline_is_new(uint32_t vendor_id,const char *device_name,uint32_t device_id=0,bool fragment_subgroups=true){
    if(vendor_id!=0x5143)return false;
    const int model=xbg_adreno_model_of(device_name,device_id);
    return (model>=600 && model<700) || model==735 || (!fragment_subgroups && !(model>=750 && model<800));
}







inline bool xbg_adreno_dispatch_overruns(uint32_t vendor_id,const char *device_name,uint32_t device_id=0){
    if(vendor_id!=0x5143)return false;
    const int model=xbg_adreno_model_of(device_name,device_id);
    return model>=800 && model<900;
}




inline bool xbg_adreno_dispatch_guard_option(bool marker_present,int marker,const char *environment,bool driver_overruns){
    if(marker_present)return marker!='0';
    if(driver_overruns)return true;
    return environment && atoi(environment)!=0;
}

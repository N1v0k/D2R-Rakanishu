








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



inline bool xbg_adreno_rewrites_fault(uint32_t vendor_id,const char *device_name){
    if(vendor_id!=0x5143)return false;
    const int model=xbg_adreno_model(device_name);
    return model>=740 && model<800;
}






inline bool xbg_adreno_rewrite_option(bool marker_present,int marker,const char *environment,bool driver_faults){
    if(marker_present)return marker!='0';
    if(driver_faults)return false;
    return environment && atoi(environment)!=0;
}








inline bool xbg_adreno_compiler_needs_inline(uint32_t vendor_id,const char *device_name){
    if(vendor_id!=0x5143)return false;
    const int model=xbg_adreno_model(device_name);
    return model>=750 && model<800;
}







inline bool xbg_adreno_dispatch_overruns(uint32_t vendor_id,const char *device_name){
    if(vendor_id!=0x5143)return false;
    const int model=xbg_adreno_model(device_name);
    return model>=800 && model<900;
}




inline bool xbg_adreno_dispatch_guard_option(bool marker_present,int marker,const char *environment,bool driver_overruns){
    if(marker_present)return marker!='0';
    if(driver_overruns)return true;
    return environment && atoi(environment)!=0;
}

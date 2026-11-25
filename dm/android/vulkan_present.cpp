

#define VK_USE_PLATFORM_ANDROID_KHR
#include "vulkan_present.h"
#include "log_policy.h"
#include "../gfx/vulkan/nvn_vk.h"
#include "../gfx/xtrace.h"
#include <android/log.h>
#include <android/native_window.h>
#include <dlfcn.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sched.h>
#include <unistd.h>
#include <time.h>
extern "C" double dm_present_period(void);
extern "C" void dm_present_vote(float hz);

extern "C" __attribute__((weak)) void dm_present_done(int crop_w,int crop_h,int extent_w,int extent_h,int native,int geometry);

extern "C" __attribute__((weak)) int front_android_letterbox(void);
extern "C" __attribute__((weak)) int front_android_window_size(int *w,int *h);


extern "C" __attribute__((weak)) int dm_vk_driver_open(void);
extern "C" __attribute__((weak)) const char *dm_vk_driver_verify(VkPhysicalDevice physical,uint32_t devices);
extern "C" __attribute__((weak)) int dm_vk_driver_fall_back(const char *reason);
extern "C" __attribute__((weak)) void dm_vk_driver_identity(VkPhysicalDevice physical);
extern "C" __attribute__((weak)) void dm_vk_driver_frame(void);


static std::atomic<uint64_t> dm_present_late{0};
extern "C" uint64_t dm_present_late_count(void){return dm_present_late.load(std::memory_order_relaxed);}
namespace {





__attribute__((format(printf,1,2))) void vlog(const char *format,...){
    char text[1100];va_list a;va_start(a,format);int n=vsnprintf(text,sizeof text-1,format,a);va_end(a);
    if(n<0)return;size_t len=std::min(size_t(n),sizeof text-2);text[len]='\n';fwrite(text,1,len+1,stderr);
}
std::mutex window_mutex;std::condition_variable window_cv;


std::mutex render_mutex;
ANativeWindow *desired=nullptr,*active=nullptr;uint64_t generation=0,active_generation=0;
VkInstance instance;VkPhysicalDevice physical;VkDevice device;VkQueue queue;uint32_t family;
VkSurfaceKHR surface;VkSwapchainKHR swapchain;VkFormat swap_format;VkExtent2D extent;
VkSurfaceTransformFlagBitsKHR created_transform;VkExtent2D created_surface_extent;


bool surface_properties_changed(){
    VkSurfaceCapabilitiesKHR c{};
    if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical,surface,&c)!=VK_SUCCESS)return true;
    return c.currentTransform!=created_transform || c.currentExtent.width!=created_surface_extent.width || c.currentExtent.height!=created_surface_extent.height;
}
std::vector<VkImage> images;VkCommandPool command_pool;VkCommandBuffer command;
VkSemaphore acquired;std::vector<VkSemaphore> finished;VkFence fence;bool ready=false,swap_dirty=false;std::string shader_path;





bool overlap=false;VkCommandPool overlap_pool;VkCommandBuffer overlap_command;VkSemaphore overlap_acquired;VkFence overlap_fence;
bool in_flight[2];unsigned flight=0;


bool present_timing=false;VkQueryPool present_queries=VK_NULL_HANDLE;bool present_stamped[2];double present_tick_ns=1,present_gpu_ns=0;unsigned present_timed=0;










unsigned pin_mask=0;int pin_ab=0;int pin_state=-1;unsigned pin_presents=0,pin_drifts=0,pin_failures=0;cpu_set_t pin_original;bool pin_have_original=false;
unsigned fastest_cores(){
    unsigned best=0,mask=0,present=0;
    for(int c=0;c<32;c++){char path[96];snprintf(path,sizeof path,"/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq",c);FILE *f=fopen(path,"rb");if(!f)continue;
        unsigned v=0;if(fscanf(f,"%u",&v)==1 && v){present|=1u<<c;if(v>best){best=v;mask=0;}if(v==best)mask|=1u<<c;}fclose(f);}
    return mask==present?0:mask;
}
void pin_update(){
    if(!pin_mask)return;bool want=true;
    if(pin_ab>0){timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);want=((ts.tv_sec/pin_ab)&1)==0;}
    bool changed=int(want)!=pin_state;if(!changed && ++pin_presents%120)return;
    if(!pin_have_original){CPU_ZERO(&pin_original);if(sched_getaffinity(0,sizeof pin_original,&pin_original))for(int c=0;c<32;c++)CPU_SET(c,&pin_original);pin_have_original=true;}
    cpu_set_t set;CPU_ZERO(&set);if(want){for(int c=0;c<32;c++)if(pin_mask>>c&1)CPU_SET(c,&set);}else set=pin_original;
    if(!changed){cpu_set_t cur;CPU_ZERO(&cur);if(!sched_getaffinity(0,sizeof cur,&cur) && CPU_EQUAL(&cur,&set))return;++pin_drifts;}
    int r=sched_setaffinity(0,sizeof set,&set),err=r?errno:0;pin_state=want;if(r)++pin_failures;


    if(changed || pin_drifts<=4 || (r && pin_failures<=4))vlog("NVN pin tid=%d %s mask=%x result=%d errno=%d drifts=%u failures=%u",int(gettid()),want?"pinned":"released",want?pin_mask:0u,r,err,pin_drifts,pin_failures);
}





std::mutex perf_sampling_mutex;dm_perf_sampler perf_sampling{30000000000ull,0,0,0,0,0,0,0};
void log_message(void*,const char *text){
    if(strncmp(text,"NVN PERF ",9)){vlog("%s",text);return;}
    timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);uint64_t now=uint64_t(ts.tv_sec)*1000000000ull+uint64_t(ts.tv_nsec);
    char note[160];int keep;{std::lock_guard<std::mutex> lock(perf_sampling_mutex);keep=dm_perf_keep(&perf_sampling,text,now);if(keep==2)dm_perf_note(&perf_sampling,note,sizeof note);}
    if(keep)vlog("%s",text);
    if(keep==2)vlog("%s",note);
}
[[noreturn]] void fatal(void*,const char *text){vlog("%s",text);__android_log_print(ANDROID_LOG_FATAL,"dm","%s",text);abort();}
void destroy_present_semaphores(){for(auto s:finished)if(s)vkDestroySemaphore(device,s,nullptr);finished.clear();}



void destroy_swap(){destroy_present_semaphores();if(swapchain)vkDestroySwapchainKHR(device,swapchain,nullptr);swapchain=VK_NULL_HANDLE;images.clear();if(surface)vkDestroySurfaceKHR(instance,surface,nullptr);surface=VK_NULL_HANDLE;if(active)ANativeWindow_release(active);active=nullptr;}
void cleanup_initialization(){
    if(queue)vkQueueWaitIdle(queue);
    if(device){if(fence)vkDestroyFence(device,fence,nullptr);if(acquired)vkDestroySemaphore(device,acquired,nullptr);if(command_pool)vkDestroyCommandPool(device,command_pool,nullptr);}
    fence=VK_NULL_HANDLE;acquired=VK_NULL_HANDLE;command_pool=VK_NULL_HANDLE;command=VK_NULL_HANDLE;
    if(device){if(overlap_fence)vkDestroyFence(device,overlap_fence,nullptr);if(overlap_acquired)vkDestroySemaphore(device,overlap_acquired,nullptr);if(overlap_pool)vkDestroyCommandPool(device,overlap_pool,nullptr);}
    overlap_fence=VK_NULL_HANDLE;overlap_acquired=VK_NULL_HANDLE;overlap_pool=VK_NULL_HANDLE;overlap_command=VK_NULL_HANDLE;overlap=false;in_flight[0]=in_flight[1]=false;flight=0;
    destroy_swap();if(device)vkDestroyDevice(device,nullptr);device=VK_NULL_HANDLE;queue=VK_NULL_HANDLE;
    if(instance)vkDestroyInstance(instance,nullptr);instance=VK_NULL_HANDLE;physical=VK_NULL_HANDLE;
}





float frame_rate_vote=120;
std::atomic<float> frame_rate_wanted{0};






#ifdef __OHOS__
bool present_native=false;
#else
bool present_native=true;
#endif
int native_w=0,native_h=0,native_geometry=0;




bool frame_rate_always=false;
int vote_frame_rate(ANativeWindow *window,float hz){
    static void *lib=dlopen("libnativewindow.so",RTLD_NOW);
    static auto set_rate=reinterpret_cast<int32_t(*)(ANativeWindow*,float,int8_t)>(lib?dlsym(lib,"ANativeWindow_setFrameRate"):nullptr);
    static auto set_rate_s=reinterpret_cast<int32_t(*)(ANativeWindow*,float,int8_t,int8_t)>(lib?dlsym(lib,"ANativeWindow_setFrameRateWithChangeStrategy"):nullptr);
    if(frame_rate_always&&set_rate_s)return set_rate_s(window,hz,ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT,1);
    return set_rate?set_rate(window,hz,ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT):-1;
}
int create_surface(){
    destroy_swap();native_w=native_h=0;
    {std::lock_guard<std::mutex> lock(window_mutex);if(!desired)return VK_NOT_READY;active=desired;ANativeWindow_acquire(active);active_generation=generation;}
    if(frame_rate_vote>0){

        int fr=vote_frame_rate(active,frame_rate_vote);
        vlog("Frame rate vote %.0f Hz result=%d (marker gfx-frame-rate; display request, not FPS)",double(frame_rate_vote),fr);
    }
    VkAndroidSurfaceCreateInfoKHR ci{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};ci.window=active;
    VkResult r=vkCreateAndroidSurfaceKHR(instance,&ci,nullptr,&surface);if(r)return r;
    if(!device)return 0;VkBool32 supported=VK_FALSE;r=vkGetPhysicalDeviceSurfaceSupportKHR(physical,family,surface,&supported);if(r||!supported)return r?r:VK_ERROR_FEATURE_NOT_PRESENT;return 0;
}
int create_swap(){
    VkSurfaceCapabilitiesKHR cap;VkResult r=vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical,surface,&cap);if(r)return r;
    if(!(cap.supportedUsageFlags&VK_IMAGE_USAGE_TRANSFER_DST_BIT))return VK_ERROR_FORMAT_NOT_SUPPORTED;
    if(!(cap.supportedTransforms&VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR))return VK_ERROR_FEATURE_NOT_PRESENT;
    uint32_t n=0;r=vkGetPhysicalDeviceSurfaceFormatsKHR(physical,surface,&n,nullptr);if(r||!n)return r?r:VK_ERROR_FORMAT_NOT_SUPPORTED;
    std::vector<VkSurfaceFormatKHR> formats(n);r=vkGetPhysicalDeviceSurfaceFormatsKHR(physical,surface,&n,formats.data());if(r)return r;
    VkSurfaceFormatKHR sf=formats[0];if(sf.format==VK_FORMAT_UNDEFINED)sf.format=VK_FORMAT_R8G8B8A8_UNORM;
    for(auto f:formats)if(f.format==VK_FORMAT_R8G8B8A8_UNORM||f.format==VK_FORMAT_B8G8R8A8_UNORM){sf=f;break;}
    extent=cap.currentExtent;if(extent.width==UINT32_MAX){extent.width=std::clamp(uint32_t(std::max(1,ANativeWindow_getWidth(active))),cap.minImageExtent.width,cap.maxImageExtent.width);extent.height=std::clamp(uint32_t(std::max(1,ANativeWindow_getHeight(active))),cap.minImageExtent.height,cap.maxImageExtent.height);}if(!extent.width||!extent.height)return VK_NOT_READY;
    VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};ci.surface=surface;
#ifdef __OHOS__
    {unsigned want=4;if(const char *e=getenv("DM_SWAP_IMAGES"))want=unsigned(std::max(2,atoi(e)));ci.minImageCount=std::max(want,cap.minImageCount);}
#else
    ci.minImageCount=std::max(2u,cap.minImageCount);
#endif
    if(cap.maxImageCount)ci.minImageCount=std::min(ci.minImageCount,cap.maxImageCount);ci.imageFormat=sf.format;ci.imageColorSpace=sf.colorSpace;ci.imageExtent=extent;ci.imageArrayLayers=1;ci.imageUsage=VK_IMAGE_USAGE_TRANSFER_DST_BIT;ci.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;ci.preTransform=VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;ci.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if(!(cap.supportedCompositeAlpha&ci.compositeAlpha))for(uint32_t b=1;b<=8;b<<=1)if(cap.supportedCompositeAlpha&b){ci.compositeAlpha=VkCompositeAlphaFlagBitsKHR(b);break;}
    ci.presentMode=VK_PRESENT_MODE_FIFO_KHR;ci.clipped=VK_TRUE;ci.oldSwapchain=swapchain;VkSwapchainKHR replacement=VK_NULL_HANDLE;r=vkCreateSwapchainKHR(device,&ci,nullptr,&replacement);


    destroy_present_semaphores();if(swapchain)vkDestroySwapchainKHR(device,swapchain,nullptr);swapchain=r?VK_NULL_HANDLE:replacement;images.clear();if(r)return r;swap_format=sf.format;
    created_transform=cap.currentTransform;created_surface_extent=cap.currentExtent;
    vlog("Swapchain: window=%dx%d extent=%ux%u currentTransform=%u supported=%u selected=%u",ANativeWindow_getWidth(active),ANativeWindow_getHeight(active),extent.width,extent.height,unsigned(cap.currentTransform),unsigned(cap.supportedTransforms),unsigned(ci.preTransform));
    r=vkGetSwapchainImagesKHR(device,swapchain,&n,nullptr);if(r)return r;images.resize(n);r=vkGetSwapchainImagesKHR(device,swapchain,&n,images.data());if(r)return r;images.resize(n);


    finished.resize(n,VK_NULL_HANDLE);VkSemaphoreCreateInfo sc{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for(auto &s:finished){r=vkCreateSemaphore(device,&sc,nullptr,&s);if(r)return r;}return VK_SUCCESS;
}


void membench(){
    VkPhysicalDeviceMemoryProperties mp;vkGetPhysicalDeviceMemoryProperties(physical,&mp);
    const VkDeviceSize n=4u<<20;std::vector<uint8_t> host(n,1),back(n);
    VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cp.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;cp.queueFamilyIndex=family;VkCommandPool pool;if(vkCreateCommandPool(device,&cp,nullptr,&pool))return;
    VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;VkCommandBuffer cmd;if(vkAllocateCommandBuffers(device,&ca,&cmd)){vkDestroyCommandPool(device,pool,nullptr);return;}
    auto now=[]{timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return double(ts.tv_sec)*1e9+double(ts.tv_nsec);};
    for(uint32_t t=0;t<mp.memoryTypeCount;t++){
        VkMemoryPropertyFlags f=mp.memoryTypes[t].propertyFlags;if(!(f&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))continue;
        VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bi.size=n;bi.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;bi.sharingMode=VK_SHARING_MODE_EXCLUSIVE;VkBuffer b;if(vkCreateBuffer(device,&bi,nullptr,&b))continue;
        VkMemoryRequirements r;vkGetBufferMemoryRequirements(device,b,&r);if(!(r.memoryTypeBits&(1u<<t))){vkDestroyBuffer(device,b,nullptr);continue;}
        VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ai.allocationSize=r.size;ai.memoryTypeIndex=t;VkDeviceMemory m;if(vkAllocateMemory(device,&ai,nullptr,&m)){vkDestroyBuffer(device,b,nullptr);continue;}
        void *p=nullptr;if(vkBindBufferMemory(device,b,m,0)||vkMapMemory(device,m,0,VK_WHOLE_SIZE,0,&p)){vkFreeMemory(device,m,nullptr);vkDestroyBuffer(device,b,nullptr);continue;}
        bool coherent=(f&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)!=0;VkMappedMemoryRange mr{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};mr.memory=m;mr.size=VK_WHOLE_SIZE;
        double w=1e30,rc=1e30,rd=1e30,flush=0,inv=0;bool ok=true;
        for(int k=0;k<4;k++){double t0=now();memcpy(p,host.data(),n);double t1=now();if(!coherent)vkFlushMappedMemoryRanges(device,1,&mr);double t2=now();w=std::min(w,t1-t0);flush=std::max(flush,t2-t1);}
        for(int k=0;k<4;k++){double t0=now();memcpy(back.data(),p,n);rc=std::min(rc,now()-t0);}
        for(int k=0;k<4;k++){
            vkResetCommandBuffer(cmd,0);VkCommandBufferBeginInfo cb{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};vkBeginCommandBuffer(cmd,&cb);
            uint32_t value=0x01020304u+uint32_t(k);vkCmdFillBuffer(cmd,b,0,n,value);
            VkMemoryBarrier mb{VK_STRUCTURE_TYPE_MEMORY_BARRIER};mb.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;mb.dstAccessMask=VK_ACCESS_HOST_READ_BIT;vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&mb,0,nullptr,0,nullptr);
            vkEndCommandBuffer(cmd);VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};si.commandBufferCount=1;si.pCommandBuffers=&cmd;if(vkQueueSubmit(queue,1,&si,VK_NULL_HANDLE)||vkQueueWaitIdle(queue)){ok=false;break;}
            double t0=now();if(!coherent)vkInvalidateMappedMemoryRanges(device,1,&mr);double t1=now();memcpy(back.data(),p,n);double t2=now();
            rd=std::min(rd,t2-t1);inv=std::max(inv,t1-t0);uint32_t got;memcpy(&got,back.data()+n-4,4);ok=ok&&got==value;
        }
        vlog("NVN membench type=%u flags=%#x write=%.2f GB/s (flush %.0f us) read=%.2f GB/s read_after_gpu=%.2f GB/s (invalidate %.0f us) ok=%d",
            t,unsigned(f),double(n)/w,flush/1e3,double(n)/rc,double(n)/rd,inv/1e3,int(ok));
        vkUnmapMemory(device,m);vkFreeMemory(device,m,nullptr);vkDestroyBuffer(device,b,nullptr);
    }
    vkDestroyCommandPool(device,pool,nullptr);
}
int has_window(void*){std::lock_guard<std::mutex> l(window_mutex);return desired!=nullptr;}
void wait_window(){std::unique_lock<std::mutex> lock(window_mutex);window_cv.wait(lock,[]{return desired!=nullptr;});}
bool window_changed(){std::lock_guard<std::mutex> lock(window_mutex);return !desired||generation!=active_generation;}
void wait_window_change(uint64_t bad){std::unique_lock<std::mutex> lock(window_mutex);window_cv.wait(lock,[bad]{return generation!=bad;});}
void wait_resize(){std::unique_lock<std::mutex> lock(window_mutex);window_cv.wait_for(lock,std::chrono::milliseconds(16),[]{return generation!=active_generation;});}
int present(void*,void*,VkImage source,VkFormat source_format,uint32_t width,uint32_t height,const int32_t crop[4],int32_t){
    if(float hz=frame_rate_wanted.exchange(0)){frame_rate_vote=hz;if(active)vlog("Frame rate vote %.0f Hz result=%d (frame rate setting, strategy %s)",double(hz),vote_frame_rate(active,hz),frame_rate_always?"always":"seamless");}
    std::lock_guard<std::mutex> render_lock(render_mutex);
    pin_update();
    VkFormatProperties sp;vkGetPhysicalDeviceFormatProperties(physical,source_format,&sp);if(!(sp.optimalTilingFeatures&VK_FORMAT_FEATURE_BLIT_SRC_BIT))return VK_ERROR_FORMAT_NOT_SUPPORTED;
    int x=0,y=0,w=int(width),h=int(height);if(crop&&crop[2]>0&&crop[3]>0){x=std::clamp(crop[0],0,w);y=std::clamp(crop[1],0,h);w=std::min(crop[2],int(width)-x);h=std::min(crop[3],int(height)-y);}if(w<=0||h<=0)return VK_ERROR_INITIALIZATION_FAILED;



    bool full=front_android_letterbox && front_android_letterbox();
    int win_w=0,win_h=0;
    bool box=full && front_android_window_size && front_android_window_size(&win_w,&win_h) && !(std::fabs(double(win_w)/win_h/(double(w)/h)-1)<0.01);
    bool native=present_native && !box;
    for(;;){
    if(native && surface && active && (native_w!=w || native_h!=h)){
        int g=ANativeWindow_setBuffersGeometry(active,w,h,0);native_w=w;native_h=h;native_geometry=g;swap_dirty=true;
        vlog("Present at the game's output size %dx%d result=%d (marker gfx-present-native)",w,h,g);}
    else if(present_native && box && surface && active && (native_w!=win_w || native_h!=win_h)){
        int g=ANativeWindow_setBuffersGeometry(active,win_w,win_h,0);native_w=win_w;native_h=win_h;native_geometry=g;swap_dirty=true;
        vlog("Present at the window size %dx%d, picture %dx%d letterboxed result=%d",win_w,win_h,w,h,g);}


    wait_window();
    VkResult r;
    if(!surface||window_changed()){
        if((r=vkQueueWaitIdle(queue)))return r;
        r=VkResult(create_surface());
        if(r==VK_NOT_READY)continue;
        if(r==VK_ERROR_SURFACE_LOST_KHR){uint64_t bad=active_generation;destroy_swap();wait_window_change(bad);continue;}
        if(r)return r;swap_dirty=true;
    }
    if(swap_dirty){
        if((r=vkQueueWaitIdle(queue)))return r;
        r=VkResult(create_swap());
        if(r==VK_NOT_READY||r==VK_ERROR_OUT_OF_DATE_KHR){wait_resize();continue;}
        if(r==VK_ERROR_SURFACE_LOST_KHR){uint64_t bad=active_generation;destroy_swap();wait_window_change(bad);continue;}
        if(r)return r;swap_dirty=false;
    }
    VkFormatProperties dp;vkGetPhysicalDeviceFormatProperties(physical,swap_format,&dp);if(!(dp.optimalTilingFeatures&VK_FORMAT_FEATURE_BLIT_DST_BIT))return VK_ERROR_FORMAT_NOT_SUPPORTED;


    unsigned f=overlap?flight:0;VkCommandPool pool=f?overlap_pool:command_pool;VkCommandBuffer cmd=f?overlap_command:command;VkSemaphore acq=f?overlap_acquired:acquired;VkFence done=f?overlap_fence:fence;
    if(in_flight[f]){XTRACE_BEGIN("dm-present-fence");r=vkWaitForFences(device,1,&done,VK_TRUE,10000000000ull);XTRACE_END();if(r)return r;if((r=vkResetFences(device,1,&done)))return r;in_flight[f]=false;
        if(present_stamped[f]){uint64_t t[2]{};if(vkGetQueryPoolResults(device,present_queries,f*2,2,sizeof t,t,8,VK_QUERY_RESULT_64_BIT)==VK_SUCCESS && t[1]>=t[0]){present_gpu_ns+=double(t[1]-t[0])*present_tick_ns;
            if(++present_timed==300){vlog("NVN PERF present_gpu_us=%.0f extent=%ux%u (marker gfx-present-timing)",present_gpu_ns/present_timed/1e3,extent.width,extent.height);present_gpu_ns=0;present_timed=0;}}present_stamped[f]=false;}}
    uint32_t index=0;XTRACE_BEGIN("dm-acquire");r=vkAcquireNextImageKHR(device,swapchain,50000000ull,acq,VK_NULL_HANDLE,&index);XTRACE_END();
    if(r==VK_TIMEOUT||r==VK_NOT_READY)continue;
    if(r==VK_ERROR_OUT_OF_DATE_KHR){swap_dirty=true;wait_resize();continue;}
    if(r==VK_ERROR_SURFACE_LOST_KHR){if((r=vkQueueWaitIdle(queue)))return r;uint64_t bad=active_generation;destroy_swap();wait_window_change(bad);continue;}
    bool rebuild=r==VK_SUBOPTIMAL_KHR;if(r!=VK_SUCCESS&&!rebuild)return r;
    r=vkResetCommandPool(device,pool,0);if(r)return r;VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};bi.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;if((r=vkBeginCommandBuffer(cmd,&bi)))return r;
    bool stamp=present_timing && overlap;if(stamp && !present_queries){VkQueryPoolCreateInfo q{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};q.queryType=VK_QUERY_TYPE_TIMESTAMP;q.queryCount=4;if(vkCreateQueryPool(device,&q,nullptr,&present_queries))stamp=false;
        VkPhysicalDeviceProperties pp;vkGetPhysicalDeviceProperties(physical,&pp);present_tick_ns=pp.limits.timestampPeriod;}
    if(stamp){vkCmdResetQueryPool(cmd,present_queries,f*2,2);vkCmdWriteTimestamp(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,present_queries,f*2);}
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.image=images[index];barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;





    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    double scale=std::min(double(extent.width)/w,double(extent.height)/h);int dw=std::max(1,int(w*scale)),dh=std::max(1,int(h*scale)),dx=(int(extent.width)-dw)/2,dy=(int(extent.height)-dh)/2;
    if(dx || dy || dw!=int(extent.width) || dh!=int(extent.height)){
        VkClearColorValue black{};vkCmdClearColorImage(cmd,images[index],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&black,1,&barrier.subresourceRange);
        barrier.oldLayout=barrier.newLayout;barrier.srcAccessMask=barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);}
    VkImageBlit blit{};blit.srcSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};blit.dstSubresource=blit.srcSubresource;blit.srcOffsets[0]={x,y,0};blit.srcOffsets[1]={x+w,y+h,1};blit.dstOffsets[0]={dx,dy,0};blit.dstOffsets[1]={dx+dw,dy+dh,1};


    VkFilter filter=VK_FILTER_NEAREST;
#ifdef __OHOS__
    if(sp.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)filter=VK_FILTER_LINEAR;
#else
    if(full && (dw!=w||dh!=h) && (sp.optimalTilingFeatures&VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))filter=VK_FILTER_LINEAR;
#endif
    vkCmdBlitImage(cmd,source,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,images[index],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&blit,filter);


    VkMemoryBarrier source_read{VK_STRUCTURE_TYPE_MEMORY_BARRIER};source_read.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;source_read.dstAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,1,&source_read,0,nullptr,0,nullptr);
    barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=0;vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,nullptr,0,nullptr,1,&barrier);
    if(stamp){vkCmdWriteTimestamp(cmd,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,present_queries,f*2+1);present_stamped[f]=true;}
    if((r=vkEndCommandBuffer(cmd)))return r;VkPipelineStageFlags stage=VK_PIPELINE_STAGE_TRANSFER_BIT;VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&acq;submit.pWaitDstStageMask=&stage;submit.commandBufferCount=1;submit.pCommandBuffers=&cmd;submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&finished[index];
    if((r=vkQueueSubmit(queue,1,&submit,done)))return r;in_flight[f]=true;VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};pi.waitSemaphoreCount=1;pi.pWaitSemaphores=&finished[index];pi.swapchainCount=1;pi.pSwapchains=&swapchain;pi.pImageIndices=&index;
#ifdef __OHOS__




    {static int on=-1;if(on<0){const char *e=getenv("DM_PRESENT_TS");on=!(e&&*e=='0');}static int64_t tgt;double per=dm_present_period();
     if(on&&per>0&&active){timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);int64_t now=int64_t(ts.tv_sec)*1000000000+ts.tv_nsec,p=int64_t(per*1e9),next=tgt?tgt+p:now;
      if(tgt&&now>tgt+p+2000000)dm_present_late.fetch_add(1,std::memory_order_relaxed);if(next<now)next=now;if(next>now+2*p)next=now+p;tgt=next;OH_NativeWindow_NativeWindowHandleOpt(active,SET_DESIRED_PRESENT_TIMESTAMP,next);}}
#endif
    XTRACE_BEGIN("dm-queue-present");VkResult pr=vkQueuePresentKHR(queue,&pi);XTRACE_END();


    if(overlap){flight^=1;if(pr!=VK_SUCCESS && pr!=VK_SUBOPTIMAL_KHR && (r=vkQueueWaitIdle(queue)))return r;}
    else{if((r=vkWaitForFences(device,1,&done,VK_TRUE,10000000000ull)))return r;if((r=vkResetFences(device,1,&done)))return r;in_flight[0]=false;if((r=vkQueueWaitIdle(queue)))return r;}
    if(pr==VK_ERROR_SURFACE_LOST_KHR){uint64_t bad=active_generation;destroy_swap();wait_window_change(bad);continue;}
    if(pr==VK_ERROR_OUT_OF_DATE_KHR){swap_dirty=true;wait_resize();continue;}
    if(pr!=VK_SUCCESS&&pr!=VK_SUBOPTIMAL_KHR)return pr;


    if((rebuild||pr==VK_SUBOPTIMAL_KHR)&&surface_properties_changed())swap_dirty=true;
    if(dm_present_done)dm_present_done(w,h,int(extent.width),int(extent.height),native?1:0,native_geometry);
    if(dm_vk_driver_frame)dm_vk_driver_frame();
    return 0;
    }
}
}
extern "C" void dm_android_vk_set_window(ANativeWindow *window){if(window)ANativeWindow_acquire(window);{std::lock_guard<std::mutex> l(window_mutex);if(desired)ANativeWindow_release(desired);desired=window;++generation;}window_cv.notify_all();}
extern "C" int dm_android_vk_has_window(){return has_window(nullptr);}
extern "C" int dm_android_vk_initialize(const char *path,char *error,size_t bytes){
    if(path){std::string dir=path;size_t cut=dir.find_last_of('/');if(FILE *m=fopen(((cut==std::string::npos?dir:dir.substr(0,cut))+"/gfx-present-native").c_str(),"rb")){present_native=fgetc(m)!='0';fclose(m);}
        if(FILE *m=fopen(((cut==std::string::npos?dir:dir.substr(0,cut))+"/gfx-present-timing").c_str(),"rb")){present_timing=true;fclose(m);}}
    std::lock_guard<std::mutex> render_lock(render_mutex);
    auto fail=[&](const char *what,VkResult r)->int{if(error&&bytes)snprintf(error,bytes,"%s: VkResult %d",what,int(r));if(!ready)cleanup_initialization();return int(r?r:VK_ERROR_INITIALIZATION_FAILED);};
    if(ready||!path||!*path)return fail("Initialization requires an external shader directory and an uninitialized renderer",VK_ERROR_INITIALIZATION_FAILED);

    frame_rate_vote=120;
    {std::string dir=path;size_t cut=dir.find_last_of('/');if(FILE *m=fopen(((cut==std::string::npos?dir:dir.substr(0,cut))+"/gfx-frame-rate").c_str(),"rb")){float v=0;frame_rate_vote=fscanf(m,"%f",&v)==1 && v>=0 && v<=240?v:0;fclose(m);}
     frame_rate_always=false;if(FILE *m=fopen(((cut==std::string::npos?dir:dir.substr(0,cut))+"/gfx-frame-rate-strategy").c_str(),"rb")){char b[16]={0};frame_rate_always=fscanf(m,"%15s",b)==1 && !strcmp(b,"always");fclose(m);}
     vlog("Frame rate strategy %s (marker gfx-frame-rate-strategy)",frame_rate_always?"always":"seamless");}

    VkApplicationInfo ai{VK_STRUCTURE_TYPE_APPLICATION_INFO};ai.pApplicationName="dm-android";ai.apiVersion=VK_API_VERSION_1_1;
    const char *ext[]={VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};VkInstanceCreateInfo ic{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ic.pApplicationInfo=&ai;ic.enabledExtensionCount=2;ic.ppEnabledExtensionNames=ext;VkResult r;
    uint32_t n=0;std::vector<VkPhysicalDevice> devices;


    if(dm_vk_driver_open)dm_vk_driver_open();
    for(;;){
        r=vkCreateInstance(&ic,nullptr,&instance);n=0;
        if(!r){vkEnumeratePhysicalDevices(instance,&n,nullptr);devices.assign(n,VK_NULL_HANDLE);vkEnumeratePhysicalDevices(instance,&n,devices.data());devices.resize(n);}
        const char *bad=r?"vkCreateInstance failed":dm_vk_driver_verify?dm_vk_driver_verify(n?devices[0]:VK_NULL_HANDLE,n):nullptr;

        if(bad&&dm_vk_driver_fall_back){if(instance)vkDestroyInstance(instance,nullptr);instance=VK_NULL_HANDLE;if(dm_vk_driver_fall_back(bad))continue;}
        break;
    }
    if(r)return fail("vkCreateInstance",r);
    if(!n)return fail("No Vulkan device",VK_ERROR_INITIALIZATION_FAILED);
    physical=devices[0];
    if(dm_vk_driver_identity)dm_vk_driver_identity(physical);


    for(;;){wait_window();r=VkResult(create_surface());if(r==VK_NOT_READY)continue;if(r==VK_ERROR_SURFACE_LOST_KHR){uint64_t bad=active_generation;destroy_swap();wait_window_change(bad);continue;}if(r)return fail("Android surface",r);break;}
    vkGetPhysicalDeviceQueueFamilyProperties(physical,&n,nullptr);std::vector<VkQueueFamilyProperties> q(n);vkGetPhysicalDeviceQueueFamilyProperties(physical,&n,q.data());bool found=false;
    for(uint32_t i=0;i<n;i++){VkBool32 p=0;vkGetPhysicalDeviceSurfaceSupportKHR(physical,i,surface,&p);if(p&&(q[i].queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))==(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT)){family=i;found=true;break;}}if(!found)return fail("No graphics/compute/present family",VK_ERROR_FEATURE_NOT_PRESENT);
    VkPhysicalDeviceShaderDrawParametersFeatures draw{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES};VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};features.pNext=&draw;vkGetPhysicalDeviceFeatures2(physical,&features);
    vkEnumerateDeviceExtensionProperties(physical,nullptr,&n,nullptr);std::vector<VkExtensionProperties> available(n);vkEnumerateDeviceExtensionProperties(physical,nullptr,&n,available.data());bool mirror=false;for(auto e:available)if(std::string(e.extensionName)==VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME)mirror=true;
    {std::string all;for(auto &e:available){all+=e.extensionName;all+=' ';}
     for(size_t at=0;at<all.size();at+=900)vlog("Device extensions [%zu]: %s",at/900,all.substr(at,900).c_str());
     VkPhysicalDeviceExternalMemoryHostPropertiesEXT host{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT};VkPhysicalDeviceProperties2 pr{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};pr.pNext=&host;vkGetPhysicalDeviceProperties2(physical,&pr);
     vlog("Device %s api=%u.%u.%u driver=%#x minImportedHostPointerAlignment=%llu",pr.properties.deviceName,VK_VERSION_MAJOR(pr.properties.apiVersion),VK_VERSION_MINOR(pr.properties.apiVersion),VK_VERSION_PATCH(pr.properties.apiVersion),pr.properties.driverVersion,(unsigned long long)host.minImportedHostPointerAlignment);}
    std::vector<const char*> dext={VK_KHR_SWAPCHAIN_EXTENSION_NAME};if(mirror)dext.push_back(VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME);

    VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR pef{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR};
    if(FILE *ps=[&]{std::string dir=path;size_t cut=dir.find_last_of('/');return fopen(((cut==std::string::npos?dir:dir.substr(0,cut))+"/gfx-pipe-stats").c_str(),"rb");}()){fclose(ps);dext.push_back(VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME);pef.pipelineExecutableInfo=VK_TRUE;pef.pNext=draw.pNext;draw.pNext=&pef;}
    float priority=1;VkDeviceQueueCreateInfo qc{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qc.queueFamilyIndex=family;qc.queueCount=1;qc.pQueuePriorities=&priority;VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dc.pNext=&draw;dc.pEnabledFeatures=&features.features;dc.queueCreateInfoCount=1;dc.pQueueCreateInfos=&qc;dc.enabledExtensionCount=uint32_t(dext.size());dc.ppEnabledExtensionNames=dext.data();if((r=vkCreateDevice(physical,&dc,nullptr,&device)))return fail("vkCreateDevice",r);vkGetDeviceQueue(device,family,0,&queue);
    for(;;){
        wait_window();
        if(!surface||window_changed()){
            r=VkResult(create_surface());if(r==VK_NOT_READY)continue;
            if(r==VK_ERROR_SURFACE_LOST_KHR){uint64_t bad=active_generation;destroy_swap();wait_window_change(bad);continue;}
            if(r)return fail("Android surface",r);
        }
        r=VkResult(create_swap());
        if(r==VK_NOT_READY||r==VK_ERROR_OUT_OF_DATE_KHR){wait_resize();continue;}
        if(r==VK_ERROR_SURFACE_LOST_KHR){uint64_t bad=active_generation;destroy_swap();wait_window_change(bad);continue;}
        if(r)return fail("Swapchain",r);break;
    }
    VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};cp.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;cp.queueFamilyIndex=family;if((r=vkCreateCommandPool(device,&cp,nullptr,&command_pool)))return fail("Present command pool",r);
    VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=command_pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;if((r=vkAllocateCommandBuffers(device,&ca,&command)))return fail("Present command",r);
    VkSemaphoreCreateInfo sc{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};if((r=vkCreateSemaphore(device,&sc,nullptr,&acquired)))return fail("Acquire semaphore",r);VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};if((r=vkCreateFence(device,&fc,nullptr,&fence)))return fail("Present fence",r);
    {std::string dir=path;size_t cut=dir.find_last_of('/');FILE *m=fopen(((cut==std::string::npos?dir:dir.substr(0,cut))+"/gfx-present-overlap").c_str(),"rb");overlap=true;if(m){overlap=fgetc(m)!='0';fclose(m);}
     auto open_marker=[&](const char *name){return fopen(((cut==std::string::npos?dir:dir.substr(0,cut))+name).c_str(),"rb");};
     pin_mask=fastest_cores();pin_ab=0;pin_state=-1;pin_have_original=false;pin_presents=pin_drifts=0;
     if(FILE *p=open_marker("/gfx-pin-cpu")){if(fscanf(p,"%x",&pin_mask)!=1)pin_mask=0;fclose(p);}
     if(FILE *n=open_marker("/gfx-no-pin")){pin_mask=0;fclose(n);}
     if(FILE *t=open_marker("/gfx-ab-target")){char target[16]={0};if(fscanf(t,"%15s",target)==1 && !strcmp(target,"pin"))if(FILE *a=open_marker("/gfx-ab-toggle")){if(fscanf(a,"%d",&pin_ab)!=1)pin_ab=0;fclose(a);}fclose(t);}
     vlog("NVN pin mask=%x ab=%d (markers gfx-no-pin, gfx-pin-cpu, gfx-ab-target=pin)",pin_mask,pin_ab);
     if(FILE *b=open_marker("/gfx-membench")){fclose(b);membench();}}
    if(overlap){if((r=vkCreateCommandPool(device,&cp,nullptr,&overlap_pool)))return fail("Present command pool",r);ca.commandPool=overlap_pool;if((r=vkAllocateCommandBuffers(device,&ca,&overlap_command)))return fail("Present command",r);
        if((r=vkCreateSemaphore(device,&sc,nullptr,&overlap_acquired)))return fail("Acquire semaphore",r);if((r=vkCreateFence(device,&fc,nullptr,&overlap_fence)))return fail("Present fence",r);}
    vlog("Present overlap=%d (marker gfx-present-overlap)",int(overlap));
    shader_path=path;dm_vk_config config{};config.size=sizeof(config);config.game.size=sizeof(config.game);config.game.physical=physical;config.game.device=device;config.game.queue=queue;config.game.queue_family=family;config.game.enabled_core=features.features;config.game.enabled_shader_draw_parameters=draw.shaderDrawParameters;config.game.fence_timeout_ns=10000000000ull;config.game.shader_directory=shader_path.c_str();config.game.present=present;config.game.log=log_message;config.game.fatal=fatal;config.sampler_mirror_clamp_to_edge=mirror;config.has_window=has_window;config.display_hz=0;
    int status=dm_vk_initialize(&config,error,bytes);if(!status)ready=true;else cleanup_initialization();return status;
}

extern "C" void dm_present_vote(float hz){frame_rate_wanted.store(hz);}

extern "C" void dm_android_perf_sampling(int seconds){std::lock_guard<std::mutex> lock(perf_sampling_mutex);perf_sampling.period_ns=seconds>0?uint64_t(seconds)*1000000000ull:0;}

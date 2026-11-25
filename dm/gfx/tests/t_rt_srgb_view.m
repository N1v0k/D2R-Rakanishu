





#include "../nvn/nvn_mtl.m"
void *front_metal_layer(void) { return NULL; }
module_t *module_of(const void *p) { (void)p; return NULL; }
module_t g_mod[MAX_MOD]; int g_nmod;
void svc_thread_dump(void) {}
uint64_t now_ns(void) { return bp_now(); }

static int fail, count;
#define EXPECT(name, want, got) do { long long w_ = (want), g_ = (got); count++; \
    printf("%s 期望 %lld → 真结果 %lld %s\n", name, w_, g_, w_ == g_ ? "✓" : "✗"); fail += w_ != g_; } while (0)



static int clear_and_read(tex_t *t, int view, uint32_t *cfmt) {
    memset(&R, 0, sizeof R); R.has_rt = 1; R.rt.n = 1; R.rt.color[0] = t;
    if (view) { R.rt.has_cview[0] = 1; R.rt.cview[0].set_format = 1; R.rt.cview[0].format = NVN_FORMAT_RGBA8_SRGB; }
    R.clear_c[0] = 1; R.cc[0] = MTLClearColorMake(0.5, 0.5, 0.5, 1);
    mtl_sub_begin(); rp_begin_ex(1); *cfmt = g_pass.cfmt[0]; rp_end();
    id<MTLBuffer> b = [g_dev newBufferWithLength:64 options:MTLResourceStorageModeShared]; res_add(b);
    [blit() copyFromTexture:tex_mtl(t) sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(4, 4, 1)
                   toBuffer:b destinationOffset:0 destinationBytesPerRow:16 destinationBytesPerImage:64];
    enc_b_end(g_blit); g_blit = nil;
    uint64_t f = mtl_sub_commit();
    EXPECT("提交完成", 1, mtl_wait(f, 5000000000ull));
    EXPECT("提交反馈无错误", 0, mtl_last_commit_error());
    return ((uint8_t *)b.contents)[0];
}

int main(void) { @autoreleasepool {
    mtl_init();
    tex_t t = {.serial = 3601};
    t.d.width = t.d.height = 4; t.d.depth = t.d.levels = t.d.samples = 1; t.d.target = NVN_TEXTURE_TARGET_2D; t.d.format = NVN_FORMAT_RGBA8;
    uint32_t cf = 0;
    EXPECT("负对照：不带视图清 0.5，底图存线性值", 128, clear_and_read(&t, 0, &cf));
    EXPECT("负对照：管线颜色格式 = 底图 RGBA8Unorm", MTLPixelFormatRGBA8Unorm, cf);
    EXPECT("经 RGBA8_SRGB 视图清 0.5，底图存 sRGB 编码值", 188, clear_and_read(&t, 1, &cf));
    EXPECT("管线颜色格式跟着视图 = RGBA8Unorm_sRGB", MTLPixelFormatRGBA8Unorm_sRGB, cf);

    MTLTextureDescriptor *d = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB width:4 height:4 mipmapped:NO];
    d.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget; d.storageMode = MTLStorageModePrivate;
    id<MTLTexture> s = [g_dev newTextureWithDescriptor:d], s2 = [g_dev newTextureWithDescriptor:d];
    id<MTLTexture> v = present_src(s);
    EXPECT("呈现：sRGB 交换链经 RGBA8Unorm 视图采样", MTLPixelFormatRGBA8Unorm, v.pixelFormat);
    EXPECT("呈现：视图的底图就是交换链贴图", 1, v.parentTexture == s);
    EXPECT("呈现：同一张交换链贴图第二次取到同一个视图（缓存）", 1, present_src(s) == v);
    id<MTLTexture> v2 = present_src(s2);
    EXPECT("呈现：另一张交换链贴图各自一个视图", 1, v2 != v && v2.parentTexture == s2);
    id<MTLTexture> plain = tex_mtl(&t);
    EXPECT("呈现：非 sRGB 交换链原样采样", 1, present_src(plain) == plain);
    EXPECT("全程没有报红", 0, g_red_once[48] + g_red_once[49] + g_red_once[50]);
    printf("== %d 项，失败 %d ==\n", count, fail); return fail != 0;
} }

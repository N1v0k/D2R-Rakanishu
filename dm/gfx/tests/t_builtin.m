



#import <Metal/Metal.h>
#include <stdio.h>
#include <string.h>
#include "../nvn/nvn_builtin.h"

enum { NREC = 40, N = 37, CAP = 64 };

static void expect(int k, const uint64_t *s, uint64_t *e) {
    for (int i = 0; i < CAP; i++) e[i] = 0xAAAAAAAAAAAAAAAAull;
    for (int i = 0; i < N; i++)
        switch (k) {
            case 0: e[i] = s[2 * i]; break;
            case 1: e[i] = s[2 * i + 1]; break;
            case 2: e[i] = s[4 * i + 2] - s[4 * i]; break;
            case 3: e[i] = s[4 * i + 3] - s[4 * i + 1]; break;
        }
}

int main(void) {
    id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
    NSError *err = nil;
    id<MTLLibrary> lib = [dev newLibraryWithSource:@(k_nvn_builtin_msl) options:nil error:&err];
    if (!lib) { printf("MSL 编译失败 %s\n", err.localizedDescription.UTF8String); return 1; }
    id<MTLCommandQueue> q = [dev newCommandQueue];


    uint64_t src[4 * NREC];
    for (int i = 0; i < 4 * NREC; i++) src[i] = 0x0123456789ABCDEFull * (uint64_t)(i + 1) ^ ((uint64_t)i << 40);
    src[2] = 5; src[0] = 9;
    uint32_t n = N;
    id<MTLBuffer> bn = [dev newBufferWithBytes:&n length:4 options:MTLResourceStorageModeShared];
    id<MTLBuffer> bs = [dev newBufferWithBytes:src length:sizeof src options:MTLResourceStorageModeShared];
    id<MTLBuffer> bd = [dev newBufferWithLength:CAP * 8 options:MTLResourceStorageModeShared];
    int fail = 0;
    for (int k = 0; k < 4; k++) {
        id<MTLComputePipelineState> ps = [dev newComputePipelineStateWithFunction:[lib newFunctionWithName:@(k_nvn_builtin_map[k].fn)] error:&err];
        int row[4];
        for (int j = 0; j < 4; j++) {
            memset(bd.contents, 0xAA, CAP * 8);
            id<MTLCommandBuffer> cb = [q commandBuffer]; id<MTLComputeCommandEncoder> ce = [cb computeCommandEncoder];
            [ce setComputePipelineState:ps]; [ce setBuffer:bn offset:0 atIndex:0]; [ce setBuffer:bs offset:0 atIndex:1]; [ce setBuffer:bd offset:0 atIndex:2];
            [ce dispatchThreadgroups:MTLSizeMake((N + 31) / 32, 1, 1) threadsPerThreadgroup:MTLSizeMake(32, 1, 1)];
            [ce endEncoding]; [cb commit]; [cb waitUntilCompleted];
            uint64_t e[CAP]; expect(j, src, e);
            row[j] = !memcmp(bd.contents, e, sizeof e);
        }
        int ok = row[k] && !row[(k + 1) % 4] && !row[(k + 2) % 4] && !row[(k + 3) % 4];
        printf("%s（%s）：对自身期望 %s；对其余 3 个期望全等数 %d（期望 0）→ %s\n", k_nvn_builtin_map[k].fn, k_nvn_builtin_map[k].ctl,
               row[k] ? "全等（含回绕、哨兵 27 个未动）" : "不等", row[(k + 1) % 4] + row[(k + 2) % 4] + row[(k + 3) % 4], ok ? "✓" : "✗");
        fail += !ok;
    }
    printf("== 4 项，失败 %d ==\n", fail);
    return fail != 0;
}

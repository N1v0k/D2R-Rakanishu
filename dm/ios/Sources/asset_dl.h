

#import <Foundation/Foundation.h>
@interface DmAssetDL : NSObject
+ (instancetype)shared;
+ (instancetype)astcPack;


- (void)prepare:(void (^)(NSString *err, int64_t remain, int64_t total, BOOL cellular))done;


@property (atomic) int64_t shortBytes, freeBytes;


- (void)run:(void (^)(int64_t done, int64_t total, NSString *note))progress done:(void (^)(NSString *err))done;
@end
void dm_asset_dl_bg_events(NSString *ident, void (^done)(void));

#import <UIKit/UIKit.h>
#include "dm_touch_controls.h"
#include "dm_touch_game_state.h"
#include "../../host/dm_hud.h"



@interface DmTouchOverlay : UIView
@property(nonatomic, assign) const dm_tc_state *controls;
@property(nonatomic, assign) BOOL navigationMode;
@property(nonatomic, assign) dm_tg_snapshot nativeMapping;
@property(nonatomic, assign) dm_hud_info hud;
@property(nonatomic, assign) BOOL dialogOnly;
@property(nonatomic, assign) BOOL hudBlocked;
- (void)controlsChanged;
- (NSDictionary *)controlMetadata;
@end

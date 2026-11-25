

#pragma once
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
void dm_rift_hud_draw(NSView *host, CGRect world_rect, BOOL hide);
#else
#import <UIKit/UIKit.h>
void dm_rift_hud_draw(UIView *host, CGRect world_rect, BOOL hide);
#endif

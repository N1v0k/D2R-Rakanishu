

#pragma once
#import <Foundation/Foundation.h>

static inline NSString *dm_mac_overlay(NSString *base, NSString *variants, int (*setting)(const char *), BOOL *missing) {
    NSFileManager *fm = NSFileManager.defaultManager;
    NSMutableArray<NSString *> *names = [NSMutableArray array];
    NSMutableSet<NSString *> *keys = [NSMutableSet setWithObject:@"item_notes"];
    for (NSString *name in [fm contentsOfDirectoryAtPath:variants ?: @"" error:NULL]) {
        NSString *path = [variants stringByAppendingPathComponent:name];
        if (![fm isReadableFileAtPath:[path stringByAppendingPathComponent:@"data/.build.config"]]) continue;
        [names addObject:name]; [keys addObjectsFromArray:[name componentsSeparatedByString:@"+"]];
    }
    NSMutableSet<NSString *> *on = [NSMutableSet set];
    for (NSString *key in keys) if (setting(key.UTF8String)) [on addObject:key];
    *missing = NO;
    if (!on.count) return base;
    for (NSString *name in [names sortedArrayUsingSelector:@selector(compare:)])
        if ([[NSSet setWithArray:[name componentsSeparatedByString:@"+"]] isEqualToSet:on])
            return [variants stringByAppendingPathComponent:name];
    *missing = YES;
    return base;
}

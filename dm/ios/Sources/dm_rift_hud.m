











#import "dm_rift_hud.h"
#import <QuartzCore/QuartzCore.h>
#if TARGET_OS_OSX
#define DmRiftView NSView
#define DmRiftFont NSFont
#define DmRiftColor NSColor
#else
#define DmRiftView UIView
#define DmRiftFont UIFont
#define DmRiftColor UIColor
#endif
#include "dm_i18n.h"
#import <CoreText/CoreText.h>
#include "../../host/dm_rift.h"

static NSData *zinflate(const uint8_t *p, size_t n, size_t cap) {
    (void)cap; if (n < 2) return nil; return [[NSData dataWithBytes:p + 2 length:n - 2] decompressedDataUsingAlgorithm:NSDataCompressionAlgorithmZlib error:nil];
}
static NSData *romfs_file(NSString *suffix) {
    const char *root = getenv("DM_ROMFS"); if (!root) return nil;
    NSString *data = [@(root) stringByAppendingPathComponent:@"data"], *psv = [NSString stringWithContentsOfFile:[data stringByAppendingPathComponent:@"build_manifest.psv"] encoding:NSUTF8StringEncoding error:nil];
    for (NSString *line in [psv componentsSeparatedByString:@"\n"]) { NSArray<NSString *> *f = [line componentsSeparatedByString:@"|"];
        if (f.count < 11 || [line hasPrefix:@"#"] || ![f[9].lowercaseString hasSuffix:suffix]) continue;
        NSString *ek = f[6]; if (ek.length < 32) return nil; uint8_t k[16]; for (int i = 0; i < 16; i++) k[i] = (uint8_t)strtol([[ek substringWithRange:NSMakeRange(i * 2, 2)] UTF8String], NULL, 16);
        uint64_t off = 0; for (int i = 11; i < 16; i++) off = off << 8 | k[i]; long rawlen = f[7].longLongValue;
        NSFileHandle *h = [NSFileHandle fileHandleForReadingAtPath:[data stringByAppendingPathComponent:[NSString stringWithFormat:@"%02d-%08d.data", k[9], k[10]]]]; if (!h) return nil;
        [h seekToFileOffset:off]; NSData *raw = [h readDataOfLength:rawlen]; [h closeFile]; const uint8_t *r = raw.bytes; if ((long)raw.length != rawlen) return nil;
        if ([f[8] isEqualToString:@"n"]) return raw;
        if ([f[8] isEqualToString:@"z"]) return zinflate(r, raw.length, 64 << 20);
        uint32_t hs = (uint32_t)r[4] << 24 | r[5] << 16 | r[6] << 8 | r[7], nch = (uint32_t)r[9] << 16 | r[10] << 8 | r[11], pos = hs; NSMutableData *o = [NSMutableData data];
        for (uint32_t i = 0; i < nch; i++) { const uint8_t *e = r + 12 + i * 24; uint32_t es = (uint32_t)e[0] << 24 | e[1] << 16 | e[2] << 8 | e[3], ds = (uint32_t)e[4] << 24 | e[5] << 16 | e[6] << 8 | e[7];
            if (pos + es > raw.length) return nil; const uint8_t *b = r + pos;
            if (b[0] == 'N') [o appendBytes:b + 1 length:es - 1]; else { NSData *d = zinflate(b + 1, es - 1, ds); if (!d) return nil; [o appendData:d]; } pos += es; }
        return o; }
    return nil;
}
static DmRiftFont *d2font(CGFloat size) {
    static NSString *name; static int tried;
    if (!tried) { tried = 1; NSData *d = romfs_file(@"/hd/ui/fonts/blizzardglobal-v5_81.ttf");
        CGDataProviderRef p = d ? CGDataProviderCreateWithCFData((__bridge CFDataRef)d) : NULL; CGFontRef f = p ? CGFontCreateWithDataProvider(p) : NULL;
        if (f && CTFontManagerRegisterGraphicsFont(f, NULL)) name = (__bridge_transfer NSString *)CGFontCopyPostScriptName(f);
        if (f) CGFontRelease(f); if (p) CGDataProviderRelease(p);
        NSLog(@"[秘境] 任务栏字体：%@", name ?: @"读不到原版 BlizzardGlobal，用系统粗体"); }
    return (name ? [DmRiftFont fontWithName:name size:size] : nil) ?: [DmRiftFont boldSystemFontOfSize:size];
}
static DmRiftColor *rgb(int r, int g, int b, CGFloat a) { return [DmRiftColor colorWithRed:r / 255.0 green:g / 255.0 blue:b / 255.0 alpha:a]; }
static CATextLayer *text_layer(void) { CATextLayer *t = [CATextLayer layer];
#if TARGET_OS_OSX
    t.contentsScale = NSScreen.mainScreen.backingScaleFactor ?: 1;
#else
    t.contentsScale = UIScreen.mainScreen.scale;
#endif
     t.shadowOpacity = 0.86; t.shadowOffset = CGSizeMake(1, 1); t.shadowRadius = 0; t.shadowColor = DmRiftColor.blackColor.CGColor; return t; }
static void set_text(CATextLayer *t, NSString *s, DmRiftFont *f, DmRiftColor *c, CGRect fr, NSString *align) {
    t.frame = fr; t.alignmentMode = align; t.foregroundColor = c.CGColor; t.font = (__bridge CFTypeRef)f; t.fontSize = f.pointSize;
    if (![t.string isEqual:s]) t.string = s;
}

void dm_rift_hud_draw(DmRiftView *host, CGRect wr, BOOL hide) {
    static CALayer *box, *lineT, *lineB, *lineL, *track, *ticks[9]; static CAGradientLayer *fill; static CAShapeLayer *pin[2]; static CATextLayer *title, *sub, *timer, *goal;
    dm_rift_hud_t h; int on = !hide && dm_rift_hud(&h);
    if (!box) { if (!on) return;
        box = [CALayer layer];
#if TARGET_OS_OSX
        box.geometryFlipped = YES;
#endif
        box.backgroundColor = rgb(0, 0, 0, 170 / 255.0).CGColor;
        lineT = [CALayer layer]; lineB = [CALayer layer]; lineL = [CALayer layer]; track = [CALayer layer]; fill = [CAGradientLayer layer];
        title = text_layer(); sub = text_layer(); timer = text_layer(); goal = text_layer();
        for (CALayer *l in @[lineT, lineB, lineL, title, sub, timer, goal, track]) [box addSublayer:l];
        [track addSublayer:fill]; for (int i = 0; i < 9; i++) { ticks[i] = [CALayer layer]; ticks[i].backgroundColor = rgb(0, 0, 0, 110 / 255.0).CGColor; [track addSublayer:ticks[i]]; }
        for (int i = 0; i < 2; i++) { pin[i] = [CAShapeLayer layer]; pin[i].fillColor = rgb(160, 135, 80, 1).CGColor; pin[i].strokeColor = rgb(60, 48, 30, 1).CGColor; [box addSublayer:pin[i]]; }
        [host.layer addSublayer:box]; }
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    box.hidden = !on;
    if (on) {
        CGFloat u = wr.size.height / 614, W = 250 * u, H = 78 * u;

#if TARGET_OS_OSX
        box.frame = CGRectMake(wr.origin.x + 28 * u, CGRectGetMaxY(wr) - 150 * u - H, W, H);
        CGFloat scale = host.window.backingScaleFactor ?: NSScreen.mainScreen.backingScaleFactor ?: 1;
        for (CATextLayer *text in @[title, sub, timer, goal]) text.contentsScale = scale;
#else
        box.frame = CGRectMake(wr.origin.x + 28 * u, wr.origin.y + 150 * u, W, H);
#endif
        lineT.frame = CGRectMake(0, 0, W, MAX(1, u)); lineB.frame = CGRectMake(0, H - MAX(1, u), W, MAX(1, u)); lineL.frame = CGRectMake(0, 0, 2 * u, H);
        lineT.backgroundColor = lineB.backgroundColor = rgb(120, 100, 60, 0.9).CGColor; lineL.backgroundColor = rgb(150, 125, 75, 1).CGColor;
        int g = h.guardian, s = h.secs_left < 0 ? 0 : h.secs_left; DmRiftColor *gold = rgb(199, 179, 119, 1), *red = rgb(200, 50, 40, 1), *white = rgb(215, 210, 200, 1), *grey = rgb(150, 145, 135, 1);
        set_text(title, @(dm_ui("dmRiftHudTitle")), d2font(17 * u), g ? red : gold, CGRectMake(10 * u, 6 * u, 40 * u, 22 * u), kCAAlignmentLeft);
        NSString *st = h.done ? (h.inc ? [NSString stringWithFormat:@(dm_ui("dmRiftHudDone")), h.tier] : [NSString stringWithFormat:@(dm_ui("dmRiftHudLate")), h.tier]) : g ? @(dm_ui("dmRiftHudGuard")) : [NSString stringWithFormat:@(dm_ui("dmRiftTierN")), h.tier];
        set_text(sub, st, d2font(14 * u), white, CGRectMake(48 * u, 9 * u, 130 * u, 19 * u), kCAAlignmentLeft);
        set_text(timer, [NSString stringWithFormat:@"%d:%02d", s / 60, s % 60], d2font(17 * u), (s <= 60 && !h.done) ? red : gold, CGRectMake(W - 90 * u, 6 * u, 80 * u, 22 * u), kCAAlignmentRight);
        NSString *gl = h.done ? (!h.inc ? @(dm_ui("dmRiftHudSame")) : h.next > h.tier ? [NSString stringWithFormat:@(dm_ui("dmRiftHudNext")), h.next] : @(dm_ui("dmRiftHudTop"))) : h.goal == 5 ? [NSString stringWithFormat:@(dm_ui("dmRiftHudOverKillN")), h.gdead, h.gtotal] : h.goal == 4 ? @(dm_ui("dmRiftHudOverKill")) : h.goal == 3 ? @(dm_ui("dmRiftHudOver"))
            : (h.goal == 1 || h.goal == 2) && h.nwave > 1 ? [NSString stringWithFormat:@(dm_ui("dmRiftHudKillWave")), h.wave, h.nwave, h.gdead, h.gtotal] : h.goal == 2 ? [NSString stringWithFormat:@(dm_ui("dmRiftHudKillN")), h.gdead, h.gtotal] : h.goal == 1 ? @(dm_ui("dmRiftHudKill")) : [NSString stringWithFormat:@(dm_ui("dmRiftHudGoal")), (int)(h.progress * 100)];
        set_text(goal, gl, d2font(12 * u), grey, CGRectMake(10 * u, 32 * u, W - 20 * u, 17 * u), kCAAlignmentLeft);
        CGRect tr = CGRectMake(11 * u, 55 * u, W - 22 * u, 10 * u);
        track.frame = tr; track.backgroundColor = rgb(25, 20, 14, 1).CGColor; track.borderColor = rgb(95, 78, 48, 1).CGColor; track.borderWidth = MAX(1, u);
        CGFloat bw = MAX(1, u); fill.frame = CGRectMake(bw, bw, (tr.size.width - 2 * bw) * (h.done ? 1 : h.progress), tr.size.height - 2 * bw);
        fill.colors = g ? @[(id)rgb(190, 50, 40, 1).CGColor, (id)rgb(110, 20, 15, 1).CGColor] : @[(id)rgb(205, 160, 80, 1).CGColor, (id)rgb(140, 100, 40, 1).CGColor];
        for (int i = 0; i < 9; i++) ticks[i].frame = CGRectMake(bw + (tr.size.width - 2 * bw) * (i + 1) / 10, bw, MAX(1, u), tr.size.height - 2 * bw);
        for (int i = 0; i < 2; i++) { CGFloat cx = i ? tr.origin.x + tr.size.width + 4 * u : tr.origin.x - 4 * u, cy = CGRectGetMidY(tr), r = 4 * u; CGMutablePathRef p = CGPathCreateMutable();
            CGPathMoveToPoint(p, NULL, cx, cy - r); CGPathAddLineToPoint(p, NULL, cx + r, cy); CGPathAddLineToPoint(p, NULL, cx, cy + r); CGPathAddLineToPoint(p, NULL, cx - r, cy); CGPathCloseSubpath(p); pin[i].path = p; CGPathRelease(p); pin[i].lineWidth = MAX(1, u * 0.8); }
    }
    [CATransaction commit];
}

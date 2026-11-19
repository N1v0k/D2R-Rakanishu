







#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "../../ios/Sources/DmTouchControlArt.h"

static const CGFloat S = 3;
static CGImageRef load(const char *path) {
    CGImageSourceRef src = CGImageSourceCreateWithURL((__bridge CFURLRef)[NSURL fileURLWithPath:@(path)], NULL);
    CGImageRef img = src ? CGImageSourceCreateImageAtIndex(src, 0, NULL) : NULL; if (src) CFRelease(src); return img;
}
static void save(CGContextRef c, NSString *dir, NSString *name) {
    CGImageRef img = CGBitmapContextCreateImage(c);
    CGImageDestinationRef d = CGImageDestinationCreateWithURL((__bridge CFURLRef)[NSURL fileURLWithPath:[dir stringByAppendingPathComponent:[name stringByAppendingString:@".png"]]],
                                                              (__bridge CFStringRef)UTTypePNG.identifier, 1, NULL);
    CGImageDestinationAddImage(d, img, NULL); CGImageDestinationFinalize(d); CFRelease(d); CGImageRelease(img);
}


static CGContextRef canvas(CGFloat side, CGFloat *pt) {
    size_t px = (size_t)llround(side * S); CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef c = CGBitmapContextCreate(NULL, px, px, 8, 0, cs, (CGBitmapInfo)kCGImageAlphaPremultipliedLast); CGColorSpaceRelease(cs);
    CGContextTranslateCTM(c, 0, px); CGContextScaleCTM(c, px / side, -(CGFloat)px / side); *pt = side; return c;
}


static void draw_image(CGContextRef c, CGImageRef img, CGRect r, CGBlendMode mode) {
    CGContextSaveGState(c); CGContextTranslateCTM(c, 0, CGRectGetMaxY(r) + CGRectGetMinY(r)); CGContextScaleCTM(c, 1, -1);
    CGContextSetBlendMode(c, mode); CGContextDrawImage(c, r, img); CGContextRestoreGState(c);
}

int main(int argc, char **argv) {
    if (argc != 4) { fprintf(stderr, "用法：art_ref <dm_mobile_socket.png> <sil_dm_native_skill_0.png> <输出目录>\n"); return 2; }
    CGImageRef socket = load(argv[1]), sil = load(argv[2]); NSString *dir = @(argv[3]);
    if (!socket || !sil) { fprintf(stderr, "读不了输入图\n"); return 1; }
    [NSFileManager.defaultManager createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil];
    CGFloat side;
    for (int h = 0; h < 2; h++) for (int e = 0; e < 2; e++) for (int p = 0; p < 2; p++) {
        CGContextRef c = canvas(80, &side); dm_button_draw(c, CGRectMake(0, 0, side, side), h, e, p);
        save(c, dir, [NSString stringWithFormat:@"button_h%de%dp%d", h, e, p]); CGContextRelease(c);
    }
    for (int h = 0; h < 2; h++) {
        CGContextRef c = canvas(80, &side); CGRect r = CGRectMake(0, 0, side, side);
        draw_image(c, socket, r, kCGBlendModeNormal); dm_socket_neutralize(c, r); if (h) dm_button_highlight(c, r);
        save(c, dir, [NSString stringWithFormat:@"socket_h%d", h]); CGContextRelease(c);
    }
    for (int a = 0; a < 2; a++) {
        CGContextRef c = canvas(120, &side); dm_joystick_base_draw(c, CGRectMake(0, 0, side, side), a);
        save(c, dir, [NSString stringWithFormat:@"joybase_a%d", a]); CGContextRelease(c);
        c = canvas(46, &side); dm_joystick_thumb_draw(c, CGRectMake(0, 0, side, side), a);
        save(c, dir, [NSString stringWithFormat:@"joythumb_a%d", a]); CGContextRelease(c);
    }
    {
        size_t w = CGImageGetWidth(sil), hh = CGImageGetHeight(sil); CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
        CGContextRef c = CGBitmapContextCreate(NULL, w, hh, 8, 0, cs, (CGBitmapInfo)kCGImageAlphaPremultipliedLast); CGColorSpaceRelease(cs);
        CGRect r = CGRectMake(0, 0, w, hh);
        CGContextDrawImage(c, r, sil); dm_skill_silver(c, r); CGContextSetBlendMode(c, kCGBlendModeDestinationIn); CGContextDrawImage(c, r, sil);
        save(c, dir, @"silver0"); CGContextRelease(c);
    }
    CGImageRelease(socket); CGImageRelease(sil); return 0;
}

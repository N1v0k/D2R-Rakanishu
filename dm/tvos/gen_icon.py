#!/usr/bin/env python3






import json, os, sys, hashlib
from PIL import Image
src, out = sys.argv[1], sys.argv[2]
digest = hashlib.sha256(open(src, 'rb').read() + open(__file__, 'rb').read()).hexdigest()
stamp = os.path.join(out, '.src.sha256')
if os.path.exists(stamp) and open(stamp).read() == digest: sys.exit(0)
icon = Image.open(src).convert('RGBA')
w0, h0 = icon.size
corner = icon.getpixel((2, 2))

def put(path, obj):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w') as f: json.dump(obj, f, indent=2)

def stack(name, w, h, scales):
    root = os.path.join(out, 'AppIcon.brandassets', name + '.imagestack')
    put(os.path.join(root, 'Contents.json'), {'info': {'version': 1, 'author': 'xcode'}, 'layers': [{'filename': 'Front.imagestacklayer'}, {'filename': 'Back.imagestacklayer'}]})
    for layer in ('Front', 'Back'):
        d = os.path.join(root, layer + '.imagestacklayer')
        put(os.path.join(d, 'Contents.json'), {'info': {'version': 1, 'author': 'xcode'}})
        images = []
        for s in scales:
            W, H = w * s, h * s
            if layer == 'Back': img = Image.new('RGBA', (W, H), corner)
            else:
                img = Image.new('RGBA', (W, H), (0, 0, 0, 0))
                side = H; fg = icon.resize((side * w0 // h0, side), Image.LANCZOS)
                img.paste(fg, ((W - fg.width) // 2, 0))
            fn = f'{layer.lower()}@{s}x.png'
            os.makedirs(os.path.join(d, 'Content.imageset'), exist_ok=True)
            img.save(os.path.join(d, 'Content.imageset', fn))
            images.append({'idiom': 'tv', 'filename': fn, 'scale': f'{s}x'})
        put(os.path.join(d, 'Content.imageset', 'Contents.json'), {'images': images, 'info': {'version': 1, 'author': 'xcode'}})

put(os.path.join(out, 'Contents.json'), {'info': {'version': 1, 'author': 'xcode'}})
put(os.path.join(out, 'AppIcon.brandassets', 'Contents.json'), {'info': {'version': 1, 'author': 'xcode'}, 'assets': [
    {'size': '400x240', 'idiom': 'tv', 'filename': 'Small.imagestack', 'role': 'primary-app-icon'},
    {'size': '1280x768', 'idiom': 'tv', 'filename': 'Large.imagestack', 'role': 'primary-app-icon'}]})
stack('Small', 400, 240, (1, 2))
stack('Large', 1280, 768, (1,))
open(stamp, 'w').write(digest)

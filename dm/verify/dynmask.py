



import sys
import numpy as np
from PIL import Image, ImageFilter
a, b, out = sys.argv[1:4]; th = int(sys.argv[4]) if len(sys.argv) > 4 else 8; dil = int(sys.argv[5]) if len(sys.argv) > 5 else 3
A = np.asarray(Image.open(a).convert('RGB'), dtype=int); B = np.asarray(Image.open(b).convert('RGB').resize((A.shape[1], A.shape[0])), dtype=int)
m = Image.fromarray(((np.abs(A - B).sum(2) > th) * 255).astype(np.uint8)).filter(ImageFilter.MaxFilter(2 * dil + 1))
m.save(out); print(f'DYNMASK 动区占比 {np.asarray(m).mean() / 255:.3f} → {out}')

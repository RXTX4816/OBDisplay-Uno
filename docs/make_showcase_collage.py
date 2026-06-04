from PIL import Image, ImageOps
import numpy as np
import os

SRC_DIR = 'CHANGE_ME'
OUT_PATH = 'CHANGE_ME/collage.jpg'

# OLED corners in EXIF-corrected portrait image (3000x4000).
# Left edge:  x = 219 + (y-100)*(-54/3500), inset +30px
# Right edge: x = 2395 + (y-100)*(-0.0882), inset -30px
# Top/bottom: y = 70 / 3630
Y_TOP, Y_BOT = 70, 3630
INSET = 30

def _le(y): return int(219 + (y-100)*(-54/3500)) + INSET
def _re(y): return int(2395 + (y-100)*(-0.0882)) - INSET

CORNERS = [
    (_le(Y_TOP), Y_TOP),
    (_re(Y_TOP), Y_TOP),
    (_re(Y_BOT), Y_BOT),
    (_le(Y_BOT), Y_BOT),
]

# Output cell — width:height ≈ 0.574 (measured display aspect in portrait)
CELL_W = 200
CELL_H = 348
GAP    = 5
MARGIN = 10
COLS   = 8
ROWS   = 3
BG     = (10, 10, 10)


def find_coeffs(src_corners, dst_w, dst_h):
    """Compute PIL PERSPECTIVE coefficients: destination (x,y) → source (x',y')."""
    dst = [(0, 0), (dst_w, 0), (dst_w, dst_h), (0, dst_h)]
    A, B = [], []
    for (xs, ys), (xd, yd) in zip(src_corners, dst):
        A.append([xd, yd, 1, 0,  0,  0, -xd*xs, -yd*xs])
        A.append([0,  0,  0, xd, yd, 1, -xd*ys, -yd*ys])
        B += [xs, ys]
    coeffs = np.linalg.solve(np.array(A, dtype=np.float64),
                             np.array(B, dtype=np.float64))
    return tuple(float(c) for c in coeffs)


def extract_display(path):
    img = Image.open(path)
    img = ImageOps.exif_transpose(img)
    coeffs = find_coeffs(CORNERS, CELL_W, CELL_H)
    cropped = img.transform((CELL_W, CELL_H), Image.PERSPECTIVE, coeffs,
                            Image.BICUBIC)
    return cropped


total_w = MARGIN*2 + CELL_W*COLS + GAP*(COLS-1)
total_h = MARGIN*2 + CELL_H*ROWS + GAP*(ROWS-1)
collage = Image.new('RGB', (total_w, total_h), BG)

files = sorted(f for f in os.listdir(SRC_DIR) if f.endswith('.jpg'))
print(f"Processing {len(files)} images → {total_w}×{total_h} collage")

for idx, fname in enumerate(files):
    row = idx // COLS
    col = idx % COLS
    x = MARGIN + col * (CELL_W + GAP)
    y = MARGIN + row * (CELL_H + GAP)
    cell = extract_display(os.path.join(SRC_DIR, fname))
    collage.paste(cell, (x, y))
    print(f"  [{idx+1:2d}/24] {fname}")

collage.save(OUT_PATH, quality=92)
print(f"\nSaved: {OUT_PATH}  ({total_w}×{total_h})")

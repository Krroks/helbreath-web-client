# Compares two 640x480 captures of the game in the RGB565 domain (native cnc-ddraw PNG vs web canvas PNG).
# Going back to 5:6:5 makes the comparison independent of how each side expanded 16-bit color to 24-bit.
# usage: python framediff.py A.png B.png OUT_diff.png [x0,y0,x1,y1 ...ignored boxes]
import sys
from PIL import Image

def to565(path):
    im = Image.open(path).convert("RGB")
    assert im.size == (640, 480), f"{path}: {im.size}"
    return [((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3) for r, g, b in im.getdata()]

def main(a, b, out, boxes):
    pa, pb = to565(a), to565(b)
    ignore = [tuple(map(int, s.split(","))) for s in boxes]
    diff = Image.new("RGB", (640, 480))
    dp = diff.load()
    bad = []
    for i, (x, y) in enumerate((i % 640, i // 640) for i in range(640 * 480)):
        if any(x0 <= x <= x1 and y0 <= y <= y1 for x0, y0, x1, y1 in ignore):
            dp[x, y] = (0, 0, 80)
            continue
        if pa[i] != pb[i]:
            bad.append((x, y))
            dp[x, y] = (255, 0, 0)
        else:
            v = ((pa[i] >> 11) << 3) // 3
            dp[x, y] = (v, v, v)
    diff.save(out)
    print(f"{len(bad)} differing pixels")
    if bad:
        xs, ys = [p[0] for p in bad], [p[1] for p in bad]
        print(f"bbox {min(xs)},{min(ys)} - {max(xs)},{max(ys)}")
        # rows with differences, grouped, to locate regions quickly
        rows = sorted(set(ys))
        groups, start = [], rows[0]
        for r0, r1 in zip(rows, rows[1:] + [None]):
            if r1 != r0 + 1:
                groups.append((start, r0)); start = r1
        for g0, g1 in groups[:15]:
            gx = [p[0] for p in bad if g0 <= p[1] <= g1]
            print(f"  rows {g0}-{g1}: {len(gx)} px, x {min(gx)}-{max(gx)}")
        for x, y in bad[:5]:
            i = y * 640 + x
            print(f"  ({x},{y}) a={pa[i]:04x} b={pb[i]:04x}")

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:])

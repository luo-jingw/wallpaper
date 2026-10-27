"""Aligns a generated part layer (for example one whale fin) to where that part is drawn in a reference image.

The part comes back moved, slightly scaled, or rotated. The search moves the part's own opaque pixels by a
similarity transform (scale and rotation about the part's centre, then a shift) and scores the colour
difference against the reference at the moved positions; a moved pixel that lands on a transparent
reference pixel costs a fixed penalty. Coarse grid first, then a finer grid around the best result.

Usage: python register_part.py <part> <reference> <output>
"""

from __future__ import annotations

import itertools
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image

MISS_PENALTY = 300.0


def transform_points(xy: np.ndarray, centre: tuple[float, float], scale: float, degrees: float,
                     dx: float, dy: float) -> np.ndarray:
    r = math.radians(degrees)
    c, s = math.cos(r) * scale, math.sin(r) * scale
    x, y = xy[:, 0] - centre[0], xy[:, 1] - centre[1]
    return np.stack([c * x - s * y + centre[0] + dx, s * x + c * y + centre[1] + dy], axis=1)


def score(ref: np.ndarray, xy: np.ndarray, colours: np.ndarray) -> float:
    h, w = ref.shape[:2]
    p = np.rint(xy).astype(int)
    inside = (p[:, 0] >= 0) & (p[:, 0] < w) & (p[:, 1] >= 0) & (p[:, 1] < h)
    p = np.clip(p, 0, [w - 1, h - 1])
    target = ref[p[:, 1], p[:, 0]]
    diff = np.abs(target[:, :3] - colours).sum(axis=1)
    diff = np.where((target[:, 3] > 200) & inside, diff, MISS_PENALTY)
    return float(diff.mean())


def main() -> None:
    part_path, ref_path, out_path = (Path(a) for a in sys.argv[1:4])
    part = Image.open(part_path).convert("RGBA")
    ref = np.asarray(Image.open(ref_path).convert("RGBA"), dtype=np.float32)
    arr = np.asarray(part, dtype=np.float32)
    ys, xs = np.nonzero(arr[:, :, 3] > 200)
    keep = (ys % 2 == 0) & (xs % 2 == 0)
    xy = np.stack([xs[keep], ys[keep]], axis=1).astype(np.float32)
    colours = arr[ys[keep], xs[keep], :3]
    centre = (float(xy[:, 0].mean()), float(xy[:, 1].mean()))

    def best(grid):
        return min((score(ref, transform_points(xy, centre, s, a, dx, dy), colours), s, a, dx, dy)
                   for s, a, dx, dy in grid)

    coarse = best(itertools.product([0.9, 0.95, 1.0, 1.05, 1.1], [-10, -5, 0, 5, 10],
                                    range(-30, 31, 3), range(-70, 11, 3)))
    print(f"coarse: mean|diff|={coarse[0]:.1f} scale={coarse[1]} angle={coarse[2]} dx={coarse[3]} dy={coarse[4]}")
    _, s0, a0, dx0, dy0 = coarse
    fine = best(itertools.product([s0 + 0.01 * i for i in range(-4, 5)], [a0 + 1.0 * i for i in range(-4, 5)],
                                  range(dx0 - 3, dx0 + 4), range(dy0 - 3, dy0 + 4)))
    print(f"fine:   mean|diff|={fine[0]:.1f} scale={fine[1]:.2f} angle={fine[2]:.1f} dx={fine[3]} dy={fine[4]}")
    print(f"identity: mean|diff|={score(ref, xy, colours):.1f}")

    _, s, a, dx, dy = fine
    # Image.transform needs the inverse map: output pixel -> source pixel.
    r = math.radians(a)
    c, si = math.cos(r) / s, math.sin(r) / s
    cx, cy = centre
    ox, oy = cx + dx, cy + dy
    coefficients = (c, si, cx - c * ox - si * oy, -si, c, cy + si * ox - c * oy)
    moved = part.convert("RGBa").transform(part.size, Image.AFFINE, coefficients, resample=Image.BICUBIC)
    moved.convert("RGBA").save(out_path)


if __name__ == "__main__":
    main()

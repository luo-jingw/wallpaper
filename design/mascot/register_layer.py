"""Finds the scale and offset that line a generated layer up with the base image, and writes the aligned layer.

Some generated layers come back slightly scaled or shifted. The search compares the layer, scaled about the
canvas centre and shifted, with 00_base_eyes_open.png inside a box where both show the same content, coarse
on a reduced copy and then fine at full size, and prints the residual difference for each stage.

Usage: python register_layer.py <input> <output> <left> <top> <right> <bottom>
"""

from __future__ import annotations

import itertools
import sys
from pathlib import Path

from PIL import Image, ImageChops

ROOT: Path = Path(__file__).resolve().parent
BASE: Path = ROOT / "source" / "00_base_eyes_open.png"


def on_white(image: Image.Image) -> Image.Image:
    bg = Image.new("RGBA", image.size, "white")
    bg.alpha_composite(image)
    return bg.convert("L")


def place(layer: Image.Image, scale: float, dx: float, dy: float) -> Image.Image:
    """Scale about the canvas centre, then shift; output keeps the canvas size."""
    w, h = layer.size
    cx, cy = w / 2, h / 2
    # Inverse map for Image.transform: source = (target - centre - shift) / scale + centre.
    a = 1 / scale
    coefficients = (a, 0, cx - (cx + dx) * a, 0, a, cy - (cy + dy) * a)
    return layer.convert("RGBa").transform(layer.size, Image.AFFINE, coefficients, resample=Image.BICUBIC).convert("RGBA")


def score(base_l: Image.Image, layer: Image.Image, box: tuple[int, int, int, int]) -> float:
    diff = ImageChops.difference(base_l.crop(box), on_white(layer).crop(box))
    values = list(diff.getdata())
    return sum(values) / len(values)


def main() -> None:
    source, target = Path(sys.argv[1]), Path(sys.argv[2])
    box = tuple(int(v) for v in sys.argv[3:7])
    base = Image.open(BASE).convert("RGBA")
    layer = Image.open(source).convert("RGBA")

    shrink = 4
    small_base = on_white(base.resize((base.width // shrink, base.height // shrink)))
    small_layer = layer.resize((layer.width // shrink, layer.height // shrink))
    small_box = tuple(v // shrink for v in box)
    coarse = min(
        (score(small_base, place(small_layer, s, dx, dy), small_box), s, dx * shrink, dy * shrink)
        for s in [0.90 + 0.01 * i for i in range(21)]
        for dx, dy in itertools.product(range(-10, 11), repeat=2)
    )
    print(f"coarse: mean|diff|={coarse[0]:.2f} scale={coarse[1]:.3f} dx={coarse[2]} dy={coarse[3]}")

    base_l = on_white(base)
    _, s0, dx0, dy0 = coarse
    fine = min(
        (score(base_l, place(layer, s, dx, dy), box), s, dx, dy)
        for s in [s0 + 0.0025 * i for i in range(-4, 5)]
        for dx in range(dx0 - 4, dx0 + 5, 2)
        for dy in range(dy0 - 4, dy0 + 5, 2)
    )
    print(f"fine:   mean|diff|={fine[0]:.2f} scale={fine[1]:.4f} dx={fine[2]} dy={fine[3]}")
    print(f"identity: mean|diff|={score(base_l, layer, box):.2f}")
    place(layer, fine[1], fine[2], fine[3]).save(target)


if __name__ == "__main__":
    main()

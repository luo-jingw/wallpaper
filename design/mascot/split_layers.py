"""Cuts the aligned mascot variants in design/mascot/source into patch layers.

Every variant was produced by editing 00_base_eyes_open.png, so the images share one 1254x1254 canvas.
Each layer keeps only one region of a variant (with a feathered edge) on a transparent canvas of the
same size, so stacking base + layers at (0, 0) reproduces the variant. Prints, per layer, how far the
stacked result is from the original variant inside and outside the region.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT: Path = Path(__file__).resolve().parent
SOURCE: Path = ROOT / "source"
LAYERS: Path = ROOT / "layers"
FEATHER: int = 6


@dataclass(frozen=True)
class Region:
    left: int
    top: int
    right: int
    bottom: int


EYES = Region(420, 460, 830, 640)
FACE = Region(420, 460, 830, 690)   # eyes, cheeks, and mouth
MOUTH = Region(578, 618, 652, 684)
ARMS = Region(370, 640, 870, 960)   # raised arms with props, and the hanging arms they replace
# 03's happy eyes and blushed cheeks without its open mouth (x 592-640, y 615-655): chewing adds its own mouth.
# The cheeks stop above the eating pose's bowl rim (y 680) and the rice on the spoon (x 660-760, y 650-690).
HAPPY_EYES = (Region(420, 460, 830, 610), Region(420, 600, 585, 674), Region(650, 600, 830, 650))


@dataclass(frozen=True)
class LayerSpec:
    name: str
    variant: str
    reference: str  # the image the variant was edited from
    regions: tuple[Region, ...]


SPECS: list[LayerSpec] = [
    LayerSpec("eyes_half", "01_eyes_half.png", "00_base_eyes_open.png", (EYES,)),
    LayerSpec("eyes_closed", "02_eyes_closed.png", "00_base_eyes_open.png", (EYES,)),
    LayerSpec("face_happy", "03_happy_blush.png", "00_base_eyes_open.png", (FACE,)),
    LayerSpec("eyes_happy", "03_happy_blush.png", "00_base_eyes_open.png", HAPPY_EYES),  # squint while chewing
    LayerSpec("mouth_chew", "04_chew.png", "00_base_eyes_open.png", (MOUTH,)),
    LayerSpec("mouth_open", "05_mouth_open.png", "00_base_eyes_open.png", (MOUTH,)),
    LayerSpec("arms_eat", "06_eat_bowl_spoon.png", "00_base_eyes_open.png", (ARMS,)),
    LayerSpec("arms_write", "07_write_notebook.png", "00_base_eyes_open.png", (ARMS,)),
    LayerSpec("eyes_down", "08_write_look_down.png", "07_write_notebook.png", (EYES,)),
]


def feathered_mask(size: tuple[int, int], regions: tuple[Region, ...]) -> Image.Image:
    mask = Image.new("L", size, 0)
    draw = ImageDraw.Draw(mask)
    for region in regions:
        draw.rectangle(
            (region.left + FEATHER, region.top + FEATHER, region.right - FEATHER, region.bottom - FEATHER), fill=255
        )
    return mask.filter(ImageFilter.GaussianBlur(FEATHER / 2))


def bounds(regions: tuple[Region, ...]) -> tuple[int, int, int, int]:
    return (min(r.left for r in regions), min(r.top for r in regions),
            max(r.right for r in regions), max(r.bottom for r in regions))


def cut(variant: Image.Image, regions: tuple[Region, ...]) -> Image.Image:
    alpha = ImageChops.multiply(variant.getchannel("A"), feathered_mask(variant.size, regions))
    layer = variant.copy()
    layer.putalpha(alpha)
    return layer


def mean_difference(a: Image.Image, b: Image.Image, box: tuple[int, int, int, int] | None) -> float:
    da = a.crop(box) if box else a
    db = b.crop(box) if box else b
    diff = ImageChops.difference(da.convert("RGB"), db.convert("RGB")).convert("L")
    values = list(diff.getdata())
    return sum(values) / len(values)


def main() -> None:
    LAYERS.mkdir(exist_ok=True)
    base = Image.open(SOURCE / "00_base_eyes_open.png").convert("RGBA")
    base.save(LAYERS / "base.png")
    print(f"base.png  size={base.size} mode={base.mode}")
    for spec in SPECS:
        variant = Image.open(SOURCE / spec.variant).convert("RGBA")
        reference = Image.open(SOURCE / spec.reference).convert("RGBA")
        layer = cut(variant, spec.regions)
        layer.save(LAYERS / f"{spec.name}.png")
        stacked = reference.copy()
        stacked.alpha_composite(layer)
        box = bounds(spec.regions)
        print(
            f"{spec.name + '.png':16s} from {spec.variant:24s} region={box} "
            f"inside mean|diff|={mean_difference(stacked, variant, box):5.2f} "
            f"whole mean|diff|={mean_difference(stacked, variant, None):5.2f} "
            f"(reference vs variant inside={mean_difference(reference, variant, box):5.2f})"
        )


AHOGE_ERASE_BELOW: int = 140  # body pixels of the ahoge above this y are erased; below, the headdress hides its root


def opaque(image: Image.Image, threshold: int = 20) -> Image.Image:
    return image.getchannel("A").point(lambda v: 255 if v > threshold else 0)


def count(mask: Image.Image) -> int:
    return sum(1 for v in mask.getdata() if v)


def ahoge_keep_mask() -> Image.Image:
    """255 except where the ahoge shows above the headdress; layers carrying the head are multiplied by it."""
    ahoge = Image.open(SOURCE / "15_ahoge.png").convert("RGBA")
    reach = opaque(ahoge).filter(ImageFilter.MaxFilter(7))
    above = Image.new("L", ahoge.size, 0)
    ImageDraw.Draw(above).rectangle((0, 0, ahoge.size[0], AHOGE_ERASE_BELOW), fill=255)
    return ImageChops.invert(ImageChops.multiply(reach, above))


def build_part_layers() -> None:
    """Moving parts drawn separately: back hair, tail, and ahoge behind the body.

    Stack order: hair_back, tail, ahoge, body | body_eat | body_write, then the face patches.
    """
    base = Image.open(SOURCE / "00_base_eyes_open.png").convert("RGBA")
    tail = Image.open(SOURCE / "10_tail.png").convert("RGBA")
    hair = Image.open(SOURCE / "11_hair_back.png").convert("RGBA")
    ahoge = Image.open(SOURCE / "15_ahoge.png").convert("RGBA")

    # The ahoge sways as its own layer, so each body keeps none of it above the headdress.
    keep = ahoge_keep_mask()
    erase = ImageChops.invert(keep)
    bodies: dict[str, Image.Image] = {}
    for name, source in [("body", "12_body.png"), ("body_eat", "13_body_eat.png"), ("body_write", "14_body_write.png")]:
        body = Image.open(SOURCE / source).convert("RGBA")
        body.putalpha(ImageChops.multiply(body.getchannel("A"), keep))
        bodies[name] = body
    print(f"ahoge pixels erased from each body: {count(ImageChops.multiply(erase, opaque(Image.open(SOURCE / '12_body.png'))))}")

    # Pixels the base shows but no part layer covers belong to the back of the head; the back hair takes them.
    stack = Image.new("RGBA", base.size, (0, 0, 0, 0))
    for part in (hair, tail, ahoge, bodies["body"]):
        stack.alpha_composite(part)
    gap = ImageChops.multiply(opaque(base), ImageChops.invert(opaque(stack)))
    filled = hair.copy()
    filled.paste(base, mask=gap)
    print(f"gap pixels moved from base into hair_back: {count(gap)}")

    for name, image in [("hair_back", filled), ("tail", tail), ("ahoge", ahoge), *bodies.items()]:
        image.save(LAYERS / f"{name}.png")

    stack = Image.new("RGBA", base.size, (0, 0, 0, 0))
    for part in (filled, tail, ahoge, bodies["body"]):
        stack.alpha_composite(part)
    uncovered = count(ImageChops.multiply(opaque(base), ImageChops.invert(opaque(stack))))
    print(
        f"hair_back+tail+ahoge+body vs base: whole mean|diff|={mean_difference(stack, base, None):5.2f} "
        f"base pixels left uncovered={uncovered}"
    )


PROP_BOX: tuple[int, int, int, int] = (360, 600, 880, 980)  # where the eating and writing arms differ from idle
OPENING_GROW: int = 4   # the eye opening extends this far past the iris, covering the head layer's own iris
SCLERA_REACH: int = 12  # white base pixels this close to an iris (and not below it) belong to its eye opening
SCLERA = (250, 250, 253)
LID_COLOR = (70, 42, 52, 255)
LID_WIDTH: int = 4      # source pixels; about two thirds of a skin pixel


def convex_hull(points: list[tuple[int, int]]) -> list[tuple[int, int]]:
    """Andrew's monotone chain; counter-clockwise, without the repeated first point."""
    pts = sorted(set(points))
    if len(pts) < 3:
        return pts

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    lower: list[tuple[int, int]] = []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    upper: list[tuple[int, int]] = []
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def is_lash(pixel: tuple[int, int, int, int]) -> bool:
    """Dark brown or black, not the dark navy of the iris."""
    r, g, b, a = pixel
    return a > 128 and (r + g + b) / 3 < 95 and b <= r + 8


def build_head_layers() -> None:
    """Layers for the separate head, the posed arms in front of it, and the moving irises.

    Stack order (bottom to top): hair_back, tail, ahoge, lower body, torso, head, sclera, iris (clipped to the
    sclera), lashes, face patches, arms_front.
    """
    keep = ahoge_keep_mask()
    # The head without its whale fins; the fins are separate layers drawn behind the head, so their roots
    # disappear under the hair (22 needed no alignment; 23 and 24 were aligned by register_part.py).
    head = Image.open(SOURCE / "22_head_no_fins.png").convert("RGBA")
    head.putalpha(ImageChops.multiply(head.getchannel("A"), keep))
    head.save(LAYERS / "head.png")
    for side, source in (("left", "24_fin_left.png"), ("right", "23_fin_right.png")):
        Image.open(SOURCE / source).convert("RGBA").save(LAYERS / f"fin_{side}.png")
    torso = Image.open(SOURCE / "18_torso_no_head.png").convert("RGBA")
    torso.save(LAYERS / "torso.png")
    idle = Image.open(LAYERS / "body.png").convert("RGBA")
    box = Image.new("L", idle.size, 0)
    ImageDraw.Draw(box).rectangle(PROP_BOX, fill=255)

    for pose in ("eat", "write"):
        posed = Image.open(LAYERS / f"body_{pose}.png").convert("RGBA")
        diff = ImageChops.difference(posed, idle)
        changed = ImageChops.lighter(ImageChops.lighter(diff.getchannel("R"), diff.getchannel("G")),
                                     ImageChops.lighter(diff.getchannel("B"), diff.getchannel("A")))
        changed = changed.point(lambda v: 255 if v > 40 else 0)
        # Opening drops the thin outline jitter of the regenerated image; the dilation covers the arm edges.
        changed = changed.filter(ImageFilter.MinFilter(5)).filter(ImageFilter.MaxFilter(5))
        prop = ImageChops.multiply(changed.filter(ImageFilter.MaxFilter(9)), box).filter(ImageFilter.GaussianBlur(1.5))
        front = posed.copy()
        front.putalpha(ImageChops.multiply(posed.getchannel("A"), prop))
        front.save(LAYERS / f"arms_front_{pose}.png")
        # The idle torso's hanging arms must go where the pose shows nothing there.
        empty = posed.getchannel("A").point(lambda v: 255 if v < 20 else 0)
        drop = ImageChops.multiply(prop, empty)
        posed_torso = torso.copy()
        posed_torso.putalpha(ImageChops.multiply(torso.getchannel("A"), ImageChops.invert(drop)))
        posed_torso.save(LAYERS / f"torso_{pose}.png")
        print(f"arms_front_{pose}: {count(prop.point(lambda v: 255 if v > 128 else 0))} px; "
              f"hanging-arm px removed from torso_{pose}: {count(ImageChops.multiply(drop.point(lambda v: 255 if v > 128 else 0), opaque(torso)))}")

    # Eyes. The opening (the white of the eye) is the whole area inside the eye outline: the iris plus the
    # white corners next to it, grown a little so it hides the head layer's own iris completely. The iris,
    # with its dark rim, moves inside the opening and is clipped by it. The lashes and outline around the
    # opening, and the upper lashes that cross the top of the iris, are copied from the head layer on top.
    irises = Image.open(SOURCE / "19_irises.png").convert("RGBA")
    base = Image.open(SOURCE / "00_base_eyes_open.png").convert("RGBA")
    iris_mask = opaque(irises, 128)
    near = iris_mask.filter(ImageFilter.MaxFilter(2 * SCLERA_REACH + 1))
    iris_bottom = iris_mask.getbbox()[3]
    whites = Image.new("L", base.size, 0)
    for y, x in ((y, x) for y in range(iris_bottom + 4) for x in range(base.size[0]) if near.getpixel((x, y))):
        r, g, b, a = base.getpixel((x, y))
        # The white of the eye is neutral or slightly blue; skin, even pale, is warm (red above blue).
        if a > 200 and min(r, g, b) > 220 and b >= r - 2 and max(r, g, b) - min(r, g, b) < 18:
            whites.putpixel((x, y), 255)
    raw_opening = ImageChops.lighter(iris_mask.filter(ImageFilter.MaxFilter(2 * OPENING_GROW + 1)), whites)
    # Each eye's opening is the convex hull of its iris and white corners: a smooth outline that a shifted
    # iris tucks under, with a thin lower-lid line drawn along its lower half.
    opening = Image.new("L", base.size, 0)
    lid_lines = Image.new("RGBA", base.size, (0, 0, 0, 0))
    draw_opening, draw_lid = ImageDraw.Draw(opening), ImageDraw.Draw(lid_lines)
    width = base.size[0]
    for half in ((0, width // 2), (width // 2, width)):
        points = [(x, y) for y in range(0, base.size[1], 2) for x in range(half[0], half[1], 2)
                  if raw_opening.getpixel((x, y))]
        if not points:
            continue
        hull = convex_hull(points)
        draw_opening.polygon(hull, fill=255)
        middle = sum(p[1] for p in hull) / len(hull)
        for a, b in zip(hull, hull[1:] + hull[:1]):
            if (a[1] + b[1]) / 2 > middle + 6:
                draw_lid.line([a, b], fill=LID_COLOR, width=LID_WIDTH)
    sclera = Image.new("RGBA", irises.size, SCLERA + (0,))
    sclera.putalpha(opening.filter(ImageFilter.GaussianBlur(0.8)))
    sclera.save(LAYERS / "sclera.png")
    irises.save(LAYERS / "iris.png")

    lashes = Image.new("RGBA", head.size, (0, 0, 0, 0))
    band = opening.filter(ImageFilter.MaxFilter(17))
    for half in ((0, width // 2), (width // 2, width)):
        eye = iris_mask.crop((half[0], 0, half[1], head.size[1])).getbbox()
        if not eye:
            continue
        top_band = eye[1] + (eye[3] - eye[1]) // 3
        left, right = half[0] + eye[0], half[0] + eye[2]
        for y in range(max(0, eye[1] - 30), min(head.size[1], eye[3] + 30)):
            for x in range(max(0, left - 40), min(width, right + 40)):
                if not band.getpixel((x, y)):
                    continue
                inside_iris = iris_mask.getpixel((x, y))
                # Outside the iris: lashes and the outline of the opening. Inside it: only the upper lashes.
                if (not inside_iris or y < top_band) and is_lash(head.getpixel((x, y))):
                    lashes.putpixel((x, y), head.getpixel((x, y)))
    lashes.alpha_composite(lid_lines)
    lashes.save(LAYERS / "lashes.png")
    print(f"opening px: {count(opening)} (iris {count(iris_mask)}, white corners {count(whites)}), "
          f"lash px: {count(opaque(lashes, 128))}")

if __name__ == "__main__":
    main()
    build_part_layers()
    build_head_layers()

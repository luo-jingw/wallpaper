"""Exports the mascot layers in design/mascot/layers to the Rainmeter skin.

Rigging (source-image coordinates; positive degrees turn clockwise on screen):
  lower body   still
  upper torso  cut at the waist (y=960, below the hands), leans around the middle of the chest, so the
               neck and the waist seam both move under half a skin pixel at the idle lean -> frames by lean
  head         turns around the neck                                         -> frames by head angle
               (the neck moves under 0.5 skin px at the idle lean, so the head is not shifted with the torso)
  face         sclera, iris, lashes and the expression patches turn with the head
  back hair    bends: rows shift sideways, nothing at the top and most at the tips, like hanging hair
                                                                             -> frames by tip shift
  ahoge        swings around its root; the script moves the frame with the root as the head turns
                                                                             -> frames by absolute angle
  fins         the whale fins, behind the head; each swings around its root, moved like the ahoge
                                                                             -> frames by absolute angle
  tail         on the lower body, swings around its root                    -> frames by angle
  arms_front   eating / writing arms in front of the head, lean with the torso

Angle steps are small enough (0.1 - 0.3 degrees, about 0.2 - 0.3 skin px at the far end of a part) that the
motion looks continuous at 50 frames per second.

Everything is rotated here, at full resolution with one resampling per frame, so the skin only shows,
hides, and (for the irises) moves prepared frames: Rainmeter's TransformationMatrix costs about a fifth of
a CPU core per image at 25 fps, and whole-figure pixel steps read as jitter.

Outputs in rainmeter/ToDo/@Resources/Mascot:
  *.png         frames cropped to their opaque box
  layout.inc    window size, angle ranges, eye centre, click areas
  frames.inc    one hidden Image meter per frame, in drawing order
"""

from __future__ import annotations

import math
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image, ImageChops

ROOT: Path = Path(__file__).resolve().parent
LAYERS: Path = ROOT / "layers"
REPO: Path = ROOT.parent.parent
OUT: Path = REPO / "rainmeter" / "ToDo" / "@Resources" / "Mascot"

CROP: tuple[int, int, int, int] = (100, 0, 1160, 1250)   # covers every frame's opaque pixels
WIDTH: int = 166                                          # skin pixels (the figure itself is about 160 wide)
PAD: int = 8
ALPHA_NOISE: int = 8                                      # alpha at or below this is treated as empty

WAIST_Y: int = 960
SEAM: int = 12


@dataclass(frozen=True)
class Point:
    x: float
    y: float


@dataclass(frozen=True)
class Angles:
    first: float
    step: float
    count: int

    def values(self) -> list[float]:
        return [round(self.first + i * self.step, 4) for i in range(self.count)]


WAIST = Point(627, WAIST_Y)
NECK = Point(627, 690)
CHEST = Point(627, 830)
CROWN = Point(627, 330)
AHOGE_ROOT = Point(600, 165)
TAIL_ROOT = Point(800, 945)
HIT_MARGIN = 3  # skin pixels added around the tail and fins for their click areas

LEAN = Angles(-3.5, 0.1, 71)     # torso lean, -3.5 .. +3.5
HEAD = Angles(-6.0, 0.25, 77)    # head angle, -6 .. +13 (eating tilts it about 10 degrees)
HAIR = Angles(-60.0, 1.5, 81)    # back hair tip shift in source pixels, -60 .. +60 (about 0.23 skin px a step)
HAIR_TOP: int = 420              # rows above this do not move (behind the head)
HAIR_TIP: int = 1215             # the lowest tips
HAIR_CURVE: float = 1.4          # shift grows as ((y - top) / (tip - top)) ** curve
FIN = Angles(-24.0, 0.5, 97)     # each fin, absolute: head angle plus its own flap, -24 .. +24
AHOGE = Angles(-36.0, 1.0, 77)   # ahoge, absolute, -36 .. +40
TAIL = Angles(-15.0, 0.3, 101)   # tail, -15 .. +15

TORSOS: dict[str, str] = {"Idle": "torso", "Eat": "torso_eat", "Write": "torso_write"}
ARMS_FRONT: dict[str, str] = {"Eat": "arms_front_eat", "Write": "arms_front_write"}
FACE: dict[str, str] = {
    "EyesHalf": "eyes_half", "EyesClosed": "eyes_closed", "EyesDown": "eyes_down", "EyesHappy": "eyes_happy",
    "MouthOpen": "mouth_open", "MouthChew": "mouth_chew", "FaceHappy": "face_happy",
}

HEAD_HIT: tuple[int, int, int, int] = (380, 190, 875, 660)    # ellipse bounds: pat
BODY_HIT: tuple[int, int, int, int] = (430, 720, 830, 1090)   # rectangle: eat

Matrix = tuple[float, float, float, float, float, float]  # x' = a x + b y + c, y' = d x + e y + f
IDENTITY: Matrix = (1, 0, 0, 0, 1, 0)


def rotation(p: Point, degrees: float) -> Matrix:
    r = math.radians(degrees)
    c, s = math.cos(r), math.sin(r)
    return (c, -s, p.x - c * p.x + s * p.y, s, c, p.y - s * p.x - c * p.y)


def compose(m: Matrix, n: Matrix) -> Matrix:
    """n first, then m."""
    a, b, c, d, e, f = m
    g, h, i, j, k, l = n
    return (a * g + b * j, a * h + b * k, a * i + b * l + c, d * g + e * j, d * h + e * k, d * i + e * l + f)


def inverse(m: Matrix) -> Matrix:
    a, b, c, d, e, f = m
    det = a * e - b * d
    return (e / det, -b / det, (b * f - c * e) / det, -d / det, a / det, (c * d - a * f) / det)


def load(name: str) -> Image.Image:
    image = Image.open(LAYERS / f"{name}.png").convert("RGBA")
    image.putalpha(image.getchannel("A").point(lambda v: 0 if v <= ALPHA_NOISE else v))
    return image


def waist_mask(size: tuple[int, int], upper: bool) -> Image.Image:
    """Upper: opaque above the seam, fading to clear across it. Lower: opaque from the top of the seam down."""
    column = Image.new("L", (1, size[1]))
    for y in range(size[1]):
        if upper:
            v = 255 if y < WAIST_Y - SEAM else 0 if y >= WAIST_Y + SEAM else round(255 * (WAIST_Y + SEAM - y) / (2 * SEAM))
        else:
            v = 255 if y >= WAIST_Y - SEAM else 0
        column.putpixel((0, y), v)
    return column.resize(size)


def bend(image: Image.Image, tip_shift: float) -> Image.Image:
    """Shifts every row sideways by tip_shift * ((y - HAIR_TOP) / (HAIR_TIP - HAIR_TOP)) ** HAIR_CURVE, with
    linear interpolation between pixels (in premultiplied colour, so edges stay clean)."""
    if tip_shift == 0:
        return image
    arr = np.asarray(image.convert("RGBa"), dtype=np.float32)
    h, w = arr.shape[:2]
    rows = np.clip((np.arange(h) - HAIR_TOP) / (HAIR_TIP - HAIR_TOP), 0, 1) ** HAIR_CURVE
    source_x = np.arange(w)[None, :] - (tip_shift * rows)[:, None]
    left = np.floor(source_x).astype(int)
    frac = (source_x - left)[:, :, None]
    def take(xs):
        valid = (xs >= 0) & (xs < w)
        out = np.take_along_axis(arr, np.clip(xs, 0, w - 1)[:, :, None].repeat(4, axis=2), axis=1)
        return out * valid[:, :, None]
    bent = take(left) * (1 - frac) + take(left + 1) * frac
    return Image.fromarray(np.clip(bent + 0.5, 0, 255).astype(np.uint8), "RGBa").convert("RGBA")


def fin_root(fin: Image.Image, side: str) -> Point:
    """The middle of the fin's edge that faces the head, 25 px back toward the tip: where it leaves the hair."""
    alpha = fin.getchannel("A").point(lambda v: 255 if v > 128 else 0)
    left, top, right, bottom = alpha.getbbox()
    edge_x = right - 1 if side == "left" else left
    column = [y for y in range(top, bottom) if alpha.getpixel((edge_x, y))]
    x = edge_x - 25 if side == "left" else edge_x + 25
    return Point(x, (column[0] + column[-1]) / 2)


def masked(image: Image.Image, mask: Image.Image) -> Image.Image:
    out = image.copy()
    out.putalpha(ImageChops.multiply(image.getchannel("A"), mask))
    return out


class Exporter:
    def __init__(self) -> None:
        left, top, right, bottom = CROP
        self.scale = WIDTH / (right - left)
        self.height = round((bottom - top) * self.scale)
        self.meters: list[str] = []
        self.bytes = 0
        self.files = 0
        self.written: dict[str, tuple[int, int, int, int]] = {}  # stem -> crop box, for frames shared by two meters

    def render(self, image: Image.Image, m: Matrix) -> Image.Image:
        if m != IDENTITY:
            image = image.convert("RGBa").transform(image.size, Image.AFFINE, inverse(m), resample=Image.BICUBIC)
            image = image.convert("RGBA")
        box = image.getchannel("A").getbbox()
        left, top, right, bottom = CROP
        if box and (box[0] < left or box[1] < top or box[2] > right or box[3] > bottom):
            raise SystemExit(f"opaque pixels {box} outside the crop {CROP}")
        scaled = image.crop(CROP).convert("RGBa").resize((WIDTH, self.height), Image.LANCZOS).convert("RGBA")
        scaled.putalpha(scaled.getchannel("A").point(lambda v: 0 if v <= 2 else v))
        return scaled

    def frame(self, image: Image.Image, m: Matrix, stem: str, meter: str, shown: bool,
              container: tuple[str, tuple[int, int]] | None = None) -> tuple[int, int]:
        """Writes one frame (once per stem) and its meter; returns the frame's top-left in skin pixels."""
        path = OUT / f"{stem}.png"
        if stem in self.written:
            box = self.written[stem]
        else:
            scaled = self.render(image, m)
            box = scaled.getchannel("A").getbbox()
            scaled.crop(box).save(path, optimize=True)
            self.written[stem] = box
            self.bytes += path.stat().st_size
            self.files += 1
        x, y = PAD + box[0], PAD + box[1]
        lines = [f"[{meter}]", "Meter=Image", f"ImageName=#@#Mascot\\{path.name}"]
        if container:
            name, (cx, cy) = container
            # Contained meters are placed relative to the container. GetX() reports them shifted by the container
            # offset twice, so the script reads the rest position from RestX / RestY instead.
            lines += [f"Container={name}", f"X={x - cx}", f"Y={y - cy}", f"RestX={x - cx}", f"RestY={y - cy}"]
        else:
            lines += [f"X={x}", f"Y={y}"]
        lines += ["AntiAlias=1", f"Hidden={0 if shown else 1}", ""]
        self.meters += lines
        return x, y

    def skin(self, x: float, y: float) -> tuple[float, float]:
        return PAD + (x - CROP[0]) * self.scale, PAD + (y - CROP[1]) * self.scale


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for old in OUT.glob("*"):
        old.unlink()
    ex = Exporter()
    rest_lean = LEAN.values().index(0.0)
    rest_head = HEAD.values().index(0.0)
    assert 0.0 in HAIR.values() and 0.0 in AHOGE.values() and 0.0 in TAIL.values()
    hair, ahoge, tail = load("hair_back"), load("ahoge"), load("tail")
    upper_mask = waist_mask(hair.size, upper=True)
    lower_mask = waist_mask(hair.size, upper=False)

    # Drawing order, bottom to top.
    for j, shift in enumerate(HAIR.values()):
        ex.frame(bend(hair, shift), IDENTITY, f"hair_{j:03d}", f"Hair{j}", shift == 0)
    for j, angle in enumerate(TAIL.values()):
        ex.frame(tail, rotation(TAIL_ROOT, angle), f"tail_{j:03d}", f"Tail{j}", angle == 0)
    fin_roots = {side: fin_root(load(f"fin_{side}"), side) for side in ("left", "right")}
    for side, root in fin_roots.items():
        fin = load(f"fin_{side}")
        meter = "FinL" if side == "left" else "FinR"
        for j, angle in enumerate(FIN.values()):
            ex.frame(fin, rotation(root, angle), f"fin_{side}_{j:03d}", f"{meter}{j}", angle == 0)
    for j, angle in enumerate(AHOGE.values()):
        ex.frame(ahoge, rotation(AHOGE_ROOT, angle), f"ahoge_{j:03d}", f"Ahoge{j}", angle == 0)
    torsos = {pose: load(layer) for pose, layer in TORSOS.items()}
    for pose, torso in torsos.items():
        ex.frame(masked(torso, lower_mask), IDENTITY, f"lower_{pose.lower()}", f"Lower{pose}", pose == "Idle")
    for pose, torso in torsos.items():
        upper = masked(torso, upper_mask)
        for i, lean in enumerate(LEAN.values()):
            ex.frame(upper, rotation(CHEST, lean), f"upper_{pose.lower()}_{i}", f"Upper{pose}{i}",
                     pose == "Idle" and i == rest_lean)
    head_image, sclera, iris, lashes = load("head"), load("sclera"), load("iris"), load("lashes")
    for i, angle in enumerate(HEAD.values()):
        m = rotation(NECK, angle)
        ex.frame(head_image, m, f"head_{i:02d}", f"Head{i}", i == rest_head)
        # The sclera is drawn, and a second copy is the clip container for the iris.
        ex.frame(sclera, m, f"sclera_{i:02d}", f"Sclera{i}", i == rest_head)
        clip = ex.frame(sclera, m, f"sclera_{i:02d}", f"IrisClip{i}", i == rest_head)
        ex.frame(iris, m, f"iris_{i:02d}", f"Iris{i}", i == rest_head, container=(f"IrisClip{i}", clip))
        ex.frame(lashes, m, f"lashes_{i:02d}", f"Lashes{i}", i == rest_head)
    for meter, layer in FACE.items():
        patch = load(layer)
        for i, angle in enumerate(HEAD.values()):
            ex.frame(patch, rotation(NECK, angle), f"{layer}_{i:02d}", f"{meter}{i}", False)
    for pose, layer in ARMS_FRONT.items():
        arms = load(layer)
        for i, lean in enumerate(LEAN.values()):
            ex.frame(arms, rotation(CHEST, lean), f"arms_{pose.lower()}_{i}", f"Arms{pose}{i}", False)

    (OUT / "frames.inc").write_text(
        "; Generated by design/mascot/export_skin_assets.py. Drawing order: bottom to top.\n" + "\n".join(ex.meters),
        encoding="ascii", newline="\r\n",
    )
    iris_box = load("iris").getchannel("A").getbbox()
    eye_x, eye_y = ex.skin((iris_box[0] + iris_box[2]) / 2, (iris_box[1] + iris_box[3]) / 2)
    hx0, hy0 = ex.skin(HEAD_HIT[0], HEAD_HIT[1])
    hx1, hy1 = ex.skin(HEAD_HIT[2], HEAD_HIT[3])
    bx0, by0 = ex.skin(BODY_HIT[0], BODY_HIT[1])
    bx1, by1 = ex.skin(BODY_HIT[2], BODY_HIT[3])
    # Tail and fins: ellipses around each part at rest, a little larger than the part.
    part_hits = []
    for name, layer in (("TailHit", "tail"), ("FinLHit", "fin_left"), ("FinRHit", "fin_right")):
        x0, y0, x1, y1 = load(layer).getchannel("A").getbbox()
        sx0, sy0 = ex.skin(x0, y0)
        sx1, sy1 = ex.skin(x1, y1)
        part_hits += [f"{name}CX={(sx0 + sx1) / 2:.1f}", f"{name}CY={(sy0 + sy1) / 2:.1f}",
                      f"{name}RX={(sx1 - sx0) / 2 + HIT_MARGIN:.1f}", f"{name}RY={(sy1 - sy0) / 2 + HIT_MARGIN:.1f}"]
    layout = [
        "; Generated by design/mascot/export_skin_assets.py. Skin pixels and degrees.",
        "[Variables]",
        f"MascotWindowW={WIDTH + 2 * PAD}",
        f"MascotWindowH={ex.height + 2 * PAD}",
        f"EyeCenterX={eye_x:.1f}",
        f"EyeCenterY={eye_y:.1f}",
        f"ChestX={ex.skin(CHEST.x, CHEST.y)[0]:.2f}",
        f"ChestY={ex.skin(CHEST.x, CHEST.y)[1]:.2f}",
        f"NeckX={ex.skin(NECK.x, NECK.y)[0]:.2f}",
        f"NeckY={ex.skin(NECK.x, NECK.y)[1]:.2f}",
        f"AhogeRootX={ex.skin(AHOGE_ROOT.x, AHOGE_ROOT.y)[0]:.2f}",
        f"AhogeRootY={ex.skin(AHOGE_ROOT.x, AHOGE_ROOT.y)[1]:.2f}",
        f"FinLRootX={ex.skin(fin_roots['left'].x, fin_roots['left'].y)[0]:.2f}",
        f"FinLRootY={ex.skin(fin_roots['left'].x, fin_roots['left'].y)[1]:.2f}",
        f"FinRRootX={ex.skin(fin_roots['right'].x, fin_roots['right'].y)[0]:.2f}",
        f"FinRRootY={ex.skin(fin_roots['right'].x, fin_roots['right'].y)[1]:.2f}",
    ]
    for name, angles in (("Lean", LEAN), ("Head", HEAD), ("Hair", HAIR), ("Ahoge", AHOGE), ("Tail", TAIL),
                         ("Fin", FIN)):
        layout += [f"{name}First={angles.first}", f"{name}Step={angles.step}", f"{name}Frames={angles.count}"]
    layout += [
        f"HeadHitCX={(hx0 + hx1) / 2:.1f}", f"HeadHitCY={(hy0 + hy1) / 2:.1f}",
        f"HeadHitRX={(hx1 - hx0) / 2:.1f}", f"HeadHitRY={(hy1 - hy0) / 2:.1f}",
        f"BodyHitX={bx0:.1f}", f"BodyHitY={by0:.1f}", f"BodyHitW={bx1 - bx0:.1f}", f"BodyHitH={by1 - by0:.1f}",
        *part_hits,
    ]
    (OUT / "layout.inc").write_text("\n".join(layout) + "\n", encoding="ascii", newline="\r\n")
    print(f"{len([m for m in ex.meters if m.startswith('[')])} meters, {ex.files} frames, {ex.bytes // 1024} KiB, "
          f"canvas {WIDTH}x{ex.height}, window {WIDTH + 2 * PAD}x{ex.height + 2 * PAD}")


if __name__ == "__main__":
    main()

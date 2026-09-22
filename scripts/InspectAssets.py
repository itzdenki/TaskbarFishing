"""Audit the supplied atlas metadata and render contact sheets for visual inspection."""
import json
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
OUT = ROOT / "build" / "captures" / "asset-review"


def main():
    data = json.loads((ASSETS / "assets.json").read_text(encoding="utf-8"))
    OUT.mkdir(parents=True, exist_ok=True)
    images = {}

    def texture(path):
        if path not in images:
            images[path] = Image.open(ASSETS / path).convert("RGBA")
        return images[path]

    def frame(path, source):
        image = texture(path)
        x, y, w, h = (source[k] for k in ("x", "y", "width", "height"))
        assert x >= 0 and y >= 0 and w > 0 and h > 0
        assert x + w <= image.width and y + h <= image.height, path
        return image.crop((x, y, x + w, y + h))

    definitions = data["player"]["animations"] + data["effects"] + data["ui"]["components"] + [data["equipment"]["rod"]]
    for item in definitions:
        paths = [item["texture"]] + [h["texture"] for h in item.get("hourly_textures", [])]
        frames = item.get("frames", item.get("states"))
        for path in paths:
            for f in frames:
                frame(path, f["source"])
        if "duration_ms" in item:
            elapsed = 0
            for f in frames:
                assert f["start_ms"] == elapsed
                elapsed += f["duration_ms"]
            assert elapsed == item["duration_ms"], item["id"]

    fish_path = ASSETS / "fish" / "fish.json"
    if fish_path.exists():
        fish = json.loads(fish_path.read_text(encoding="utf-8"))
        for item in fish["species"]:
            frame(item["icon"], item["icon_source"])
            frame(fish["icon_atlas"]["texture"], item["icon_atlas_source"])
            elapsed = 0
            for f in item["animation"]["frames"]:
                frame(item["texture"], f["source"])
                assert f["start_ms"] == elapsed
                elapsed += f["duration_ms"]
            assert elapsed == item["animation"]["duration_ms"]
            hook = item["hook_anchor"]
            assert 0 <= hook["x"] < item["frame_size"]["width"]
            assert 0 <= hook["y"] < item["frame_size"]["height"]

    special_path = ASSETS / "special" / "special.json"
    if special_path.exists():
        special = json.loads(special_path.read_text(encoding="utf-8"))
        for item in special["items"]:
            frame(item["icon"], item["icon_source"])
            for animation in item["animations"]:
                elapsed = 0
                for f in animation["frames"]:
                    frame(animation["texture"], f["source"])
                    assert f["start_ms"] == elapsed
                    elapsed += f["duration_ms"]
                    hook = f["hook_anchor"]
                    assert 0 <= hook["x"] < f["source"]["width"]
                    assert 0 <= hook["y"] < f["source"]["height"]
                assert elapsed == animation["duration_ms"]

    canvas = Image.new("RGB", (1048, 24 * 166 // 2), "#0f1a23")
    draw = ImageDraw.Draw(canvas)
    for hour in data["background"]["hours"]:
        path = hour["texture"]
        for f in data["background"]["frames"]:
            frame(path, f["source"])
        x, y = (hour["hour"] % 2) * 524, (hour["hour"] // 2) * 166
        draw.text((x + 6, y + 2), f'{hour["hour"]:02d}:00', fill="white")
        image = frame(path, data["background"]["frames"][0]["source"])
        canvas.paste(image, (x + 6, y + 18))
    canvas.save(OUT / "hours.png")

    for group, items in (("player", data["player"]["animations"]), ("effects", data["effects"] + [data["equipment"]["rod"]])):
        rows = []
        for item in items:
            frames = item.get("frames", item.get("states"))
            size = item.get("frame_size", data["player"]["frame_size"])
            w, h = size["width"], size["height"]
            row = Image.new("RGB", (max(800, len(frames) * (w * 2 + 4)), h * 2 + 24), "#172731")
            ImageDraw.Draw(row).text((4, 4), item["id"], fill="white")
            for i, f in enumerate(frames):
                image = frame(item["texture"], f["source"]).resize((w * 2, h * 2), Image.Resampling.NEAREST)
                row.paste(image, (i * (w * 2 + 4), 22), image)
            rows.append(row)
        sheet = Image.new("RGB", (max(r.width for r in rows), sum(r.height for r in rows)), "#172731")
        y = 0
        for row in rows:
            sheet.paste(row, (0, y))
            y += row.height
        sheet.save(OUT / f"{group}.png")
    print(f"PASS: {len(images)} PNGs; all rectangles and animation timings valid. Contact sheets: {OUT}")


if __name__ == "__main__":
    main()

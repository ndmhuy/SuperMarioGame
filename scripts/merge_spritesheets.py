#!/usr/bin/env python3
"""
Spritesheet & Metadata Merger CLI Tool
Merges two or more sprite sheet PNG images and their corresponding JSON metadata files,
automatically offsetting frame coordinates and handling key collisions.
"""

import sys
import os
import json
import argparse

def merge_jsons(json_paths, image_sizes, mode="vertical", padding=0):
    merged_frames = {}
    merged_width = 0
    merged_height = 0

    if mode == "vertical":
        merged_width = max(w for w, h in image_sizes)
        current_y = 0

        for idx, (path, (w, h)) in enumerate(zip(json_paths, image_sizes)):
            with open(path, "r") as f:
                data = json.load(f)
            
            frames = data.get("frames", {})
            for key, val in frames.items():
                final_key = key
                if final_key in merged_frames:
                    final_key = f"{key}_sheet{idx+1}"

                orig_frame = val.get("frame", {})
                new_frame = {
                    "x": orig_frame.get("x", 0),
                    "y": orig_frame.get("y", 0) + current_y,
                    "w": orig_frame.get("w", 0),
                    "h": orig_frame.get("h", 0)
                }

                new_val = dict(val)
                new_val["frame"] = new_frame
                merged_frames[final_key] = new_val

            current_y += h + padding

        merged_height = max(0, current_y - padding)

    else: # horizontal
        merged_height = max(h for w, h in image_sizes)
        current_x = 0

        for idx, (path, (w, h)) in enumerate(zip(json_paths, image_sizes)):
            with open(path, "r") as f:
                data = json.load(f)
            
            frames = data.get("frames", {})
            for key, val in frames.items():
                final_key = key
                if final_key in merged_frames:
                    final_key = f"{key}_sheet{idx+1}"

                orig_frame = val.get("frame", {})
                new_frame = {
                    "x": orig_frame.get("x", 0) + current_x,
                    "y": orig_frame.get("y", 0),
                    "w": orig_frame.get("w", 0),
                    "h": orig_frame.get("h", 0)
                }

                new_val = dict(val)
                new_val["frame"] = new_frame
                merged_frames[final_key] = new_val

            current_x += w + padding

        merged_width = max(0, current_x - padding)

    return {
        "frames": merged_frames,
        "meta": {
            "app": "Spritesheet Merger Studio",
            "version": "1.0",
            "size": {"w": merged_width, "h": merged_height}
        }
    }, merged_width, merged_height

def main():
    parser = argparse.ArgumentParser(description="Merge sprite sheet images and JSON metadata.")
    parser.add_argument("--json", nargs="+", required=True, help="List of JSON metadata files to merge")
    parser.add_argument("--out-json", required=True, help="Output path for merged JSON file")
    parser.add_argument("--mode", choices=["vertical", "horizontal"], default="vertical", help="Stacking direction")
    parser.add_argument("--padding", type=int, default=0, help="Pixel spacing between sheets")

    args = parser.parse_args()

    # Read image sizes from JSON meta or prompt user
    image_sizes = []
    for jp in args.json:
        with open(jp, "r") as f:
            d = json.load(f)
        meta = d.get("meta", {})
        size = meta.get("size", {})
        w = size.get("w", 0)
        h = size.get("h", 0)

        # Fallback if meta size missing: calculate bounding box max
        if w == 0 or h == 0:
            frames = d.get("frames", {})
            w = max((f["frame"]["x"] + f["frame"]["w"]) for f in frames.values()) if frames else 0
            h = max((f["frame"]["y"] + f["frame"]["h"]) for f in frames.values()) if frames else 0

        image_sizes.append((w, h))

    merged_data, mw, mh = merge_jsons(args.json, image_sizes, mode=args.mode, padding=args.padding)

    with open(args.out_json, "w") as f:
        json.dump(merged_data, f, indent=2)

    print(f"Successfully merged {len(args.json)} JSON metadata files into {args.out_json}")
    print(f"Merged Sheet Dimensions: {mw}x{mh} px, Total Frames: {len(merged_data['frames'])}")
    print("Tip: Use tools/spritesheet-merger/spritesheet_merger.html in browser to preview and download the combined PNG image!")

if __name__ == "__main__":
    main()

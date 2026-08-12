#!/usr/bin/env python3
"""
Sprite Metadata Editor CLI Tool
Provides command-line commands to add, delete, rename, or modify frame bounding box rectangles in TexturePacker JSON files.
"""

import sys
import os
import json
import argparse

def load_json(filepath):
    with open(filepath, 'r') as f:
        return json.load(f)

def save_json(filepath, data):
    with open(filepath, 'w') as f:
        json.dump(data, f, indent=2)

def main():
    parser = argparse.ArgumentParser(description="Modify sprite sheet JSON metadata.")
    parser.add_argument("json_file", help="Path to JSON metadata file")
    
    subparsers = parser.add_subparsers(dest="action", required=True, help="Action to perform")

    # Add Frame
    add_parser = subparsers.add_parser("add", help="Add a new frame entry")
    add_parser.add_argument("--name", required=True, help="Frame key name")
    add_parser.add_argument("--x", type=int, required=True, help="X coordinate")
    add_parser.add_argument("--y", type=int, required=True, help="Y coordinate")
    add_parser.add_argument("--w", type=int, required=True, help="Width")
    add_parser.add_argument("--h", type=int, required=True, help="Height")

    # Delete Frame
    del_parser = subparsers.add_parser("delete", help="Delete a frame entry (PNG is not modified)")
    del_parser.add_argument("--name", required=True, help="Frame key name to delete")

    # Modify Frame
    mod_parser = subparsers.add_parser("modify", help="Modify existing frame name or bounding box")
    mod_parser.add_argument("--name", required=True, help="Target frame key name")
    mod_parser.add_argument("--new-name", help="New frame key name")
    mod_parser.add_argument("--x", type=int, help="New X coordinate")
    mod_parser.add_argument("--y", type=int, help="New Y coordinate")
    mod_parser.add_argument("--w", type=int, help="New width")
    mod_parser.add_argument("--h", type=int, help="New height")

    args = parser.parse_args()

    data = load_json(args.json_file)
    frames = data.get("frames", {})

    if args.action == "add":
        if args.name in frames:
            print(f"Error: Frame '{args.name}' already exists in {args.json_file}")
            sys.exit(1)

        frames[args.name] = {
            "frame": {"x": args.x, "y": args.y, "w": args.w, "h": args.h},
            "rotated": False,
            "trimmed": False,
            "spriteSourceSize": {"x": 0, "y": 0, "w": args.w, "h": args.h},
            "sourceSize": {"w": args.w, "h": args.h}
        }
        print(f"Added new frame '{args.name}' at ({args.x}, {args.y}, {args.w}, {args.h})")

    elif args.action == "delete":
        if args.name not in frames:
            print(f"Error: Frame '{args.name}' not found in {args.json_file}")
            sys.exit(1)

        del frames[args.name]
        print(f"Deleted frame '{args.name}' from metadata (PNG left intact).")

    elif args.action == "modify":
        if args.name not in frames:
            print(f"Error: Frame '{args.name}' not found in {args.json_file}")
            sys.exit(1)

        target_key = args.name
        frame_obj = frames[target_key]

        if args.new_name and args.new_name != target_key:
            if args.new_name in frames:
                print(f"Error: Frame '{args.new_name}' already exists.")
                sys.exit(1)
            
            # Re-key
            new_frames = {}
            for k in frames:
                if k == target_key:
                    new_frames[args.new_name] = frame_obj
                else:
                    new_frames[k] = frames[k]
            data["frames"] = new_frames
            frames = new_frames
            target_key = args.new_name
            print(f"Renamed frame '{args.name}' -> '{args.new_name}'")

        fr = frames[target_key]["frame"]
        if args.x is not None: fr["x"] = args.x
        if args.y is not None: fr["y"] = args.y
        if args.w is not None: fr["w"] = args.w
        if args.h is not None: fr["h"] = args.h
        
        frames[target_key]["spriteSourceSize"]["w"] = fr["w"]
        frames[target_key]["spriteSourceSize"]["h"] = fr["h"]
        frames[target_key]["sourceSize"]["w"] = fr["w"]
        frames[target_key]["sourceSize"]["h"] = fr["h"]

        print(f"Updated frame '{target_key}' rectangle to ({fr['x']}, {fr['y']}, {fr['w']}, {fr['h']})")

    save_json(args.json_file, data)
    print(f"Saved changes to {args.json_file}")

if __name__ == "__main__":
    main()

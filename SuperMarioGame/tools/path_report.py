#!/usr/bin/env python3
"""Compare an agent's recorded path against the level's certified route.

eval_level --path writes where the agent actually went (4 Hz samples per life,
plus death positions). The waypoint sidecar says where the oracle's kindest
winnable route goes. The difference is the evaluation a progress scalar cannot
be: it names WHERE the agent leaves the intended line, which reads two ways —

    agent diagnostic:  the divergence point is what the agent cannot execute
    map diagnostic:    a divergence shared by every policy marks a feature of
                       the MAP (unfair gap, invisible device, dead end)

Metrics per run:
    routeCoverage    fraction of route nodes the path came within 1.5 tiles of
    firstDivergence  x (tiles) where coverage first breaks for good
    meanDeviation    mean distance (tiles) from each path sample to the route
    deathClusters    death positions, clustered to tiles

Usage:
    path_report.py <path.json> [--svg out.svg]     # metrics to stdout
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import solvability  # noqa: E402

TILE = 32.0


GAME_ROOT = Path(__file__).resolve().parent.parent


def _resolve(level_path: str) -> Path:
    """The recorded level path is relative to wherever eval_level ran —
    normally the game root. Try as-is, then against the game root."""
    p = Path(level_path)
    if p.exists():
        return p
    return GAME_ROOT / level_path


def load(path_file: str):
    data = json.loads(Path(path_file).read_text())
    level = _resolve(data["level"])
    data["level"] = str(level)
    sidecar = level.with_name(level.stem + ".waypoints.json")
    waypoints = []
    if sidecar.exists():
        waypoints = [(w["x"], w["y"]) for w in
                     json.loads(sidecar.read_text())["waypoints"]]
    return data, waypoints


def analyse(data: dict, waypoints: list) -> dict:
    samples = [p for life in data["lives"] for p in life]
    if not samples or not waypoints:
        return {"error": "no samples or no route"}

    # Coverage: a route node counts as visited if any sample came close.
    visited = []
    for wx, wy in waypoints:
        near = min((sx - wx) ** 2 + (sy - wy) ** 2 for sx, sy in samples)
        visited.append(near <= (1.5 * TILE) ** 2)
    coverage = sum(visited) / len(visited)

    # First divergence: the first route node after which nothing is visited.
    first_divergence = None
    for i, v in enumerate(visited):
        if not any(visited[i:]):
            first_divergence = waypoints[i][0] / TILE
            break

    # Mean deviation of path samples from the route polyline (nearest node).
    def nearest(sx, sy):
        return min(((sx - wx) ** 2 + (sy - wy) ** 2) ** 0.5
                   for wx, wy in waypoints)
    mean_dev = sum(nearest(*s) for s in samples) / len(samples) / TILE

    # Death clusters, tile-resolution.
    clusters = {}
    for dx, dy in data.get("deaths", []):
        key = (int(dx // TILE), int(dy // TILE))
        clusters[key] = clusters.get(key, 0) + 1

    return {
        "routeCoverage": round(coverage, 4),
        "firstDivergenceTileX": first_divergence,
        "meanDeviationTiles": round(mean_dev, 2),
        "lives": len(data["lives"]),
        "deathClusters": sorted(
            ({"x": k[0], "y": k[1], "count": v} for k, v in clusters.items()),
            key=lambda c: -c["count"])[:8],
    }


def svg(data: dict, waypoints: list, out: Path) -> None:
    """Minimap: terrain silhouette, certified route, agent path, deaths."""
    level = solvability.Level.load(data["level"])
    w_px, h_px = level.w * 4, level.h * 4          # 4 px per tile
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w_px} {h_px + 20}" '
             f'style="background:#0e1116">']
    # terrain
    for y in range(level.h):
        for x in range(level.w):
            if level.solid(x, y):
                parts.append(f'<rect x="{x*4}" y="{y*4}" width="4" height="4" fill="#3a4454"/>')
            elif level.deadly(x, y):
                parts.append(f'<rect x="{x*4}" y="{y*4}" width="4" height="4" fill="#a33"/>')
    # certified route (world px -> band tiles: y/32 - BAND_TOP)
    import level_tensor as lt
    def to_map(px, py):
        return px / TILE * 4, (py / TILE - lt.BAND_TOP) * 4
    if waypoints:
        points = " ".join(f"{to_map(wx, wy)[0]:.0f},{to_map(wx, wy)[1]:.0f}"
                          for wx, wy in waypoints)
        parts.append(f'<polyline points="{points}" fill="none" stroke="#4aa3ff" '
                     f'stroke-width="1.5" opacity="0.9"/>')
    # agent path, one polyline per life
    for life in data["lives"]:
        if len(life) < 2:
            continue
        points = " ".join(f"{to_map(px, py)[0]:.0f},{to_map(px, py)[1]:.0f}"
                          for px, py in life)
        parts.append(f'<polyline points="{points}" fill="none" stroke="#ffd23f" '
                     f'stroke-width="1" opacity="0.55"/>')
    # deaths
    for dx, dy in data.get("deaths", []):
        x, y = to_map(dx, dy)
        parts.append(f'<circle cx="{x:.0f}" cy="{y:.0f}" r="3" fill="none" '
                     f'stroke="#ff5470" stroke-width="1.5"/>')
    parts.append(f'<text x="4" y="{h_px + 14}" fill="#8b96a5" font-size="10" '
                 f'font-family="monospace">{Path(data["level"]).name} · '
                 f'{data.get("policy","?")} · blue=certified route · '
                 f'yellow=agent · red=death</text>')
    parts.append("</svg>")
    out.write_text("\n".join(parts))


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("path_file")
    p.add_argument("--svg", help="also render a minimap SVG here")
    args = p.parse_args(argv)

    data, waypoints = load(args.path_file)
    report = analyse(data, waypoints)
    print(json.dumps(report, indent=1))
    if args.svg:
        svg(data, waypoints, Path(args.svg))
        print(f"svg: {args.svg}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

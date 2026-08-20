#!/usr/bin/env python3
"""The acceptance test that defines when a generated map batch is DONE.

"Make the maps better" never terminates; this gate does. A batch passes when
every level in it clears all four bars, and the loop's job is to iterate
until the gate passes — after which further evolution is taste, not need.

The bars, and why each number:

  1. CERTIFIED    winnable by the oracle, required difficulty inside the
                  batch's declared band. Non-negotiable — a broken level is
                  not a hard level.
  2. FORGIVING    forgiveness >= 0.85. Measured basis: the one level agents
                  complete scored 0.94; the level that ate six lives at one
                  pit scored 0.84. The bar sits just above the observed
                  failure.
  3. FAIR         no single tile may account for more than half of an
                  evaluated agent's deaths when the agent dies 6+ times
                  (needs eval reports; skipped with a warning if absent).
                  A death-cluster monopoly marks a trap, not a challenge.
  4. DISTINCT     min pairwise column-histogram distance >= 1.0 — no two
                  levels in the batch are reskins of each other.

Usage:
    map_quality_gate.py <dir> --band lo:hi [--paths <dir-with-path-jsons>]
Exit 0 = batch passes; 1 = fails (per-level reasons printed).
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import solvability  # noqa: E402
from evolve import tile_signature, signature_distance  # noqa: E402


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("batch_dir")
    p.add_argument("--band", required=True)
    p.add_argument("--paths", help="dir of eval --path JSONs for the FAIR bar")
    p.add_argument("--forgiveness", type=float, default=0.85)
    args = p.parse_args(argv)

    lo, hi = (float(x) for x in args.band.split(":"))
    levels = sorted(f for f in Path(args.batch_dir).glob("*.json")
                    if ".waypoints." not in f.name and f.name != "manifest.json")
    if not levels:
        print("no levels in batch"); return 1

    failures = 0
    signatures = {}
    for f in levels:
        reasons = []
        level = solvability.Level.load(str(f))
        report = solvability.analyse(level)
        if not report["winnable"]:
            reasons.append("BROKEN")
        elif not (lo <= report["requiredDifficulty"] <= hi):
            reasons.append(f"off-band ({report['requiredDifficulty']})")
        if report.get("forgiveness", 1.0) < args.forgiveness:
            reasons.append(f"unforgiving ({report['forgiveness']})")
        signatures[f.name] = tile_signature(f)

        if args.paths:
            path_file = Path(args.paths) / f"path_{f.stem}.json"
            if path_file.exists():
                data = json.loads(path_file.read_text())
                deaths = data.get("deaths", [])
                if len(deaths) >= 6:
                    clusters = {}
                    for dx, dy in deaths:
                        key = int(dx // 32)
                        clusters[key] = clusters.get(key, 0) + 1
                    worst = max(clusters.values())
                    if worst > len(deaths) / 2:
                        reasons.append(
                            f"death monopoly (x={max(clusters, key=clusters.get)}"
                            f" takes {worst}/{len(deaths)})")
            else:
                print(f"  note: no path report for {f.name}; FAIR bar skipped")

        status = "PASS" if not reasons else "FAIL: " + ", ".join(reasons)
        print(f"  {f.name:<22} required={report['requiredDifficulty']:<7}"
              f" forgive={report.get('forgiveness', 1.0):<7} {status}")
        failures += bool(reasons)

    # DISTINCT bar, batch-level
    names = list(signatures)
    for i in range(len(names)):
        for j in range(i + 1, len(names)):
            d = signature_distance(signatures[names[i]], signatures[names[j]])
            if d < 1.0:
                print(f"  DISTINCT fail: {names[i]} ~ {names[j]} (distance {d:.2f})")
                failures += 1

    print(f"gate: {'PASS' if failures == 0 else f'FAIL ({failures} problem(s))'}")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())

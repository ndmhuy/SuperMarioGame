#!/usr/bin/env python3
"""Evolutionary level generation with a certified fitness function.

The loop the literature runs with an agent in it, run with the oracle instead:

    genome --render_genome--> level.json --solvability oracle--> fitness

Fitness never asks a player to play. It asks the level three questions the
oracle answers exactly:
  1. Is it winnable at all?              (hard gate — broken levels score 0)
  2. Is its REQUIRED difficulty inside the target band?  (curriculum control)
  3. Is it different from what we already kept?          (diversity pressure)

Because the map generator itself is deterministic given a genome, evolution
searches genome space, not level space: continuous genes (pit rate, roughness,
enemy rate, ...) get Gaussian mutation and blend crossover; the seed gene is a
diversity jackpot that mutates by reroll.

Usage:
    evolve.py --band 0.35:0.55 --generations 8 --population 24 \
              --keep 6 --out assets/levels/generated

Writes the kept levels plus their .waypoints.json sidecars (the trainer's goal
channel needs them), and a manifest.json describing genome + score of each.
"""

from __future__ import annotations

import argparse
import json
import random
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import solvability  # noqa: E402
import level_tensor as lt  # noqa: E402

RENDER = Path(__file__).resolve().parent.parent / "build" / "render_genome"

THEMES = ["overworld", "underground", "castle", "ice"]
DIFFS = ["easy", "medium", "hard"]

# gene -> (min, max, mutation sigma)
CONTINUOUS = {
    "pit":   (0.02, 0.30, 0.04),
    "pipe":  (0.00, 0.15, 0.03),
    "enemy": (0.05, 0.40, 0.05),
    "coin":  (0.05, 0.40, 0.05),
    "rough": (0.10, 0.70, 0.08),
}


def random_genome(rng: random.Random) -> dict:
    g = {k: rng.uniform(lo, hi) for k, (lo, hi, _s) in CONTINUOUS.items()}
    g["theme"] = rng.choice(THEMES)
    g["difficulty"] = rng.choice(DIFFS)
    g["lava"] = rng.random() < 0.5
    g["moving"] = rng.random() < 0.7
    g["seed"] = rng.randrange(1, 2**31)
    return g


def mutate(g: dict, rng: random.Random) -> dict:
    child = dict(g)
    for k, (lo, hi, sigma) in CONTINUOUS.items():
        if rng.random() < 0.6:
            child[k] = min(hi, max(lo, child[k] + rng.gauss(0.0, sigma)))
    if rng.random() < 0.15:
        child["theme"] = rng.choice(THEMES)
    if rng.random() < 0.15:
        child["difficulty"] = rng.choice(DIFFS)
    if rng.random() < 0.10:
        child["lava"] = not child["lava"]
    if rng.random() < 0.10:
        child["moving"] = not child["moving"]
    # The seed gene has no locality: mutating it is a reroll, and rerolling it
    # often is what keeps the phenotype pool from collapsing onto one layout.
    if rng.random() < 0.5:
        child["seed"] = rng.randrange(1, 2**31)
    return child


def crossover(a: dict, b: dict, rng: random.Random) -> dict:
    child = {}
    for k, (lo, hi, _s) in CONTINUOUS.items():
        t = rng.random()
        child[k] = min(hi, max(lo, t * a[k] + (1.0 - t) * b[k]))
    for k in ("theme", "difficulty", "lava", "moving", "seed"):
        child[k] = (a if rng.random() < 0.5 else b)[k]
    return child


def render(genome: dict, out_path: Path) -> bool:
    cmd = [str(RENDER), str(out_path),
           "--seed", str(genome["seed"]),
           "--theme", genome["theme"],
           "--difficulty", genome["difficulty"],
           "--pit", f"{genome['pit']:.4f}",
           "--pipe", f"{genome['pipe']:.4f}",
           "--enemy", f"{genome['enemy']:.4f}",
           "--coin", f"{genome['coin']:.4f}",
           "--rough", f"{genome['rough']:.4f}",
           "--lava", "1" if genome["lava"] else "0",
           "--moving", "1" if genome["moving"] else "0",
           "--name", f"Evolved {genome['theme']} ({genome['seed'] % 10000})"]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.returncode == 0 and out_path.exists()


def tile_signature(level_path: Path):
    """Coarse phenotype signature for diversity: column-wise terrain histogram."""
    level = solvability.Level.load(str(level_path))
    sig = []
    for x in range(0, level.w, 4):
        solid = sum(1 for y in range(level.h) if level.solid(x, y))
        hazard = sum(1 for y in range(level.h) if level.deadly(x, y))
        sig.append((solid, hazard))
    return sig


def signature_distance(a, b) -> float:
    n = min(len(a), len(b))
    if n == 0:
        return 0.0
    d = sum(abs(a[i][0] - b[i][0]) + 2 * abs(a[i][1] - b[i][1]) for i in range(n))
    return d / n


def evaluate(genome: dict, band: tuple, kept_signatures: list, workdir: Path,
             index: int) -> dict:
    """Render + oracle. Returns dict with fitness and the analysis report."""
    out = workdir / f"cand_{index:04d}.json"
    if not render(genome, out):
        return {"fitness": 0.0, "reason": "render failed"}
    try:
        level = solvability.Level.load(str(out))
        report = solvability.analyse(level)
    except Exception as e:  # malformed output is a zero, not a crash
        return {"fitness": 0.0, "reason": f"oracle error: {e}"}

    if not report["winnable"]:
        return {"fitness": 0.0, "reason": "broken", "report": report}

    lo, hi = band
    d = report["requiredDifficulty"]
    # Inside the band scores 1; outside decays linearly to 0 at band-width away.
    width = max(hi - lo, 1e-6)
    band_score = 1.0 if lo <= d <= hi else max(0.0, 1.0 - abs(d - (lo if d < lo else hi)) / width)

    sig = tile_signature(out)
    if kept_signatures:
        nearest = min(signature_distance(sig, s) for s in kept_signatures)
        diversity = min(nearest / 4.0, 1.0)   # ~4 tiles of column delta = fully novel
    else:
        diversity = 1.0

    # A level whose optional content is richer than its required line is a
    # better level (the completionist gap), worth a small bonus.
    optional_gap = max(0.0, report["hardestOptional"] - d)

    fitness = band_score * (0.6 + 0.3 * diversity + 0.1 * min(optional_gap * 2, 1.0))
    return {"fitness": fitness, "reason": "ok", "report": report,
            "signature": sig, "path": out}


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--band", default="0.35:0.55",
                   help="target requiredDifficulty band lo:hi")
    p.add_argument("--generations", type=int, default=8)
    p.add_argument("--population", type=int, default=24)
    p.add_argument("--keep", type=int, default=6, help="levels to write out")
    p.add_argument("--out", default="assets/levels/generated")
    p.add_argument("--rng-seed", type=int, default=None,
                   help="seed the SEARCH for reproducibility")
    args = p.parse_args(argv)

    lo, hi = (float(x) for x in args.band.split(":"))
    rng = random.Random(args.rng_seed)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    if not RENDER.exists():
        print(f"render_genome not built at {RENDER}", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory() as tmp:
        workdir = Path(tmp)
        population = [random_genome(rng) for _ in range(args.population)]
        kept = []            # (fitness, genome, report, path-in-tmp, signature)
        kept_signatures = []
        counter = 0

        for gen in range(args.generations):
            scored = []
            for genome in population:
                result = evaluate(genome, (lo, hi), kept_signatures, workdir, counter)
                counter += 1
                scored.append((result.get("fitness", 0.0), genome, result))
            scored.sort(key=lambda t: t[0], reverse=True)

            best = scored[0]
            winnable = sum(1 for f, _g, r in scored
                           if r.get("report", {}).get("winnable"))
            print(f"gen {gen}: best fitness {best[0]:.3f} "
                  f"(difficulty {best[2].get('report', {}).get('requiredDifficulty', '-')}), "
                  f"{winnable}/{len(scored)} winnable")

            # Bank this generation's elite into the keep pool.
            for fitness, genome, result in scored[:3]:
                if fitness <= 0.0 or "path" not in result:
                    continue
                kept.append((fitness, genome, result))
                kept_signatures.append(result["signature"])
            kept.sort(key=lambda t: t[0], reverse=True)
            kept = kept[: args.keep * 2]          # bounded pool

            # Next generation: elitism + tournament children.
            elite = [g for _f, g, _r in scored[:4]]
            children = list(elite)
            while len(children) < args.population:
                a = max(rng.sample(scored, 3), key=lambda t: t[0])[1]
                b = max(rng.sample(scored, 3), key=lambda t: t[0])[1]
                child = crossover(a, b, rng)
                children.append(mutate(child, rng))
            population = children

        # Write the winners with sidecars and a manifest.
        manifest = []
        for rank, (fitness, genome, result) in enumerate(kept[: args.keep]):
            name = f"evolved_{rank:02d}.json"
            dest = out_dir / name
            dest.write_bytes(Path(result["path"]).read_bytes())
            solvability.write_waypoints(str(dest), result["report"])
            manifest.append({"file": name, "fitness": round(fitness, 4),
                             "genome": genome,
                             "requiredDifficulty": result["report"]["requiredDifficulty"],
                             "hardestOptional": result["report"]["hardestOptional"]})
            print(f"kept {dest}  fitness={fitness:.3f} "
                  f"difficulty={result['report']['requiredDifficulty']}")
        (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=1))
        print(f"wrote {len(manifest)} levels + sidecars + manifest to {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

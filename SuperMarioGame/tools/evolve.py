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


def _solid_tile_type(level_json: dict) -> str:
    """The level's own ground vocabulary, for repairs that match its theme."""
    from collections import Counter
    solid_types = [t["type"] for t in level_json.get("tiles", [])
                   if lt._TILE_TO_CLASS.get(t["type"]) in (lt.SOLID,)]
    return Counter(solid_types).most_common(1)[0][0] if solid_types else "ground"


def repair(path: Path, report: dict) -> bool:
    """One oracle-guided repair: a stepping stone inside the hardest edge.

    The bottleneck names both ends of the move that sets the level's required
    difficulty. A solid tile halfway along it splits that move into two easier
    ones — repeat, and required difficulty descends stepwise. This is the
    repair instruction the oracle's docstring promises: the level is edited at
    the exact cells the analysis names, never blindly.
    """
    bn = report.get("bottleneck") or {}
    if "fromX" not in bn:
        return False
    mx = (bn["fromX"] + bn["x"]) // 2
    # Midpoint in HEIGHT too. The first version put the stone at the lower
    # foothold's height, which splits a long flat jump but does nothing for a
    # rise-dominated one — a 3-up-4-across bottleneck kept its full 3-tile
    # climb and the descent never descended. Halfway up, it becomes two
    # half-climbs.
    my = (bn["fromY"] + bn["y"]) // 2
    if mx in (bn["fromX"], bn["x"]):
        return False                         # edge too short to split
    level_json = json.loads(path.read_text())
    stone = {"type": _solid_tile_type(level_json), "x": int(mx), "y": int(my) + 1}
    for t in level_json.get("tiles", []):    # never double-place
        if t["x"] == stone["x"] and t["y"] == stone["y"]:
            return False
    level_json["tiles"].append(stone)
    path.write_text(json.dumps(level_json))
    return True


def add_safety_nets(path: Path, report: dict) -> bool:
    """Floor tiles under the route's punishing edges.

    The oracle names every required edge whose failure lands in hazard or the
    void (punishingEdges). A net 4 tiles below the edge's foothold turns that
    death into a survivable drop WITHOUT changing the jump itself — the demand
    stays, the punishment goes, which is precisely the forgiveness the fitness
    asks for and mutation of generator knobs cannot deliver.
    """
    edges = report.get("punishingEdges") or []
    if not edges:
        return False
    level_json = json.loads(path.read_text())
    tile_type = _solid_tile_type(level_json)
    existing = {(t["x"], t["y"]) for t in level_json.get("tiles", [])}
    added = 0
    # A net below the map is no net: world rows run 0..height-1, and a net for
    # a bottom-pit edge computed at foothold+4 fell out of the level and
    # silently protected nothing. Clamp into the bottom rows — a recessed
    # floor inside the pit; the band guard upstream rejects the repair if the
    # floor turns the gap into a walkway.
    max_y = int(level_json.get("height", 23)) - 2
    for edge in edges:
        nx, ny = int(edge["x"]), min(int(edge["y"]) + 4, max_y)
        for candidate in ((nx, ny), (nx - 1, ny), (nx + 1, ny)):
            if candidate not in existing:
                level_json["tiles"].append(
                    {"type": tile_type, "x": candidate[0], "y": candidate[1]})
                existing.add(candidate)
                added += 1
    if added:
        path.write_text(json.dumps(level_json))
    return added > 0


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

    # Repair descent: a winnable level that is too HARD for the band gets
    # stepping stones at its bottleneck until it fits (or repairs stop
    # helping). The generator's grammar has a difficulty floor (~0.685 — some
    # standard chunk always demands a near-5-tile jump), so without repair the
    # easy bands are simply unreachable: a 6x16 search returned zero levels
    # under 0.25.
    # A level is usually limited by SEVERAL copies of the same hard chunk, so
    # the required number only drops once every copy is repaired. Keep going
    # while each repair either lowers the number or MOVES the bottleneck to a
    # new cell; stop only when a repair changes nothing (same cell, same
    # demand), breaks the level, or the budget runs out.
    repairs = 0
    while report["requiredDifficulty"] > hi and repairs < 24:
        before = (report["bottleneck"].get("x"), report["bottleneck"].get("y"),
                  report["requiredDifficulty"]) if report.get("bottleneck") else None
        if not repair(out, report):
            break
        level = solvability.Level.load(str(out))
        new_report = solvability.analyse(level)
        if not new_report["winnable"]:
            break                    # a stone must never break the level
        after = (new_report["bottleneck"].get("x"), new_report["bottleneck"].get("y"),
                 new_report["requiredDifficulty"]) if new_report.get("bottleneck") else None
        if after == before:
            report = new_report
            break                    # stone placed, nothing changed: stuck
        report = new_report
        repairs += 1
    d = report["requiredDifficulty"]
    # Inside the band scores 1; outside decays linearly to 0 at band-width away.
    width = max(hi - lo, 1e-6)
    band_score = 1.0 if lo <= d <= hi else max(0.0, 1.0 - abs(d - (lo if d < lo else hi)) / width)

    # Forgiveness repair: nets under the punishing edges, then re-certify —
    # iterated, because each pass surfaces the NEXT-worst edges (one pass took
    # a level 0.794 -> 0.825; the bar is 0.85). Reverted (by keeping the old
    # report) if the nets somehow broke the level or pushed the required
    # difficulty out of band — a net can shortcut.
    for _pass in range(4):
        if report.get("forgiveness", 1.0) >= 0.85:
            break
        if not add_safety_nets(out, report):
            break
        netted = solvability.analyse(solvability.Level.load(str(out)))
        if not (netted["winnable"] and lo <= netted["requiredDifficulty"] <= hi + 1e-9):
            break
        if netted.get("forgiveness", 1.0) <= report.get("forgiveness", 1.0):
            report = netted
            break
        report = netted

    sig = tile_signature(out)
    if kept_signatures:
        nearest = min(signature_distance(sig, s) for s in kept_signatures)
        diversity = min(nearest / 4.0, 1.0)   # ~4 tiles of column delta = fully novel
    else:
        diversity = 1.0

    # A level whose optional content is richer than its required line is a
    # better level (the completionist gap), worth a small bonus.
    optional_gap = max(0.0, report["hardestOptional"] - d)

    # Forgiveness joins the quality terms: a level that puts death under its
    # own required moves trains (and entertains) worse than one that lets a
    # muffed jump be retried. Measured basis: the one level ever completed
    # scored highest (0.94); the level that ate six lives at one pit scored
    # lowest (0.84).
    forgiveness = report.get("forgiveness", 1.0)
    fitness = band_score * (0.45 + 0.25 * diversity +
                            0.2 * forgiveness +
                            0.1 * min(optional_gap * 2, 1.0))
    fitness -= 0.02 * repairs   # prefer genomes that are natively in band
    return {"fitness": max(fitness, 0.0), "reason": "ok", "report": report,
            "signature": sig, "path": out, "repairs": repairs}


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

            # Bank this generation's elite into the keep pool — but never a
            # reskin. The soft diversity term only DISCOURAGED clones, and the
            # pool ended up holding the same level three times (signature
            # distance 0.00 between easy_01/03/04); their identical eval
            # numbers were one level measured thrice. Distance >= 1.0 to every
            # already-kept level is a hard constraint at the door.
            for fitness, genome, result in scored[:3]:
                if fitness <= 0.0 or "path" not in result:
                    continue
                if any(signature_distance(result["signature"], sig) < 1.0
                       for sig in kept_signatures):
                    continue
                # FORGIVING is a gate bar, so it is a door constraint too: a
                # kept level must clear it, not merely be penalised for
                # missing it. Batches may come back smaller than --keep;
                # smaller and honest beats padded and failing.
                if result["report"].get("forgiveness", 1.0) < 0.85:
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

        # Write the winners with sidecars and a manifest — into a CLEAN dir.
        # A smaller batch beside a larger old one left stale levels that
        # failed the gate on the new batch's behalf: the door guaranteed every
        # fresh keeper cleared 0.85 forgiveness, and the gate still reported
        # sub-0.85 levels, because they were last run's leftovers.
        for stale in list(out_dir.glob("evolved_*.json")):
            stale.unlink()
        manifest = []
        for rank, (fitness, genome, result) in enumerate(kept[: args.keep]):
            name = f"evolved_{rank:02d}.json"
            dest = out_dir / name
            dest.write_bytes(Path(result["path"]).read_bytes())
            solvability.write_waypoints(str(dest), result["report"])
            manifest.append({"file": name, "fitness": round(fitness, 4),
                             "genome": genome,
                             "requiredDifficulty": result["report"]["requiredDifficulty"],
                             "hardestOptional": result["report"]["hardestOptional"],
                             "forgiveness": result["report"].get("forgiveness", 1.0)})
            print(f"kept {dest}  fitness={fitness:.3f} "
                  f"difficulty={result['report']['requiredDifficulty']}")
        (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=1))
        print(f"wrote {len(manifest)} levels + sidecars + manifest to {out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

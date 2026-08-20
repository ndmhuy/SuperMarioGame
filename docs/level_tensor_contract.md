# Level tensor contract

> **Side project.** Part of the map-generation half of the RL side project
> (`A/mapgen-gan`, stacked on `A/rl-neural-policy`). Nothing on `dev` reads
> this file or the tool it describes. See `docs/mapgen_gan_rl_plan.md` §0.

The executable half is `SuperMarioGame/tools/level_tensor.py`. This file
explains the choices; the tool enforces them.

A learned map generator works on a fixed-size numeric grid. The game works on
`assets/levels/*.json`. This is the one mapping between those two, versioned so
that a model trained against one vocabulary can never be silently decoded with
another — the same reason `kAIObservationVersion` exists on the policy side.

```
CONTRACT_VERSION = 1
```

Bump it whenever the vocabulary, the band or the encoding rules change. It is
written into every `.npz` and into the `generator` field of every generated
level.

---

## The grid

**14 rows × W columns**, one integer class per cell. `W` is the level width in
tiles (200 for a campaign level, 28 for a training window).

One-hot (`14 × W × 9` float32) is a *derived view*, produced by `one_hot()`
only when a trainer needs it. The canonical form is the label grid, which keeps
`encode`/`decode`/`check` free of any third-party dependency and makes the
intermediate format a diffable text file you can read.

### Why 14 rows, and which 14

Levels are 23 tiles tall. Rows `y=9..22` inclusive.

This is measured, not chosen. Across `level_1`, `level_2`, `level_3` and
`bonus_1`:

| Rows | Contents |
| :--- | :--- |
| 0–1 | Ceiling — present only in Underground and Castle, full width, no variation |
| 2–8 | Empty in every level |
| 9–11 | Empty in the shipped levels; kept as jump headroom |
| 12–22 | **Every** remaining tile and **every** entity |

Asking a generator to produce rows 2–8 would be asking it to learn to output
zeros. The ceiling is regenerated deterministically per theme on decode.

14 is also exactly the height of the VGLC Super Mario Bros. corpus, so a VGLC
level maps in with no rescaling — which is the whole point, since our own
levels cannot serve as training data (`mapgen_gan_rl_plan.md` §3).

---

## The vocabulary

Nine semantic classes. Deliberately **not** the game's `TileType` enum.

| id | name | glyph | encoded from | decoded to |
| -: | :--- | :-: | :--- | :--- |
| 0 | `empty` | `-` | absence | absence |
| 1 | `solid` | `X` | `ground`, `ice`, `conveyor`, `used` | theme's solid tile |
| 2 | `breakable` | `S` | `brick` | theme's breakable tile |
| 3 | `question` | `Q` | `question` tile, `question_block` entity | `question_block` entity |
| 4 | `pipe` | `P` | `pipe` tile, `pipe` entity | `pipe` tile |
| 5 | `hazard` | `v` | `water`, `lava` | `lava` in Castle, else `water` |
| 6 | `coin` | `o` | `coin_tile`, `coin` entity | `coin_tile` |
| 7 | `enemy` | `*` | any enemy entity | a species from the theme's cycle |
| 8 | `platform` | `=` | `moving_platform`, `falling_platform`, `trampoline` | `moving_platform` |

### Why classes and not tile types

`Ground`, `Ice` and `Conveyor` are one class because they are the same thing to
a player and to a generator: a surface you stand on. Which one gets emitted is
the **theme's** decision, made at decode time from a lookup table. A model that
learned to distinguish them would be spending capacity to reproduce a config
field, and it would need retraining to add a theme.

The same argument covers `water`/`lava` — one `hazard` class, because "this
cell kills you" is the entire gameplay content of both.

### Why enemies are in the grid

This reverses the first draft of the plan, which placed every entity by rule
after generation.

*Where* an enemy sits — on a narrow ledge, just past a blind jump, in the
middle of a long safe run — is **structure**, and it is a large part of what
separates a level that plays well from one that does not. It is exactly what a
generator trained on real levels can learn and what independent probability
rolls (today's `MapGenerator`) cannot. VGLC encodes enemies in-grid for this
reason and MarioGAN trains on them that way.

*Which* species appears is not structure. A rule does it perfectly: the decoder
cycles the theme's species list so a level gets a mix rather than thirty
identical Goombas.

### What is deliberately excluded

Encoded as nothing, placed by the decoder or by hand:

- `flagpole`, `spawnPoint` — exactly one each, at the ends. Fixed role.
- `star_coin` — exactly three per level by spec. A count, not a structure.
- `bowser`, `boom_boom` — a boss room is authored, not sampled.
- `pow_block`, `pswitch` — puzzle items tied to hand-designed intent.
- power-ups (`mushroom`, `fire_flower`, …) — these come out of `question_block`s
  at runtime, so they are already implied by class 3.

An entity name that is neither mapped nor in the ignore list prints a warning
and is skipped, rather than being silently dropped — an unmapped entity means
the vocabulary is out of date, which is worth knowing at encode time.

---

## Lossiness, and the roundtrip test

`encode` is lossy **on purpose**: species collapse into classes, excluded
furniture disappears. So `JSON → grid → JSON` is not the identity and is not
expected to be.

What must hold is that the *second* hop is a fixed point:

```
encode(decode(encode(L))) == encode(L)
```

Everything the tensor can express survives a full trip through the game's own
format. This is what `check` asserts:

```bash
python3 tools/level_tensor.py check assets/levels/*.json
```

All 7 shipped levels pass. Run it after any vocabulary change — a change that
breaks the decoder shows up here as a cell-level diff, instead of as a level
that loads and plays subtly wrong.

---

## Commands

```bash
# JSON -> label grid (text; prints to stdout without an output path)
python3 tools/level_tensor.py encode assets/levels/level_1.json level_1.grid

# label grid -> JSON the game's LevelLoader reads
python3 tools/level_tensor.py decode level_1.grid out.json --theme castle

# roundtrip self-test
python3 tools/level_tensor.py check assets/levels/*.json

# sliding-window training set (needs numpy; this is the only command that does)
python3 tools/level_tensor.py corpus data/corpus.npz assets/levels/*.json \
        --window 28 --stride 1
```

The text grid format is one header line of JSON metadata (width, theme, name,
spawn, flagpole) prefixed with `#`, then 14 glyph rows. It is meant to be read
by eye — the fastest way to judge a generated level before running it is to
look at it:

```
--------X-----------------------SQSSQo-----oQQo------------------------
--------XX---------------X-----------------------------X-----Q---------
--------XXX--------------XX----------------------------XX--P---P-------
--XXXXXXXXXX--XXX-----=--XXX---*------=-----*------=---XXX---=-P----=--
--XXXXXXXXXX--XXXXXXX---XXXXXXXXXXXXX---XXXXXXXXXX---XXXXXXXXXXXXXX---X
```

That is `level_1`, columns 40–110. It also shows the problem the generator is
meant to solve: long flat runs, isolated single enemies, and no composed
obstacle anywhere — the signature of independent per-column probability rolls.

---

## Sliding windows

`corpus` slices each level into overlapping `14 × window` windows. With
`--window 28 --stride 1`, one 200-wide level yields 173 samples; ~30 VGLC
levels yield several thousand.

This is the standard trick that makes a GAN trainable on a corpus of a few
dozen levels, and it is why the generator's natural output unit is a **chunk**
rather than a whole level. Full levels are assembled from chunks and then
repaired (`mapgen_gan_rl_plan.md` §5).

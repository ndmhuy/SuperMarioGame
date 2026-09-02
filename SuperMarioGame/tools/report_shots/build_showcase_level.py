#!/usr/bin/env python3
"""Builds assets/levels/custom/report_showcase.json: a single flat corridor
that stages every enemy/item/block/power-up type not reachable within a short
walk of a shipped level's own spawn, for the Phase 3A screenshot catalogue.

Precedent: tests/scripts/verify_r21i_lighting_daynight.txt's header documents
the exact same technique (a python snippet writing straight into
assets/levels/custom/*.json) to reach a level the editor's ImGui palette
cannot be scripted into (scripted mouse/key events never reach
ImGui::SFML::ProcessEvent -- see Game::run(), src/Core/Game.cpp -- so
console `spawn` and the editor's ImGui palette clicks are both unscriptable).
Direct JSON authorship is the sanctioned workaround.

The exact tile positions here are also read by
tools/report_shots/build_showcase_walk_script.py, which generates
tests/scripts/report_showcase_walk.txt from them -- edit ENEMY_ZONE /
POWER_ZONE / ITEMS_BLOCKS_ZONE below and regenerate both if the layout
changes.

Layout (tileSize 32, same convention as level_1.json): floor solid on rows
21-22 for the whole width, spawnPoint (3,19) matching the shipped levels.
Three zones, left to right:
  E (enemy showcase)   x=10..64   -- 11 of the catalogue's 13 enemy types,
                                      approached but not touched (stopped one
                                      tile short) so nothing is stomped/lost
                                      before the shot.
  P (power-up ladder)  x=76..126  -- mushroom->cape_feather->(goomba touch,
                                      Cape->Super)->fire_flower->(goomba touch,
                                      Fire->Super)->(goomba touch, Super->
                                      Small)->mini_mushroom->star->
                                      mega_mushroom. Every state in the
                                      Small/Super/Fire/Cape/Mini chain plus
                                      both decorators, in one pass.
  I (items & blocks)   x=134..198 -- the remaining items and every block type,
                                      the floating ones (question/brick/hidden/
                                      ice/moving/falling/conveyor/pipe) parked
                                      at y=17 so they never block the y=20
                                      ground walk.
  Flagpole+castle at x=204/207 end the level (LevelComplete -> VictoryState).
"""
import json

TILESIZE = 32.0
WIDTH = 220
HEIGHT = 23

tiles = [
    {"type": "ground", "x": 0, "y": 21, "w": WIDTH},
    {"type": "ground", "x": 0, "y": 22, "w": WIDTH},
]

# (x, type, y, label) -- label is only for the script generator below.
ENEMY_ZONE = [
    (10, "koopa_troopa", 20, "koopa_troopa"),
    (16, "koopa_paratroopa", 16, "koopa_paratroopa"),
    (22, "spiny", 20, "spiny"),
    (28, "boo", 18, "boo"),
    (34, "piranha_plant", 20, "piranha_plant"),
    (40, "bullet_bill", 19, "bullet_bill"),
    (46, "hammer_bro", 20, "hammer_bro"),
    (52, "thwomp", 14, "thwomp"),
    (58, "chain_chomp", 20, "chain_chomp"),
    (64, "lakitu", 10, "lakitu"),
    (70, "goomba", 20, "goomba"),
]

POWER_ZONE = [
    (78, "mushroom", 20, "mushroom (Small->Super)"),
    (84, "cape_feather", 20, "cape_feather (Super->Cape)"),
    (90, "goomba", 20, "goomba touch (Cape->Super)"),
    (96, "fire_flower", 20, "fire_flower (Super->Fire)"),
    (102, "goomba", 20, "goomba touch (Fire->Super)"),
    (108, "goomba", 20, "goomba touch (Super->Small)"),
    (114, "mini_mushroom", 20, "mini_mushroom (Small->Mini)"),
    (120, "star", 20, "star (decorator)"),
    (126, "mega_mushroom", 20, "mega_mushroom (decorator)"),
]

ITEMS_BLOCKS_ZONE = [
    (134, "coin", 20, "coin"),
    (140, "star_coin", 20, "star_coin"),
    (146, "oneup_mushroom", 20, "oneup_mushroom"),
    (152, "pow_block", 20, "pow_block"),
    (158, "pswitch", 20, "pswitch"),
    (164, "trampoline", 20, "trampoline"),
    (170, "brick_block", 17, "brick_block"),
    (174, "question_block", 17, "question_block"),
    (178, "hidden_block", 17, "hidden_block"),
    (182, "ice_block", 17, "ice_block"),
    (186, "moving_platform", 17, "moving_platform"),
    (190, "falling_platform", 17, "falling_platform"),
    (194, "conveyor_belt", 17, "conveyor_belt"),
    (198, "pipe", 17, "pipe"),
]

entities = []
for x, t, y, _label in ENEMY_ZONE + POWER_ZONE + ITEMS_BLOCKS_ZONE:
    e = {"type": t, "x": x, "y": y}
    if t == "question_block":
        e["itemType"] = 0
    entities.append(e)

STOMP_GOOMBA = (6, 20)
FLAGPOLE = (204, 15)
CASTLE = (207, 15)

entities.append({"type": "goomba", "x": STOMP_GOOMBA[0], "y": STOMP_GOOMBA[1]})
entities.append({"type": "flagpole", "x": FLAGPOLE[0], "y": FLAGPOLE[1]})
entities.append({"type": "castle", "x": CASTLE[0], "y": CASTLE[1]})

level = {
    "name": "Report Showcase",
    "theme": "overworld",
    "tileSize": TILESIZE,
    "width": WIDTH,
    "height": HEIGHT,
    "spawnPoint": {"x": 3, "y": 19},
    "flagpole": {"x": 204, "y": 15},
    "tiles": tiles,
    "entities": entities,
}

import pathlib
out = pathlib.Path("assets/levels/custom/report_showcase.json")
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(level, indent=2))
print("wrote", out, "with", len(entities), "entities")

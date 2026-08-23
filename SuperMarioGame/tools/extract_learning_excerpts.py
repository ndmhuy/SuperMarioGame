#!/usr/bin/env python3
"""Pull the load-bearing code excerpts for a learning record, and refuse to
guess.

Why a script rather than copy-paste
-----------------------------------
g-rule-21 permits a learning record to quote code verbatim — it is a teaching
snapshot pinned to a named commit, not a live reference — on the condition that
the location caption is real. A hand-pasted excerpt with a hand-typed line
number is a copy with no integrity check at all: the code moves, the caption
does not, and the record quietly starts teaching the wrong lines.

So every excerpt is addressed by an ANCHOR — the exact text its first and last
line must have — and by a line range. If the range no longer starts and ends on
those anchors, this script fails loudly instead of emitting the wrong code.
That is the whole point: a shifted line number must break the build of the
document, not the reader's understanding.

Usage:
    python3 tools/extract_learning_excerpts.py            # verify + print HTML
    python3 tools/extract_learning_excerpts.py --check    # verify only
"""

import subprocess
import sys
from html import escape
from pathlib import Path

SRC  = Path(__file__).resolve().parents[1]      # .../SuperMarioGame/SuperMarioGame (the game)
REPO = SRC.parent                               # the git repository root

# Each excerpt: an id, a file, the anchors its range must begin and end on, and
# the commit whose code it shows. The commit is NOT always the record's stamp:
# an excerpt of code that has not moved since an older commit is captioned
# against that older commit, which is honest about when it was last true.
EXCERPTS = [
    dict(
        id="unsafe-loop",
        path="src/Core/PlayingState.cpp",
        first="    // 3. Update all active entities",
        last="    m_physicsEngine.update(m_entities, m_tileMap, dt);",
        commit="4f80862",
        caption="The two loops that walk m_entities every frame",
    ),
    dict(
        id="event-bus-publish",
        path="src/Core/EventBus.cpp",
        first="void EventBus::publish(const GameEvent& event) {",
        last="}",
        commit="f0a61f4",
        caption="EventBus::publish — delivery is synchronous, on the caller's stack",
    ),
    dict(
        id="queue",
        path="src/Core/PlayingState.cpp",
        first="void PlayingState::queueSpawn(std::unique_ptr<Entity> entity) {",
        last="}",
        commit="4f80862",
        caption="The fix: queueSpawn() and flushPendingSpawns()",
        span_to="void PlayingState::flushPendingSpawns() {",
    ),
    dict(
        id="flush-site",
        path="src/Core/PlayingState.cpp",
        first="    // 3a2. Admit everything a handler asked for during the two loops above.",
        last="    flushPendingSpawns();",
        commit="4f80862",
        caption="The one point in the frame where m_entities may grow",
    ),
]


def read(path):
    return (SRC / path).read_text(encoding="utf-8").splitlines()


def locate(lines, first, last, span_to=None):
    """Find the range whose first line is `first` and which ends at the first
    `last` at or after it. Returns (start_index, end_index) 0-based inclusive."""
    try:
        start = lines.index(first)
    except ValueError:
        raise SystemExit(f"ANCHOR LOST: first line not found:\n  {first!r}")

    # For a multi-function excerpt, keep going until we have passed the second
    # function's opening line, then close on its terminating brace.
    search_from = start + 1
    if span_to is not None:
        try:
            search_from = lines.index(span_to, start) + 1
        except ValueError:
            raise SystemExit(f"ANCHOR LOST: span_to not found after start:\n  {span_to!r}")

    for i in range(search_from, len(lines)):
        if lines[i] == last:
            return start, i
    raise SystemExit(f"ANCHOR LOST: last line {last!r} not found after {first!r}")


def main():
    check_only = "--check" in sys.argv
    out = []
    for spec in EXCERPTS:
        lines = read(spec["path"])
        start, end = locate(lines, spec["first"], spec["last"], spec.get("span_to"))

        # The assertion that earns the quote: re-verify both anchors against the
        # range we are about to emit, not against where we hoped it was.
        assert lines[start] == spec["first"], spec["id"]
        assert lines[end] == spec["last"], spec["id"]

        body = "\n".join(lines[start:end + 1])
        loc = f"SuperMarioGame/{spec['path']}:{start + 1}-{end + 1} @ {spec['commit']}"
        print(f"  ok  {spec['id']:<18} {loc}", file=sys.stderr)

        out.append(
            f'<div class="excerpt" id="ex-{spec["id"]}">\n'
            f'<div class="excerpt-cap">{escape(spec["caption"])} &mdash; '
            f'<code>{escape(loc)}</code></div>\n'
            f'<pre><code>{escape(body)}</code></pre>\n'
            f'</div>'
        )

    if not check_only:
        print("\n\n".join(out))


if __name__ == "__main__":
    main()

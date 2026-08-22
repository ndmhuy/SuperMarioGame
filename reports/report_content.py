#!/usr/bin/env python3
"""Prose and layout for the CS202 final report. Built by build_report.py."""

CSS = """
:root{--bg:#fbfaf8;--surface:#fff;--line:#e2dfd8;--tx:#1b1a18;--mut:#57534c;--dim:#8b867d;
  --accent:#b5322a;--accent-soft:#fdefed;--ok:#0f7b4f;--warn:#96610a;--code-bg:#f4f2ee;
  --serif:'Iowan Old Style','Palatino Linotype',Palatino,Georgia,serif;
  --ui:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;
  --mono:ui-monospace,'SF Mono',Menlo,Consolas,monospace}
@media(prefers-color-scheme:dark){:root:not([data-theme="light"]){--bg:#15161a;--surface:#1c1e23;
  --line:#2f323a;--tx:#eae8e4;--mut:#a9a59d;--dim:#7c786f;--accent:#f08a80;--accent-soft:#2c1c1b;
  --ok:#4ec98a;--warn:#e0a83c;--code-bg:#23252c}}
:root[data-theme="dark"]{--bg:#15161a;--surface:#1c1e23;--line:#2f323a;--tx:#eae8e4;--mut:#a9a59d;
  --dim:#7c786f;--accent:#f08a80;--accent-soft:#2c1c1b;--ok:#4ec98a;--warn:#e0a83c;--code-bg:#23252c}
*{box-sizing:border-box}
html{background:var(--bg)}
body{margin:0;background:var(--bg);color:var(--tx);font-family:var(--serif);font-size:16.5px;
  line-height:1.62;padding:44px 20px 90px}
main{max-width:74ch;margin:0 auto;min-width:0}
.cover{text-align:center;padding:36px 0 22px;border-bottom:3px double var(--line);margin-bottom:30px}
.cover .uni{font-family:var(--ui);font-size:12px;letter-spacing:.15em;text-transform:uppercase;color:var(--dim)}
.cover h1{font-size:clamp(28px,6vw,44px);margin:16px 0 6px;letter-spacing:-.02em;line-height:1.1}
.cover .sub{font-family:var(--ui);font-size:15px;color:var(--mut);margin:0}
.cover .meta{font-family:var(--mono);font-size:12.5px;color:var(--dim);margin-top:18px}
h2{font-family:var(--ui);font-size:12.5px;font-weight:700;letter-spacing:.11em;text-transform:uppercase;
  color:var(--accent);margin:46px 0 4px;padding-bottom:7px;border-bottom:1px solid var(--line)}
h3{font-size:19px;margin:28px 0 6px;letter-spacing:-.01em}
h4{font-family:var(--ui);font-size:14px;margin:20px 0 4px;color:var(--tx)}
p,li{color:var(--mut)}
p{margin:12px 0}
strong{color:var(--tx);font-weight:600}
code{font-family:var(--mono);font-size:.86em;background:var(--code-bg);padding:1.5px 5px;
  border-radius:4px;border:1px solid var(--line)}
pre{background:var(--code-bg);border:1px solid var(--line);border-radius:9px;padding:14px 16px;
  overflow-x:auto;margin:14px 0}
pre code{background:none;border:none;padding:0;font-size:12.8px;line-height:1.55}
.tbl{overflow-x:auto;border:1px solid var(--line);border-radius:9px;margin:16px 0}
table{border-collapse:collapse;width:100%;font-family:var(--ui);font-size:13.5px;min-width:460px}
th,td{text-align:left;padding:9px 13px;border-bottom:1px solid var(--line);vertical-align:top}
th{font-size:10.5px;text-transform:uppercase;letter-spacing:.07em;color:var(--dim);font-weight:700;
  background:var(--code-bg)}
td{color:var(--mut)}
tr:last-child td{border-bottom:none}
figure{margin:22px 0;border:1px solid var(--line);border-radius:10px;overflow:hidden;background:var(--surface)}
figure img{display:block;width:100%;height:auto;max-width:100%}
figcaption{font-family:var(--ui);font-size:13px;color:var(--mut);padding:10px 14px;border-top:1px solid var(--line)}
blockquote{margin:16px 0;padding:12px 16px;border-left:3px solid var(--warn);background:var(--surface);
  border-radius:0 8px 8px 0;color:var(--mut);font-size:15px}
blockquote.good{border-left-color:var(--ok)}
.toc{background:var(--surface);border:1px solid var(--line);border-radius:10px;padding:16px 22px;margin:24px 0}
.toc ol{font-family:var(--ui);font-size:14px;margin:0;padding-left:20px}
.toc li{margin:5px 0}
.toc a{color:var(--mut);text-decoration:none}
.toc a:hover{color:var(--accent)}
.kpi{display:grid;grid-template-columns:repeat(auto-fit,minmax(112px,1fr));gap:10px;margin:18px 0}
.kpi div{background:var(--surface);border:1px solid var(--line);border-radius:9px;padding:12px 8px;text-align:center}
.kpi b{display:block;font-family:var(--ui);font-size:22px;color:var(--accent);line-height:1.1}
.kpi span{font-family:var(--ui);font-size:10.5px;text-transform:uppercase;letter-spacing:.06em;color:var(--dim)}
.tag{font-family:var(--ui);font-size:10.5px;font-weight:700;padding:2px 7px;border-radius:5px;
  text-transform:uppercase;letter-spacing:.05em;white-space:nowrap}
.tag.yes{background:rgba(15,123,79,.14);color:var(--ok)}
.tag.part{background:rgba(150,97,10,.16);color:var(--warn)}
.tag.no{background:rgba(181,50,42,.14);color:var(--accent)}
a{color:var(--accent)}
footer{max-width:74ch;margin:56px auto 0;padding-top:18px;border-top:1px solid var(--line);
  font-family:var(--ui);font-size:12.5px;color:var(--dim)}
@media print{body{padding:0;font-size:10.5pt}h2{page-break-after:avoid}
  pre,.tbl,figure,blockquote{page-break-inside:avoid}.toc{page-break-after:always}
  .cover{page-break-after:always;border:none}}
"""

def render(F, img):
    return f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Super Mario Game — CS202 Final Report</title>
<style>{CSS}</style></head><body><main>

<div class="cover">
  <div class="uni">University of Science &middot; VNU-HCM &mdash; CS202 Object-Oriented Programming</div>
  <h1>Super Mario Game</h1>
  <p class="sub">Final Project Report &middot; Group 52 &middot; Class 25A01</p>
  <p class="sub" style="margin-top:14px">
    Nguy&#7877;n &#272;&#236;nh Minh Huy (25125083) &middot; Tr&#7847;n Gia Huy (25125084)</p>
  <p class="meta">C++17 &middot; SFML 3.0.2 &middot; ImGui-SFML &middot; CMake<br>
  Verified against commit {F['head']} &middot; 2026-08-22</p>
</div>

<div class="toc"><ol>
<li><a href="#abstract">Abstract</a></li>
<li><a href="#intro">Introduction and objectives</a></li>
<li><a href="#group">Group information and contributions</a></li>
<li><a href="#coverage">Requirements coverage against the rubric</a></li>
<li><a href="#arch">Project architecture</a></li>
<li><a href="#oop">OOP design: hierarchy, encapsulation, polymorphism</a></li>
<li><a href="#patterns">Design patterns</a></li>
<li><a href="#impl">Implementation details</a></li>
<li><a href="#storage">Data storage and serialization</a></li>
<li><a href="#problems">Technical problems and solutions</a></li>
<li><a href="#demo">Features demonstration</a></li>
<li><a href="#verify">Verification, CI and known gaps</a></li>
<li><a href="#conclusion">Conclusion and future work</a></li>
<li><a href="#refs">References</a></li>
<li><a href="#appendix">Appendix: build, run and controls</a></li>
</ol></div>

<h2 id="abstract">1 &middot; Abstract</h2>
<p>This report documents a 2D side-scrolling platformer in the style of <em>Super Mario Bros.</em>,
written in C++17 with SFML 3.0.2 and ImGui-SFML. The project's purpose is not the game as such but the
architecture behind it: a four-level abstract inheritance tree over {F['enemies']} enemy,
{F['items']} item, {F['blocks']} block and {F['players']} player classes, driven polymorphically by a
fixed-timestep engine, and organised by ten software design patterns. The deliverable is
{F['loc']:,} lines across {F['sources']} translation units and {F['headers']} headers, with
{F['harnesses']} verification harnesses, {F['ctests']} of which are registered with CTest and run by CI on every push.</p>
<p>Two things distinguish this report from a feature list. First, every capability claimed here has
been checked to be <strong>reachable from <code>main()</code> and observed running</strong> &mdash; a
class that compiles and a harness that constructs it are not evidence, and the project has a documented
history of confusing the two. Second, &sect;12 states plainly what is <em>not</em> done, including one
rubric line the project deliberately forfeits.</p>

<div class="kpi">
<div><b>{F['loc']:,}</b><span>lines C++</span></div>
<div><b>{F['sources']+F['headers']}</b><span>files</span></div>
<div><b>10</b><span>patterns</span></div>
<div><b>{F['ctests']}</b><span>CI tests</span></div>
<div><b>{F['commits']}</b><span>commits</span></div>
</div>

<h2 id="intro">2 &middot; Introduction and objectives</h2>
<p>The assignment asks for a 2D Mario-style game demonstrating inheritance, encapsulation,
polymorphism and abstraction, with at least five design patterns, three levels of increasing
difficulty, collision detection, sound, state management and save/load. We proposed and were granted an
expanded scope of 110 features, on the argument that a larger surface is what makes the patterns
<em>load-bearing</em> rather than decorative: a Factory that builds three types is a switch statement
with a nice name; one that builds {F['enemies']+F['items']+F['blocks']+F['players']} types, configured
from JSON, is the reason the level loader does not know what a Goomba is.</p>
<p>Three engineering objectives shaped the result:</p>
<ol>
<li><strong>No entity may know about the world.</strong> An enemy that wants to throw a hammer cannot
reach the entity list; it publishes an event. This one constraint is what keeps the dependency graph
acyclic, and &sect;10.1 describes what happened when the mechanism behind it was used carelessly.</li>
<li><strong>Behaviour is composed, not inherited.</strong> Movement is a swappable strategy, player form
is a state object, temporary power-ups are decorators around it. Adding "a Koopa that flies" is a
constructor argument, not a subclass.</li>
<li><strong>Nothing is complete until it has been seen running.</strong> Enforced by CI, and by a rule
that a checkbox may not be ticked on the basis that a file compiles.</li>
</ol>

<h2 id="group">3 &middot; Group information and contributions</h2>
<p>Work was split into two <em>vertical</em> domain slices rather than horizontal layers, so that both
members wrote engine code and presentation code rather than one owning systems and the other owning UI.</p>
<div class="tbl"><table>
<thead><tr><th>Member</th><th>Domain</th><th>Systems work</th><th>Presentation work</th><th>Commits</th></tr></thead>
<tbody>
<tr><td><strong>A</strong> &mdash; Nguy&#7877;n &#272;&#236;nh Minh Huy<br>25125083</td>
    <td>Player &amp; World</td>
    <td>Core engine and game loop, physics and collision pipeline, player entities and states, items,
        TileMap and level loading, camera, save/load, time rewind, map generator</td>
    <td>Parallax backdrop, screen transitions, Menu / WorldMap / Playing / Options states, two-player
        modes, level editor</td>
    <td>179</td></tr>
<tr><td><strong>B</strong> &mdash; Tr&#7847;n Gia Huy<br>25125084</td>
    <td>Enemies &amp; Interaction</td>
    <td>Input manager and command objects, sound manager, enemies and AI movement strategies, blocks,
        entity factory, object pool, replay recorder, debug console</td>
    <td>Sprite sheets and animation, HUD, minimap, particles, death effects, audio wiring, character
        select, pause / game over / victory, statistics, achievements, boss fights</td>
    <td>57</td></tr>
</tbody></table></div>
<p>Commit counts are from <code>git shortlog -sne</code> on the integration branch and are a measure of
activity, not of value: a large share of Member A's total is engine refactoring and defect work, which
produces many small commits. Both members' work is present in every subsystem the game runs.</p>

<h2 id="coverage">4 &middot; Requirements coverage against the rubric</h2>
<p>Each row names where the capability lives and how it was confirmed. "Observed" means the game was
run and the behaviour seen; "test" means a CTest case exercises it.</p>
<div class="tbl"><table>
<thead><tr><th>Rubric item</th><th>Status</th><th>Where it lives, and the evidence</th></tr></thead>
<tbody>
<tr><td>Player inputs, movement, collision <em>(20)</em></td><td><span class="tag yes">Done</span></td>
 <td><code>InputManager</code> + 8 command objects; <code>PhysicsEngine</code> with a fixed 1/60&nbsp;s
 step, coyote time, jump buffering, wall slide, ground pound; <code>SpatialHash</code> broad phase into
 AABB narrow phase and <code>CollisionResolver</code>. Observed; covered by
 <code>verify_regressions</code> and <code>verify_all</code>.</td></tr>
<tr><td>Enemy behaviour <em>(10)</em></td><td><span class="tag yes">Done</span></td>
 <td>{F['enemies']} enemy classes including two bosses, driven by 8 interchangeable
 <code>IMovementStrategy</code> implementations (patrol, chase, fly, tethered chase, hammer throw,
 timer emergence, linear, proximity trigger). Observed; <code>verify_enemies</code>,
 <code>verify_enemies_new</code>.</td></tr>
<tr><td>Power-ups and items <em>(10)</em></td><td><span class="tag yes">Done</span></td>
 <td>{F['items']} item classes; five player forms as State objects plus two Decorators (Star, Mega).
 Observed; <code>verify_blocks</code>, <code>verify_regressions</code>.</td></tr>
<tr><td>Three levels <em>(15)</em></td><td><span class="tag yes">Done</span></td>
 <td>1-1 Grassland, 1-2 Ice Cavern, 1-3 Bowser's Castle, plus a bonus stage and three pipe-reachable
 sub-levels. All are JSON, loaded through <code>LevelLoader</code>; 1-3 ends in a boss fight. Observed
 end to end.</td></tr>
<tr><td>Sound <em>(10)</em></td><td><span class="tag yes">Done</span></td>
 <td>29 effects and 12 music tracks. <code>SoundManager</code> subscribes to 19 event types, so audio is
 wired by the Observer bus rather than called from gameplay code. Observed; <code>verify_sound_visual</code>.</td></tr>
<tr><td>Object-oriented design <em>(10)</em></td><td><span class="tag yes">Done</span></td>
 <td>&sect;6. Four-level abstract hierarchy, private state with action-oriented methods rather than
 trivial accessors, and polymorphic dispatch through <code>Entity*</code> in every engine pass.</td></tr>
<tr><td>Five design patterns <em>(25)</em></td><td><span class="tag yes">Ten</span></td>
 <td>&sect;7 names all ten with their file and the problem each solves.</td></tr>
<tr><td>AI <em>(5, additional)</em></td><td><span class="tag yes">Done</span></td>
 <td>Two tiers. Per-enemy AI through movement strategies, and a full CPU <em>opponent</em>:
 <code>AIController</code> builds an observation of the world each frame and drives a second Player
 through the same command objects a human uses, with selectable skill and archetype. Observed &mdash;
 &sect;11, figure 4.</td></tr>
<tr><td>Multiple players <em>(5, additional)</em></td><td><span class="tag yes">Done</span></td>
 <td>Four modes: Versus (two humans), Versus CPU, Co-op (shared lives and score, friendly fire off) and
 Shadow Chase (one human pursued by a 3-second-delayed replay of themselves). Independent key bindings
 per player; shared-screen camera framing the midpoint. Observed.</td></tr>
<tr><td>3D game <em>(5, additional)</em></td><td><span class="tag no">Not attempted</span></td>
 <td><strong>Deliberately forfeited.</strong> The brief offers 2D or 3D, and the project chose 2D:
 rebuilding the renderer in 3D would have consumed the effort that went into the pattern architecture
 without demonstrating any more OOP. We record this as a real 5-point cost rather than describing the
 parallax backdrop as "2.5D".</td></tr>
<tr><td>Save / load <em>(requirement)</em></td><td><span class="tag yes">Done</span></td>
 <td><code>Serializer</code>, JSON, three slots plus auto-save at checkpoints; separate files for
 settings, high scores and campaign progress. <code>verify_save_load</code>.</td></tr>
<tr><td>Game state management <em>(requirement)</em></td><td><span class="tag yes">Done</span></td>
 <td>Eight <code>IGameState</code> implementations on a stack-capable
 <code>GameStateManager</code>. <code>verify_frontend_states</code>.</td></tr>
<tr><td>Level editor <em>(bonus)</em></td><td><span class="tag yes">Done</span></td>
 <td>In-game ImGui editor (F1): full tile and entity palette, free camera, undo/redo via Command
 objects, JSON export/import. Observed &mdash; figure 5.</td></tr>
<tr><td>Character switching <em>(bonus)</em></td><td><span class="tag yes">Done</span></td>
 <td>Four playable characters with distinct physics (Luigi jumps 1.2&times; higher, moves 0.85&times;,
 falls 0.9&times;, and double-jumps) and a selection screen.</td></tr>
</tbody></table></div>

<h2 id="arch">5 &middot; Project architecture</h2>
<p>The engine is layered, and the dependency arrows point one way only. Nothing in
<code>Entities</code> includes anything from <code>Core</code> except the singletons and the event bus;
nothing in <code>Physics</code> knows what a Goomba is.</p>
<pre><code>            +-----------------------------------------------+
            |  Core        Game loop, states, input, audio,  |
            |              events, commands, rewind          |
            +-----------------------------------------------+
               |  owns states           ^  publish/subscribe
               v                        |
            +-----------------------------------------------+
            |  Entities    Entity -> Character -> Player     |
            |              / Enemy; Item; Block; strategies  |
            +-----------------------------------------------+
               |  vector&lt;unique_ptr&lt;Entity&gt;&gt;   ^ AABB, resolve
               v                                |
            +-----------------------------------------------+
            |  Physics     SpatialHash, CollisionDetector,   |
            |              CollisionResolver, PhysicsEngine  |
            +-----------------------------------------------+
               |  reads positions               ^ tile queries
               v                                |
            +-----------------------------------------------+
            |  Graphics    Camera, SpriteSheet, Animator,    |
            |              HUD, particles, parallax, minimap |
            +-----------------------------------------------+
                                |
                                v
            +-----------------------------------------------+
            |  Utils       TileMap, LevelLoader, Serializer, |
            |              MapEditor, MapGenerator, catalogue|
            +-----------------------------------------------+
</code></pre>
<p>The frame is a fixed-timestep loop: input is drained from the OS event queue, the accumulator runs
<code>update(1/60)</code> as many times as it owes, and rendering interpolates between the last two
states. Physics is therefore deterministic and independent of frame rate, which is what makes the
Memento-based time rewind (&sect;7) reproducible.</p>

<h2 id="oop">6 &middot; OOP design</h2>
<h3>6.1 Inheritance and abstraction</h3>
<pre><code>Entity (abstract: update, render, getBoundingBox, getTypeName)
 |
 +-- Character (abstract: adds velocity, facing, onGround, jump/move)
 |    |
 |    +-- Player (abstract: adds IPlayerState, lives, score, combo)
 |    |    +-- Mario   Luigi   Toad   Peach   ShadowMario
 |    |
 |    +-- Enemy (abstract: adds IMovementStrategy*, onStomped, onHitByFireball)
 |         +-- Goomba  KoopaTroopa  KoopaParatroopa  Spiny  Boo
 |         +-- PiranhaPlant  BulletBill  HammerBro  Thwomp
 |         +-- ChainChomp  Lakitu
 |         +-- Boss (abstract: health bar, phases, i-frames, arena, stagger)
 |              +-- Bowser   BoomBoom
 |
 +-- Item (abstract: activate(Player&amp;), collect)
 |    +-- Mushroom  FireFlower  CapeFeather  Star  OneUpMushroom
 |    +-- MegaMushroom  MiniMushroom  Coin  StarCoin
 |    +-- POWBlock  PSwitch  Trampoline  BridgeAxe
 |
 +-- Block (abstract: onHitFromBelow(Player&amp;))
 |    +-- BrickBlock  QuestionBlock  HiddenBlock  IceBlock  Pipe
 |    +-- MovingPlatform  FallingPlatform  ConveyorBelt  Flagpole  Castle
 |
 +-- Projectile (abstract) --> Fireball  Hammer  BossFireball
</code></pre>
<p>The abstract classes are abstract in the strict sense: <code>Entity::update</code>,
<code>Item::activate</code> and <code>Block::onHitFromBelow</code> are pure virtual, so a leaf class
cannot exist without deciding what it does.</p>

<h3>6.2 Polymorphism where it earns its keep</h3>
<p>The engine never asks what an entity is. <code>PhysicsEngine::update</code> takes
<code>vector&lt;unique_ptr&lt;Entity&gt;&gt;</code> and calls virtual methods:
<code>getGravityMultiplier()</code> returns 0 for a block and 1 for a Goomba, so the same integrator
handles both; <code>collidesWithTiles()</code> returns false for a dying player, which is what lets the
corpse fall through the floor without a special case anywhere in the physics code.</p>
<p>The one place a <code>dynamic_cast</code> chain used to exist &mdash; a 30-branch function turning an
entity into its serialised name &mdash; was replaced by a virtual <code>getTypeName()</code>. That change
also fixed a real defect: the cast chain tested base classes before derived ones, so several types were
shadowed by their parent and saved under the wrong name.</p>

<h3>6.3 Encapsulation</h3>
<p>State is private or protected and exposed through action-oriented methods:
<code>player.takeDamage(1)</code> rather than a settable health field, <code>boss.tryStomp()</code>
rather than a public health counter. Where a getter exists it answers a question the caller has a right
to ask (<code>isInvulnerable()</code>), never handing out an internal container or a raw window pointer.
The physics classes are declared <code>friend</code> of <code>Entity</code> so they can write positions
during resolution &mdash; a deliberate, narrow exception, preferred over public position setters that
any code could reach.</p>

<h2 id="patterns">7 &middot; Design patterns</h2>
<p>Ten, each solving a problem the codebase actually had. The rubric asks for five.</p>
<div class="tbl"><table>
<thead><tr><th>Pattern</th><th>Where</th><th>The problem it solves</th></tr></thead>
<tbody>
<tr><td><strong>Factory</strong></td><td><code>EntityFactory::create</code></td>
 <td>Levels are JSON. The loader reads the string <code>"koopa_paratroopa"</code> and must produce an
 object without including its header. The factory is the single construction point for all
 {F['enemies']+F['items']+F['blocks']+F['players']} types, and applies <code>entities.json</code>
 tuning on top.</td></tr>
<tr><td><strong>Singleton</strong></td><td>13 managers: <code>Game</code>, <code>ResourceManager</code>,
 <code>SoundManager</code>, <code>InputManager</code>, <code>EventBus</code>,
 <code>AchievementManager</code>, &hellip;</td>
 <td>One audio device, one texture cache, one event bus. Meyers singletons, so construction order is
 defined and no global is initialised before <code>main()</code>.</td></tr>
<tr><td><strong>Observer</strong></td><td><code>EventBus</code>, 35+ event types</td>
 <td>Collecting a coin must update the HUD, play a sound, advance statistics and possibly unlock an
 achievement. Without the bus, <code>Coin::activate</code> would have to know about all four.
 Subscriptions are tombstoned rather than erased so a handler may unsubscribe during delivery.</td></tr>
<tr><td><strong>State</strong></td><td><code>IGameState</code> (8 screens); <code>IPlayerState</code>
 (5 forms); <code>FallingPlatform</code>, <code>Thwomp</code></td>
 <td>Mario's jump height, hitbox and reaction to damage all change with his form. As states, the
 transition table lives in one place; as flags, it was a growing pile of conditionals.</td></tr>
<tr><td><strong>Strategy</strong></td><td>8 <code>IMovementStrategy</code> implementations</td>
 <td>A Koopa and a Paratroopa differ only in how they move. Composition means "the same enemy but
 flying" is a constructor argument.</td></tr>
<tr><td><strong>Command</strong></td><td>8 input commands; <code>IEditorCommand</code>;
 <code>IConsoleCommand</code></td>
 <td>Rebindable keys, and an editor whose undo/redo is free because every edit is already an object
 that knows how to reverse itself.</td></tr>
<tr><td><strong>Decorator</strong></td><td><code>StarDecorator</code>, <code>MegaDecorator</code> around
 <code>IPlayerState</code></td>
 <td>A Star is temporary and orthogonal to form: Fire Mario with a Star is still Fire Mario underneath.
 Wrapping the active state preserves what it wraps; a sixth state would have to be exited back into the
 right one.</td></tr>
<tr><td><strong>Memento</strong></td><td><code>GameSnapshot</code>, <code>TimeRewindManager</code>,
 <code>ReplayRecorder</code></td>
 <td>Hold R to rewind five seconds. Entities are restored <em>by id</em>, not by index, because pruning
 and spawning permute the list between capture and restore.</td></tr>
<tr><td><strong>Object Pool</strong></td><td><code>ObjectPool&lt;T&gt;</code> for fireballs, hammers,
 boss fireballs</td>
 <td>Projectiles are created and destroyed several times a second. The pool trades in
 <code>unique_ptr</code>, so pooled and unpooled entities are stored identically and the entity list
 never learns that pooling exists.</td></tr>
<tr><td><strong>Template Method</strong></td><td><code>Boss::update</code> is <code>final</code> and
 calls <code>updateBehaviour()</code>; <code>IMovementStrategy::execute</code></td>
 <td>Every boss needs i-frames, phase transitions and a defeat sequence in the right order. Sealing the
 skeleton means a new boss writes only its attack pattern and <em>cannot</em> forget the rest.</td></tr>
</tbody></table></div>

<h2 id="impl">8 &middot; Implementation details</h2>
<h3>8.1 The frame</h3>
<pre><code>Game::run()
  poll OS events ........... InputManager records held keys from the event stream
  accumulator += dt
  while accumulator >= 1/60:
      PlayingState::update(1/60)
          rewind check (R held) -> restore a Memento and return
          input -> command objects -> Player
          AIController decides for a CPU opponent
          for each entity: update()          <-- may request spawns
          PhysicsEngine::update(entities, tilemap)
              broad phase: SpatialHash
              narrow phase: AABB, axis-separated X then Y
              CollisionResolver: stomp / damage / collect / bump
          flushPendingSpawns()               <-- the only point the list may grow
          prune inactive, recycle pooled types
          hazards: lava, void plane, warp pipes, boss arena
          record one Memento
      accumulator -= 1/60
  render with interpolation
</code></pre>
<p>The ordering of <code>flushPendingSpawns()</code> is not cosmetic; &sect;10.1 is the incident that put
it there.</p>

<h3>8.2 Collision</h3>
<p>Broad phase buckets entities into a spatial hash keyed by tile coordinates, so an <em>n</em>-entity
level costs <em>O</em>(n) pair tests against neighbours rather than <em>O</em>(n&sup2;) against
everything. Narrow phase is AABB overlap with the axes resolved separately &mdash; X first, then Y &mdash;
which is what makes walking into a wall while jumping behave correctly. Tile collision is a direct grid
lookup, <em>O</em>(1) per probe, since the tilemap is a dense array.</p>

<h3>8.3 The entity catalogue</h3>
<p>One table maps every <code>EntityType</code> to its serialised name, display label and palette
category. The JSON parser and the level editor's palette both read it, and a CTest case asserts that
every entry is constructible by the factory and that each class's own <code>getTypeName()</code> matches
its catalogue name &mdash; because a mismatch means a level saved from the editor loads back as a
different entity. Before this existed the list was hand-written in three places and had drifted:
&sect;10.3.</p>

<h2 id="storage">9 &middot; Data storage and serialization</h2>
<p>Everything persistent is JSON via <code>nlohmann/json</code>, chosen over a binary format because a
level file that a human can read and hand-edit is worth more during development than a few kilobytes.</p>
<div class="tbl"><table>
<thead><tr><th>File</th><th>Holds</th></tr></thead>
<tbody>
<tr><td><code>assets/levels/*.json</code></td><td>Tile spans (run-length encoded by <code>w</code>),
 entity placements with per-instance fields such as Bowser's arena, spawn point, theme, dimensions.</td></tr>
<tr><td><code>assets/config/entities.json</code></td><td>Per-type tuning &mdash; speed, score, health
 &mdash; applied by the factory after construction, so balance changes need no rebuild.</td></tr>
<tr><td><code>saves/slot_N.json</code></td><td>Three save slots: form, lives, coins, score, position,
 star coins.</td></tr>
<tr><td><code>saves/progress.json</code></td><td>Campaign completion and unlocks; drives the world map
 and the New Game+ counter.</td></tr>
<tr><td><code>saves/config.json</code>, <code>highscores.json</code></td><td>Key bindings, volumes;
 the high-score table.</td></tr>
</tbody></table></div>
<p>Tile-name and entity-name conversion each exist in exactly one function. That is a rule with a scar
behind it: a duplicated string-to-enum chain in the level loader once drifted from the canonical one and
silently dropped every coin tile in all seven level files.</p>

<h2 id="problems">10 &middot; Technical problems and solutions</h2>

<h3>10.1 A crash on Windows that could not be reproduced on macOS</h3>
<p>A teammate reported that the game crashed in 1-3, and that deleting Bowser from the level file made
it stop. The same build played 1-3 correctly on macOS.</p>
<p>The cause was not in Bowser. Every spawn in the game travels through a synchronous event: an entity
that wants to create another entity publishes a request, and <code>PlayingState</code> performs the
spawn in a subscriber. That subscriber called <code>m_entities.push_back()</code> &mdash; on the stack of
the <code>for (auto&amp; entity : m_entities)</code> loop that was calling the publisher. A range-for
caches <code>begin()</code> and <code>end()</code> once, so when a <code>push_back</code> exceeded the
vector's capacity and reallocated, the loop's iterators pointed into freed memory.</p>
<p>That is undefined behaviour, and undefined behaviour is allowed to work: the macOS allocator left the
freed page mapped and readable, while the Windows toolchain faulted. Bowser mattered only because he is
the only entity in the game that spawns another entity several times per second, so 1-3 reaches a
reallocation almost immediately.</p>
<blockquote class="good"><strong>Solution.</strong> Spawns queue in a second vector and are admitted at
exactly one point per frame, after both the update loop and the physics pass have finished. The
invariant is structural rather than remembered: every spawn already goes through one function, so there
is nowhere for a future spawn to bypass it. A full write-up with the amortised-growth analysis, the call
stack and a worked trace is in <code>docs/learning/mid-frame-entity-spawn-crash.html</code>.</blockquote>

<h3>10.2 A boss that could not be beaten</h3>
<p>Bowser was reported as near-impossible, and the reason was structural rather than a matter of
numbers. He is immune to fire; a boss carries a second of invulnerability after every hit, during which
the collision resolver treats contact as ordinary contact and damages the player; and he walks forward
breathing fire throughout. The only legal input was five clean descending stomps under continuous
pressure, with no way to create an opening.</p>
<blockquote class="good"><strong>Solution &mdash; two routes, both from the series.</strong> Fireballs
still cost Bowser no health, but four of them stagger him: he stops attacking, loses his i-frames, and
for three seconds any contact lands a hit without hurting whoever lands it. The hit closes the window,
so every point of the health bar costs the same four fireballs. Separately, the level now ends on a
bridge over lava with an axe beyond it &mdash; reaching the axe drops the bridge and takes the boss with
it. The HUD shows how many fireballs remain to the next opening, because a mechanic whose state the
player cannot see is one they will not find.</blockquote>

<h3>10.3 A level editor that could not place a Goomba</h3>
<p>The editor's palette listed 16 of the game's ~40 entity types, and not one of them was an enemy or a
block. The list existed in three hand-written copies &mdash; the factory, the JSON parser, and the
palette &mdash; and they had drifted apart, silently, because nothing compared them.</p>
<blockquote class="good"><strong>Solution.</strong> One catalogue, read by both the parser and the
palette, plus a parity test that fails the build when a type is added in one place and not the others.
The general lesson, and the project's rule since: any fact that must exist in two places gets a test
that fails when the copies disagree, in the same commit that creates the second copy.</blockquote>

<h3>10.4 Decorations standing a tile inside the ground</h3>
<p>The parallax backdrop drew its hills, bushes and fences on a hardcoded screen line of 640&nbsp;px.
The real ground surface renders at 656. Two successive attempts to fix this each got half of it: the
first anchored to the lowest solid row, which is the <em>bottom</em> of a two-row floor slab; the second
correctly found the top of the slab, but the renderer still decided whether a layer stood on the ground
by comparing its authored baseline against the runtime ground line &mdash; so every ground layer was
reclassified as sky and pinned to the stale constant anyway.</p>
<blockquote class="good"><strong>Solution.</strong> Whether a decoration stands on the ground is a fact
about the decoration, so the layer states it instead of it being inferred. And the ground row is now the
widest solid row <em>in the lower half of the map</em> &mdash; without that restriction, 1-3's
200-tile-wide castle ceiling beat its 170-tile floor. Confirmed by measuring pixel columns in captured
frames: hill bottom at y=655, brick surface at y=656.</blockquote>

<h3>10.5 Six subsystems that were complete and inert</h3>
<p>An audit found six features marked done that no code path ever reached: they compiled, and a
<code>verify_*</code> harness constructed them, but the game never did. The P-Switch is the clearest
case &mdash; it published an event, the HUD had a field for the countdown, and nothing in between existed,
so pressing it played a sound and changed nothing.</p>
<blockquote class="good"><strong>Solution &mdash; a rule, not a patch.</strong> A task is complete only
when the code is reachable from <code>main()</code> and has been observed running. A harness proves a
class works in isolation; it does not prove the game ever constructs it. Every claim in &sect;4 of this
report is held to that standard.</blockquote>

<h2 id="demo">11 &middot; Features demonstration</h2>
<p>All five figures are unretouched frames captured from the running game by a scripted input driver
(<code>--script</code>), which synthesises events into the same path the OS event queue feeds. Nothing
outside the process is touched, so a verification run is reproducible.</p>
{img('01_menu.png', 'Main menu', 'Figure 1 &mdash; The main menu, over the parallax backdrop. Seven entries including the four multiplayer modes, the map editor and a procedurally generated level.')}
{img('02_world_1_3.png', 'World 1-3', 'Figure 2 &mdash; World 1-3, Bowser&#39;s Castle. The HUD reads WORLD 1-3, taken from the level catalogue rather than a constant. The castle towers and fencing of the parallax backdrop stand on the floor the player walks on, at the correct depth tint for the castle theme.')}
{img('03_level_end.png', 'End of 1-3', 'Figure 3 &mdash; The rebuilt end of 1-3, seen through the editor&#39;s free camera. Left to right: lava spanned by the brick bridge Bowser paces, the axe that drops it, the goal flagpole standing on the ground, and the castle drawn from the atlas&#39;s castle_end art. The editor&#39;s tile palette is open on the right.')}
{img('04_versus_cpu.png', 'Versus CPU', 'Figure 4 &mdash; Versus CPU. Both players have an icon and a life count in the HUD; the bottom strip carries the score line and who is leading. The dev panel reports the opponent&#39;s policy and what it currently believes it is doing (&quot;crossing gap&quot;) &mdash; the CPU drives a real Player through the same commands a human uses.')}
{img('05_map_editor.png', 'Map editor', 'Figure 5 &mdash; The in-game level editor (F1). The entity palette is generated from the single catalogue and grouped into Players, Enemies, Items, Blocks and Scenery, with a filter box and the serialised name on hover. Edits go through Command objects, so undo and redo come for free.')}

<h2 id="verify">12 &middot; Verification, CI and known gaps</h2>
<p>The tree holds {F['harnesses']} verification harnesses. {F['targets']} are built by CMake and
{F['ctests']} of those are registered as CTest cases and run by GitHub Actions on every push to the
integration branches &mdash; the difference is the harnesses that open a window and cannot run on a
headless runner, which are compiled but not executed there. The registered suite is currently
{F['ctests']}/{F['ctests']} green, with 497 individual checks in the regression binary alone. Cases are added whenever a defect escapes to the audit stage, so the suite is a
record of every bug the project has shipped.</p>
<h3>What is not done</h3>
<ul>
<li><strong>No 3D renderer.</strong> The 5-point rubric line is forfeited, deliberately (&sect;4).</li>
<li><strong>The Windows crash fix is not confirmed on Windows.</strong> Both verification runs were on
macOS, where the bug was already invisible. They show the fix breaks nothing; the argument that it cures
the crash is the analysis in &sect;10.1. A Windows playtest of 1-3 is the outstanding confirmation.</li>
<li><strong>The rebalanced Bowser fight is unplaytested.</strong> The stagger cost (four fireballs) and
its duration (three seconds) are reasoned, not measured against a player.</li>
<li><strong>The test suite is not hermetic.</strong> It reads and writes the real <code>saves/</code>
directory; one run was observed deleting campaign progress, and one high-score assertion passes or fails
depending on what is already on disk. This violates the project's own CI rule and is open work.</li>
<li><strong>Playtest coverage is thin.</strong> 4 recorded playtests against 86 test harnesses is the
project's standing imbalance, and the reason several of the defects in &sect;10 survived as long as they
did.</li>
</ul>

<h2 id="conclusion">13 &middot; Conclusion and future work</h2>
<p>The project delivers a complete, playable platformer whose value as coursework lies in its
architecture: ten design patterns, each introduced because a concrete problem demanded it, over a
four-level abstract hierarchy that the engine drives without ever asking what an object is. Adding a new
enemy means one class and one catalogue row; adding a new level means a JSON file.</p>
<p>The more useful outcome is what the defects taught. Three of the five problems in &sect;10 were the
same failure in different clothes &mdash; a fact duplicated in two places, drifting apart, with nothing
comparing them. The project's answer, now a standing rule, is that the commit which creates the second
copy also creates the test that fails when the copies disagree. The fourth, the Windows crash, is a
reminder that "it works on my machine" is exactly the evidence undefined behaviour is best at
manufacturing.</p>
<h3>Future work</h3>
<ul>
<li>Make the test suite hermetic, then add an AddressSanitizer job to CI &mdash; the tool that would have
found &sect;10.1 on its first run.</li>
<li>Wrap the entity list in a type whose <code>push_back</code> only the flush step can reach, so the
&sect;10.1 invariant is impossible to break rather than merely easy to keep.</li>
<li>Playtest and tune the Bowser fight, and record enough playtests to correct the 86:4 imbalance.</li>
<li>Finish the procedural generator's integration &mdash; it produces winnable levels today but is not
part of the campaign.</li>
</ul>

<h2 id="refs">14 &middot; References</h2>
<ul>
<li>Repository: <a href="https://github.com/ndmhuy/SuperMarioGame">github.com/ndmhuy/SuperMarioGame</a></li>
<li><code>SPEC.md</code> &mdash; frozen behavioural specification (constants, schemas, entity rules)</li>
<li><code>AGENTS.md</code> &mdash; engineering rules, and the incident history behind each</li>
<li><code>docs/learning/mid-frame-entity-spawn-crash.html</code> &mdash; full analysis of &sect;10.1</li>
<li>SFML 3.0.2 &mdash; <a href="https://www.sfml-dev.org/">sfml-dev.org</a>; Dear ImGui + ImGui-SFML;
    nlohmann/json</li>
<li>Gamma, Helm, Johnson, Vlissides, <em>Design Patterns</em> (1994) &mdash; Factory, Singleton,
    Observer, State, Strategy, Command, Decorator, Memento, Template Method</li>
<li>Nystrom, <em>Game Programming Patterns</em> &mdash; Object Pool, Game Loop, Component</li>
<li>Fiedler, "Fix Your Timestep!" &mdash; the fixed-step accumulator in &sect;8.1</li>
<li>Assets are from <em>Super Mario Bros.</em> (Nintendo, 1985), used for non-commercial educational
    purposes only. All rights remain with their owners.</li>
</ul>

<h2 id="appendix">15 &middot; Appendix</h2>
<h3>15.1 Build and run</h3>
<pre><code>cd SuperMarioGame
mkdir -p build &amp;&amp; cd build
cmake ..                 # fetches SFML, ImGui-SFML and nlohmann/json
cmake --build . -j8
ctest --output-on-failure # 13 verification targets
./SuperMarioGame
</code></pre>
<h3>15.2 Controls</h3>
<div class="tbl"><table>
<thead><tr><th>Action</th><th>Player 1</th><th>Player 2</th></tr></thead>
<tbody>
<tr><td>Move</td><td>A / D or &larr; / &rarr;</td><td>&larr; / &rarr;</td></tr>
<tr><td>Jump</td><td>W, &uarr; or Space</td><td>&uarr;</td></tr>
<tr><td>Crouch / ground pound</td><td>S or &darr;</td><td>&darr;</td></tr>
<tr><td>Run (hold)</td><td>Left Shift</td><td>Right Shift</td></tr>
<tr><td>Fireball</td><td>F or J</td><td>M</td></tr>
<tr><td>Rewind time (hold)</td><td>R</td><td>&mdash;</td></tr>
<tr><td>Level editor</td><td>F1</td><td>&mdash;</td></tr>
<tr><td>Dev panel / debug console</td><td>F12 / ~</td><td>&mdash;</td></tr>
<tr><td>Pause</td><td>Escape</td><td>Escape</td></tr>
</tbody></table></div>
<h3>15.3 Figures at a glance</h3>
<div class="tbl"><table>
<thead><tr><th>Measure</th><th>Value</th></tr></thead>
<tbody>
<tr><td>C++ source files / headers</td><td>{F['sources']} / {F['headers']}</td></tr>
<tr><td>Lines of C++ (excluding third-party)</td><td>{F['loc']:,}</td></tr>
<tr><td>Concrete entity classes</td><td>{F['players']} players, {F['enemies']} enemies,
    {F['items']} items, {F['blocks']} blocks</td></tr>
<tr><td>Design patterns</td><td>10</td></tr>
<tr><td>Verification harnesses</td><td>{F['harnesses']} source files, {F['targets']} built,
    {F['ctests']} registered as CTest cases</td></tr>
<tr><td>Commits on the integration branch</td><td>{F['commits']}</td></tr>
<tr><td>Sound effects / music tracks</td><td>29 / 12</td></tr>
<tr><td>Achievements</td><td>12</td></tr>
</tbody></table></div>

<footer>Group 52 &middot; CS202 Object-Oriented Programming &middot; University of Science, VNU-HCM<br>
Generated by <code>reports/build_report.py</code> from the repository at commit {F['head']}.
Self-contained: opens from disk, no network, prints to PDF.</footer>

</main></body></html>
"""

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
figure.uml{background:var(--code-bg)}
.uml-scroll{overflow-x:auto;padding:18px 16px}
.uml-scroll svg{display:block;min-width:340px}
figcaption .note{color:var(--dim);font-size:12px}
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

TITLE = "CS202 Super Mario Report"


def render(F, img, uml):
    """The page CONTENT: title, styles and body.

    Deliberately not a whole HTML document. build_report.py wraps this in a
    doctype/head/body skeleton for the file on disk, and publishes the same
    string unwrapped as an Artifact, which supplies its own skeleton. One
    source, two renderings - rather than two copies of the prose that would be
    a paragraph apart within a week (g-rule-22).
    """
    return f"""<title>{TITLE}</title>
<style>{CSS}</style>
<main>

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
<li><a href="#process">Process and project history</a></li>
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
<p>Every diagram in this section is <strong>generated from the headers</strong> by
<code>SuperMarioGame/tools/gen_class_diagram.py</code> at the moment this report is built, so it cannot
drift from the code. That is not a stylistic preference: the project's previous hand-written
<code>class_diagram.md</code> had rotted to the point of omitting <code>Boss</code>,
<code>Bowser</code>, <code>BoomBoom</code>, <code>Spiny</code>, <code>Lakitu</code>,
<code>ShadowMario</code>, <code>AIController</code>, <code>ObjectPool</code> and
<code>TimeRewindManager</code> &mdash; most of the architecture worth drawing &mdash; while still
reading as authoritative. It is now generated by the same script.</p>
<p>A dashed border and a <em>&#171;stereotype&#187;</em> tag mark an abstract class or an interface;
the hollow triangle is UML generalization, pointing at the base.</p>

<h3>6.1 The entity tree</h3>
{uml('Entity', 2, 'Figure 6 &mdash; The Entity hierarchy, two levels deep.',
     'Four branches under one abstract root. Character adds motion state; Item adds activate/collect; '
     'Block adds onHitFromBelow; Projectile carries no gravity. The engine holds all of them as Entity*.')}
<p>The root is abstract in the strict sense &mdash; <code>Entity::update</code>,
<code>Entity::render</code> and <code>Entity::getBoundingBox</code> are pure virtual, so no leaf can
exist without deciding what it does. The three second-level abstracts each add exactly one obligation:
<code>Item::activate(Player&amp;)</code>, <code>Block::onHitFromBelow(Player&amp;)</code>, and
<code>Character</code>'s velocity and grounded state.</p>

<h3>6.2 The other two branches: Item and Block</h3>
{uml('Item', 2, 'Figure 6a &mdash; Twelve items behind one activate() call.',
     'A Mushroom, a Star and a 1-Up all answer the same question &mdash; what happens to the player who '
     'touches me &mdash; with a different body. QuestionBlock never asks which one it is holding.')}
{uml('Block', 2, 'Figure 6b &mdash; Ten blocks, three of them stateful.',
     'FallingPlatform and Thwomp are State machines in their own right (Idle/Shaking/Falling/Respawning; '
     'wind-up/slam/rest/climb); Pipe and Flagpole are not &mdash; they fire once. Block does not know '
     'the difference, which is the point of putting it here rather than in each concrete class.')}
<p><code>Item</code> and <code>Block</code> look similar &mdash; both are collidable, both are mostly
inert until the player reaches them &mdash; and the project's own history has a concrete argument for
keeping them as separate branches rather than merging them into one "InteractableEntity". A block's
defining question is <em>what is on the other side of this collision, structurally</em>
(<code>collidesWithTiles()</code>, <code>onHitFromBelow</code>) &mdash; the physics engine needs to know
before it resolves a frame. An item's defining question is <em>what happens to the player who reaches
it</em> (<code>activate(Player&amp;)</code>) &mdash; physics does not need to know in advance, because
touching a Star does not change how the world resolves collisions this frame. Collapsing the two would
have handed the physics engine's collision-resolution code a branch on "is this actually solid" for
every entity that is nominally a block, which is exactly the kind of type-testing OOP's dispatch is
supposed to replace. <code>PSwitch</code> is the edge case that proves the split is real work and not
just naming: it is a <code>Block</code> (grounded, collidable) whose <code>onHitFromBelow</code> triggers
a level-wide event rather than reacting locally &mdash; still a block, because the physics engine still
needs to resolve a jump-bump against it, but the one that most resembles an item.</p>
<p>Ten concrete blocks and twelve concrete items ship in this project, twice what the rubric's five-item
minimum would need per SPEC's rubric-alignment reasoning (&sect;2) &mdash; the same doubled-scope
argument the feature list makes, made visible in a diagram instead of a count.</p>

<h3>6.3 Characters, enemies and bosses</h3>
{uml('Character', 2, 'Figure 7 &mdash; Character splits into Player and Enemy.',
     'Five playable characters including ShadowMario, the delayed replay of the human player used by '
     'Shadow Chase mode. Enemy is where the movement strategy is held.')}
{uml('Enemy', 2, 'Figure 8 &mdash; The enemy branch, with the Boss sub-hierarchy.',
     'Boss inserts a whole layer between Enemy and its two concrete fights: health bar, phases, '
     'i-frames, arena and stagger window, sequenced by a final update() so no boss can skip them.')}
<p><code>Boss</code> is the clearest argument in the codebase for a deep hierarchy rather than a flag.
It is not "an enemy with more health": it seals <code>update()</code> as <code>final</code> and calls
a pure-virtual <code>updateBehaviour()</code>, so <em>every</em> boss gets its invulnerability frames,
phase transitions and defeat sequence in the correct order and a new one writes only its attack
pattern. Bowser adds fire breath, a phase-2 leap and the fireball-stagger window; BoomBoom adds a
charge. Neither can forget the rest, because neither is given the chance.</p>

<h3>6.4 Player form: State plus Decorator</h3>
{uml('IPlayerState', 2, 'Figure 9 &mdash; Five forms and two decorators.',
     'PlayerStateDecorator wraps whatever base state is active, which is what lets a Star be '
     'temporary and orthogonal: Fire Mario with a Star is still Fire Mario underneath.')}
<p>Form changes Mario's jump height, hitbox and reaction to damage. As five state objects plus a
transition table, that lives in one place; as booleans it was a growing pile of conditionals. The
decorators are the part worth noticing &mdash; a sixth "Star state" would have to remember which form
to exit back into, whereas wrapping preserves it for free.</p>

<h3>6.5 The pattern interfaces</h3>
{uml('IGameState', 1, 'Figure 10 &mdash; Eight screens behind one interface (State).',
     'GameStateManager holds a stack, so PauseState can overlay PlayingState rather than replace it.')}
{uml('ICommand', 1, 'Figure 11 &mdash; Input as objects (Command).',
     'Every action the player can take is an object, which is what makes key rebinding possible '
     'and lets a second player use the same commands through a different binding table.')}
{uml('IMovementStrategy', 1, 'Figure 12 &mdash; Eight interchangeable movements (Strategy).',
     'A Koopa and a Paratroopa differ only in which of these they hold, so "the same enemy but '
     'flying" is a constructor argument rather than a subclass.')}

<h3>6.6 Polymorphism where it earns its keep</h3>
<p>The engine never asks what an entity is. <code>PhysicsEngine::update</code> takes
<code>vector&lt;unique_ptr&lt;Entity&gt;&gt;</code> and calls virtual methods:
<code>getGravityMultiplier()</code> returns 0 for a block and 1 for a Goomba, so one integrator handles
both; <code>collidesWithTiles()</code> returns false for a dying player, which is what lets a corpse
fall through the floor without a single special case in the physics code.</p>
<p>The one place a <code>dynamic_cast</code> chain used to exist &mdash; a 30-branch function turning
an entity into its serialised name &mdash; was replaced by a virtual <code>getTypeName()</code>. That
change also fixed a live defect: the chain tested base classes before derived ones, so several types
were shadowed by their parent and saved to disk under the wrong name.</p>

<h3>6.7 Encapsulation</h3>
<p>State is private or protected and reached through action-oriented methods:
<code>player.takeDamage(1)</code> rather than a settable health field; <code>boss.tryStomp()</code>,
which returns whether the hit actually landed, rather than a public health counter the resolver would
have to interpret. Where a getter exists it answers a question the caller has a right to ask
(<code>isInvulnerable()</code>, <code>isStaggered()</code>), never handing out an internal container or
a raw window pointer.</p>
<p>Two deliberate exceptions, both narrow. The physics classes are declared <code>friend</code> of
<code>Entity</code> so they can write positions during resolution &mdash; preferred over public
position setters that any code could reach. And <code>Serializer</code>'s file-path helpers stay
private even though the tests wanted them: the tests were given
<code>Serializer::clearHighScores()</code> instead, because what they needed was the table cleared, not
the path it lives at.</p>

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

<h3>7.1 The naive alternative, for four of them</h3>
<p>"The rubric asks for five" is not a reason to use a pattern; the table above names the actual problem
each one solved. Four are worth walking through the alternative that was rejected, because the
alternative is not a straw man &mdash; it is what an early, smaller version of this codebase actually
looked like before the pattern replaced it.</p>
<p><strong>Factory.</strong> The naive alternative is a level loader that <code>#include</code>s every
concrete entity header and switches on the type string:
<code>if (name == "goomba") return new Goomba(pos); else if (name == "koopa_troopa") ...</code>. It works
for the first ten types. By {F['enemies']+F['items']+F['blocks']+F['players']} types it is a
{F['enemies']+F['items']+F['blocks']+F['players']}-branch function that every new entity must be added
to, in a file (the level loader) that has no other reason to know a Spiny exists. <code>EntityFactory</code>
moves that one decision into one file whose entire job is making that decision, and the loader asks a
question ("build me a Spiny") instead of making one.</p>
<p><strong>Observer.</strong> The naive alternative is <code>Coin::activate(Player&amp; p)</code> calling
<code>p.addCoins(1); hud.refresh(); soundManager.play("coin"); stats.recordCoin();
achievements.checkCoinMilestones();</code> directly &mdash; and then <code>Star::activate</code>,
<code>OneUpMushroom::activate</code> and every other item repeating some subset of the same list, because
each item's designer has to remember which systems care this time. <code>EventBus::publish</code> lets
<code>Coin::activate</code> know only that something happened, not who is listening &mdash; and a fifth
system (say, a future daily-challenge coin counter) subscribes without <code>Coin.cpp</code> changing at
all.</p>
<p><strong>State + Decorator, together.</strong> The naive alternative is not a competing pattern but the
thing both of these replaced: a pile of booleans on <code>Player</code> &mdash;
<code>isSuper, isFire, isCape, isMini, isStarred, isMega</code> &mdash; and an
<code>if/else</code> chain in every place form matters (hitbox, jump height, death behaviour, rendering).
The moment a Star is picked up as Fire Mario, that boolean pile has to encode "currently Fire, but also
temporarily starred" as a combination no single flag names, and every one of those <code>if</code> chains
has to be taught the combination separately. Five state objects handle the base forms; two decorators
wrap whichever one is active without either of them needing to know what they are wrapping. The
combination was never a special case to design for &mdash; it falls out of the two patterns being
orthogonal to begin with.</p>
<p><strong>Template Method.</strong> The naive alternative is copying <code>Bowser</code>'s
<code>update()</code> into <code>BoomBoom</code> and editing the attack pattern in place &mdash; which is
exactly how the second boss <em>would</em> have been built without this pattern, and is the standard way
an i-frame check or a phase-transition line quietly diverges between two copies over several edits.
Sealing <code>Boss::update()</code> as <code>final</code> is a compiler-enforced version of "do not copy
this function": a third boss can only be added by writing <code>updateBehaviour()</code>, which is not
capable of skipping the sequencing the base class already owns.</p>

<h2 id="impl">8 &middot; Implementation details</h2>

<h3>8.1 The frame</h3>
<p>A fixed-timestep accumulator: input is drained from the OS queue, <code>update(1/60)</code> runs as
many times as the accumulator owes, and rendering interpolates between the last two states. Physics is
therefore deterministic and independent of frame rate &mdash; which is what makes the Memento rewind
(&sect;8.7) reproducible, and what lets the CPU opponent be judged against the same simulation a human
plays.</p>
<pre><code>Game::run()
  poll OS events ............ InputManager records held keys from the event STREAM,
                              not from sf::Keyboard::isKeyPressed - see 8.3
  accumulator += frameTime
  while accumulator &gt;= 1/60:
      PlayingState::update(1/60)
          0.  rewind check (R held) -&gt; restore a Memento and return
          1.  held keys -&gt; command objects -&gt; Player
          2.  AIController decides, for a CPU opponent
          2c. Shadow Mario samples this frame's inputs
          3.  for each entity: update()            &lt;-- may REQUEST spawns
          4.  PhysicsEngine::update(entities, tilemap)     see 8.2
          5.  flushPendingSpawns()                 &lt;-- only point the list may grow
          6.  prune inactive; recycle pooled types into their ObjectPool
          7.  hazards: lava underfoot, the void plane, warp pipes, boss arena
          8.  P-Switch clock, HUD sync, boss HUD
          9.  record one Memento
      accumulator -= 1/60
  render, interpolated
</code></pre>
<p>Step 5's position is not cosmetic; &sect;10.6 is the crash that put it there.</p>

<h3>8.2 The physics pipeline</h3>
<p><code>PhysicsEngine::update</code> runs ten ordered stages, and the order is load-bearing at almost
every step.</p>
<div class="tbl"><table>
<thead><tr><th>Stage</th><th>What it does, and why it is here</th></tr></thead>
<tbody>
<tr><td>1. Conveyor push</td><td>Applied using the <em>previous</em> frame's ground status, because this
frame's has not been computed yet and a conveyor must not act on someone who has already left it.</td></tr>
<tr><td>1.1 Interactive tiles</td><td>Coin tiles are not solid, so the collision detector never reports
them. The player's footprint is scanned separately &mdash; see &sect;10.5.</td></tr>
<tr><td>1.5 Acceleration and friction</td><td>Per-character: Luigi accelerates to 0.85&times; the speed
and keeps more momentum. Ice reduces friction; this is where surface type enters.</td></tr>
<tr><td>2. Gravity</td><td>Scaled by <code>getGravityMultiplier()</code>, so blocks (0) and projectiles
(0) are skipped, water is 0.3&times;, and Luigi falls at 0.9&times;. Clamped at
<code>TERMINAL_VELOCITY</code> = 600&nbsp;px/s, or 60 in water.</td></tr>
<tr><td>3. Integrate X, resolve X</td><td><strong>Only the maximum horizontal overlap is
resolved.</strong> See &sect;10.1: resolving every overlapping tile summed the push-outs.</td></tr>
<tr><td>4. Integrate Y, resolve Y</td><td>Separately from X, which is what makes walking into a wall
while jumping behave correctly. Brick destruction from below happens here.</td></tr>
<tr><td>4.5 Map bounds</td><td>Everything is clamped to <code>[0, width &times; 32]</code>, so nothing
walks off the side of the world.</td></tr>
<tr><td>5. Broad phase</td><td><code>SpatialHash</code> with 64&nbsp;px cells &mdash; two tiles, so a
32&nbsp;px entity touches at most four buckets. Pairs are tested only against bucket neighbours.</td></tr>
<tr><td>6. Narrow phase and resolution</td><td>AABB overlap, then
<code>CollisionResolver</code> dispatches on the pair's types: stomp, damage, collect, bump, shell
kick, boss.</td></tr>
<tr><td>7. Intent flags</td><td>Cleared last, once acceleration and resolution have consumed
them.</td></tr>
</tbody></table></div>
<p><strong>Cost.</strong> The broad phase turns an <em>O</em>(n&sup2;) all-pairs test into
<em>O</em>(n) expected work against bucket neighbours; tile queries are a direct index into a dense
array, so <em>O</em>(1) per probe. With a 200&times;23 map and a few dozen live entities the frame is
dominated by rendering, not physics.</p>
<p><strong>Feel.</strong> Two forgiveness windows, both six frames (100&nbsp;ms):
<em>coyote time</em> lets a jump register just after walking off a ledge, and the <em>jump buffer</em>
lets a jump pressed just before landing fire on touchdown. Neither is physical; both are what stops
the game feeling stiff.</p>

<h3>8.3 Input: commands, and why not <code>isKeyPressed</code></h3>
<p>Each action is an <code>ICommand</code> object, so bindings are data:
<code>InputManager</code> holds a per-player action&rarr;key table loaded from
<code>config.json</code>, and Player 2 is the same commands through a second table. Rebinding is a map
write.</p>
<p>Held state comes from the <em>event stream</em>, not <code>sf::Keyboard::isKeyPressed</code>. That
is a deliberate correction: <code>isKeyPressed</code> reads global OS state, which on macOS returns
false without Input Monitoring permission &mdash; so movement and the rewind key were dead for an
invisible, machine-specific reason. Reading the stream also means losing window focus releases
everything, rather than leaving a key stuck down.</p>

<h3>8.4 Entities: one door in, one door out</h3>
<p>Everything enters the world through <code>EntityFactory::create()</code> and then
<code>admitEntity()</code>, which wires its animations and applies the difficulty and New Game+
modifiers. Because that is the single door, a difficulty change reaches an entity that does not know
difficulty exists.</p>
<p>Leaving is symmetric: the prune step uses <code>std::stable_partition</code> rather than
<code>remove_if</code> &mdash; <code>remove_if</code> leaves the tail moved-from, so the dead objects
would already be gone before they could be offered back to a pool. Partitioning permutes instead, and
the tail is real objects. <code>forgetEntity()</code> then clears every raw pointer the state holds
into the list (the active boss, the shadow, Player 2, a carried shell) <em>before</em> the
<code>unique_ptr</code> behind it is released.</p>
<p><code>ObjectPool&lt;T&gt;</code> trades in <code>unique_ptr&lt;T&gt;</code> rather than owning its
objects, so a pooled fireball is stored exactly like an unpooled Goomba and the entity list never
learns that pooling exists. The pool caps what it retains: an unbounded free list is a leak that never
frees.</p>

<h3>8.5 Enemy behaviour and the boss template</h3>
<p>An enemy holds an <code>IMovementStrategy*</code> and delegates to it. Eight exist &mdash; patrol
(turns at ledges and hazards), chase, fly, tethered chase (Chain Chomp), hammer throw, timer emergence
(Piranha Plant), linear (Bullet Bill) and proximity trigger (Thwomp).</p>
<p>A boss is not a strategy, because a strategy describes one repeatable motion and a fight is a
sequence whose behaviour depends on its own health. <code>Boss::update()</code> is <code>final</code>
and sequences the shared parts &mdash; i-frame countdown, stagger clock, phase transition, defeat
animation &mdash; calling the pure-virtual <code>updateBehaviour()</code> for the subclass's attack
pattern. Bowser adds fire breath on a phase-dependent interval, a phase-2 leap, and the
fireball-stagger window; BoomBoom adds a charge.</p>

<h3>8.6 The tile map and level loading</h3>
<p>A dense <code>vector&lt;TileType&gt;</code> of 11 types, indexed directly. Level JSON stores tile
<em>spans</em> (<code>{{"type":"ground","x":0,"y":21,"w":170}}</code>) rather than cells, so a
200&times;23 level is a few dozen lines. Themes select the atlas frames and the parallax backdrop.</p>
<p>Loading resolves its path against several candidate roots so the game runs from either the source
tree or <code>build/</code>. That fallback had a real bug worth remembering: a single
<code>ifstream</code> was reused across candidates, and the first miss set <code>failbit</code>, so
every later <code>open()</code> was silently ignored (&sect;10.9).</p>
<p>After load, three passes run over the finished level: the backdrop's ground line is derived from the
widest solid row in the lower half of the map; the flagpole and castle are settled onto whatever floor
is beneath them; and the void plane is set one tile below the deepest floor.</p>

<h3>8.7 Time rewind, replay and the snapshot</h3>
<p><code>GameSnapshot</code> holds both players' stats, the level timer, the camera centre, and one
record per entity. Holding <strong>R</strong> pops snapshots off a 300-frame deque &mdash; five seconds
at 60&nbsp;Hz &mdash; and applies them.</p>
<p>Two details make it correct rather than approximately correct. Entities are restored <strong>by
id</strong>, not by index: pruning and spawning permute the vector between capture and restore, so
index restoration assigned positions to the wrong entities. And applying a snapshot now <em>cancels an
in-progress death</em>, because restoring a position while leaving the player in its dying state put
them back above the pit still falling.</p>
<p><code>ReplayRecorder</code> is the same snapshot stream kept longer and thinned, capped at 6000
frames &mdash; roughly ten minutes &mdash; because a recorder with no cap is a memory leak with a
feature name.</p>

<h3>8.8 The CPU opponent</h3>
<p><code>AIController</code> drives a real <code>Player</code> through the same command objects a human
uses. It has no privileged access to the world, only a wider view of it: each frame it builds an
<code>AIObservation</code> &mdash; a 21&times;15 tile grid centred on itself, each cell classified
<em>Unknown / Empty / Solid / Hazard / Reward / Enemy</em>, plus normalised offsets to its objective
and to the opponent, its own velocity, and three flags.</p>
<p><code>Unknown</code> is a distinct state from <code>Empty</code> on purpose: difficulty is expressed
as vision radius, so an Easy bot genuinely cannot see a pit it has not reached rather than seeing one
and pretending otherwise. Difficulty also sets reaction latency and action noise; the archetype
(Speedrunner, Hunter, Collector) sets what the objective <em>is</em>. The policy sits behind an
<code>IAIPolicy</code> interface, so the heuristic implementation can be swapped without the controller
changing.</p>

<h3>8.9 Multiplayer</h3>
<p>Four modes on one shared screen. The camera frames the <em>midpoint</em> of both players and a
tether keeps them together &mdash; hard in Versus, where whoever falls behind is shoved along at the
edge, and soft in Co-op, where the leader is blocked at the right edge instead so the screen simply
waits. Split screen was rejected: every screen-space overlay in the game would have had to learn about
viewports.</p>
<p>Shadow Chase is the odd one. <code>ShadowMario</code> is a <code>Player</code> replaying the human's
own input stream three seconds late, with a correction lerp to stop it desyncing. It is a contact
hazard and is unmoved by collision, because shoving it off its path would desync it from the recording
driving it.</p>

<h3>8.10 Graphics</h3>
<p>Sprites come from JSON-described atlases (<code>player.json</code> alone has 92 validated frames);
<code>Animator</code> plays named clips against them. The camera does look-ahead in the direction of
travel, clamps to level bounds, and applies screen shake <em>between two clamps</em> so a shake cannot
push the view outside the map. The parallax backdrop composes per-theme layers at fractional scroll
rates, placing decorations by a hash of world position so they do not shimmer as the camera passes.
Particles run on an object pool with a fixed capacity, so a fifty-coin burst allocates nothing.</p>

<h3>8.11 The entity catalogue</h3>
<p>One table maps every <code>EntityType</code> to its serialised name, display label and palette
category. The JSON parser and the editor's palette both read it, and a CTest case asserts that every
entry is constructible by the factory and that each class's own <code>getTypeName()</code> matches its
catalogue name &mdash; because a mismatch means a level saved from the editor loads back as a different
entity. Before this existed the list was hand-written in three places and had drifted: &sect;10.4.</p>

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
<p>Grouped by what they have in common rather than by the week they happened, because the groupings are
the lesson. Each is drawn from the weekly reports (<code>docs/Group52_04/</code>&ndash;<code>10/</code>)
and the session log, where the diagnosis was written down at the time.</p>

<h3>10.1 Collision: the ground was not solid</h3>
<p>Five separate defects, all in the first month, all producing "the player behaves oddly on flat
floor". They are worth listing together because each looks like a physics-tuning problem and none of
them was.</p>
<div class="tbl"><table>
<thead><tr><th>Symptom</th><th>Actual cause</th><th>Fix</th></tr></thead>
<tbody>
<tr><td>The player sticks at random points while walking across flat ground</td>
 <td>A horizontal overlap at the seam between two floor tiles was being read as a side-wall
 collision, so <code>velocity.x</code> was cancelled and <code>onWall</code> set.</td>
 <td><code>CollisionDetector</code> now looks at the tile <em>above</em> the one it hit: if that is
 empty, this is a floor seam, not a wall, and the push-out is converted to vertical.</td></tr>
<tr><td><code>onGround</code> flickers true/false every frame, and the player jitters</td>
 <td><code>resolveEntityVsTile</code> moved <code>position</code> but not
 <code>boundingBox</code>. The stale box was still inside the tile next frame, re-triggering the
 overlap.</td>
 <td>The bounding box is synchronised at the point of displacement.</td></tr>
<tr><td>Standing across two tiles launches the player slightly into the air</td>
 <td>Both tile collisions resolved in the same frame and the two push-outs <em>summed</em>.</td>
 <td>Resolve only the collision with the <strong>maximum penetration</strong> per axis per frame.
 That is sufficient: clearing the deepest overlap on a flat surface clears the shallower ones with
 it, so one correction does the work of all of them without double-counting.</td></tr>
<tr><td>Holding a direction against a wall in mid-air lets the player glide up it</td>
 <td>Nothing removed vertical momentum on a mid-air wall contact.</td>
 <td>Wall friction: upward velocity is zeroed on an airborne horizontal hit, and falling is capped at
 <code>WALL_SLIDE_SPEED</code> = 50&nbsp;px/s &mdash; which is also where the wall-slide mechanic came
 from.</td></tr>
<tr><td>Mario cannot enter pipes or drop through one-tile shafts</td>
 <td>His hitbox was the full 32&nbsp;px tile width, so every one-tile gap was an exact fit and caught
 on the corner AABBs either side.</td>
 <td>Hitboxes narrowed and centred inside the sprite: 24&times;30 Small, 24&times;60 Super, 14&times;14
 Mini. A 4&nbsp;px margin each side is the whole fix.</td></tr>
</tbody></table></div>
<p>The pattern: four of the five were a <em>bookkeeping</em> error &mdash; state updated in one place
and not the matching one &mdash; rather than wrong physics. The fifth was a geometry assumption nobody
had written down.</p>

<h3>10.2 Gravity applied to things that should not fall</h3>
<p><code>Block</code> derives from <code>Entity</code>, and <code>Entity</code>'s
<code>getGravityMultiplier()</code> returns 1. <code>Block</code> did not override it, so the physics
engine accelerated every question block and brick downwards at 1800&nbsp;px/s&sup2; and the level
geometry fell out of the bottom of the map. The fix is one line &mdash; <code>return 0.0f</code>
&mdash; but the interesting part is why it was not caught: the blocks were correct in the level file,
correct on the first rendered frame, and wrong a second later.</p>
<p>The same defect in a different costume: a mushroom dispensed from a block had no resting state, so
<code>resolveEntityVsTile</code> corrected its penetration without zeroing its accumulated downward
velocity, and it eventually slipped through a seam and fell out of the world. <code>Item</code> gained
an <code>m_onGround</code> flag and gravity now skips a resting item.</p>

<h3>10.3 Object lifetime, four times</h3>
<p>The most expensive category in the project, and the one whose members look least alike.</p>
<ul>
<li><strong>Dangling event subscriptions.</strong> <code>PlayingState</code> subscribed to
<code>PlayerShotFireball</code> and never unsubscribed, so an event published after the state was
destroyed called into freed memory. Every subscription in the state now has a matching
<code>unsubscribe()</code> in <code>exit()</code>.</li>
<li><strong>An invalid-handle sentinel that was a valid index.</strong> Subscription IDs were compared
as raw integers, so an "unset" handle could address a real subscriber. The sentinel is
<code>static_cast&lt;size_t&gt;(-1)</code>.</li>
<li><strong>Shutdown order.</strong> Closing the window destroyed the singleton managers while game
states were still alive, and the state destructors called into them &mdash; a mutex failure on the way
out. <code>Game::shutdown()</code> now pops every state <em>before</em> shutting the managers down, and
closes the window before that, because SFML's OpenGL context was being torn down while the render
window still held handles to it (an immediate <code>SIGABRT</code> at exit).</li>
<li><strong>The mid-frame spawn crash</strong> &mdash; &sect;10.6, the culmination of this theme and the
only one that was platform-dependent.</li>
</ul>

<h3>10.4 One fact, two places</h3>
<p>The project's signature failure. In each case nothing failed to compile; the program simply did
something different from what the code appeared to say.</p>
<div class="tbl"><table>
<thead><tr><th>The duplicated fact</th><th>What it cost</th></tr></thead>
<tbody>
<tr><td>The tile name &harr; <code>TileType</code> mapping, re-implemented in
<code>LevelLoader</code> alongside the canonical one in <code>SerializationUtils</code></td>
<td>The copy drifted and silently dropped <strong>every coin tile in all seven level files</strong>.
Both directions now live in one pair of functions, and a round-trip test guards them.</td></tr>
<tr><td>The entity type list, hand-written in the factory, the parser and the editor palette</td>
<td>The editor could not place a Goomba, a Koopa, a pipe or a flagpole &mdash; 16 of ~40 types, no
enemies at all. Now one <code>EntityCatalogue</code> with a parity test (&sect;8.3).</td></tr>
<tr><td>The flagpole's height: 168&nbsp;px of sprite against a 300&nbsp;px collision box</td>
<td>The pole you could touch and the pole you could see were different objects, and the catch-height
score was measured against one that did not exist.</td></tr>
<tr><td>The backdrop's ground line: a hardcoded screen constant against the real tile geometry</td>
<td>&sect;10.7 &mdash; it took three attempts.</td></tr>
<tr><td>The class diagram, hand-maintained against the headers</td>
<td>Rotted until it omitted most of the architecture while still reading as authoritative. Now
generated (&sect;6).</td></tr>
</tbody></table></div>
<p><strong>The standing rule this produced:</strong> the commit that creates the second copy also
creates the test that fails when the copies disagree. Writing this report exercised it &mdash; the
build script counts what it claims, and caught a class count inflated by a substring match and "23 test
files" being reported as "23 things CI runs".</p>

<h3>10.5 Interaction that was never wired</h3>
<p>Several features existed as far as the event that announced them and no further.</p>
<ul>
<li><strong>Coins on the tile grid could not be collected.</strong> <code>TileType::Coin</code> is not
solid, and the collision detector only reports solid tiles &mdash; so walking through one did nothing
and the coin stayed on the grid forever. The physics engine now scans the player's footprint for
non-solid <em>interactive</em> tiles after integration.</li>
<li><strong>The coin counter ignored its own payload.</strong> <code>AchievementManager</code> did
<code>m_coinsThisRun++</code> on <code>CoinCollected</code> rather than reading the amount, so a block
dispensing ten coins counted as one.</li>
<li><strong>The P-Switch and POW block</strong> published an event that reached a sound and nothing
else &mdash; no brick/coin swap, no enemies flipped, and the HUD's countdown field was never set.</li>
<li><strong>Six subsystems were complete and inert:</strong> minimap, particle system, death effects
and screen transitions all compiled, all had passing harnesses, and none were ever constructed by the
game.</li>
</ul>
<blockquote class="good"><strong>The rule, not the patch.</strong> A task is complete only when the code
is reachable from <code>main()</code> and has been observed running. A harness proves a class works in
isolation; it does not prove the game ever builds one. That is why &sect;4 records how each capability
was confirmed. The menu music, which had been failing silently at startup for weeks, was found by
running the game for six seconds &mdash; not by any of the 86 harnesses.</blockquote>

<h3>10.6 A crash on Windows that could not be reproduced on macOS</h3>
<p>A teammate reported that the game crashed in 1-3, and that deleting Bowser from the level file made
it stop. The same build played 1-3 correctly on macOS.</p>
<p>The cause was not in Bowser. Every spawn travels through a synchronous event: an entity that wants
to create another publishes a request, and <code>PlayingState</code> performs the spawn in a
subscriber. That subscriber called <code>m_entities.push_back()</code> &mdash; on the stack of the
<code>for (auto&amp; entity : m_entities)</code> loop that was calling the publisher. A range-for
caches <code>begin()</code> and <code>end()</code> once, so when a <code>push_back</code> exceeded
capacity and reallocated, the loop's iterators pointed into freed memory.</p>
<p>That is undefined behaviour, and undefined behaviour is allowed to work: the macOS allocator left
the freed page mapped and readable while the Windows toolchain faulted. Bowser mattered only because he
is the one entity that spawns another several times per second, so 1-3 reaches a reallocation almost
immediately.</p>
<blockquote class="good"><strong>Solution.</strong> Spawns queue and are admitted at exactly one point
per frame, after both the update loop and the physics pass have finished. Full analysis, with the
amortised-growth argument and a worked trace, in
<code>docs/learning/mid-frame-entity-spawn-crash.html</code>.</blockquote>

<h3>10.7 A boss that could not be beaten, and a backdrop that took three tries</h3>
<p>Bowser was near-impossible for a structural reason rather than a numeric one: he is immune to fire,
a boss carries a second of invulnerability after every hit during which contact <em>damages the
player</em>, and he advances breathing fire throughout. The only legal input was five clean descending
stomps with no way to create an opening. Two routes were added, both from the series &mdash; four
fireballs now stagger him into a three-second window where a stomp is safe and lands, and the level
ends on a bridge over lava with an axe beyond it that drops both.</p>
<p>The backdrop is the better cautionary tale. The parallax layers drew on a hardcoded screen line of
640&nbsp;px; the real ground renders at 656. The <em>first</em> fix anchored them to the lowest solid
row &mdash; which is the bottom of a two-row floor slab, so they were buried a tile instead of floating
one. The <em>second</em> correctly found the top of the slab, but the renderer still decided
ground-versus-sky by comparing a layer's authored baseline against the runtime ground line, so every
ground layer was reclassified as sky and pinned to the stale constant anyway. Only the third fix
&mdash; making "this layer stands on the ground" a property of the layer, and restricting the ground
scan to the lower half of the map so a castle ceiling cannot win &mdash; was correct. It was confirmed
by measuring pixel columns in captured frames, not by reading the code again.</p>

<h3>10.8 Process failures, and the rules they produced</h3>
<p>Three of the most costly problems were not in the code.</p>
<blockquote><strong>A destructive git command deleted a week's work.</strong> A session ran
<code>git reset --hard &amp;&amp; git clean -fd</code> to unblock a build, permanently destroying
untracked files including the entire Week 8 report. <em>Rule:</em> never discard uncommitted work to
unblock a git operation &mdash; committing is reversible, discarding is not &mdash; and record the
before/after commit hashes of every version-control operation.</blockquote>
<blockquote><strong>An audit was published from a stale checkout and had to be retracted.</strong> Its
first revision was written against a local <code>dev</code> nine commits behind, and on that basis
declared the entire audio system missing. The audio system was implemented and merged.
<em>Rule:</em> fetch before describing repository state; a local branch is not
evidence.</blockquote>
<blockquote><strong>~3,100 lines of finished work were sitting uncommitted</strong> &mdash; three
sub-level maps, the tools directory and a week's report &mdash; one careless clean from being lost.
Found and committed during the audit.</blockquote>
<p>A fourth, found while writing this report: the test suite read and wrote the developer's real
<code>saves/</code> directory, and a <code>ctest</code> run was observed <strong>deleting actual
campaign progress</strong>. One high-score assertion also passed under <code>ctest</code> and failed
running the same binary from <code>build/</code>, because the two have different working directories.
Fixed at the root &mdash; every harness now redirects saves to a temporary directory, and a test
asserts that containment.</p>

<h3>10.9 Toolchain and porting</h3>
<p>Smaller, but each cost real time.</p>
<ul>
<li><strong>SFML 3.0.2 removed <code>sf::Vertex</code>'s constructors.</strong> Parenthesis
construction no longer compiles against an aggregate; brace initialisation satisfies both 2.x and
3.x.</li>
<li><strong>A stream that stayed failed.</strong> <code>LevelLoader</code> tried candidate paths in a
loop with one <code>ifstream</code>. The first miss set <code>failbit</code>, and every subsequent
<code>open()</code> was silently ignored &mdash; so sub-levels failed to load depending only on which
directory the game was launched from. One <code>file.clear()</code> per iteration.</li>
<li><strong>Non-uniform randomness.</strong> <code>rand() % range</code> for particle spread angles
clustered visibly; replaced with <code>std::uniform_real_distribution</code> over
<code>std::mt19937</code>.</li>
<li><strong>Save files could contain nonsense.</strong> Out-of-range player coordinates were written
straight into the slot JSON and crashed on load; coordinates are clamped to the level bounds before
serialisation.</li>
<li><strong>Stale achievement toasts.</strong> Loading a save did not clear the active toast queue, so
previously-unlocked achievements popped up again after every load.</li>
</ul>

<h2 id="demo">11 &middot; Features demonstration</h2>
<p>All five figures are unretouched frames captured from the running game by a scripted input driver
(<code>--script</code>), which synthesises events into the same path the OS event queue feeds. Nothing
outside the process is touched, so a verification run is reproducible.</p>
{img('01_menu.png', 'Main menu', 'Figure 1 &mdash; The main menu, over the parallax backdrop. Seven entries including the four multiplayer modes, the map editor and a procedurally generated level.')}
{img('02_world_1_3.png', 'World 1-3', 'Figure 2 &mdash; World 1-3, Bowser&#39;s Castle. The HUD reads WORLD 1-3, taken from the level catalogue rather than a constant. The castle towers and fencing of the parallax backdrop stand on the floor the player walks on, at the correct depth tint for the castle theme.')}
{img('03_level_end.png', 'End of 1-3', 'Figure 3 &mdash; The rebuilt end of 1-3, seen through the editor&#39;s free camera. Left to right: lava spanned by the brick bridge Bowser paces, the axe that drops it, the goal flagpole standing on the ground, and the castle drawn from the atlas&#39;s castle_end art. The editor&#39;s tile palette is open on the right.')}
{img('04_versus_cpu.png', 'Versus CPU', 'Figure 4 &mdash; Versus CPU. Both players have an icon and a life count in the HUD; the bottom strip carries the score line and who is leading. The dev panel reports the opponent&#39;s policy and what it currently believes it is doing (&quot;crossing gap&quot;) &mdash; the CPU drives a real Player through the same commands a human uses.')}
{img('05_map_editor.png', 'Map editor', 'Figure 5 &mdash; The in-game level editor (F1). The entity palette is generated from the single catalogue and grouped into Players, Enemies, Items, Blocks and Scenery, with a filter box and the serialised name on hover. Edits go through Command objects, so undo and redo come for free.')}

<h2 id="process">12 &middot; Process and project history</h2>
<p>The project ran from 29 May to 22 August 2026 across {F['weeklies']} weekly reporting periods
(<code>docs/Group52_04/</code> through <code>Group52_10/</code>), {F['commits']} commits and
{F['sessions']} recorded working sessions in <code>logs/agent_history.log</code>. That log is not a changelog: each entry records the
commit before and after, whether the remotes were fetched, whether the work is reachable from
<code>main()</code>, and how it was verified. Several sections of this report are drawn from it,
including the failures below.</p>

<div class="tbl"><table>
<thead><tr><th>Period</th><th>Dates</th><th>What landed</th></tr></thead>
<tbody>
<tr><td><strong>W04</strong></td><td>29 May &ndash; 4 Jul</td>
 <td>Environment and architecture. Game loop and <code>GameStateManager</code>, collision primitives
 and spatial hash, math utilities, the entity skeletons. Member B started the input and sound spine.</td></tr>
<tr><td><strong>W05</strong></td><td>5 &ndash; 11 Jul</td>
 <td>Physics made real: camera, momentum and friction centralised into
 <code>PhysicsEngine</code>, wall sliding, and the JSON <code>LevelLoader</code>. Member B's gameplay
 entities, blocks and the animation system.</td></tr>
<tr><td><strong>W06</strong></td><td>12 &ndash; 18 Jul</td>
 <td>Persistence and tooling: save slots, statistics, achievements, the pause and settings panel, and
 the first version of the level editor with its undo/redo Command framework.</td></tr>
<tr><td><strong>W07</strong></td><td>19 &ndash; 25 Jul</td>
 <td>First real integration of both members' branches, the consolidated <code>verify_all</code>
 runner, the entity factory pipeline, and the design for two-player and Shadow Mario.</td></tr>
<tr><td><strong>W08</strong></td><td>26 Jul &ndash; 1 Aug</td>
 <td>Particles on an object pool; repository synchronisation after the branches had diverged.</td></tr>
<tr><td><strong>W09</strong></td><td>2 &ndash; 8 Aug</td>
 <td>Fireballs through the pool, flagpole scoring, trampolines, checkpoints, the Memento time-rewind,
 and the first procedural generator.</td></tr>
<tr><td><strong>W10</strong></td><td>9 &ndash; 15 Aug</td>
 <td>The three-world campaign with warp-pipe sub-levels, physics normalisation for static bodies, and
 hitbox tuning.</td></tr>
<tr><td><strong>Delivery</strong></td><td>16 &ndash; 22 Aug</td>
 <td>The audit and its remediation, the first green CI, the boss fights, multiplayer and the CPU
 opponent, and the Windows-playtest defects in &sect;10.</td></tr>
</tbody></table></div>

<h3>12.1 The audit, and what it cost</h3>
<p>On 18 August a full review of both domains produced <strong>37 findings, 7 of them critical</strong>
(<code>docs/issues/code_audit_2026-08-18.md</code>, GitHub issue #11). It is the hinge of the project,
and three things about it are worth more than the findings themselves.</p>
<blockquote><strong>The audit's first revision was wrong, and had to be retracted publicly.</strong>
It was written against a local <code>dev</code> that was nine commits stale, and on that basis declared
the entire audio system missing. The audio system was implemented and merged on
<code>origin/dev</code>. The finding was withdrawn on issue #11.</blockquote>
<blockquote><strong>About 3,100 lines of finished work were sitting uncommitted.</strong> Three
sub-level maps, the tools directory and a week's report were in the working tree with no commit, one
careless <code>git clean</code> from being lost. They were committed before anything else
happened.</blockquote>
<blockquote><strong>Six subsystems were "complete" and inert.</strong> The minimap, particle system,
death effects and screen transitions all compiled, all had passing harnesses, and none were
constructed by the game. They were wired in the same session &mdash; and the menu music, which had
been silently failing at startup for weeks, was found by <em>running the game for six seconds</em>,
not by any test.</blockquote>
<p>Each of those became a rule rather than a patch, recorded in <code>AGENTS.md</code> with the
incident that motivates it: fetch before describing repository state; never discard uncommitted work
to unblock a git operation; and "complete" means reachable from <code>main()</code> and observed
running, not that a file exists and compiles. The report you are reading is written to that third
rule, which is why &sect;4 says how each capability was confirmed and &sect;13 says what is not done.</p>

<h3>12.2 What the history actually shows</h3>
<p>The defect record has a shape. Of the five problems in &sect;10, three are the same failure wearing
different clothes &mdash; <strong>one fact stored in two places, drifting apart, with nothing
comparing them</strong>. The entity list in the factory, the parser and the editor palette. The tile
name-to-enum mapping duplicated in the loader, which silently dropped every coin tile in all seven
level files. The flagpole's collision height (300) against its sprite height (168). None of these
produced a compiler error; all of them produced wrong behaviour that looked like a design choice.</p>
<p>The project's answer, now standing practice, is that <strong>the commit which creates the second
copy also creates the test that fails when the copies disagree</strong>. The entity catalogue's parity
test (&sect;8.3) is the model. The counterexample is instructive too: this very report is generated by
a script that counts what it claims, and doing so caught two wrong figures before they were
printed &mdash; a class count inflated by a substring match, and "23 test files" reported as if it
meant "23 things CI runs".</p>
<p>The second pattern is thinner: <strong>86 test harnesses against 4 recorded playtests</strong> at
the time of the audit. Every defect in &sect;10.1, &sect;10.4 and &sect;10.5 was invisible to the test
suite and obvious within seconds of looking at the running game. The suite is not the problem &mdash;
it is what makes the fixes stick &mdash; but it verifies what someone already thought to doubt.</p>

<h2 id="verify">13 &middot; Verification, CI and known gaps</h2>
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

<h2 id="conclusion">14 &middot; Conclusion and future work</h2>
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

<h2 id="refs">15 &middot; References</h2>
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

<h2 id="appendix">16 &middot; Appendix</h2>
<h3>16.1 Build and run</h3>
<pre><code>cd SuperMarioGame
mkdir -p build &amp;&amp; cd build
cmake ..                 # fetches SFML, ImGui-SFML and nlohmann/json
cmake --build . -j8
ctest --output-on-failure # 13 verification targets
./SuperMarioGame
</code></pre>
<h3>16.2 Controls</h3>
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
<h3>16.3 Figures at a glance</h3>
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

</main>
"""

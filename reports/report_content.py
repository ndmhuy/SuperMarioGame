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


def render(F, img, uml, arch):
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
  Verified against commit {F['head']} &middot; 2026-08-31</p>
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
 <td>29 effects and 12 music tracks. <code>SoundManager</code> subscribes to 20 event types, so audio is
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
 <td>Nine <code>IGameState</code> implementations on a stack-capable
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
<figure class="uml"><div class="uml-scroll">{arch()}</div>
<figcaption>Figure 5 &mdash; The five-layer architecture.<br><span class="note">Solid arrows are the
ownership/data direction (Core owns the states that hold entities; entities are read as a flat list by
physics; physics writes positions graphics then draws). Dashed arrows are the narrower channel each
layer answers back through &mdash; an event, a resolved collision, a tile lookup &mdash; never a
direct call in the other direction.</span></figcaption></figure>
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
{uml('Item', 2, 'Figure 6a &mdash; Thirteen items behind one activate() call.',
     'A Mushroom, a Star and a 1-Up all answer the same question &mdash; what happens to the player who '
     'touches me &mdash; with a different body. QuestionBlock never asks which one it is holding.')}
{uml('Block', 2, 'Figure 6b &mdash; Ten blocks, two of them stateful.',
     'FallingPlatform and Thwomp each drive themselves through phases (Idle/Shaking/Falling/Respawning; '
     'wind-up/slam/rest/climb) held as a plain enum &mdash; state machines, but not State-pattern '
     'participants, which is argued in &sect;7.4. Pipe and Flagpole are stateless: they fire once. Block '
     'does not know the difference, which is the point of putting it here rather than in each concrete class.')}
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
<p>Ten concrete blocks and thirteen concrete items ship in this project, more than twice what the rubric's five-item
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
{uml('IGameState', 1, 'Figure 10 &mdash; Nine screens behind one interface (State).',
     'GameStateManager holds the stack as a vector, not a std::stack, so render() can walk down through '
     'an overlay: PauseState draws over PlayingState rather than replacing it.')}
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
<p>Two properties of the whole tree are worth stating because they can be checked rather than asserted.
First, <strong>no <code>class</code> in the project exposes a mutable data member</strong>: every
declared field sits behind <code>private</code> or <code>protected</code>, and the only public data
anywhere are <code>struct</code> aggregates whose whole purpose is to carry values across a boundary
(<code>GameSnapshot</code>, <code>PlayerSnapshot</code>, <code>EntitySnapshot</code>,
<code>EntityConfigEntry</code>, <code>EntityCatalogue::Entry</code>) &mdash; a simplification argued for
in &sect;7.13 and charged as a cost there. The one getter that broke the rule is worth recording
because of how it was found and closed rather than because it survives:
<code>Camera::getView()</code> used to return a mutable <code>sf::View&amp;</code>, handing any caller
the camera's internals. The audit written for this submission sweep flagged it as the class's only
encapsulation leak, and it is now <code>const sf::View&amp; getView() const</code>, with the single
caller that genuinely had to reshape the view &mdash; <code>EditorState</code>, fitting the canvas
rectangle &mdash; served by two named operations, <code>Camera::setViewSize</code> and
<code>Camera::setViewViewport</code>. That is the shape the fix should take: not a wider getter, but the
narrowest operation the real caller needs.</p>
<p>Second, <strong>ownership is uniform</strong>. Every owning pointer in the project's own code is a
<code>std::unique_ptr</code>, and there is not a single <code>delete</code> anywhere in
<code>src/</code> or <code>include/</code> &mdash; the eight textual matches are all the word appearing
in comments or in <code>= delete</code>. Raw <code>new</code> now occurs exactly once, in the
deliberately never-deleted <code>ResourceManager</code> instance of &sect;7.3, whose 27-line rationale
explains why. It had occurred eight times: the other seven were spawner lambdas in
<code>DevPanel</code> that wrapped a raw <code>new</code> in a <code>unique_ptr</code> on the same line,
and they now call <code>EntityFactory::create</code> instead, which closed a raw allocation and an
open/closed violation in one edit. Shared ownership appears exactly once in the project's own code, for
<code>ICommand</code>, and &sect;7.8 explains why.</p>

<h3>6.8 SOLID, principle by principle</h3>
<p>The five principles are taken one at a time below, each with the strongest evidence the codebase can
offer <em>and</em> the place where it is knowingly not followed. The second half of each paragraph is the
more useful one: a principle stated without its exception is a slogan, and every exception here was
decided rather than drifted into.</p>

<p><strong>Single responsibility.</strong> The evidence for is structural: physics does not know what a
Goomba is, entities do not resolve their own collisions, <code>EntityCatalogue</code> is the single
declaration of what an entity type <em>is</em>, and the class that owns a fight
(<code>Boss</code>) is separate from the class that owns a level. The evidence against is one file.
<code>PlayingState</code> is a god class: its <code>.cpp</code> is past four thousand lines and its
header past eight hundred, and it orchestrates level loading, spawning, physics stepping, the camera,
HUD synchronisation, boss arenas, two-player rules, time rewind, pipe transitions, cheats, the editor
bridge and endless-mode chunking. The next largest source file in the project is under a quarter of its
size. <strong>The decision was to document it rather than split it in submission week</strong>, and the
reason is that the direction has already been demonstrated: <code>DevPanel</code>,
<code>DebugCheats</code>, <code>LightingRenderer</code>, <code>PipeRenderer</code> and the
<code>EditorBridge</code> port were all carved out of this class, each extraction verified by playing the
game afterwards. Five more extractions in the last week, each touching the one class every mode runs
through, would put the working build at risk to improve a number in a report. The trend is the argument;
the remaining size is the honest cost, and it is listed in &sect;13 as work rather than as an
achievement.</p>

<p><strong>Open/closed.</strong> This is the principle the project can demonstrate mechanically rather
than assert. Adding an entity type is <em>one row</em> in <code>EntityCatalogue</code> &mdash; the
factory, the level parser and the editor palette all read that table, and
<code>verify_r21_entity_registry</code> walks every enumerator from <code>0</code> to
<code>EntityType::Count</code> and fails the suite for any row that is missing, so the openness is
enforced by a test rather than by a convention. The same shape holds on four more axes: a new enemy
behaviour is one <code>IMovementStrategy</code>, a new difficulty one
<code>IDifficultyStrategy</code>, a new screen one <code>IGameState</code>, a new undoable editor
operation one <code>IEditorCommand</code> &mdash; and none of them requires editing a
<code>switch</code>. Where it is not held: run-time type dispatch survives in three places, each named
with its reason. The decorator chain of &sect;7.12 is inspectable only by <code>dynamic_cast</code>, at
seventeen sites, which is an inherent cost of that pattern rather than a lapse. Per-type serialization in
<code>Serializer</code> and <code>LevelLoader</code> still branches on type, because the on-disk schema is
a compatibility surface that deliberately does not follow the class hierarchy. The third had no defence
and so was fixed rather than argued for: <code>PlayingState::recycleEntity</code> used to run three
sequential <code>dynamic_cast</code>s to decide which <code>ObjectPool</code> an expiring projectile
belonged to. It now switches on <code>Entity::poolTag()</code>, a virtual returning an
<code>Entity::PoolTag</code> that <code>Fireball</code>, <code>Hammer</code> and
<code>BossFireball</code> each override &mdash; one v-table lookup instead of three RTTI walks, and a
fourth pooled type needs an override rather than another cast block. That is the difference this section
is trying to draw: two of the three remaining type tests are the price of a decision, and the third was
simply a lapse, which is why only the third was removed.</p>

<p><strong>Liskov substitution.</strong> The evidence is that the engine's hot paths never ask what an
entity is. <code>PhysicsEngine::update</code> takes a
<code>std::vector&lt;std::unique_ptr&lt;Entity&gt;&gt;</code> and integrates all of it through virtuals:
<code>getGravityMultiplier()</code> returns 0 for a block and 1 for a Goomba, so one integrator handles
both, and <code>collidesWithTiles()</code> returns false for a dying player, which is what lets a corpse
fall through the floor with no special case anywhere in the physics code. The sharpest measurement is in
<code>CollisionResolver</code>: identifying both sides of a contact once cost up to twelve sequential
<code>dynamic_cast</code>s per colliding pair per frame, and the file now contains <strong>zero</strong>
&mdash; the four textual matches left in it are comments recording the change. Contact rules are virtual
hooks (<code>onStomped</code>, <code>onHitFromBelow</code>, <code>onPlayerTouch</code>) over an ordered
category pair, and every hierarchy root in the project &mdash; <code>Entity</code>,
<code>IGameState</code>, <code>ICommand</code>, <code>IMovementStrategy</code>,
<code>IPlayerState</code>, <code>IEditorCommand</code>, <code>IConsoleCommand</code>,
<code>IAIPolicy</code>, <code>IEntityAdmitter</code>, <code>IDifficultyStrategy</code> &mdash; declares
a <code>virtual</code> destructor, so deleting through a base pointer is always defined.
<code>PlayerStateDecorator</code> is the cleanest case of the principle: it substitutes for
<code>IPlayerState</code> by forwarding every operation to what it wraps, which is exactly why a Star can
be layered over any form. Two honest qualifications. Four of the five player forms satisfy their
interface partly by <em>doing nothing</em> (&sect;7.4), so their substitutability is cheap rather than
earned. And <code>EntityFactory::createUnconfigured</code> may return <code>nullptr</code>, a weakened
postcondition every caller has to check &mdash; accepted, because level files are hand-editable and a bad
name must not be fatal.</p>

<p><strong>Interface segregation.</strong> The interfaces added most recently are the narrowest, which
is the direction to want. <code>IEntityAdmitter</code> has two methods and a contract written into the
header saying what each must guarantee; <code>IEditorCommand</code> has three;
<code>IAIPolicy</code> has three, one of them defaulted. <code>IGameState</code> asks five things every
screen genuinely does and puts the rest &mdash; <code>isOverlay</code>, <code>onSuspend</code>,
<code>onResume</code> &mdash; behind defaults, so a screen implements only what it needs.
<code>IMovementStrategy</code> is segregated in a second sense: the public surface is
<code>execute</code> plus three defaulted queries, while the three sequencing hooks are
<code>protected</code>, so a caller cannot reach a phase and a subclass cannot reorder them. Three
failures, all real. <code>ICommand::execute(Character&amp;)</code> is a narrow interface with too fat a
parameter and no argument channel, which is precisely why the debug console needed a second hierarchy
(&sect;7.10). <code>IPlayerState</code> demands five methods of implementers that mostly need one. And
the widest interface in the codebase is not an interface at all: <code>Entity</code> and
<code>Character</code> each declare <strong>twelve</strong> <code>friend</code> classes &mdash;
<code>PhysicsEngine</code>, <code>CollisionResolver</code>, <code>PlayingState</code>, and then
<code>IMovementStrategy</code> together with all eight of its concrete strategies. That list is the bill
for &sect;7.5: moving movement out of the entity means the thing that moves it is no longer the entity,
and it still has to write velocity. Friendship was chosen over public position and velocity setters,
because a setter is reachable by <em>every</em> caller while a friend list is at least enumerable and
reviewable &mdash; but nine of the twelve entries are one pattern's cost, and the smaller fix (a single
<code>protected</code> mutation API the strategies inherit access to) is recorded in &sect;13 rather than
claimed here.</p>

<p><strong>Dependency inversion.</strong> Where a dependency crosses a layer boundary, it is inverted
through an interface, and the examples are the load-bearing ones: <code>PhysicsEngine</code> and
<code>CollisionResolver</code> depend on <code>Entity</code> and on <code>EntityCategory</code>, never on
<code>Goomba</code>; <code>GameStateManager</code> depends on <code>IGameState</code>;
<code>MapEditor</code> depends on <code>IEditorCommand</code>; the editor's entity commands depend on
<code>IEntityAdmitter</code> rather than on <code>PlayingState</code>, which is the whole point of
&sect;7.15; <code>AIController</code> depends on <code>IAIPolicy</code>; and <code>Game</code> consults
<code>IDifficultyStrategy</code> instead of comparing a string. Then the cost, owned rather than
minimised. The specification says this project has four singletons. It has <strong>twelve</strong>, and
they are reached from everywhere: <code>Game::getInstance()</code> appears at 143 call sites in 39 files,
<code>SoundManager::getInstance()</code> at 86 in 31, <code>EventBus::getInstance()</code> at 54 in 28.
Every one of those is a dependency that does not appear in a constructor signature and cannot be
substituted for a fake, which is the textbook DIP failure and is stated here as such. The reason it was
chosen is real but partial: SFML's audio device, texture cache and event bus are genuinely process-wide,
and a Meyers singleton gives a defined construction order where a file-scope global would load a texture
before the graphics context exists. What that does <em>not</em> excuse is the number.
<code>ScreenTransitionManager</code> and <code>EntityDeathEffect</code> each have exactly one caller
outside their own implementation file &mdash; <code>PlayingState</code> &mdash; so both are singletons by
habit rather than by necessity, and each would be better as a member of whatever owns it. Injecting
twelve managers through the constructors of a class the size of <code>PlayingState</code> is a week's
work carrying live regression risk, so the decision was the same as for single responsibility: document
the number, keep the dependencies that were <em>dangerous</em> under control, and put the injection work
in &sect;13 where a reader can see it is known rather than missed. The dangerous ones were the lifetime
hazards, and each now has a named mechanism: <code>EventBus::ScopedSubscription</code> for subscriber
lifetime, which <code>SoundManager</code> was the last holdout against &mdash; its twenty subscriptions
were raw ids that never unsubscribed, and are now a
<code>std::vector&lt;EventBus::ScopedSubscription&gt;</code>, closing the last place where a callback
could outlive its owner by omission rather than by design; a constant-initialised
<code>g_eventBusAlive</code> flag so a <code>ScopedSubscription</code> destructor running after the bus
is gone cannot resurrect a destroyed singleton; and the written rationale above
<code>ResourceManager</code>'s never-deleted instance, which explains why leaking one fixed-size
allocation at process exit is the correct answer to an undefined static destruction order. The pattern
across all four of those is worth naming, because it is the honest answer to a reader who counts twelve
singletons and stops there: the number was not reduced, and the failure modes it creates were closed one
at a time, each with a mechanism a compiler or a destructor enforces rather than a rule someone has to
remember.</p>

<h2 id="patterns">7 &middot; Design patterns</h2>
<p>The rubric asks for five patterns. The specification claims ten. This section takes each of the ten
in turn, adds the four the codebase grew without ever claiming them, and names the two it deliberately
does <em>without</em> &mdash; because a pattern that was considered and rejected for a stated reason is
better evidence of design than a pattern that happens to be present.</p>
<p>Every subsection answers the same four questions in the same order, and the order is the argument:
<strong>the problem</strong> this codebase actually had, <strong>the naive alternative</strong> that was
in the code or would have been, <strong>why this pattern</strong> answers it, and <strong>what it
cost</strong> &mdash; because a pattern with no cost has not been used for anything. Participants are
named with the file they live in, so every claim here can be checked against the tree rather than taken
on trust.</p>

<div class="tbl"><table>
<thead><tr><th>&sect;</th><th>Pattern</th><th>Root participant</th><th>Claimed?</th></tr></thead>
<tbody>
<tr><td>7.1</td><td>Factory</td><td><code>EntityFactory::create</code></td><td>yes</td></tr>
<tr><td>7.2</td><td>Registry / Type Object</td><td><code>EntityCatalogue::Entry</code></td><td><em>no</em></td></tr>
<tr><td>7.3</td><td>Singleton</td><td>12 managers</td><td>yes</td></tr>
<tr><td>7.4</td><td>State</td><td><code>IGameState</code>, <code>IPlayerState</code></td><td>yes</td></tr>
<tr><td>7.5</td><td>Strategy</td><td><code>IMovementStrategy</code></td><td>partly</td></tr>
<tr><td>7.6</td><td>Template Method</td><td><code>Boss::update</code></td><td>yes</td></tr>
<tr><td>7.7</td><td>Command (input)</td><td><code>ICommand</code></td><td>yes</td></tr>
<tr><td>7.8</td><td>Composite</td><td><code>CompositeCommand</code></td><td><em>no</em></td></tr>
<tr><td>7.9</td><td>Command, undoable</td><td><code>IEditorCommand</code></td><td><em>no</em></td></tr>
<tr><td>7.10</td><td>Command (console)</td><td><code>IConsoleCommand</code></td><td>partly</td></tr>
<tr><td>7.11</td><td>Observer</td><td><code>EventBus</code></td><td>yes</td></tr>
<tr><td>7.12</td><td>Decorator</td><td><code>PlayerStateDecorator</code></td><td>yes</td></tr>
<tr><td>7.13</td><td>Memento</td><td><code>GameSnapshot</code></td><td>yes</td></tr>
<tr><td>7.14</td><td>Object Pool</td><td><code>ObjectPool&lt;T&gt;</code></td><td>yes</td></tr>
<tr><td>7.15</td><td>Adapter</td><td><code>IEntityAdmitter</code></td><td><em>no</em></td></tr>
<tr><td>7.16</td><td>Rejected on purpose</td><td>Visitor, Flyweight</td><td>&mdash;</td></tr>
</tbody></table></div>

<p>A note on figures before the patterns themselves. The class diagrams in &sect;6 and &sect;16.4 are
generated from the headers by <code>gen_class_diagram.py</code>, which draws <em>inheritance</em> edges.
A pattern whose structure <em>is</em> a hierarchy therefore has a figure and is pointed at below; a
pattern whose structure is an association &mdash; Factory, Registry, Observer, Memento, Object Pool,
Adapter &mdash; has no generated figure, because there is no generalization arrow for the tool to find.
Two hierarchies that do exist are also missing from the figures for a duller reason:
<code>CompositeCommand</code> and the eleven <code>IConsoleCommand</code> classes are declared inside
<code>.cpp</code> files, and the generator only scans headers. That is recorded as a known gap in
&sect;13 rather than papered over with a hand-drawn diagram that would then rot.</p>

<h3>7.1 Factory</h3>
<p><strong>The problem.</strong> Levels are JSON. <code>LevelLoader</code> reads the string
<code>"koopa_paratroopa"</code> out of a file a human may have hand-edited, and has to end up holding a
<code>std::unique_ptr&lt;Entity&gt;</code> without knowing that a <code>KoopaParatroopa</code> class
exists. So do <code>MapGenerator</code> (which builds endless chunks at runtime),
<code>PlaceEntityCommand</code> (the editor's brush), <code>PlayingState::spawnProjectile</code>
(anything asked for at run time through an <code>EntitySpawnRequested</code> event) and
<code>DevPanel</code>'s seven spawn buttons: five callers, one question.</p>
<p><strong>The naive alternative.</strong> Each caller includes every concrete entity header and
switches on the type: <code>if (name == "goomba") return new Goomba(pos); else if ...</code>. It is
fine for ten types. At the
{F['players']+F['enemies']+F['items']+F['blocks']} concrete entity classes this project ships it is a
{F['players']+F['enemies']+F['items']+F['blocks']}-branch function that has to be edited in
<em>five</em> places for every new entity, in files whose real job is reading a file, generating terrain,
painting tiles, stepping a frame and drawing a debug panel respectively. That is not hypothetical: it is
what the code did, and &sect;7.2 records what it cost. <code>DevPanel</code> was the last caller still
doing it by hand &mdash; seven <code>new X(p)</code> lambdas naming seven concrete item classes &mdash;
and it was routed through the factory during this submission sweep.</p>
<p><strong>Why this pattern.</strong> <code>EntityFactory::create(EntityType, sf::Vector2f)</code> is the
single construction point. Callers ask a question &mdash; build me this &mdash; instead of making a
decision. And because construction is funnelled, the factory can insert a step no caller knows about:
<code>applyConfig()</code> looks the new entity's <code>getTypeName()</code> up in
<code>assets/config/entities.json</code> and overrides speed and score value where the file has an
opinion. Balance tuning became a data edit without a single call site changing, which is the payoff a
factory is actually for &mdash; not saving a <code>switch</code>, but owning a policy step.</p>
<p><strong>What it cost.</strong> Two things, both real. Construction is now indirect, so a stack trace
from a broken constructor passes through a function pointer rather than naming the caller. And the
factory returns <code>nullptr</code> for a type it cannot build instead of failing to compile: the
compile-time exhaustiveness a <code>switch</code> over an enum would have given up is gone. That cost is
paid for deliberately &mdash; level files are hand-editable, so a bad name must not crash the game &mdash;
and bought back by a test rather than by the compiler (&sect;7.2).</p>
<p><strong>Participants.</strong> <code>EntityFactory</code> (creator),
<code>EntityCatalogue::Entry::create</code> (the concrete creators),
<code>EntityConfig</code> / <code>EntityConfigEntry</code> (the tuning policy), <code>Entity</code> and
its subclasses (products). This is a <em>Simple Factory with a registry</em>, not GoF Factory Method:
there is no hierarchy of creators, and deliberately so &mdash; a creator subclass per entity type would
have doubled the class count to remove a table.</p>

<h3>7.2 Registry / Type Object</h3>
<p><strong>The problem.</strong> This is the pattern the specification never claimed and the one with
the sharpest evidence, because the bug it fixed was invisible. The same list of entity types was
written out by hand in four places: <code>EntityFactory::create</code> knew 40 types,
<code>SerializationUtils</code> knew 40 names, the <code>MapEditor</code> palette knew 16 &mdash; none of
them enemies &mdash; and <code>PlaceEntityCommand</code>'s <code>if</code>-chain knew 16 name strings and
<em>built nothing</em>. The consequence: the level editor, whose entire purpose is placing entities,
could not place a Goomba, a Koopa, a pipe, a question block or a flagpole. Clicking the button did
nothing and nothing anywhere reported a failure.</p>
<p><strong>The naive alternative.</strong> Fix the four lists and remember to edit four files next time.
That is what was actually tried first, and then guarded with a regression test that compared the factory
switch against the table &mdash; which is a guard on a hazard, not the removal of one.</p>
<p><strong>Why this pattern.</strong> <code>EntityCatalogue::Entry</code> makes the type itself an object:
one row per entity type carrying its <code>EntityType</code> enumerator, its canonical serialised
<code>name</code>, the <code>label</code> the editor shows a human, its palette <code>Category</code>, and
&mdash; the field that turns a description into a registry &mdash; a <code>Creator</code> function pointer
that builds one. The parser, the palette and the factory all read this one table. Adding an entity type is
one row. The factory's 42-case <code>switch</code> is gone, and <code>createUnconfigured</code> is two
lines: look the entry up, call its creator.</p>
<p><strong>What it cost.</strong> The compile-time exhaustiveness of &sect;7.1, bought back as a test:
<code>verify_r21_entity_registry</code> walks every enumerator from <code>0</code> to
<code>EntityType::Count</code> and fails the suite for any that has no row, so a type added to the enum
and forgotten cannot become a palette button that silently places nothing. A function pointer rather than
<code>std::function</code> keeps the storage cost at zero &mdash; every creator is a capture-less lambda
&mdash; and, more usefully, cannot be left empty in a way that would fail only at the call site. The three
types needing more than a position (a moving platform's travel range, two projectiles' launch velocities)
supply it inside their own lambda, which is the honest wrinkle: the uniform creator signature is uniform
because three rows hide a constant inside themselves.</p>
<p><strong>Participants.</strong> <code>EntityCatalogue::Entry</code>, <code>EntityCatalogue::all()</code>,
<code>findByName</code> / <code>findByType</code>, <code>EntityCatalogue::Category</code>;
<code>verify_r21_entity_registry</code> as the mechanism that keeps it total. The data-driven half of the
same idea is <code>EntityConfigEntry</code>, where negative and empty fields mean <em>the file did not
say</em> so a half-filled JSON entry cannot silently zero a value the constructor had set.</p>

<h3>7.3 Singleton</h3>
<p><strong>The problem.</strong> There is exactly one audio device, one texture cache and one event bus
in a process, and the objects that need them are everywhere: an <code>Item</code> deep in the entity
vector wants to publish an event; a state being constructed wants a font. Handing each of them a
reference would mean threading three parameters through every constructor in the entity tree.</p>
<p><strong>The naive alternative.</strong> File-scope globals. Those are initialised before
<code>main()</code> in an order the standard does not define across translation units, which for an SFML
texture cache means loading a texture before the graphics context exists.</p>
<p><strong>Why this pattern.</strong> Meyers singletons &mdash; a function-local <code>static</code>
returned by <code>getInstance()</code> &mdash; are constructed on first use, so the order is defined by
the program's own call sequence and nothing is built before <code>main()</code>. Copy and move are
deleted on each, so the single instance is enforced by the compiler rather than by convention.</p>
<p><strong>What it cost.</strong> This is the trade-off the report is least comfortable with, and it is
stated rather than hidden. There are <strong>twelve</strong> of them, not the four the specification
claims: <code>Game</code>, <code>SoundManager</code>, <code>EventBus</code>, <code>InputManager</code>,
<code>ResourceManager</code>, <code>AchievementManager</code>, <code>ScreenTransitionManager</code>,
<code>StatisticsTracker</code>, <code>DebugConsole</code>, <code>ReplayRecorder</code>,
<code>ParticleSystem</code> and <code>EntityDeathEffect</code>. <code>Game::getInstance()</code> alone
appears at 143 call sites across 39 files. Every one of those is a dependency that does not appear in a
constructor signature and cannot be substituted in a test &mdash; the dependency-inversion cost is
discussed as a decision in &sect;6.8. One of the twelve is a deliberate exception worth naming:
<code>ResourceManager</code>'s instance is a heap allocation that is never deleted, because destruction
order between this translation unit's statics and SFML's is not something the program gets to choose. It
is one fixed-size allocation reported as <em>still reachable</em> rather than lost, and the real run still
frees everything deterministically through <code>Game::shutdown()</code> calling <code>clear()</code>
while the window is alive. The 27-line comment above it exists so a later reader does not "fix" it.</p>

<h3>7.4 State</h3>
<p><strong>The problem.</strong> Two independent instances of the same problem. At screen level: pausing
must draw the level underneath the pause menu, and a state must be able to ask for its own removal.
At player level: Mario's hitbox, jump behaviour and reaction to damage all change with his form.</p>
<p><strong>The naive alternative.</strong> An <code>enum m_screen</code> plus a <code>switch</code> in
<code>handleInput</code>, <code>update</code> and <code>render</code>; and a pile of booleans on
<code>Player</code>. The enum cannot express "paused <em>over</em> playing" at all &mdash; before the
stack existed, <code>render()</code> drew only the top of the stack, so pushing anything hid the game
entirely.</p>
<p><strong>Why this pattern.</strong> <code>IGameState</code> has five pure virtuals
(<code>enter</code>, <code>exit</code>, <code>handleInput</code>, <code>update</code>,
<code>render</code>) and <strong>nine</strong> concrete screens: <code>MenuState</code>,
<code>CharacterSelectState</code>, <code>WorldMapState</code>, <code>PlayingState</code>,
<code>PauseState</code>, <code>OptionsState</code>, <code>GameOverState</code>,
<code>VictoryState</code> and <code>EditorState</code>. Nine, not the ten the specification lists:
there is no <code>StatisticsState</code> class, because the statistics screen is a <em>page</em> of
<code>OptionsState</code> (<code>OptionsState::Page::Statistics</code>, alongside
<code>Settings</code>, <code>Controls</code>, <code>HighScores</code> and <code>Achievements</code>)
rather than a state of its own &mdash; which is the right call, since those five pages share one
navigation model and pushing five states to get five pages would be the pattern applied for its own
sake. <code>GameStateManager</code> holds the nine in a
<code>std::vector</code> rather than a <code>std::stack</code> precisely so <code>render()</code> can
walk <em>down</em> through overlays: <code>isOverlay()</code> returning true means "I do not own the
screen, keep drawing what is beneath me". <code>IPlayerState</code> is the second axis, with five
concrete forms (<code>SmallState</code>, <code>SuperState</code>, <code>FireState</code>,
<code>CapeState</code>, <code>MiniState</code>).</p>
<p><strong>What it cost.</strong> Three costs, and the third is a design criticism of our own code.
First, nine classes and a heap allocation per transition. Second, the transitions had to become
<em>deferred</em>: <code>pushState</code>, <code>popState</code> and <code>changeState</code> queue a
<code>PendingOp</code> applied at a frame boundary, because a state that pops itself would otherwise be
destroyed while its own member function was still on the stack; <code>IPlayerState::isExpired()</code>
exists for the same reason on the player axis, so a timed state reports expiry instead of swapping itself
out from inside <code>update()</code>. Third and most honestly: <strong>four of the five player forms
have empty method bodies</strong>. Only <code>CapeState</code> carries behaviour &mdash; the glide, and
the cape spin. The other four differ by their <code>getSize()</code> return value alone. The State axis
on the player is paid for in five classes and under-used; the behaviour that <em>does</em> vary lives in
the Decorator layer above it (&sect;7.12). It is kept because <code>CapeState</code> proves the axis is
the right shape and because form-specific behaviour has somewhere obvious to go &mdash; but a reader
should know that today it is mostly a size table with a virtual interface on it.</p>
<p><strong>Not State participants.</strong> The specification lists <code>FallingPlatform</code> and
<code>Thwomp</code> as State participants. They are not. Each drives itself through a plain
<code>enum class</code> and switches on it: <code>FallingPlatformState</code> is
<code>Idle</code>/<code>Shaking</code>/<code>Falling</code>/<code>Respawning</code>, and the Thwomp's
phases live in its strategy as <code>ProximityState</code> &mdash;
<code>Idle</code>/<code>Slamming</code>/<code>Resting</code>/<code>Rising</code>. Both are state
<em>machines</em>, which is a technique, not the State pattern, which is a set of polymorphic objects.
Four phases in one class do not earn four classes, and promoting an <code>enum</code> to a hierarchy to
raise a pattern count would be the wrong trade &mdash; naming both here is the more useful answer than
counting them. Turning the Thwomp's bare <code>int</code> into a named <code>enum class</code> did fix a
real defect, though: it had been picking its own sprite from <code>position.y &gt; 140.0f</code> instead
of asking the machine what it was doing.</p>
<p><strong>Participants and figures.</strong> <code>IGameState</code> and its nine screens &mdash;
Figure 10; <code>GameStateManager</code> (context, no figure: it holds states rather than deriving from
anything). <code>IPlayerState</code> and its five forms &mdash; Figure 9.</p>

<h3>7.5 Strategy</h3>
<p><strong>The problem.</strong> A Koopa Troopa and a Koopa Paratroopa are the same enemy with a
different way of moving &mdash; and a stomped Paratroopa is supposed to <em>become</em> a walking Koopa,
at runtime, mid-frame.</p>
<p><strong>The naive alternative.</strong> <code>switch (m_enemyType)</code> inside
<code>Enemy::update</code>, which puts every enemy's movement in one function and makes "the same enemy
but flying" a new subclass. Runtime conversion under that design means destroying one object and
constructing another, at the moment something is holding a pointer to it.</p>
<p><strong>Why this pattern.</strong> <code>Enemy</code> holds a
<code>std::unique_ptr&lt;IMovementStrategy&gt; m_aiStrategy</code> and delegates. There are
<strong>eight</strong> implementations &mdash; <code>PatrolStrategy</code>, <code>ChaseStrategy</code>,
<code>FlyStrategy</code>, <code>LinearStrategy</code>, <code>TimerEmergenceStrategy</code>,
<code>HammerThrowStrategy</code>, <code>TetheredChaseStrategy</code> and
<code>ProximityTriggerStrategy</code> &mdash; and "flying" is a constructor argument:
<code>KoopaParatroopa</code> takes a <code>FlyStrategy</code> and, when stomped, calls
<code>setStrategy(std::make_unique&lt;PatrolStrategy&gt;(true, false))</code>. The object survives; only
its behaviour is replaced. The specification also lists a <code>SwimStrategy</code>: <strong>it does not
exist</strong>, and swimming is descoped &mdash; eight, not nine.</p>
<p><strong>The same pattern on two more axes.</strong> <code>IDifficultyStrategy</code> has three
implementations (<code>EasyDifficulty</code>, <code>NormalDifficulty</code>, <code>HardDifficulty</code>)
answering four questions with different numbers: <code>enemySpeedScale</code>,
<code>startingLives</code>, <code>levelTimeScale</code>, <code>bossHealthScale</code>. Its history is the
argument for it: the difficulty string had been saved, persisted and edited by the options screen since
save/load was written, and <em>read by nothing</em> &mdash; picking Hard changed a word in
<code>config.json</code>. Turning it into a strategy turned a dead string into four consulted numbers
without spreading a difficulty <code>switch</code> across the level loader, the player and the bosses.
And <code>IAIPolicy::decide(const AIObservation&amp;) &rarr; AIAction</code> is the same seam for the CPU
opponent, with <code>AIController</code> sensing and actuating around it. Reported honestly: it has
<strong>one</strong> implementation today, <code>HeuristicPolicy</code>. A one-implementation interface
is not yet a Strategy earning its keep; it is a seam placed where a learned policy would plug in, and
&sect;14 says so rather than counting it as a fourth axis.</p>
<p><strong>What it cost.</strong> Per-enemy state migrated <em>into</em> the strategy objects, and that
leaked. A <code>PiranhaPlant</code>'s pipe mouth and a <code>ChainChomp</code>'s post are anchors held by
the strategy, so when the endless-mode chunk translation moved an enemy, its strategy dragged it straight
back to the old anchor on the next tick. The fix was a new virtual,
<code>IMovementStrategy::translateAnchor(sf::Vector2f)</code>, defaulted to do nothing &mdash; asked of
the strategy rather than cast for by the enemy, because which strategies hold an anchor is the
strategies' own business. That is the recurring cost of delegating behaviour: the interface grows a
method every time the outside world needs to know something the strategy is now hiding.</p>
<p><strong>Participants and figure.</strong> <code>IMovementStrategy</code> (strategy),
<code>Enemy</code> (context), the eight concretes &mdash; Figure 12. <code>IDifficultyStrategy</code> and
<code>IAIPolicy</code> have no generated figure; both are association-held interfaces.</p>

<h3>7.6 Template Method</h3>
<p><strong>The problem.</strong> Two places where a sequence must not be got wrong. Every movement
strategy has to sense, then move, then clamp &mdash; and clamping before moving produces an enemy that
walks through walls for one frame. Every boss has to run its invulnerability frames, phase transitions
and defeat sequence in the right order.</p>
<p><strong>The naive alternative.</strong> Each of the eight strategies writes its own
<code>execute()</code>, and each of the two bosses writes its own <code>update()</code> &mdash; which
means copying <code>Bowser::update</code> into <code>BoomBoom</code> and editing the attack pattern in
place. That is exactly how a second boss gets built without this pattern, and exactly how an i-frame
check quietly diverges between two copies over a few edits.</p>
<p><strong>Why this pattern.</strong> <code>IMovementStrategy::execute(Enemy&amp;, float)</code> is
<em>non-virtual</em> and its whole body is three calls:
<code>calculateTarget</code> &rarr; <code>applyMovement</code> &rarr; <code>checkConstraints</code>.
Only <code>applyMovement</code> is pure virtual; the other two are protected hooks defaulted to nothing,
so <code>LinearStrategy</code> overrides one and <code>TetheredChaseStrategy</code> overrides all three,
and neither can reorder the phases. <code>Boss::update(float dt)</code> is declared <code>final</code>
and calls a pure-virtual <code>updateBehaviour(float)</code>: sealing it is a compiler-enforced "do not
copy this function", so a third boss <em>can only</em> be added by writing its attack pattern.</p>
<p><strong>What it cost.</strong> Rigidity, which is the point and also the price. A strategy that needs
a fourth phase &mdash; something after clamping &mdash; has nowhere to put it and must either abuse
<code>checkConstraints</code> or change the skeleton for all eight. Sealing <code>Boss::update</code>
has the same shape of cost: a boss that genuinely needed to run its behaviour before the i-frame check
cannot, and would force the base class to grow a hook.</p>
<p><strong>Participants and figures.</strong> <code>IMovementStrategy</code> with
<code>calculateTarget</code>/<code>applyMovement</code>/<code>checkConstraints</code> &mdash; Figure 12
shows the hooks and their visibility. <code>Boss</code> &rarr; <code>Bowser</code>,
<code>BoomBoom</code> &mdash; Figure 8.</p>

<h3>7.7 Command &mdash; player input</h3>
<p><strong>The problem.</strong> Keys must be rebindable from the options screen, and a second player
has to trigger the same actions from a different key.</p>
<p><strong>The naive alternative.</strong> <code>if (sf::Keyboard::isKeyPressed(Key::W)) player.jump();</code>
scattered through the update loop. Rebinding then means finding every literal key code in the codebase,
and a second player means duplicating all of them.</p>
<p><strong>Why this pattern.</strong> <code>ICommand</code> declares one operation,
<code>execute(Character&amp;)</code>, and each action is an object: <code>JumpCommand</code>,
<code>MoveLeftCommand</code>, <code>MoveRightCommand</code>, <code>FireCommand</code>,
<code>RunCommand</code>, <code>CrouchCommand</code>, <code>GroundPoundCommand</code>,
<code>WallJumpCommand</code>. <code>InputManager</code> keeps an action table
(<code>m_commandsByAction</code>: <code>"jump"</code>, <code>"fire"</code>, <code>"left"</code> &hellip;)
and per-player key maps, so <code>applyBindings()</code> can move a command to a different key without
knowing how it was constructed, and player 2's map points at the very same command objects.</p>
<p><strong>What it cost.</strong> An indirection between a key press and a jump, so a movement bug is
now read in two files rather than one. And the interface is narrow in the wrong direction: it takes a
whole <code>Character&amp;</code> but carries no arguments, which is why the debug console could not
reuse it and needed a second hierarchy (&sect;7.10).</p>
<p><strong>What this Command is <em>not</em>.</strong> Two claims in the specification are corrected
here. <code>ICommand</code> has <strong>no <code>undo</code></strong> &mdash; input commands are fire
and forget; undo lives in a different hierarchy, &sect;7.9. And it has <strong>no
serialization</strong>: the replay system does not record commands, it records snapshots, which makes
replay a Memento (&sect;7.13), not a Command. Saying so is more useful than the claim, because the
reason is a real engineering constraint &mdash; see &sect;7.13.</p>
<p><strong>Participants and figure.</strong> <code>ICommand</code> (command),
<code>InputManager</code> (invoker), <code>Character</code> (receiver), the eight concretes &mdash;
Figure 11.</p>

<h3>7.8 Composite</h3>
<p><strong>The problem.</strong> One key has to do two things. Pressing jump should perform an ordinary
jump, or a wall jump when the player is against a wall &mdash; and which one applies is decided by the
commands themselves, not by the input layer.</p>
<p><strong>The naive alternative.</strong> Give <code>InputManager</code> a special case: bind jump to
<code>JumpCommand</code> and, at the call site, also invoke <code>WallJumpCommand</code>. The input
layer then knows that two particular commands are related, which is precisely the knowledge the Command
pattern was introduced to remove from it.</p>
<p><strong>Why this pattern.</strong> <code>CompositeCommand</code> derives from <code>ICommand</code>
and holds a <code>std::vector&lt;std::shared_ptr&lt;ICommand&gt;&gt;</code>; its <code>execute</code>
forwards to each child in order. A composite <em>is</em> a command, so the binding table stores it in
the same slot as a leaf and <code>applyBindings()</code> cannot tell the difference. Rebinding jump
rebinds the pair.</p>
<p><strong>What it cost.</strong> <code>ICommand</code> had to be held by
<code>std::shared_ptr</code> rather than <code>std::unique_ptr</code>, because the same leaf command is
referenced by both the action table and a composite. <code>std::shared_ptr&lt;ICommand&gt;</code> in
<code>InputManager</code> is the <em>only</em> shared ownership in the project's own code &mdash; every
other owning pointer is a <code>std::unique_ptr</code> &mdash; so this pattern is the single exception to
the ownership rule &sect;6.8 argues from, and it is named rather than glossed. And
<code>CompositeCommand</code> is declared in <code>InputManager.cpp</code>, so no generated figure can
show it.</p>
<p><strong>Participants.</strong> <code>CompositeCommand</code> (composite), <code>JumpCommand</code> and
<code>WallJumpCommand</code> (leaves), <code>ICommand</code> (component).</p>

<h3>7.9 Command with undo &mdash; the level editor</h3>
<p><strong>The problem.</strong> A level editor without undo is unusable, and this one has to undo two
different kinds of change: tiles in a <code>TileMap</code> and entities in a <em>live</em>
<code>PlayingState</code>'s entity vector.</p>
<p><strong>The naive alternative.</strong> Snapshot the whole level before every edit and restore the
last snapshot on <code>Ctrl+Z</code>. For a 200-tile-wide map with an entity list that is being
simulated, that is a large copy per brush stroke &mdash; and it cannot describe what it is about to
undo, so the editor could never show a history.</p>
<p><strong>Why this pattern.</strong> <code>IEditorCommand</code> declares three operations &mdash;
<code>execute()</code>, <code>undo()</code>, <code>describe()</code> &mdash; and each of the
<strong>seven</strong> concretes stores its own inverse at construction, not the world's state:
<code>PlaceTileCommand</code> and <code>EraseTileCommand</code> remember the
<code>m_oldType</code> they overwrote, <code>FillRectCommand</code> the rectangle it painted over,
<code>MoveEntityCommand</code> the position it came from, <code>SetEntityPropertyCommand</code> the
previous value, and <code>PlaceEntityCommand</code> / <code>EraseEntityCommand</code> the entity
identity. <code>MapEditor</code> then needs nothing clever: two vectors,
<code>m_undoStack</code> and <code>m_redoStack</code>, with <code>undo()</code> moving the top of one to
the other. <code>describe()</code> returns one present-tense line ("Place Goomba") which is what makes
the editor's History panel possible &mdash; a snapshot scheme has nothing to print. Bound to
<code>Ctrl+Z</code> and <code>Ctrl+Shift+Z</code> in <code>EditorState::handleInput</code>, and to the
same two menu items.</p>
<p><strong>Why this is the stronger Command.</strong> The specification claims undo for the input
commands, where there is none. It exists here, and this is where undo is <em>worth</em> having: an editor
is the one part of the program whose user expects to take an action back.
<code>FillRectCommand</code> is the clearest evidence that the granularity is a design decision rather
than an accident &mdash; painting a 20&times;10 region a tile at a time put 200 entries on the stack, so
undoing one rectangle meant pressing <code>Ctrl+Z</code> two hundred times. One command, one rectangle,
one undo. The same reasoning made a drag into a single <code>MoveEntityCommand</code> capturing where the
drag began, rather than one command per frame of mouse movement.</p>
<p><strong>What it cost.</strong> Every editing operation must be expressible as an object with an
inverse, which rules out an edit whose inverse is not knowable &mdash; and it forced a second interface
into existence. <code>PlaceEntityCommand</code> used to push straight into the entity vector and
<code>EraseEntityCommand</code> destroyed whatever it was handed; both bypassed the state that owns the
vector, which is the only thing that knows an entity needs <code>setupAnimations()</code> before it can
draw anything but a coloured placeholder box, and that <code>PlayingState::m_player</code>,
<code>Game::setPlayer</code> and <code>InputManager::registerPlayer</code> hold raw pointers into it.
That is &sect;7.15.</p>
<p><strong>Participants.</strong> <code>IEditorCommand</code> (command), the seven concretes,
<code>MapEditor</code> (invoker and history), <code>TileMap</code> and <code>IEntityAdmitter</code>
(receivers), <code>EditorState</code> (client). No generated figure: <code>IEditorCommand</code> is not
yet one of the diagram tool's roots (&sect;13).</p>

<h3>7.10 Command &mdash; the debug console</h3>
<p><strong>The problem.</strong> A developer types <code>spawn bowser</code> or <code>give star</code>
into an in-game console and expects text back, including an error message when the argument is wrong.</p>
<p><strong>The naive alternative.</strong> One long <code>if/else</code> over the typed verb inside the
console's own render function, with the help text written out separately &mdash; two lists to keep in
step, which is the failure &sect;7.2 already paid for once.</p>
<p><strong>Why a <em>separate</em> interface.</strong> <code>ICommand::execute(Character&amp;)</code>
cannot express this: a console command takes an argument list, acts on the game as a whole rather than on
one character, and <em>returns</em> text. So <code>IConsoleCommand</code> declares
<code>name()</code>, <code>help()</code> and
<code>execute(const std::vector&lt;std::string&gt;&amp;) &rarr; std::string</code>, and there are
<strong>eleven</strong> concretes: <code>help</code>, <code>give</code>, <code>lives</code>,
<code>teleport</code>, <code>god</code>, <code>spawn</code>, <code>difficulty</code>,
<code>level</code>, <code>progress</code>, <code>replay</code>, <code>clear</code>. The
<code>help</code> command is a loop over <code>DebugConsole::commandNames()</code> rather than a
hand-written list, and a command whose arguments are wrong answers by returning its own
<code>help()</code> line &mdash; so a command and its usage text cannot drift apart, which is the same
one-fact-two-places failure &sect;7.2 paid for once already.</p>
<p><strong>What it cost.</strong> A second Command hierarchy rather than one, which is the honest
consequence of <code>ICommand</code>'s signature having been designed for exactly one use. Errors are
returned as text rather than thrown, deliberately: a typo in a console is not exceptional. And every
console command reaches the world through the singletons of &sect;7.3 &mdash; which is what keeps the
console independent of whichever state is on top, and simultaneously the clearest example of the
dependency cost in &sect;6.8.</p>
<p><strong>Participants.</strong> <code>IConsoleCommand</code>, its eleven concretes,
<code>DebugConsole</code> (invoker and registry). Declared inside
<code>DebugConsole.cpp</code>, so no generated figure can reach them.</p>

<h3>7.11 Observer</h3>
<p><strong>The problem.</strong> Collecting a coin has to update the score, play a sound, advance the
statistics counters and possibly unlock an achievement. Defeating a boss has to do a different four
things. Neither the <code>Coin</code> nor the <code>Bowser</code> class has any business knowing what
those systems are.</p>
<p><strong>The naive alternative.</strong> <code>Coin::activate(Player&amp; p)</code> calling
<code>p.addCoins(1); hud.refresh(); soundManager.play("coin"); stats.recordCoin();
achievements.checkCoinMilestones();</code> &mdash; and then <code>Star::activate</code>,
<code>OneUpMushroom::activate</code> and every other item repeating some subset of the same list, because
each item's author has to remember which systems care this time.</p>
<p><strong>Why this pattern.</strong> <code>EventBus::publish(const GameEvent&amp;)</code> takes an
<code>EventType</code> and a <code>std::any</code> payload; subscribers register a
<code>std::function</code> against a type. There are <strong>29</strong> event types today. The real
subscribers, counted from the code rather than from the specification, are
<code>SoundManager</code> (20 subscriptions), <code>PlayingState</code> (15),
<code>AchievementManager</code> (9), <code>Camera</code> (7), <code>StatisticsTracker</code> (4) and
<code>Minimap</code> (1). A seventh listener &mdash; a future daily-challenge coin counter &mdash;
subscribes without <code>Coin.cpp</code> changing at all. The bus also solved a structural problem the
naive version cannot: an entity has no handle on the world's entity list, so
<code>EntitySpawnRequested</code> with an <code>EntitySpawnRequest</code> payload is how Lakitu drops
Spinies and Hammer Bro throws hammers &mdash; the requester never touches the vector.</p>
<p><strong>Correcting the specification.</strong> <code>Hud</code> is listed as a subscriber and
<strong>subscribes to nothing</strong>. It is push-fed instead: <code>PlayingState</code> assembles a
<code>HudData</code> struct and hands it over through <code>Hud::sync</code> once per frame, so the HUD
is a pure renderer of state given to it and holds no reference to a player or a bus at all. That is a
defensible design for something which redraws unconditionally every frame &mdash; a subscription would
buy it nothing &mdash; but it is <em>not</em> Observer and is not counted as one here.
<code>ComboTracker</code> and <code>AchievementTracker</code> appear in the specification's participant
list and <strong>do not exist</strong> as classes; combos are tracked inside <code>PlayingState</code>
and achievements by <code>AchievementManager</code>.</p>
<p><strong>What it cost.</strong> Three costs, all paid. Control flow is untraceable at the call site: a
reader of <code>Coin::activate</code> can see that something was published and cannot see what happens
next, which is a real loss of local reasoning in exchange for a real gain in decoupling. The
<code>std::any</code> payload moves a type error from compile time to run time. And subscriber lifetime
had to be engineered rather than assumed &mdash; a callback that outlives its object is a use-after-free
the next time that event fires. Three mechanisms answer that: cancelled subscriptions are
<em>tombstoned</em> rather than erased, so a handler that unsubscribes during delivery cannot invalidate
the vector being walked; an <code>m_publishDepth</code> counter defers compaction while
<code>publish()</code> is on the stack, including re-entrantly; and
<code>EventBus::ScopedSubscription</code> is a move-only RAII handle that unsubscribes in its destructor,
so a token cannot be forgotten. <code>PlayingState</code> holds fifteen of them, and
<code>SoundManager</code>'s twenty &mdash; which were raw ids that never unsubscribed at all, the last
such holdout in the codebase &mdash; are now a <code>std::vector</code> of them. That third mechanism is
a pattern in its own right, RAII scope-bound resource management, and it is here for a reason worth
stating: counting on thirty-five hand-written unsubscribes across two destructors is counting on a
person, and this bus had already been given an alive-flag guard precisely because someone once did not
write one.</p>
<p><strong>Participants.</strong> <code>EventBus</code> (subject), <code>EventType</code> /
<code>GameEvent</code> (the event), <code>EventBus::Callback</code> subscribers,
<code>EventBus::ScopedSubscription</code> (lifetime). No generated figure: subscribers relate to the bus
by association, not inheritance.</p>

<h3>7.12 Decorator</h3>
<p><strong>The problem.</strong> A Star makes the player invincible for ten seconds; a Mega Mushroom
makes him giant for eight. Both are <em>temporary</em> and both are <em>orthogonal to form</em> &mdash;
Fire Mario with a Star is still Fire Mario, and has to still be Fire Mario when the Star runs out.</p>
<p><strong>The naive alternative.</strong> Add <code>StarState</code> and <code>MegaState</code> to the
five forms of &sect;7.4. Each then has to remember which form to exit back into, and the two together are
a cross-product: starred-and-mega-and-fire is a state nobody wrote. The cruder alternative is worse
still &mdash; booleans on <code>Player</code> (<code>isSuper</code>, <code>isFire</code>,
<code>isCape</code>, <code>isMini</code>, <code>isStarred</code>, <code>isMega</code>) with an
<code>if</code> chain in every place form matters, each chain having to be taught the combination
separately.</p>
<p><strong>Why this pattern.</strong> <code>PlayerStateDecorator</code> derives from
<code>IPlayerState</code> and <em>holds</em> a <code>std::unique_ptr&lt;IPlayerState&gt;</code>, forwarding
<code>enter</code>, <code>exit</code>, <code>handleInput</code>, <code>update</code> and
<code>getSize</code> to whatever it wraps. <code>StarDecorator</code> adds a countdown and invincibility;
<code>MegaDecorator</code> adds a countdown and <em>scales the wrapped state's</em>
<code>getSize()</code> rather than returning a constant, which is what makes "giant Fire Mario" fall out
instead of being designed. When a decorator expires, <code>releaseWrappedState()</code> hands the inner
state back and the player is exactly what he was. The combination was never a special case: it is a
consequence of the two axes being orthogonal.</p>
<p><strong>What it cost.</strong> The chain is only inspectable by <code>dynamic_cast</code>, and
there are <strong>seventeen</strong> such sites across <code>Player</code>, <code>Serializer</code>,
<code>CollisionDetector</code> and <code>DevPanel</code>. The sharpest is
<code>Player::setBaseState</code>, which must
<em>hand-walk</em> the chain: cast the current state to <code>PlayerStateDecorator*</code>, loop inward
through <code>getWrappedState()</code> until the cast fails, then swap the base form underneath the
innermost decorator &mdash; which is what lets a Fire Flower picked up during a Star survive the Star
expiring. That is a genuine cost of the pattern, not a bug: a decorator chain deliberately hides its
depth, so any code that needs to reach <em>through</em> it has to ask at run time. An alternative would be
a <code>getInnermost()</code> virtual on <code>IPlayerState</code> &mdash; which widens the interface every
implementer must satisfy in order to serve two call sites, and was rejected on that basis.</p>
<p><strong>Honest scope note.</strong> <code>StarDecorator</code> is exercised in normal play: four
shipped question blocks carry <code>QuestionBlock::Content::Star</code>, one each in
<code>level_1</code>, <code>level_2</code>, <code>level_3</code> and <code>bonus_1</code>.
<code>MegaDecorator</code> is <em>not</em>: no shipped level places a Mega Mushroom, so it is reachable
only through the debug console's <code>give</code> command or the level editor. The same is true of
<code>MiniState</code> from &sect;7.4 &mdash; no level's question block carries
<code>MiniMushroom</code> either. All three are production code paths rather than harness-only ones, but
a grader who only plays the campaign will meet the Star and neither of the other two, and the honest
place to fix that is a level edit rather than a paragraph.</p>
<p><strong>Participants and figure.</strong> <code>IPlayerState</code> (component),
<code>SmallState</code>&hellip;<code>MiniState</code> (concrete components),
<code>PlayerStateDecorator</code> (decorator), <code>StarDecorator</code> and <code>MegaDecorator</code>
(concrete decorators) &mdash; Figure 9, which shows the wrapping member alongside the inheritance edge.</p>

<h3>7.13 Memento</h3>
<p><strong>The problem.</strong> Holding <code>R</code> rewinds the last five seconds of play. Attract
mode replays a recorded run on the menu screen. Both need the world's past.</p>
<p><strong>The naive alternative.</strong> Record the player's inputs and re-simulate. That is the usual
way and it was rejected for a stated reason: it requires the simulation to be bit-for-bit deterministic,
and this one is not &mdash; float physics, an entity list that spawns and prunes, and enemy strategies
that read a shared <code>Game</code> singleton. Storing state costs more disk and plays back exactly what
happened.</p>
<p><strong>Why this pattern.</strong> <code>GameSnapshot</code> is the memento:
<code>PlayerSnapshot</code> for each player, a <code>std::vector&lt;EntitySnapshot&gt;</code>, the level
timer and the camera centre. <code>PlayingState</code> is the originator, building one per frame;
<code>TimeRewindManager</code> is the caretaker, holding a <code>std::deque</code> capped at 300 frames
&mdash; five seconds at 60&nbsp;fps. <code>ReplayRecorder</code> reuses the <em>same</em> memento rather
than inventing a second mechanism: a replay is that snapshot stream kept for longer, thinned to every
Nth frame, and written to disk. The load-bearing detail is that <code>EntitySnapshot</code> keys on
<code>Entity::getId()</code> and <strong>not</strong> on a position in the entity vector, because indices
shift whenever an entity is pruned or spawned between record and restore &mdash; which is a defect that
actually shipped: rewind assigned positions to the wrong entities.</p>
<p><strong>What it cost.</strong> Two things, stated plainly. First, this is a <em>partial</em> snapshot,
not "full game state": per entity it captures id, position, velocity and active flag, and per player
position, velocity, score, coins and lives. Anything else &mdash; an enemy's strategy phase, a block's
broken state, a decorator's remaining time &mdash; is not rewound. That is a deliberate trade for a
per-frame capture cost, and the report should not be read as claiming more. Second,
<code>GameSnapshot</code> is a fully public <code>struct</code>, so the originator's internals are
readable by anything that can see the header &mdash; the narrow-interface half of the GoF pattern is
absent. That is the common C++ simplification (a memento as an aggregate) and it is a real encapsulation
cost; making the fields private with <code>PlayingState</code> and <code>Player</code> as friends would
close it and add to the friend count &sect;6.8 already argues about. Even the partial capture has been
wrong once in a way worth recording: it captured player 1 only, so rewinding a two-player match rolled
one player's score back and left the other's alone &mdash; silent, because nothing on screen looks wrong
until you read the numbers. <code>hasSecondPlayer</code> and <code>secondPlayerState</code> exist because
of it.</p>
<p><strong>Participants.</strong> <code>GameSnapshot</code>, <code>PlayerSnapshot</code>,
<code>EntitySnapshot</code> (mementos), <code>PlayingState</code> (originator),
<code>TimeRewindManager</code> and <code>ReplayRecorder</code> (caretakers). No generated figure: these
are aggregates and holders, with no inheritance among them.</p>

<h3>7.14 Object Pool</h3>
<p><strong>The problem.</strong> Fireballs, hammers and boss fireballs are created and destroyed several
times a second during a fight, each one a heap allocation inside the frame budget.</p>
<p><strong>The naive alternative.</strong> The textbook pool owns its objects and hands out raw
pointers. That does not fit this game at all: <code>PlayingState</code> owns the world as
<code>std::vector&lt;std::unique_ptr&lt;Entity&gt;&gt;</code> and prunes inactive entries every frame, so
a pool that kept ownership would mean rewriting how every entity in the game is stored.</p>
<p><strong>Why this pattern, shaped this way.</strong> <code>ObjectPool&lt;T&gt;</code> trades in
<code>std::unique_ptr&lt;T&gt;</code>: <code>acquire()</code> hands one over, <code>release()</code> takes
it back, and in between the object is owned exactly the way an unpooled one would be. The entity list does
not know pooling exists; the only change at the call site is that the prune step offers spent objects back
instead of dropping them. <code>acquire()</code> forwards its arguments to <code>T</code>'s constructor on
a miss and to <code>T::resetForPool</code> on a hit, so both paths leave the caller holding the object it
asked for &mdash; and only <code>T</code> knows what "just-constructed again" means, which is why that
requirement is on <code>T</code> rather than in the pool.</p>
<p><strong>What it cost.</strong> A pool that grows without bound is a leak that never frees, so
<code>m_maxRetained</code> caps the free list and <code>release()</code> simply lets the object die past
the cap. The larger cost is structural and shows how a pattern leaks: recycling means the <em>owner</em>
has to know which pool an expiring entity belongs to, even though pooling was supposed to be invisible
to the entity list. <code>PlayingState::recycleEntity</code> answered that with three sequential
<code>dynamic_cast</code>s &mdash; exactly the run-time type test &sect;6.8 argues against elsewhere,
and indefensible rather than a trade-off, so it was fixed rather than written up. <code>Entity</code>
now carries a virtual <code>poolTag()</code> returning an <code>Entity::PoolTag</code>, which
<code>Fireball</code>, <code>Hammer</code> and <code>BossFireball</code> override; recycling switches on
the answer. The residual cost is honest and small: <code>Entity</code>, the root of the whole hierarchy,
now knows that pooling exists, which it did not before. That is a real widening of the base class in
exchange for removing three RTTI walks per expiring projectile, and it is the trade this subsection
would defend if asked.</p>
<p><strong>Honest scope.</strong> Three types are pooled: <code>ObjectPool&lt;Fireball&gt;</code>,
<code>ObjectPool&lt;Hammer&gt;</code> and <code>ObjectPool&lt;BossFireball&gt;</code>. The specification
also claims particles and Bullet Bills. Neither is true: the particle system does its own recycling over
a flag array, and no <code>ObjectPool&lt;BulletBill&gt;</code> exists. Three, not five.</p>
<p><strong>Participants.</strong> <code>ObjectPool&lt;T&gt;</code> (pool), the three pooled projectile
classes with their <code>resetForPool</code>, <code>PlayingState</code> (client, via
<code>spawnProjectile</code> and <code>recycleEntity</code>). No generated figure: the pool is a template
held by composition.</p>

<h3>7.15 Adapter</h3>
<p><strong>The problem.</strong> An editor command has to add an entity to, or remove one from, a
<em>live</em> game state &mdash; but it must not depend on <code>PlayingState</code>, and there are two
facts it cannot know: that an entity which has not had <code>setupAnimations()</code> called on it renders
as a flat coloured placeholder box, and that three separate places hold raw non-owning pointers into the
entity vector.</p>
<p><strong>The naive alternative.</strong> What the code did: <code>PlaceEntityCommand</code> pushed
straight into the vector, so every entity the editor placed drew as a coloured box; and
<code>EraseEntityCommand</code> destroyed whatever it was handed, so erasing a Player left
<code>PlayingState::m_player</code>, <code>Game::setPlayer</code> and
<code>InputManager::registerPlayer</code> all dangling.</p>
<p><strong>Why this pattern.</strong> <code>IEntityAdmitter</code> is a two-method port &mdash;
<code>admit(Entity*)</code> and <code>release(Entity*)</code>, neither taking ownership &mdash; with a
contract stated in the header: <code>admit</code> must leave the entity fully drawable,
<code>release</code> must have dropped every observer pointer into it before it returns.
<code>PlayingState::EditorBridge</code> implements it and is the adapter: it presents the state's own
knowledge through an interface the editor can depend on without depending on the state.</p>
<p><strong>What it cost.</strong> One more interface and one more indirection for what is, in the
common case, a <code>push_back</code>. It buys back two shipped defects and, more importantly, it makes
the requirement <em>writable</em> &mdash; the invariant now lives in a header contract rather than in a
maintainer's memory.</p>
<p><strong>Participants.</strong> <code>IEntityAdmitter</code> (target),
<code>PlayingState::EditorBridge</code> (adapter), <code>PlayingState</code> (adaptee),
<code>PlaceEntityCommand</code> / <code>EraseEntityCommand</code> (clients). No generated figure: the
implementer is a nested class, which the diagram tool does not index.</p>

<h3>7.16 Two patterns deliberately not used</h3>
<p>These belong in a design section as much as the fifteen subsections above, because each was the
obvious answer to a real problem in this codebase and each lost for a reason that can be stated.</p>
<p><strong>Visitor, and double dispatch generally.</strong> <code>CollisionResolver</code> must decide what
happens when two entities touch, which is a two-argument dispatch C++ does not give you. The textbook
answers are Visitor &mdash; an <code>accept</code>/<code>visit</code> pair on <code>Entity</code> &mdash;
or a chain of <code>dynamic_cast</code>s. The chain is what the code had: up to twelve sequential casts per
colliding pair per frame. Visitor was rejected too, on two grounds. It would put a
<code>visitPlayer</code>/<code>visitEnemy</code>/<code>visitItem</code>/<code>visitBlock</code>/<code>visitProjectile</code>
surface on a base class that has no other reason to know those categories exist, and it makes the
<em>entity</em> hierarchy closed instead of the operation: adding a new collision <em>rule</em> would be
cheap, but the resolver is stable and the entity list is not, so Visitor optimises the axis that does not
change here. What the code does instead is ask each side once: <code>Entity::getCategory()</code> returns
an <code>EntityCategory</code>, and the resolver switches on the <em>ordered pair</em> of the two
categories, halving the case count because <code>(Enemy, Player)</code> is handled as
<code>(Player, Enemy)</code> with the collision normal flipped. Two virtual calls and one switch replaced
twelve casts. The enum is deliberately a <em>category</em> and not a type id &mdash; it answers "how does
this collide?", which is the only question the resolver asks; anything needing the concrete type is told
to add a virtual of its own instead of widening the enum. The result is measurable: the resolver contains
<strong>zero</strong> real <code>dynamic_cast</code>, and the four textual matches in that file are
comments recording this decision.</p>
<p><strong>Flyweight.</strong> It would be easy, and wrong, to call
<code>ResourceManager</code> and <code>SpriteSheet</code> a Flyweight and claim one more pattern.
Flyweight splits an object's intrinsic state from extrinsic state passed in at use, so that many
fine-grained objects can share one instance. What these two do is simpler and different: they are a
<strong>cache</strong>. <code>ResourceManager</code> holds
<code>unordered_map</code>s of textures, fonts and sound buffers keyed by path and hands out references;
<code>SpriteSheet</code> holds a non-owning <code>const sf::Texture*</code> into that cache plus a map of
named frame rectangles. Sharing a texture between a hundred Goombas is what any sane resource loader does;
there is no intrinsic/extrinsic split and no flyweight object. Calling it a cache is the accurate
description, and one fewer claimed pattern is a better report than one more false one.</p>
<p>Also confirmed absent, checked rather than assumed: Prototype, Builder, Null Object, Facade, Chain of
Responsibility, a custom Iterator, Abstract Factory and Bridge. None appears in the codebase, and none is
claimed.</p>

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
<p><code>PhysicsEngine::update</code> runs ten ordered steps &mdash; nine numbered stages in the code,
with the broad and narrow phases sharing one stage and split into two rows below for readability &mdash;
and the order is load-bearing at almost every step.</p>
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
<p>Sprites come from JSON-described atlases (<code>player.json</code> alone has 236 frames &mdash; 59 per playable character);
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

<h3>8.12 Procedural generation, solvability and Endless Mode</h3>
<p><code>MapGenerator</code> builds a level from a seed: multi-tier elevation, biome-specific ceilings,
lava pits, reachability-guarded platform gaps and a threat-pacing curve. It drives three player-facing
modes &mdash; the date-seeded Daily Challenge (deterministic for every player on the same day), a
"Generate &amp; Play/Edit" page, and Endless Mode.</p>
<p><strong>Endless Mode</strong> is a true infinite runner, not a wide level. The tilemap starts as one
generated chunk; <code>PlayingState::extendEndlessLevelIfNeeded()</code>, called once per frame, appends
a fresh 100-tile chunk whenever the player nears the current edge. Each chunk is generated into an
<em>isolated</em> tilemap and entity list &mdash; the generator unconditionally clears whatever entity
vector it is handed, which would otherwise delete the live player &mdash; then spliced in at an x-offset
with a flat safety bridge across the seam. Pit probability, enemy rate, terrain roughness and the
difficulty tier all rise with the chunk index; no flagpole ever exists, and distance travelled is the
score, recorded distinctly from the other modes on the Game Over screen.</p>
<p><strong>Solvability.</strong> Every generated level and every Endless chunk is checked by
<code>LevelSolvability</code>, an independent column-reachability BFS bounded by the game's own jump and
run constants from <code>Constants.hpp</code> (a moving or falling platform counts as standable ground
at its column, so a platform-bridged pit is not a false rejection).
<code>MapGenerator::generateSolvable()</code> retries with a new seed on failure, bounded; if every
attempt fails it keeps the last layout and logs the failure rather than blocking play &mdash; a check
with retries, not a shipping gate (&sect;13). The idea was salvaged from an abandoned GAN/RL side branch
whose quality gate outlived its framework; the shipped implementation is a dependency-free rewrite,
stress-tested by a CTest case across every theme and difficulty the menu can produce.</p>

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
running the game for six seconds &mdash; not by any of the test harnesses.</blockquote>

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
{img('01_menu.png', 'Main menu', 'Figure 1 &mdash; The main menu, over the parallax backdrop. Eight entries including the four multiplayer modes, the map editor and the procedural generator page.')}
{img('02_world_1_3.png', 'World 1-3', 'Figure 2 &mdash; World 1-3, Bowser&#39;s Castle. The HUD reads WORLD 1-3, taken from the level catalogue rather than a constant. The castle towers and fencing of the parallax backdrop stand on the floor the player walks on, at the correct depth tint for the castle theme.')}
{img('03_level_end.png', 'End of 1-3', 'Figure 3 &mdash; The rebuilt end of 1-3, seen through the editor&#39;s free camera. Left to right: lava spanned by the brick bridge Bowser paces, the axe that drops it, the goal flagpole standing on the ground, and the castle drawn from the atlas&#39;s castle_end art. The editor&#39;s tile palette is open on the right.')}
{img('04_versus_cpu.png', 'Versus CPU', 'Figure 4 &mdash; Versus CPU. Both players have an icon and a life count in the HUD; the bottom strip carries the score line and who is leading. The dev panel reports the opponent&#39;s policy and what it currently believes it is doing (&quot;crossing gap&quot;) &mdash; the CPU drives a real Player through the same commands a human uses.')}
{img('05_map_editor.png', 'Map editor', 'Figure 5 &mdash; The in-game level editor (F1). The entity palette is generated from the single catalogue and grouped into Players, Enemies, Items, Blocks and Scenery, with a filter box and the serialised name on hover. Edits go through Command objects, so undo and redo come for free.')}
{img('11_main_features.png', 'Main features contact sheet', 'Figure 6 &mdash; A contact sheet of the core screens (Main Menu, Character Select, World Map, a level in play, Pause) and headline gameplay (the Small/Super/Fire/Cape power-up chain plus Mini and the Star and Mega decorators, and three of the roster&#39;s enemies), captured with a scripted <code>--script</code> input driver. The power-up and enemy cells are one continuous scripted walk through a small custom level (<code>assets/levels/custom/report_showcase.json</code>, authored directly as JSON rather than through the in-game editor or the debug console&#39;s <code>spawn</code> command, since a scripted run&#39;s synthetic input never reaches ImGui and so cannot drive either one) that carries the player through the mushroom, cape feather, fire flower, mini mushroom, star and mega mushroom in a single pass. This sheet covers the catalogue&#39;s main features rather than every family (every enemy, item and block type, every mode) to fit the time remaining in this pass; it is additional to Figures 1&ndash;5 above, not a replacement.')}

<h2 id="process">12 &middot; Process and project history</h2>
<p>The project ran from 29 May to 2 September 2026 across {F['weeklies']} weekly reporting periods
(<code>docs/Group52_04/</code> through <code>Group52_13/</code>), {F['commits']} commits and
{F['sessions']} recorded working sessions in <code>logs/agent_history.log</code>. This section is
about the third of those numbers rather than the first two: <strong>how the project checked itself,
what the checking produced, and why its engineering rules read the way they do</strong>. The
individual defects are deliberately not restated here &mdash; the five worth reading are worked
through in &sect;10, and the full ledgers live in the audit documents named below.</p>

<h3>12.1 Timeline</h3>
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
<tr><td><strong>W11</strong></td><td>16 &ndash; 22 Aug</td>
 <td>The first checkpoint audit and its remediation, the first green CI, the boss fights, multiplayer
 and the CPU opponent, and the Windows crash root-cause in &sect;10.1. The heaviest week of the
 project.</td></tr>
<tr><td><strong>W12</strong></td><td>23 &ndash; 29 Aug</td>
 <td>One commit, on every branch. Reported as a quiet week rather than padded with the adjacent
 weeks' work.</td></tr>
<tr><td><strong>W13</strong></td><td>30 Aug &ndash; 5 Sep<br><em>reported to 2 Sep</em></td>
 <td>The specification audit, its remediation batches, and the submission sweep: pipe entry
 modes, save-slot selection, the lighting renderer, the level editor's custom levels, the entity
 registry, and the documents this report is part of.</td></tr>
</tbody></table></div>

<h3>12.2 The procedure: checkpoint audits, repeated</h3>
<p>The project's verification is not one final review. <strong>Four times, work stopped and the tree
was audited against something outside itself</strong> &mdash; the specification, the reports' own
claims, or a reader who had written none of it. Each audit produced a numbered defect ledger; each
ledger was worked off in batches; each batch left a test behind.</p>
<p><strong>18 August &mdash; code audit.</strong> A full read of both members' domains against the
code reachable from <code>main()</code>. It produced <strong>37 findings, 7 of them critical</strong>
(GitHub issue #11), and it is the hinge of the project: three of the rules in
<code>AGENTS.md</code> exist because of what it found, not because of what it fixed
(&sect;12.4).</p>
<p><strong>31 August &mdash; specification and claim audit.</strong> Three parallel read-only passes
reconciled <code>SPEC.md</code> v2.0's frozen 110-feature specification against the code, and
&mdash; the part that matters for a report &mdash; against <em>every claim the project's own
documents made</em>: the features list, this report's prose, both task checklists. Twenty-eight
defects were raised. On re-verification <strong>four were struck</strong>: one refuted by live
evidence in the same session that recorded it, one describing an animation that does not exist in
the codebase, one already fixed twelve days earlier, and one whose stated mechanism is absent from
the code (&sect;13). The audit recorded its own methodological failure &mdash; observations had been
folded in without re-verification against the commit being audited &mdash; and made
re-verification-before-recording a standing requirement.</p>
<p><strong>2 September &mdash; submission sweep.</strong> The audit this report was written inside.
It re-derived the tree's facts from the tree, rebuilt every stale document, and ran five defect
lanes in isolated worktrees off one frozen baseline. Its own execution record is
<code>docs/issues/submission_sweep_plan_2026-09-02.md</code> &sect;8, written as the work happened
rather than afterwards.</p>
<p><strong>The sweep's own per-lane review round.</strong> The fourth audit is the one inside the
third. No lane's report was accepted on its word: <strong>the reviewer re-ran the lane's own
measurement</strong> &mdash; its <code>ctest</code> run, its commit range count, its build &mdash;
before accepting the branch. Several lanes were sent back for rework, one of them three times over
counts it had typed but could not measure. Several others came back having <em>corrected the plan
that briefed them</em>: one lane was told to archive a document that turned out to be cited by
section number from seven live engine sources, and refused; another was told to fix a defect that
did not exist, and proved it (&sect;12.4). That is the outcome a review round exists to
produce.</p>
<p><strong>Remediation in numbered batches.</strong> Every defect got a number and its own small
project: a branch, a fix, a guard that fails without the fix, and a log entry recording what was
actually run. The two audits' remediation ran as numbered batches <strong>R1 through R21</strong>
(one of which, R18, was cancelled after the defect it targeted was struck). The numbering is the
useful part &mdash; a defect that is cited by number in a commit
message, a test name (<code>verify_r21_flagpole_softlock</code>), a code comment and a log entry
cannot quietly become "probably fixed".</p>
<p><strong>A guard does not count until it has been watched failing.</strong> Every new test is
mutation-tested: the fix is reverted, the guard is observed to <em>fail</em>, the fix is restored,
and the guard is observed to pass again. This catches the failure mode a green suite cannot &mdash;
a test that passes for the wrong reason. The clearest instance in the sweep: a guard for a
<code>Spiny</code> that must hatch from an egg before it walks. The obvious mutation, reverting the
constructor default, would have left the guard <strong>passing vacuously</strong>, because a Spiny
that was never an egg satisfies "ends up walking" from its first frame. The lane recognised that and
mutated the hatch trigger itself instead. Honesty runs the other way too: of one lane's seven
mutation attempts, <strong>one survived</strong> &mdash; the estimate it targeted already floored to
the same value &mdash; and it is recorded as a surviving mutation in the log rather than quietly
replaced with a luckier one.</p>
<p><strong>"Complete" means reachable from <code>main()</code> and observed running.</strong> Not
that a file exists, compiles, and has a passing harness. This is the project's most expensive lesson
priced in advance: the 18 August audit found <strong>six subsystems that were "complete" and
inert</strong> &mdash; compiled, harnessed, green, and constructed by nothing the game ever
reached. Every claim in &sect;4 of this report is graded against the stronger definition, and where
only the weaker one holds, &sect;13 says so.</p>
<p><strong>The log is the audit trail.</strong> <code>logs/agent_history.log</code> holds
{F['sessions']} entries, and it is not a changelog. Each records the commit before and after,
whether the remotes were fetched, whether the work is reachable from <code>main()</code>, how it was
verified, and &mdash; required, not optional &mdash; the mistakes made getting there. It is
append-only and <strong>union-merged</strong>: when two branches both appended, the resolution is to
keep both entries, never to pick a side, because a deletion destroys evidence while a duplicate
merely wastes a line. Several passages of this report, including &sect;12.4, are drawn from it.</p>
<p><strong>CI is the part that does not depend on anyone remembering.</strong> GitHub Actions builds
the game and runs the registered test suite on every push to the integration branches
(<code>.github/workflows/ci.yml</code>), inside a virtual framebuffer and against a dummy audio
device so that a harness needing a display or a sound card still runs on a headless runner. A rule
only a human applies is not enforced; CI is what makes "reachable and observed" checkable by
something other than good intentions.</p>

<h3>12.3 What the audits produced</h3>
<div class="tbl"><table>
<thead><tr><th>Mechanism</th><th>What it produced</th><th>Standing effect</th></tr></thead>
<tbody>
<tr><td><strong>Code audit</strong><br>18 Aug</td>
 <td>37 findings, 7 critical. Six subsystems complete and inert. About 3,100 lines of finished work
 sitting uncommitted. The audit's own first revision retracted.</td>
 <td>Three <code>AGENTS.md</code> rules, each naming its incident. The first green CI.</td></tr>
<tr><td><strong>Specification and claim audit</strong><br>31 Aug</td>
 <td>110 spec features reconciled against code and against every document claim. 28 defects raised,
 4 struck on re-verification.</td>
 <td>Batches R1&ndash;R20. An observation must be re-verified against the audited commit before it
 is recorded.</td></tr>
<tr><td><strong>Submission sweep</strong><br>2 Sep</td>
 <td>Five defect lanes off one frozen baseline; every stale document rebuilt from the tree; one
 claimed defect withdrawn; a defect found in this report's own fact extractor.</td>
 <td>Counts in the generated documents come from the tree, never from prose. The sweep's execution
 record is itself a document.</td></tr>
<tr><td><strong>Per-lane review round</strong><br>within the sweep</td>
 <td>Every lane's measurement re-run rather than trusted. Several lanes reworked; several came back
 having corrected the plan that briefed them.</td>
 <td>A lane with no way to measure a number will still write one down. Review re-measures.</td></tr>
<tr><td><strong>Mutation testing</strong></td>
 <td>Every guard observed failing with its fix reverted. One vacuous guard caught before it shipped;
 one surviving mutation recorded rather than dropped.</td>
 <td>A green test is evidence only once it has been seen to go red.</td></tr>
<tr><td><strong>Reachable-and-observed rule</strong></td>
 <td>Six inert subsystems wired in one session; the menu music, silently failing for weeks, found by
 running the game for six seconds.</td>
 <td>&sect;4 grades every capability by how it was confirmed; &sect;13 lists what only compiles.</td></tr>
<tr><td><strong>The log</strong></td>
 <td>{F['sessions']} entries carrying fingerprints, fetch status, reachability, verification and
 mistakes.</td>
 <td>Union on merge conflict &mdash; keep both entries, never choose a side.</td></tr>
<tr><td><strong>CI</strong></td>
 <td>Build plus the registered suite on every push to the integration branches.</td>
 <td>The rules stop depending on whoever is at the keyboard.</td></tr>
</tbody></table></div>

<h3>12.4 The rules came out of the failures</h3>
<p>This is the part of the process worth defending, because none of the project's engineering rules
were adopted on principle. Each is a scar, and <code>AGENTS.md</code> records the incident beside
the rule so a later reader can judge whether it still applies.</p>
<blockquote><strong>A <code>git reset --hard &amp;&amp; git clean -fd</code> destroyed a week's
progress report.</strong> It was run to unblock a git operation. The Week 8 report in
<code>docs/Group52_08/</code> was uncommitted, and it did not come back. A later session then found
about 3,100 lines of finished work &mdash; three sub-level maps, the tools directory, a week's
report &mdash; one careless clean from the same fate. <em>Rule:</em> never discard uncommitted work
to unblock a git operation; commit it, because committing is reversible and discarding is not. Only
<code>imgui.ini</code>, which the UI regenerates, is exempt.</blockquote>
<blockquote><strong>An audit was written against a stale local branch and had to be retracted in
public.</strong> The 18 August audit's first revision was written against a local <code>dev</code>
nine commits behind, and on that basis declared the entire audio system missing. The audio system
was implemented and merged on <code>origin/dev</code>. The finding was withdrawn on issue #11.
<em>Rule:</em> <code>git fetch --all</code> before any task whose output describes repository state,
and record in the log that it was done. Every audit and weekly report in this project since carries
that line.</blockquote>
<blockquote><strong>The same defect kept arriving in different clothes.</strong> Three of the five
problems in &sect;10 are one failure: <strong>a fact stored in two places, drifting apart, with
nothing comparing them.</strong> The entity list in the factory, the parser and the editor palette.
The tile name-to-enum mapping duplicated in the loader, which silently dropped every coin tile in
all seven level files. The flagpole's collision height against its sprite height. None produced a
compiler error; all produced wrong behaviour that looked like a design choice. <em>Rule:</em> the
commit that creates the second copy also creates the test that fails when the copies disagree. The
entity catalogue's registry walk (&sect;8) is the model.</blockquote>
<p>The last of those rules caught this report. The document you are reading is generated by a script
that counts what it claims, and the counting has already found two wrong figures before they were
printed &mdash; a class count inflated by a substring match, and "23 test files" reported as if it
meant "23 things CI runs". The sweep found a third: the extractor that counts registered test cases
could not see the guards registered through a second CMake mechanism, so the report was
<em>understating</em> its own verification. A number that is computed can still be computed wrongly;
what a parity check buys is that the error surfaces as a failure rather than as prose.</p>
<h4>The audit corrects itself</h4>
<p>The strongest evidence that this procedure works is not a defect it fixed but a defect it
<strong>withdrew</strong>. The submission sweep's own plan asserted that the third axe in the Hard
Bowser fight stood behind the arena enclosure, unreachable by ordinary movement, making that fight
unwinnable &mdash; a critical, plausible, level-data defect with an obvious remedy. Told to fix it,
the lane instead <strong>tried to reproduce it</strong>: it confirmed in code that the Hard
difficulty really does require all three axes, then drove a real <code>Player</code> through real
<code>PlayingState::update()</code> frames and reached the axe cleanly on the unmodified level file.
It also tried the remedy the plan proposed &mdash; removing the enclosure's solid tile &mdash; and
found it changed nothing. <strong>No level data was edited.</strong> The claim was withdrawn, and a
mutation-tested reachability guard was left in its place, one that fails if the axe is ever moved
outside the arena's boundary. A procedure that only ever confirms its own findings is not an audit;
this one is allowed to say it was wrong, and did.</p>

<h2 id="verify">13 &middot; Verification, CI and known gaps</h2>
<p>The tree holds {F['harnesses']} verification harnesses; {F['targets']} of them are built by CMake,
and the {F['ctests']} registered as CTest cases are run by GitHub Actions on every push to the
integration branches. Not every harness is registered: those that open a window cannot run on a
headless runner, so they are compiled but not executed there &mdash; "number of test files" and
"number of things CI runs" are different figures, and reporting the first as the second would
overstate what is verified automatically. Alongside the harnesses, CMake registers the standalone
hygiene guards <code>guard_saves_hermeticity</code> and <code>guard_asset_single_source</code>,
which check the repository rather than the gameplay. Cases are added whenever a defect escapes to an
audit, so the suite is a record of every bug the project has shipped rather than a plan someone
wrote in advance.</p>
<p>The suite is hermetic (R11, <code>docs/issues/spec_feature_audit_2026-08-31.md</code>): every
harness's <code>main()</code> opens a <code>TestSaveSandbox</code>
(<code>tests/TestSaveSandbox.hpp</code>) that points <code>Serializer::setSaveDirectory()</code> at
a throwaway per-process temp directory before any other code runs, so nothing under test can reach
the developer's real <code>saves/</code>. That the seam exists is not proof every harness uses it
correctly, so <code>guard_saves_hermeticity</code>
(<code>tests/guard_saves_hermeticity.cpp</code>) checks it empirically: a CTest fixture snapshots
the content of the real <code>saves/</code> directory before any <code>verify_*</code> case runs and
re-snapshots it after every one has finished, failing the run if a single byte differs or a file was
added or removed. Verified by mutation, not merely by a green run: disabling one harness's sandbox
line reproduced the original incident &mdash; the guard caught both a rewritten
<code>config.json</code> and a newly created <code>profile.json</code> &mdash; and re-enabling it
turned the guard green again on the next run.</p>

<h3>13.1 Known gaps</h3>
<p>What follows is the honest state of the project on the day it was submitted, grouped by the kind
of gap rather than by severity. It is written to the project's own definition of complete
(&sect;12.2): reasoning about code is not observation of it, so anything verified only by reasoning
is listed here even where the reasoning is sound.</p>

<h4>Claims this project cannot verify with the environments it has</h4>
<ul>
<li><strong>No 3D renderer.</strong> The rubric's 5-point line is forfeited, deliberately and with
the trade-off argued in &sect;4.</li>
<li><strong>The Windows platform claim is unconfirmed, and will stay that way.</strong> CI runs on
<code>ubuntu-latest</code> only (<code>.github/workflows/ci.yml</code>), and no session in this
project's history has had a Windows or MSVC environment. The crash's <em>fix</em> is independently
verified: the undefined-behaviour analysis in
<code>docs/learning/mid-frame-entity-spawn-crash.html</code> plus scripted runs that hold World 1-3
through Bowser's fireballs without a crash on macOS. What is unconfirmed is specifically the claim
that a Windows build was produced and run &mdash; a log entry asserting one was struck as
unsubstantiated during the 31 August audit, because it modified no source and no build
configuration and there has never been Windows CI that could have produced it. The honest status is
the earlier entry's: not confirmed on Windows.</li>
</ul>

<h4>A defect neither fixed nor dismissed</h4>
<ul>
<li><strong>The death-sound complaint is not demonstrable as worded.</strong> The playtest
observation was that the death SFX is cut off by the respawn transition. Investigated:
<code>lost_life.wav</code> runs 3.267&nbsp;s, longer than any fall-plus-respawn window; no code
stops it; the channel pool only reuses channels that have already stopped; and it plays on over the
resumed BGM. So the stated mechanism is absent from the code, while the observation itself came from
someone who was listening. It is recorded as <em>neither fixed nor invalid</em>, and it needs
someone with working audio to characterise what was actually heard. Quietly closing it as "cannot
reproduce" would have been the dishonest option.</li>
</ul>

<h4>Verification that stops short of observation</h4>
<ul>
<li><strong>Four level-placed Spiny hatches are unit-tested, not observed.</strong> A
<code>Spiny</code> now starts as an egg by default, which was necessary because every production
construction path used the one-argument constructor and so produced a Spiny already walking in
mid-air. That default also changes behaviour for four placements in the shipped campaign &mdash;
three in <code>level_2.json</code>, one in <code>level_3.json</code> &mdash; and those four were
<em>not</em> watched in a live run. They are covered by a mutation-tested post-condition (a Spiny
built the way <code>LevelLoader</code> builds one must reach the walking state once grounded, which
asserts the outcome rather than the default, so the test cannot license the bug it guards). Reading
<code>Spiny::update()</code> shows an egg resting on ground hatches on its first grounded update
&mdash; but that is reasoning, and this project's rule is explicit that reasoning is not
observation.</li>
<li><strong>The Bowser stagger is observed but not tuned.</strong> Four fireballs and a three-second
window (<code>Bowser::FIRE_HITS_PER_STAGGER</code>, <code>STAGGER_SECONDS</code>) were watched
firing live &mdash; the counter running 1 to 4, the HUD switching to the stomp prompt, contact
refused while he guards. What no run measures is whether those two numbers are <em>right</em> for a
human player. They are reasoned and observed, not balanced.</li>
<li><strong>Playtest coverage still lags harness coverage.</strong> The 18 August audit named this
imbalance and it has narrowed rather than closed: the log now records live scripted runs across
every shipped world, driven by committed input scripts, but the harnesses still outnumber the
recorded sessions by a wide margin. Every defect in &sect;10.1, &sect;10.4 and &sect;10.5 was
invisible to the test suite and obvious within seconds of watching the game run. The suite is not
the problem &mdash; it is what makes the fixes stick &mdash; but it only verifies what someone
already thought to doubt.</li>
<li><strong>No sanitizer job in CI.</strong> AddressSanitizer would have found &sect;10.1 on its
first run. The suite is now hermetic, which is the precondition for adding one; nothing else blocks
it.</li>
</ul>

<h4>Performance left on the table, measured rather than guessed</h4>
<ul>
<li><strong><code>PhysicsEngine::update()</code> is not distance-gated.</strong> It runs five loops
over every entity, every frame, however far behind the camera that entity is. The off-camera freeze
that Endless Mode needed landed on the game-state side of the update instead, and its own
measurements are the reason this entry exists: at the third appended chunk, 23 of 90 entities were
frozen while the deliberate exemptions accounted for 43 &mdash; a real but modest win whose
dominant term is the exemption list, not the gate. <code>PhysicsEngine.cpp</code> sat outside that
lane's file budget and is the larger remaining cost. Deferred with the census counters left in
place, so whoever takes it can measure the result instead of asserting one.</li>
</ul>

<h4>Design trade-offs accepted rather than fixed</h4>
<p>These are decisions, not oversights, and &sect;6 argues each one on its merits with the code that
motivates it. They are repeated here because a known cost belongs in the list of known gaps.</p>
<ul>
<li><strong><code>PlayingState</code> is a god class</strong> &mdash; the largest file in the
project by a wide margin, carrying level load, spawning, physics orchestration, camera, HUD sync,
boss arenas, two-player, rewind, pipes, cheats, the editor bridge and endless chunk extension. Five
subsystems have already been extracted from it; the remaining decomposition was not attempted in
submission week, because a refactor of the file every other lane was editing is the wrong change to
make under a deadline.</li>
<li><strong>Twelve singletons, not four.</strong> The specification claims four; the code has twelve
reached from across the tree. &sect;6 owns the real number and argues the construction-order
rationale rather than restating the claim.</li>
<li><strong>Friend sprawl.</strong> <code>Entity</code> and <code>Character</code> each grant twelve
friend declarations, giving the physics engine, the collision resolver, <code>PlayingState</code>
and the movement strategies direct write access to protected state. Formally accepted as a
deliberate trade-off in <code>SPEC.md</code> &sect;22 with its rationale; the alternative was a
mutator interface wide enough to be equivalent.</li>
<li><strong>The <code>IPlayerState</code> axis is paid for and under-used.</strong> Four of its five
forms have empty bodies and the forms differ only by the size they report; only the cape form
carries behaviour. Real per-form behaviour lives in the Decorator layer instead. Two axes were
built; one of them does most of the work.</li>
<li><strong><code>GameSnapshot</code> is a fully public aggregate.</strong> A C++ Memento written as
a plain <code>struct</code> has no narrow interface, so the Originator's internals are readable
anywhere the type is. A common simplification, and still a weakened encapsulation boundary.</li>
</ul>

<h4>Specification items that were not built</h4>
<ul>
<li><strong>The descope list is explicit and dated.</strong> <code>SPEC.md</code> &sect;21 carries a
"Descope Addendum (2026-08-31)" naming every specified feature that will not be implemented, each
with a reason: dynamic music layers, autoscroll sections, timed bonus rooms, climbing, swimming as a
distinct state, the skid and hover-pause mechanics, knockback input-lock, red enemy variants, the
cape swoop, the Mini form's abilities, the character-switch hotkey, floating score text, extra
object pools, A* pathfinding and split-screen speedrun. The addendum exists because the audit found
the alternative in progress: features quietly missing from the code while the documents still
claimed them. One entry has since been <em>un</em>-descoped &mdash; the GLSL lighting and weather
line, because the shader was subsequently built for real (weather remains out of scope) &mdash;
which is worth more than the list itself, since it shows the addendum is maintained rather than
written once.</li>
<li><strong>The solvability oracle records its own failure but does not gate on it.</strong> If
every bounded reseed attempt fails, <code>MapGenerator::generateSolvable</code> keeps the last
unverified layout and returns <code>false</code>. All three call sites do consume that result: they
set an <code>m_lastLevelUnverified</code> flag, warn on standard error, and the dev panel reports
"layout unverified" in its generator section. What no caller does is <em>refuse to play the
level</em>. "Checked with retries, and told when the check failed" is true; "verified before
shipping" is not.</li>
</ul>

<h2 id="conclusion">14 &middot; Conclusion and future work</h2>
<p>The project delivers a complete, playable platformer whose value as coursework lies in its
architecture: ten design patterns, each introduced because a concrete problem demanded it, over a
four-level abstract hierarchy that the engine drives without ever asking what an object is. Adding a
new enemy means one class and one catalogue row; adding a new level means a JSON file.</p>
<p>The more useful outcome is what the defects taught, and it is a single lesson repeated: a fact
stored in two places drifts apart, and nothing notices unless something compares them. The standing
answer &mdash; the commit that creates the second copy also creates the test that fails when the
copies disagree &mdash; is now the project's most-used rule, and &sect;12.4 traces it back to the
three defects that bought it. The Windows crash taught the other half: "it works on my machine" is
exactly the evidence undefined behaviour is best at manufacturing.</p>

<h3>14.1 Future work: the far horizons</h3>
<blockquote><strong>None of what follows is part of this submission.</strong> Two of the three exist
as real branches that are <em>deliberately</em> kept off <code>dev</code>; the third is a direction
with no branch at all. The game on <code>dev</code> ships the hand-authored campaign and the
heuristic AI opponent, and plays identically with every branch below dropped &mdash; which is the
property that lets them be experiments rather than liabilities. Nothing here should be read as a
feature that nearly landed. What is <em>missing</em> from this submission, as opposed to beyond it,
is in &sect;13.</blockquote>

<h4>Learned agents: a trained policy behind the seam that already exists</h4>
<p><span class="tag no">Not delivered</span> &mdash; <strong>branch
<code>A/rl-neural-policy</code>, deliberately unmerged.</strong></p>
<p>The shipped game already draws the line a learning agent would need. <code>AIController</code>
does the sensing and the actuating &mdash; it reads the tilemap and the entity list into an
<code>AIObservation</code>, and it turns an <code>AIAction</code> back into calls on a real
<code>Player</code>. What it does not do is decide. That sits behind one virtual function,
<code>IAIPolicy::decide(const AIObservation&amp;)</code>, returning an <code>AIAction</code>
(<code>include/Entities/IAIPolicy.hpp</code>), with the shipped <code>HeuristicPolicy</code> as the
baseline any trained policy would have to beat. Four properties of that seam were chosen for this
horizon specifically, because each is cheap now and expensive to retrofit: the observation is
<strong>fixed-size and normalised</strong>, so an easy opponent sees <code>Unknown</code> outside
its vision radius rather than a smaller grid and one network accepts every difficulty;
<code>AIAction</code> is <strong>the same seven buttons a human presses</strong> and that
<code>PlayerFramePacket</code> already records for Shadow Mario, so a recorded human match is
imitation-learning data without a conversion step; the exploration noise lives in the controller
rather than the policy, so it applies identically to both implementations; and
<code>decide()</code> is synchronous and allocation-free on the hot path.</p>
<p>What a trained policy would still need from the engine is the honest part of this horizon, and it
is three things, none of them small:</p>
<ul>
<li><strong>Headless, accelerated stepping.</strong> Training wants thousands of episodes; the game
runs at 60&nbsp;fps with a window open. This needs a loop that steps the simulation without
rendering and without sleeping, and <code>PlayingState</code> assumes a render target exists in
several places.</li>
<li><strong>Determinism, or an explicit decision to live without it.</strong>
<code>ReplayRecorder</code>'s own header documents why the simulation is not reproducible from
inputs alone: float physics, an entity list that spawns and prunes mid-frame, and strategies that
read a shared singleton. Model-free learning survives that, because it only needs transitions &mdash;
but anything that must replay a trajectory exactly is ruled out until the sources of divergence are
removed one at a time.</li>
<li><strong>A reward log.</strong> The branch's <code>RewardTracker</code> shows the shape this
should take: it subscribes to events the game <em>already publishes</em>, so no gameplay code knows
a reward exists, and it writes one row per <em>decision</em> rather than per frame, because logging
idle frames fills the file with duplicates of the same state.</li>
</ul>
<p>The branch carries <code>NeuralPolicy</code>, <code>RewardTracker</code>,
<code>ExperienceLog</code>, a verification harness and a design document
(<code>docs/rl_training.md</code>). One detail there is worth repeating as engineering, whatever
happens to the learning: the observation layout is treated as a <em>versioned contract</em>, and
loading a weight file whose version does not match is refused rather than guessed, because a
silently misaligned input layer produces a policy that acts confidently and arbitrarily &mdash; the
hardest failure to diagnose from outside.</p>

<h4>Learned level generation, with the game as the oracle</h4>
<p><span class="tag no">Not delivered</span> &mdash; <strong>branch
<code>A/mapgen-gan-plan</code>, stacked on the RL branch and equally unmerged.</strong></p>
<p>The shipped <code>MapGenerator</code> is a single left-to-right pass of <em>independent</em>
probability rolls: an elevation profile, a per-column pit roll, uniform decoration and enemy rates.
That structure cannot express what makes a Mario level good, and the limitation is architectural
rather than a matter of tuning: independent rolls produce no rhythm or phrasing, no difficulty curve
across the level's length, and almost never a <em>composed</em> challenge where a pit and an enemy
and a moving platform are one obstacle rather than three coincidences. A learned generator attacks
exactly those three, because it learns tile <em>neighbourhoods</em> from levels a designer actually
made. The direction is a DCGAN over the public VGLC <em>Super Mario Bros.</em> corpus, with levels
authored in this project's own editor as a second corpus, generating chunks that are stitched,
repaired and filtered rather than played raw.</p>
<p>The interesting half is the loop between the two horizons, and it has a name in the literature
&mdash; agent-in-the-loop generation, and adversarial curriculum generation after it. Each side
supplies what the other cannot: the generator gives an agent unlimited varied levels so it does not
overfit three hand-made maps, and the agent gives the generator a <strong>playability
oracle</strong>. A GAN learns what levels look like; only something that plays them knows what they
play like. In this project's plan the cheap half of that oracle runs first, before any agent:
<code>LevelSolvability::isPathReachable</code> certifies that walking and jumping can connect the
start column to the goal, so an unplayable candidate is rejected without spending an episode on
it.</p>
<p>That oracle is also the one piece of this side project that has already paid rent on the main
line, and it is the model for how the rest should behave if it ever does. The original branch built
a full oracle &mdash; per-frame ballistic simulation plus a bottleneck-cost search that grades
difficulty. What came back to <code>dev</code> is a deliberately simplified, dependency-free
reimplementation: a column-reachability search bounded by the game's own walk, run and jump
constants, keeping the question the project actually needs answered ("is a required move impossible
at all?") and dropping the one it does not ("how hard is the hardest required move?"). The header
says so in as many words. Cherry-picking one honest reimplementation is the whole of what these
branches owe the submission; the rest stays where it is.</p>

<h4>One engine horizon: two players over a network</h4>
<p><span class="tag no">Not delivered</span> &mdash; <strong>a direction, with no branch.</strong></p>
<p>Two-player is local today: both players share one process, one keyboard and one camera. Putting
them on two machines is a genuine horizon rather than a task, and the reason is a decision the
project already made for other purposes. <code>ReplayRecorder</code> records a stream of
<code>GameSnapshot</code> <em>state</em>, not a stream of inputs, and the header explains why: the
simulation is not deterministic, so re-running the inputs would not reproduce the run. That single
fact rules out the cheap netcode &mdash; lockstep input exchange, which is what a deterministic
simulation buys you &mdash; and points at authoritative state replication instead, which is the
shape the existing snapshot stream already has. What is missing is a transport, a decision about who
simulates authoritatively, delta encoding (a snapshot is a whole entity list, sent whole), and
client-side reconciliation for the latency that remains. Each of those is a project, and naming them
is more useful than promising the feature.</p>
<p>Which is the point of listing only three horizons. Everything here is a direction with its first
real obstacle named, on the reasoning that a future-work section whose items have no known obstacles
has not been thought about yet.</p>

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
ctest --output-on-failure # {F['ctests']} verification targets registered as CTest cases
./SuperMarioGame
</code></pre>
<h3>16.2 Controls</h3>
<div class="tbl"><table>
<thead><tr><th>Action</th><th>Player 1</th><th>Player 2</th></tr></thead>
<tbody>
<tr><td>Move</td><td>A / D</td><td>&larr; / &rarr;</td></tr>
<tr><td>Jump</td><td>W or Space</td><td>&uarr; or Right Shift</td></tr>
<tr><td>Crouch / ground pound</td><td>S</td><td>&darr;</td></tr>
<tr><td>Run (hold)</td><td>Left Shift</td><td>N</td></tr>
<tr><td>Fireball</td><td>F</td><td>M</td></tr>
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
<tr><td>Design patterns</td><td>see &sect;7 for the full list, each with the problem it solves</td></tr>
<tr><td>Verification harnesses</td><td>{F['harnesses']} source files, {F['targets']} built,
    {F['ctests']} registered as CTest cases</td></tr>
<tr><td>Commits on the integration branch</td><td>{F['commits']}</td></tr>
<tr><td>Sound effects / music tracks</td><td>29 / 12</td></tr>
<tr><td>Achievements</td><td>12</td></tr>
</tbody></table></div>

<h3>16.4 UML diagrams, collected</h3>
<p>Every class diagram in this report, in one place, at full size &mdash; not a redrawn summary, the
same figures &sect;6 discusses in context, collected here for a reader who wants the whole structure in
one pass rather than hunting back through the prose. Each is regenerated from the current headers at
build time by the same <code>gen_class_diagram.py</code> pass that draws the inline copies
(&sect;6's opening note), so this appendix cannot go stale relative to them the way a hand-drawn
"master diagram" done once and forgotten would.</p>
<p>Seven of the nine show <strong>every attribute and method, with its real visibility</strong> —
a classic three-compartment UML box, not just a name in a box — specifically so encapsulation is
something a reader can verify by eye (how much of each box is <code>-</code>/<code>#</code> versus
<code>+</code>) rather than take &sect;6's prose about it on faith. Static members are underlined and
pure-virtual operations italicised, both standard UML notation; the same dashed border and
&#171;stereotype&#187; tag from &sect;6 still mark an abstract class or interface. Two roots
(<code>Entity</code>, <code>Character</code>) are kept to the compact, name-only form instead: both
contain <code>Player</code> (109 members) or <code>PlayingState</code> (132), and a full listing for
either one alone makes the whole diagram many times taller than every other class on it combined —
measured at roughly seven times the width in height before this exception was made, against one-to-two
times for every other group here. Their full member lists are not missing, only shown a different way:
Entity's own branches (Item, Block, Enemy) and Character's own children (Enemy, and Player's siblings
inside it) are the very next diagrams, each in full detail.</p>
<div class="tbl"><table>
<thead><tr><th>Figure</th><th>Root class</th><th>Discussed in</th><th>Detail</th><th>What it shows</th></tr></thead>
<tbody>
<tr><td>6</td><td><code>Entity</code></td><td>&sect;6.1</td><td>compact</td><td>The four branches under the abstract root: Character, Item, Block, Projectile.</td></tr>
<tr><td>6a</td><td><code>Item</code></td><td>&sect;6.2</td><td>full</td><td>Thirteen concrete power-ups and pickups behind one <code>activate()</code> call.</td></tr>
<tr><td>6b</td><td><code>Block</code></td><td>&sect;6.2</td><td>full</td><td>Ten concrete blocks, three of them (FallingPlatform, Thwomp) State machines in their own right.</td></tr>
<tr><td>7</td><td><code>Character</code></td><td>&sect;6.3</td><td>compact</td><td>Player and Enemy split, including the ShadowMario replay rival.</td></tr>
<tr><td>8</td><td><code>Enemy</code></td><td>&sect;6.3</td><td>full</td><td>The enemy branch, with the Boss sub-hierarchy sealed by Template Method.</td></tr>
<tr><td>9</td><td><code>IPlayerState</code></td><td>&sect;6.4</td><td>full</td><td>Five player forms plus the two Decorators that wrap them.</td></tr>
<tr><td>10</td><td><code>IGameState</code></td><td>&sect;6.5</td><td>full</td><td>{F['states']} screens behind one State interface.</td></tr>
<tr><td>11</td><td><code>ICommand</code></td><td>&sect;6.5</td><td>full</td><td>Input as objects — the Command pattern's own hierarchy.</td></tr>
<tr><td>12</td><td><code>IMovementStrategy</code></td><td>&sect;6.5</td><td>full</td><td>Eight interchangeable enemy movements — the Strategy pattern's own hierarchy.</td></tr>
</tbody></table></div>

{uml('Entity', None, 'Figure 6 (full tree, compact boxes) — The complete Entity hierarchy.',
     'Every concrete Entity subclass the game has, four branches deep from one abstract root. Compact '
     'rather than detailed — see the note above the index table.')}
{uml('Item', None, 'Figure 6a (full detail) — The complete Item hierarchy, every attribute and method.', None, detailed=True)}
{uml('Block', None, 'Figure 6b (full detail) — The complete Block hierarchy, every attribute and method.', None, detailed=True)}
{uml('Character', None, 'Figure 7 (full tree, compact boxes) — The complete Character hierarchy.',
     'Every playable character and every enemy, including the Boss sub-branch, in one tree. Compact '
     'rather than detailed for the same reason as Figure 6.')}
{uml('Enemy', None, 'Figure 8 (full detail) — The complete Enemy/Boss hierarchy, every attribute and method.', None, detailed=True)}
{uml('IPlayerState', None, 'Figure 9 (full detail) — The complete player-form hierarchy, every attribute and method.', None, detailed=True)}
{uml('IGameState', None, 'Figure 10 (full detail) — The complete screen/State hierarchy, every attribute and method.', None, detailed=True)}
{uml('ICommand', None, 'Figure 11 (full detail) — The complete Command hierarchy, every attribute and method.', None, detailed=True)}
{uml('IMovementStrategy', None, 'Figure 12 (full detail) — The complete Strategy hierarchy, every attribute and method.', None, detailed=True)}

<footer>Group 52 &middot; CS202 Object-Oriented Programming &middot; University of Science, VNU-HCM<br>
Generated by <code>reports/build_report.py</code> from the repository at commit {F['head']}.
Self-contained: opens from disk, no network, prints to PDF.</footer>

</main>
"""

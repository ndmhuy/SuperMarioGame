#include "Core/MenuState.hpp"
#include "Core/AchievementManager.hpp"
#include "Core/CharacterSelectState.hpp"
#include "Core/DebugConsole.hpp"
#include "Core/EditorState.hpp"
#include "Core/OptionsState.hpp"
#include "Core/PlayingState.hpp"
#include "Core/Game.hpp"
#include "Core/InputManager.hpp"
#include "Core/SoundManager.hpp"
#include "Utils/Constants.hpp"
#include "Utils/LevelCatalog.hpp"
#include "Utils/MetaGame.hpp"
#include "Utils/Serializer.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Window/Event.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

// Main-menu rows, in display order.
enum MainRow { ROW_START = 0, ROW_LOAD, ROW_VERSUS, ROW_DAILY, ROW_EDITOR, ROW_CUSTOM,
               ROW_GENERATOR, ROW_RECORDS, ROW_OPTIONS, ROW_QUIT, ROW_COUNT };

const char* const kThemes[] = {"OVERWORLD", "UNDERGROUND", "CASTLE", "ICE"};
constexpr int kThemeCount = 4;
const char* const kDifficulties[] = {"EASY", "MEDIUM", "HARD"};
constexpr int kDifficultyCount = 3;


std::string percent(float value) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(0) << (value * 100.0f) << "%";
    return ss.str();
}

std::string toUpper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

// A slot's value column used to be formatted here. It moved onto
// SaveSlotPreview::summary() when the pause menu's slot picker needed the same
// string: two formatters would let the two screens describe the same slot
// differently, and the picker's whole job is to be trusted about what it is
// overwriting.

// Confirm rows on Page::LoadDelete, in display order.
constexpr int DELETE_ROW_KEEP = 0;
constexpr int DELETE_ROW_DELETE = 1;
constexpr int DELETE_ROW_COUNT = 2;

// The multiplayer modes, in the order the page cycles them. Split-screen is
// absent on purpose rather than shown greyed out: Camera holds a single sf::View
// and is deliberately non-movable, and every screen-space overlay in the game
// would need to learn about viewports before a second view could exist. Offering
// a row that cannot work is worse than not offering it.
const GameMode kMultiplayerModes[] = {
    GameMode::VersusHuman, GameMode::VersusCPU, GameMode::CoopHuman,
    GameMode::ShadowChase
};
constexpr int kMultiplayerModeCount = 4;

// One-line explanation per mode, shown under the list. A player choosing
// "SHADOW CHASE" from a label alone has no way to know what it does.
const char* modeBlurb(GameMode mode) {
    switch (mode) {
        case GameMode::VersusHuman:
            return "TWO HUMANS, ONE SCREEN. STOMP EACH OTHER.";
        case GameMode::VersusCPU:
            return "RACE AND FIGHT AN AI OPPONENT.";
        case GameMode::CoopHuman:
            return "SHARED LIVES. BOUNCE OFF YOUR PARTNER.";
        case GameMode::ShadowChase:
            return "YOUR OWN PATH, 3 SECONDS BEHIND YOU.";
        case GameMode::SinglePlayer:
            break;
    }
    return "";
}

int indexOfMode(GameMode mode) {
    for (int i = 0; i < kMultiplayerModeCount; ++i) {
        if (kMultiplayerModes[i] == mode) return i;
    }
    return 0;
}

// SPEC 10.2's 30s is the shipped default (Constants::ATTRACT_MODE_IDLE_SECONDS).
// A verification script that actually waited out 30 real seconds per run would
// be exactly the kind of unreliable long wait R9/R16 already ran into, so this
// env var lets a `--script` shorten it — read once per process, not per frame,
// so a script cannot change it mid-run by surprise.
float attractIdleThresholdSeconds() {
    static const float threshold = [] {
        if (const char* env = std::getenv("SUPERMARIO_ATTRACT_IDLE_SECONDS")) {
            try {
                const float parsed = std::stof(env);
                if (parsed > 0.0f) return parsed;
            } catch (...) {
                // Fall through to the shipped default.
            }
        }
        return Constants::ATTRACT_MODE_IDLE_SECONDS;
    }();
    return threshold;
}

} // namespace

void MenuState::enter() {
    std::cout << "Entering MenuState" << std::endl;
    // Pass the registered track key, not a raw path — SoundManager maps
    // "title_screen" to assets/bgm/main_menu.mp3 and resolves it per platform.
    SoundManager::getInstance().playMusic("title_screen");

    m_playerSheet  = SpriteSheet::loadAtlas("player");
    m_scenerySheet = SpriteSheet::loadAtlas("world_scenery_item");
    m_background.setTheme(BackgroundTheme::Overworld);
    m_background.setSpriteSheet(m_scenerySheet.get());
    // No tilemap behind the menu, so the backdrop has to close off its own
    // ground or the hills float over open sky.
    m_background.setDrawGroundBand(true);

    m_mainItems.clear();
    // The New Game+ cycle is shown on the start row, so a player can see the
    // campaign has reset rather than wondering why 1-2 is locked again.
    m_mainItems.emplace_back("START GAME", MetaGame::newGamePlusLabel());
    m_mainItems.emplace_back("LOAD GAME");
    m_mainItems.emplace_back("MULTIPLAYER", "4 MODES");
    m_mainItems.emplace_back("DAILY CHALLENGE", MetaGame::todaysChallengeName());
    m_mainItems.emplace_back("MAP EDITOR");
    // The other half of the editor: a level you author is worthless if there is
    // no way to play it, and until now there was none.
    LevelCatalog::refreshCustomLevels();
    m_mainItems.emplace_back("CUSTOM LEVELS",
                             std::to_string(LevelCatalog::customLevels().size()));
    m_mainItems.emplace_back("PROCEDURAL LEVEL");
    // Achievement progress on the row itself, so the player can see there is
    // something to chase without opening the page first.
    {
        const auto& achievements = AchievementManager::getInstance().getAchievements();
        int unlocked = 0;
        for (const auto& a : achievements) if (a.unlocked) ++unlocked;
        m_mainItems.emplace_back("RECORDS", std::to_string(unlocked) + "/" +
                                            std::to_string(achievements.size()));
    }
    m_mainItems.emplace_back("OPTIONS");
    m_mainItems.emplace_back("QUIT");

    // Returning here from a finished run must not leave the screen unusable.
    m_dismissed = false;
    m_page = Page::Main;
}

void MenuState::exit() {
    std::cout << "Exiting MenuState" << std::endl;
}

void MenuState::applyDifficultyPreset(int index) {
    m_selectedDifficultyIdx = index;
    m_generatorConfig.difficulty = static_cast<MapDifficulty>(index);
    switch (index) {
        case 0:  m_generatorConfig.pitProbability = 0.05f; m_generatorConfig.enemySpawnRate = 0.10f; break;
        case 1:  m_generatorConfig.pitProbability = 0.12f; m_generatorConfig.enemySpawnRate = 0.20f; break;
        default: m_generatorConfig.pitProbability = 0.22f; m_generatorConfig.enemySpawnRate = 0.35f; break;
    }
}

bool MenuState::isMultiplayerRowEnabled(MpRow row) const {
    switch (row) {
        case MpRow::Opponent:
            // Only the two versus modes have an opponent to choose. Co-op is
            // human-only by definition, and a shadow is not an opponent that can
            // be configured.
            return m_match.isVersus();
        case MpRow::Difficulty:
        case MpRow::Archetype:
            return m_match.isCpuOpponent();
        default:
            return true;
    }
}

std::vector<UiMenuItem> MenuState::buildMultiplayerItems() const {
    std::vector<UiMenuItem> rows;
    rows.emplace_back("MODE", toString(m_match.mode));
    rows.emplace_back("OPPONENT",
                      m_match.isCpuOpponent() ? "CPU" : "HUMAN",
                      isMultiplayerRowEnabled(MpRow::Opponent));
    rows.emplace_back("AI SKILL", toString(m_match.aiDifficulty),
                      isMultiplayerRowEnabled(MpRow::Difficulty));
    rows.emplace_back("AI STYLE", toString(m_match.aiArchetype),
                      isMultiplayerRowEnabled(MpRow::Archetype));
    rows.emplace_back("START");
    rows.emplace_back("BACK");
    return rows;
}

void MenuState::refreshSlotPreviews() {
    for (int slot = 1; slot <= 3; ++slot) {
        m_slotPreviews[static_cast<std::size_t>(slot - 1)] = Serializer::getSlotPreview(slot);
    }
}

std::vector<UiMenuItem> MenuState::buildLoadItems() const {
    std::vector<UiMenuItem> rows;
    for (int i = 0; i < 3; ++i) {
        rows.emplace_back("SLOT " + std::to_string(i + 1),
                          m_slotPreviews[static_cast<std::size_t>(i)].summary());
    }
    rows.emplace_back("BACK");
    return rows;
}

void MenuState::requestSlotDelete() {
    if (m_loadSelected < 0 || m_loadSelected >= 3) {
        // The BACK row has no slot behind it.
        SoundManager::getInstance().playSound("bump");
        return;
    }
    if (!m_slotPreviews[static_cast<std::size_t>(m_loadSelected)].exists) {
        // Same convention as confirming a locked card or an empty slot: a
        // blocked action gets the "bump" cue rather than silently doing nothing.
        SoundManager::getInstance().playSound("bump");
        return;
    }

    m_pendingDeleteSlot = m_loadSelected + 1;
    m_deleteConfirmSelected = DELETE_ROW_KEEP;
    m_page = Page::LoadDelete;
}

void MenuState::performSlotDelete() {
    if (m_pendingDeleteSlot < 1 || m_pendingDeleteSlot > 3) return;

    // Serializer owns the path. Resolving "saves/slot_n.json" here instead
    // would be a second source of truth for it, which is the bug
    // guard_asset_single_source exists about — and the one time this suite
    // hand-rolled a save path it deleted the developer's real files.
    const bool removed = Serializer::deleteSlot(m_pendingDeleteSlot);

    // Game::getActiveSlot() is deliberately left pointing at the slot that was
    // just freed. It is what the checkpoint autosave writes to, so re-pointing
    // it at one of the surviving saves would make the next autosave quietly
    // overwrite a save the player did not touch; the emptied slot is the one
    // place a write cannot destroy anything.
    m_pendingDeleteSlot = 0;
    m_page = Page::Load;

    // Re-read from disk rather than clearing the cached preview: the page must
    // show what is actually there now, including a delete that failed.
    refreshSlotPreviews();

    // "break_block" rather than a positive chime: something was destroyed, and
    // the cue should not sound like a reward. "bump" is the blocked-action cue
    // the rest of the menus use, so a delete that removed nothing sounds like
    // the no-op it was.
    SoundManager::getInstance().playSound(removed ? "break_block" : "bump");
}

std::vector<UiMenuItem> MenuState::buildCustomLevelItems() const {
    std::vector<UiMenuItem> rows;
    for (const LevelEntry& entry : LevelCatalog::customLevels()) {
        // The file stem as the value column, so two levels that named themselves
        // the same are still told apart.
        std::string stem = std::filesystem::path(entry.path).stem().string();
        rows.emplace_back(entry.displayName, stem + ".json");
    }
    if (rows.empty()) {
        rows.emplace_back("NO CUSTOM LEVELS YET", "MAP EDITOR > SAVE AS", false);
    }
    rows.emplace_back("BACK");
    return rows;
}

void MenuState::moveSelection(int delta) {
    // SPEC 17.3: "Menu selection: click sound on highlight change". CharSelect
    // and WorldMapState only ever play "bump" for a *blocked* move (a locked
    // slot, the array edge); no state actually had the plain per-row cue this
    // asks for (R7 audit — the plan's premise that CharSelect/Pause already had
    // one did not hold on inspection). Reusing "bump" here rather than adding a
    // new asset, same short blip the blocked-move case already uses.
    SoundManager::getInstance().playSound("bump");

    if (m_page == Page::Main) {
        m_mainSelected = (m_mainSelected + delta + ROW_COUNT) % ROW_COUNT;
        return;
    }
    if (m_page == Page::Generator) {
        const int n = static_cast<int>(GenRow::COUNT);
        m_genSelected = (m_genSelected + delta + n) % n;
        return;
    }
    if (m_page == Page::CustomLevels) {
        const int n = static_cast<int>(buildCustomLevelItems().size());
        if (n > 0) m_customSelected = (m_customSelected + delta + n) % n;
        return;
    }
    if (m_page == Page::Load) {
        // Every row (three slots plus Back) is always selectable, even an empty
        // slot — activating one just plays the blocked-move cue rather than
        // hiding the row, so a new player can see there are three slots at all.
        const int n = static_cast<int>(LoadRow::COUNT);
        m_loadSelected = (m_loadSelected + delta + n) % n;
        return;
    }
    if (m_page == Page::LoadDelete) {
        m_deleteConfirmSelected =
            (m_deleteConfirmSelected + delta + DELETE_ROW_COUNT) % DELETE_ROW_COUNT;
        return;
    }

    // Skip over rows this mode does not offer, so the cursor never parks on a
    // greyed-out line where Left/Right silently does nothing.
    const int n = static_cast<int>(MpRow::COUNT);
    const int step = (delta >= 0) ? 1 : -1;
    for (int i = 0; i < n; ++i) {
        m_mpSelected = (m_mpSelected + step + n) % n;
        if (isMultiplayerRowEnabled(static_cast<MpRow>(m_mpSelected))) return;
    }
}

void MenuState::adjustSelection(int direction) {
    if (m_page == Page::LoadDelete) {
        // The confirm page is a two-row choice; Left/Right flips it as readily
        // as Up/Down, so neither is a dead key on it.
        moveSelection(direction);
        return;
    }
    if (m_page == Page::Multiplayer) {
        switch (static_cast<MpRow>(m_mpSelected)) {
            case MpRow::Mode: {
                const int next = (indexOfMode(m_match.mode) + direction +
                                  kMultiplayerModeCount) % kMultiplayerModeCount;
                m_match.mode = kMultiplayerModes[next];
                // The cursor may now be sitting on a row this mode does not
                // offer — step it back to somewhere meaningful.
                if (!isMultiplayerRowEnabled(static_cast<MpRow>(m_mpSelected))) {
                    m_mpSelected = static_cast<int>(MpRow::Mode);
                }
                break;
            }
            case MpRow::Opponent:
                if (!m_match.isVersus()) break;
                // Two choices, so direction only has to flip it.
                m_match.mode = m_match.isCpuOpponent() ? GameMode::VersusHuman
                                                       : GameMode::VersusCPU;
                break;
            case MpRow::Difficulty: {
                if (!m_match.isCpuOpponent()) break;
                constexpr int kCount = 3;
                const int next = (static_cast<int>(m_match.aiDifficulty) + direction +
                                  kCount) % kCount;
                m_match.aiDifficulty = static_cast<AIDifficulty>(next);
                break;
            }
            case MpRow::Archetype: {
                if (!m_match.isCpuOpponent()) break;
                constexpr int kCount = 3;
                const int next = (static_cast<int>(m_match.aiArchetype) + direction +
                                  kCount) % kCount;
                m_match.aiArchetype = static_cast<AIArchetype>(next);
                break;
            }
            default:
                break;
        }
        return;
    }

    if (m_page != Page::Generator) return;

    // Sliders move in 1% steps; the ranges match what the old ImGui sliders used.
    switch (static_cast<GenRow>(m_genSelected)) {
        case GenRow::Theme:
            m_selectedThemeIdx = (m_selectedThemeIdx + direction + kThemeCount) % kThemeCount;
            m_generatorConfig.theme = static_cast<MapTheme>(m_selectedThemeIdx);
            break;
        case GenRow::Difficulty:
            applyDifficultyPreset((m_selectedDifficultyIdx + direction + kDifficultyCount) % kDifficultyCount);
            break;
        case GenRow::PitProbability:
            m_generatorConfig.pitProbability =
                std::clamp(m_generatorConfig.pitProbability + 0.01f * direction, 0.0f, 0.40f);
            break;
        case GenRow::PipeFrequency:
            m_generatorConfig.pipeFrequency =
                std::clamp(m_generatorConfig.pipeFrequency + 0.01f * direction, 0.0f, 0.20f);
            break;
        case GenRow::EnemyRate:
            m_generatorConfig.enemySpawnRate =
                std::clamp(m_generatorConfig.enemySpawnRate + 0.01f * direction, 0.0f, 0.50f);
            break;
        case GenRow::CoinRate:
            m_generatorConfig.coinClusterRate =
                std::clamp(m_generatorConfig.coinClusterRate + 0.01f * direction, 0.0f, 0.50f);
            break;
        default:
            break;
    }
}

void MenuState::activateSelection() {
    if (m_dismissed) return;
    Game& game = Game::getInstance();

    if (m_page == Page::Main) {
        switch (m_mainSelected) {
            case ROW_START:
                m_dismissed = true;
                game.changeState(std::make_unique<CharacterSelectState>(false, false));
                break;
            case ROW_LOAD:
                // Read the slots fresh every time this opens: a save made from
                // the pause menu since the game last returned here must show up,
                // and an entry removed elsewhere must stop showing up.
                m_page = Page::Load;
                m_loadSelected = 0;
                refreshSlotPreviews();
                break;
            case ROW_VERSUS:
                // Opens the multiplayer page rather than starting a match. This
                // row used to drop straight into a hardcoded human-vs-human
                // shared-screen game on 1-1, which was the only match that
                // existed; there are four modes now and a CPU opponent to
                // configure.
                m_page = Page::Multiplayer;
                m_mpSelected = static_cast<int>(MpRow::Mode);
                break;
            case ROW_DAILY: {
                // Date-seeded, so everyone playing today gets the same level —
                // which is the only thing that makes it a challenge.
                m_dismissed = true;
                const MapGeneratorConfig daily =
                    MetaGame::dailyChallengeConfig(MetaGame::todaysSeed());
                game.changeState(std::make_unique<CharacterSelectState>(false, true, daily));
                break;
            }
            case ROW_EDITOR:
                // A screen of its own, with a blank canvas. This row used to
                // open PlayingState(true, false) — the editor as one floating
                // ImGui window over a silently loaded copy of World 1-1, with
                // nothing on screen saying which level that was.
                m_dismissed = true;
                game.changeState(std::make_unique<EditorState>());
                break;
            case ROW_CUSTOM:
                LevelCatalog::refreshCustomLevels();
                m_page = Page::CustomLevels;
                m_customSelected = 0;
                break;
            case ROW_GENERATOR:
                m_page = Page::Generator;
                m_genSelected = 0;
                break;
            case ROW_RECORDS:
                // Same screen, opened on the statistics page. Everything it
                // shows was already being tracked and persisted with nowhere in
                // the game to see it.
                game.pushState(std::make_unique<OptionsState>(OptionsState::Page::Statistics));
                break;
            case ROW_OPTIONS:
                // Overlay: it pops straight back to this menu.
                game.pushState(std::make_unique<OptionsState>());
                break;
            case ROW_QUIT:
                m_dismissed = true;
                game.quit();
                break;
            default:
                break;
        }
        return;
    }

    if (m_page == Page::CustomLevels) {
        const auto& levels = LevelCatalog::customLevels();
        const int backRow = static_cast<int>(levels.size());
        if (m_customSelected >= backRow || levels.empty()) {
            m_page = Page::Main;
            return;
        }
        m_dismissed = true;
        // Addressed by PATH, not by a campaign index: an authored level is not
        // part of the campaign and must not renumber it.
        game.changeState(std::make_unique<PlayingState>(
            false, false, MapGeneratorConfig(), 0, 0, MatchConfig{},
            /*isEndless=*/false, /*pendingLoadSlot=*/0, /*isAttractDemo=*/false,
            levels[static_cast<std::size_t>(m_customSelected)].path));
        return;
    }

    if (m_page == Page::LoadDelete) {
        if (m_deleteConfirmSelected == DELETE_ROW_DELETE) {
            performSlotDelete();
        } else {
            // KEEP returns to the slot list, not to the main menu: the player
            // was browsing their saves and still is.
            m_pendingDeleteSlot = 0;
            m_page = Page::Load;
        }
        return;
    }

    if (m_page == Page::Load) {
        switch (static_cast<LoadRow>(m_loadSelected)) {
            case LoadRow::Back:
                m_page = Page::Main;
                break;
            default: {
                // LoadRow::Slot1..Slot3 are 0..2, matching Serializer's 1-based
                // slot numbering by +1 — same mapping DevPanel's Save/Load Slots
                // loop uses (`for (int slot = 1; slot <= 3; ++slot)`).
                const int slot = m_loadSelected + 1;
                if (m_slotPreviews[static_cast<std::size_t>(m_loadSelected)].exists) {
                    m_dismissed = true;
                    // A fresh Level-1 PlayingState exists only to give
                    // loadFromSlot somewhere to run — see its constructor's
                    // pendingLoadSlot doc. This is the same private method
                    // DevPanel's Load button calls on an already-running
                    // instance; nothing here reimplements it.
                    game.changeState(std::make_unique<PlayingState>(
                        false, false, MapGeneratorConfig(), 0, 0, MatchConfig{},
                        /*isEndless=*/false, /*pendingLoadSlot=*/slot));
                } else {
                    // Same convention as CharacterSelectState::confirmSelection()
                    // confirming a locked card: a blocked action gets the "bump"
                    // cue rather than silently doing nothing.
                    SoundManager::getInstance().playSound("bump");
                }
                break;
            }
        }
        return;
    }

    if (m_page == Page::Multiplayer) {
        switch (static_cast<MpRow>(m_mpSelected)) {
            case MpRow::Start:
                // Straight into 1-1: a multiplayer match is one level, and
                // routing it through the world map would imply a shared
                // campaign that these modes do not have.
                m_dismissed = true;
                game.changeState(std::make_unique<PlayingState>(
                    false, false, MapGeneratorConfig(), 0, 0, m_match));
                break;
            case MpRow::Back:
                m_page = Page::Main;
                break;
            default:
                // Value rows confirm as a nudge, so Enter is never a dead key.
                adjustSelection(1);
                break;
        }
        return;
    }

    switch (static_cast<GenRow>(m_genSelected)) {
        case GenRow::GeneratePlay:
            m_dismissed = true;
            game.changeState(std::make_unique<CharacterSelectState>(false, true, m_generatorConfig));
            break;
        case GenRow::GenerateEdit:
            m_dismissed = true;
            game.changeState(std::make_unique<PlayingState>(true, true, m_generatorConfig));
            break;
        case GenRow::PlayEndless:
            m_dismissed = true;
            game.changeState(std::make_unique<PlayingState>(
                false, true, m_generatorConfig, 0, 0, MatchConfig{}, /*isEndless=*/true));
            break;
        case GenRow::Back:
            m_page = Page::Main;
            break;
        default:
            // Value rows confirm as a nudge, so Enter is never a dead key.
            adjustSelection(1);
            break;
    }
}

void MenuState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) return;
    using Key = sf::Keyboard::Key;

    // Any key this menu actually handles counts as activity, on every page —
    // not just Page::Main. Reset unconditionally rather than only from the
    // trigger's own page check, so idle time browsing a submenu does not carry
    // over and fire the instant the player steps back to Page::Main.
    m_idleTime = 0.0f;

    switch (keyPressed->code) {
        case Key::Up:
        case Key::W:
            moveSelection(-1);
            break;
        case Key::Down:
        case Key::S:
            moveSelection(1);
            break;
        case Key::Left:
        case Key::A:
            adjustSelection(-1);
            break;
        case Key::Right:
        case Key::D:
            adjustSelection(1);
            break;
        case Key::Enter:
        case Key::Space:
            activateSelection();
            break;
        case Key::X:
        case Key::Delete:
            // Delete the highlighted save. Deliberately a distinct key from
            // Enter/LOAD — the two are next to each other in intent and must
            // not be next to each other in consequence. It only opens the
            // confirmation page; requestSlotDelete() refuses an empty slot.
            //
            // X is the key the page names, and it is the one that is actually
            // there on every keyboard: on this project's own development Macs
            // the large key is Backspace (already "back") and forward-delete
            // needs Fn. Delete stays wired as an alias for the players who
            // reach for it anyway.
            if (m_page == Page::Load) requestSlotDelete();
            break;
        case Key::Escape:
        case Key::Backspace:
            // Back out one level at a time. The confirm page steps back to the
            // slot list rather than all the way out, and cancelling it must not
            // delete anything. From the top level, quitting is the only thing
            // left — but make the player pick it deliberately.
            if (m_page == Page::LoadDelete) {
                m_pendingDeleteSlot = 0;
                m_page = Page::Load;
            } else if (m_page != Page::Main) {
                m_page = Page::Main;
            } else {
                m_mainSelected = ROW_QUIT;
            }
            break;
        default:
            break;
    }
}

void MenuState::update(float dt) {
    m_elapsed += dt;

    // Clouds drift left; the walker patrols the ground line and wraps around.
    m_cloudScroll = std::fmod(m_cloudScroll + dt * 14.0f, static_cast<float>(Constants::WINDOW_WIDTH) + 200.0f);
    m_walkerX += dt * 70.0f;
    if (m_walkerX > static_cast<float>(Constants::WINDOW_WIDTH) + 64.0f) {
        m_walkerX = -64.0f;
    }

    // F5 attract mode (SPEC 10.2). Gated to Page::Main: the Load/Multiplayer/
    // Generator submenus must never be interrupted mid-navigation, and this
    // MenuState is the only place that page lives, so checking it here is
    // enough — there is no separate "picker" state to strand. Also gated on
    // the debug console being closed: unlike a pushed overlay (Options,
    // Records), which suspends this state's update() entirely, the console
    // does not — Game::run() keeps calling m_gsm.update() while it is open, so
    // without this check typing a long console command would silently start a
    // demo underneath it.
    if (!m_dismissed && m_page == Page::Main && !DebugConsole::getInstance().isVisible()) {
        m_idleTime += dt;
        if (m_idleTime >= attractIdleThresholdSeconds()) {
            m_dismissed = true;
            // Plain defaults throughout (SinglePlayer, not endless, no pending
            // load): the guard against ever landing in Endless or versus mode
            // is simply never asking for either here.
            Game::getInstance().changeState(std::make_unique<PlayingState>(
                /*startInEditor=*/false, /*isProcedural=*/false, MapGeneratorConfig(),
                /*characterIndex=*/0, /*levelIndex=*/0, MatchConfig{},
                /*isEndless=*/false, /*pendingLoadSlot=*/0, /*isAttractDemo=*/true));
        }
    }
}

void MenuState::drawBackground(sf::RenderTarget& target) const {
    // Was a hand-rolled backdrop of sf::CircleShape clouds and hills, written
    // before BackgroundRenderer existed. It read as coloured blobs next to the
    // pixel art everywhere else, which is exactly what it looked like.
    m_background.render(target, AABB{m_cloudScroll * 3.0f, 0.0f,
                                     static_cast<float>(Constants::WINDOW_WIDTH),
                                     static_cast<float>(Constants::WINDOW_HEIGHT)});

    // The walking character stays: it is the one animated thing on the screen.
    if (m_playerSheet) {
        const std::string frame = "mario_small_walk_" + std::to_string(
            static_cast<int>(m_elapsed / 0.12f) % 2);
        if (m_playerSheet->hasFrame(frame)) {
            sf::Sprite walker = m_playerSheet->getSprite(frame);
            walker.setScale({2.0f, 2.0f});
            const auto bounds = walker.getLocalBounds();
            // 640 is BackgroundRenderer's ground line: the walker has to stand on the
            // same line the backdrop layers sit on, or it floats.
            walker.setPosition({m_walkerX, 640.0f - bounds.size.y * 2.0f});
            target.draw(walker);
        }
    }
}

void MenuState::render(sf::RenderTarget& target) {
    target.setView(target.getDefaultView());
    drawBackground(target);

    const float centerX = Constants::WINDOW_WIDTH * 0.5f;

    // Title bobs on a sine so the screen is never completely static.
    const float titleY = 74.0f + std::sin(m_elapsed * 2.0f) * 6.0f;
    UiRenderer::drawShadowedText(target, "SUPER MARIO", {centerX, titleY}, 44,
                                 sf::Color(255, 216, 0), true);
    UiRenderer::drawShadowedText(target, "CS202 FINAL PROJECT", {centerX, titleY + 58.0f}, 14,
                                 sf::Color(255, 255, 255), true);

    if (m_page == Page::Main) {
        // Height comes from the row count. It was a hardcoded 250px, which fit
        // the five rows it was written for; adding 2P Versus and Daily Challenge
        // pushed the last two rows straight out through the bottom of the panel
        // and on top of the hint line.
        constexpr float ROW_HEIGHT = 40.0f;
        constexpr float PANEL_TOP = 214.0f;
        constexpr float PADDING = 26.0f;
        const float panelHeight = PADDING * 2.0f + ROW_HEIGHT * static_cast<float>(m_mainItems.size());

        // Wide enough that the longest label ("DAILY CHALLENGE", 15 characters
        // at 15px) clears the value column instead of printing through it.
        UiRenderer::drawPanel(target, {centerX - 320.0f, PANEL_TOP}, {640.0f, panelHeight},
                              sf::Color(0, 0, 0, 170));
        UiRenderer::drawMenuItems(target, m_mainItems, m_mainSelected,
                                  {centerX - 270.0f, PANEL_TOP + PADDING}, ROW_HEIGHT, 15,
                                  centerX + 40.0f, m_elapsed, centerX + 320.0f);
        // Shadowed: this line sits over the parallax bushes, and plain white
        // text on light green foliage is unreadable.
        UiRenderer::drawShadowedText(target, "UP/DOWN  SELECT      ENTER  CONFIRM",
                                     {centerX, PANEL_TOP + panelHeight + 20.0f}, 11,
                                     sf::Color(255, 255, 255), true);
        return;
    }

    if (m_page == Page::Multiplayer) {
        const std::vector<UiMenuItem> rows = buildMultiplayerItems();

        // Which keys each participant gets — the one thing a second player at
        // the same keyboard has to be told before the level starts.
        //
        // Read from InputManager rather than written out, because these are
        // rebindable and a hint that names the defaults is worse than none: this
        // line said "P1 WASD" while a config.json in the repo had Player 1 on the
        // arrow keys, which also means both players were sharing them.
        const InputManager& input = InputManager::getInstance();
        const bool twoHumans = (m_match.mode == GameMode::VersusHuman || m_match.isCoop());
        bool padsCollide = false;
        if (twoHumans) {
            for (const char* action : {"left", "right", "jump"}) {
                if (!input.getBoundKeyName(action, 0).empty() &&
                    input.getBoundKeyName(action, 0) == input.getBoundKeyName(action, 1)) {
                    padsCollide = true;
                    break;
                }
            }
        }

        // The panel is sized from what is actually going in it. It was a fixed
        // 330px, so the key summary and the shared-keys warning — both added
        // later, and both conditional — were clipped straight through the bottom
        // edge of the frame.
        constexpr float kTop = 196.0f;
        constexpr float kRowHeight = 34.0f;
        // Named because four things now measure against it: the frame itself,
        // the row list's right clip, and the two centred summary lines below.
        constexpr float kPanelHalfW = 280.0f;
        // One character cell of clearance at each edge, matching the gutter
        // drawMenuItems uses so the centred lines and the rows agree.
        constexpr float kTextBudget = kPanelHalfW * 2.0f - 22.0f * 2.0f;
        const float rowsTop = kTop + 58.0f;
        const float rowsBottom = rowsTop + kRowHeight * static_cast<float>(rows.size());
        const float blurbY = rowsBottom + 16.0f;
        const float keysY = blurbY + 22.0f;
        const float warnY = keysY + 18.0f;
        const float contentBottom = padsCollide ? warnY : (twoHumans ? keysY : blurbY);
        const float panelHeight = (contentBottom + 22.0f) - kTop;

        UiRenderer::drawPanel(target, {centerX - kPanelHalfW, kTop},
                              {kPanelHalfW * 2.0f, panelHeight}, sf::Color(0, 0, 0, 200));
        UiRenderer::drawText(target, "MULTIPLAYER", {centerX, kTop + 20.0f}, 14,
                             sf::Color(255, 170, 220), true);
        UiRenderer::drawMenuItems(target, rows, m_mpSelected,
                                  {centerX - 210.0f, rowsTop}, kRowHeight, 13,
                                  centerX + 90.0f, m_elapsed, centerX + kPanelHalfW);

        // What the highlighted mode actually does. The labels alone do not say.
        UiRenderer::drawTextFitted(target, modeBlurb(m_match.mode), {centerX, blurbY}, 11,
                                   sf::Color(200, 200, 200), kTextBudget, true);

        if (twoHumans) {
            auto padSummary = [&input](int pad) {
                const std::string left  = input.getBoundKeyName("left", pad);
                const std::string right = input.getBoundKeyName("right", pad);
                const std::string jump  = input.getBoundKeyName("jump", pad);
                return left + "/" + right + " + " + jump;
            };
            // Fitted, not fixed: the key names are rebindable, so this line has
            // no maximum length the layout can be written against. "LShift" on
            // both pads already made it 58 characters — 637px in a 560px panel.
            UiRenderer::drawTextFitted(target, "P1  " + padSummary(0) + "      P2  " + padSummary(1),
                                       {centerX, keysY}, 11, sf::Color(150, 220, 150),
                                       kTextBudget, true);

            // Both pads on the same key is unplayable, and the only place it can
            // be noticed before the level starts is here.
            if (padsCollide) {
                UiRenderer::drawTextFitted(target, "BOTH PLAYERS SHARE KEYS - SEE OPTIONS/KEYS",
                                           {centerX, warnY}, 10, sf::Color(255, 140, 140),
                                           kTextBudget, true);
            }
        }

        UiRenderer::drawShadowedText(target, "LEFT/RIGHT  ADJUST      ESC  BACK",
                                     {centerX, kTop + panelHeight + 18.0f}, 11,
                                     sf::Color(220, 220, 220), true);
        return;
    }

    if (m_page == Page::CustomLevels) {
        const std::vector<UiMenuItem> rows = buildCustomLevelItems();

        constexpr float kTop = 200.0f;
        constexpr float kRowHeight = 34.0f;
        const float rowsTop = kTop + 56.0f;
        const float panelHeight = (rowsTop + kRowHeight * static_cast<float>(rows.size()) + 30.0f) - kTop;
        constexpr float kPanelHalfW = 320.0f;

        UiRenderer::drawPanel(target, {centerX - kPanelHalfW, kTop},
                              {kPanelHalfW * 2.0f, panelHeight}, sf::Color(0, 0, 0, 200));
        UiRenderer::drawText(target, "CUSTOM LEVELS", {centerX, kTop + 20.0f}, 14,
                             sf::Color(120, 220, 160), true);
        UiRenderer::drawMenuItems(target, rows, m_customSelected,
                                  {centerX - 250.0f, rowsTop}, kRowHeight, 11,
                                  0.0f, m_elapsed, centerX + kPanelHalfW);
        // The directory, on the screen that lists the files: "where is it" has
        // to be answerable from outside the editor too.
        UiRenderer::drawShadowedText(target,
                                     toUpper(LevelCatalog::customDirectory()),
                                     {centerX, kTop + panelHeight + 18.0f}, 10,
                                     sf::Color(190, 190, 190), true);
        UiRenderer::drawShadowedText(target, "ENTER  PLAY      ESC  BACK",
                                     {centerX, kTop + panelHeight + 36.0f}, 11,
                                     sf::Color(220, 220, 220), true);
        return;
    }

    if (m_page == Page::Load) {
        const std::vector<UiMenuItem> rows = buildLoadItems();

        constexpr float kTop = 220.0f;
        constexpr float kRowHeight = 38.0f;
        const float rowsTop = kTop + 56.0f;
        const float panelHeight = (rowsTop + kRowHeight * static_cast<float>(rows.size()) + 30.0f) - kTop;

        constexpr float kPanelHalfW = 300.0f;

        UiRenderer::drawPanel(target, {centerX - kPanelHalfW, kTop},
                              {kPanelHalfW * 2.0f, panelHeight}, sf::Color(0, 0, 0, 200));
        UiRenderer::drawText(target, "LOAD GAME", {centerX, kTop + 20.0f}, 14,
                             sf::Color(120, 220, 160), true);
        // No explicit value column: the labels are "SLOT n" and "BACK", so
        // letting drawMenuItems derive the column from the widest of them hands
        // the summary every pixel the panel has. The old hardcoded column at
        // centerX+10 left it 290px for a string that is never shorter than 31
        // characters and typically 35-37 — it printed 81-153px out through the
        // right border of the frame on every save that existed.
        UiRenderer::drawMenuItems(target, rows, m_loadSelected,
                                  {centerX - 230.0f, rowsTop}, kRowHeight, 12,
                                  0.0f, m_elapsed, centerX + kPanelHalfW);
        // The delete key is named on the screen it works on, because a key with
        // no row of its own is invisible otherwise.
        UiRenderer::drawShadowedText(target,
                                     "ENTER  LOAD      X  DELETE SAVE      ESC  BACK",
                                     {centerX, kTop + panelHeight + 18.0f}, 11,
                                     sf::Color(220, 220, 220), true);
        return;
    }

    if (m_page == Page::LoadDelete) {
        constexpr float kTop = 240.0f;
        constexpr float kPanelHalfW = 300.0f;
        constexpr float kPanelHeight = 210.0f;

        // Red border: the only page in this menu that destroys anything should
        // not look like the ones that do not.
        UiRenderer::drawPanel(target, {centerX - kPanelHalfW, kTop},
                              {kPanelHalfW * 2.0f, kPanelHeight},
                              sf::Color(0, 0, 0, 225), sf::Color(255, 120, 120, 240));
        UiRenderer::drawText(target,
                             "DELETE SLOT " + std::to_string(m_pendingDeleteSlot) + "?",
                             {centerX, kTop + 22.0f}, 16, sf::Color(255, 140, 140), true);

        // Name the run being erased. "Are you sure?" alone does not tell the
        // player which campaign they are about to lose.
        const std::size_t index = (m_pendingDeleteSlot >= 1 && m_pendingDeleteSlot <= 3)
                                ? static_cast<std::size_t>(m_pendingDeleteSlot - 1) : 0;
        UiRenderer::drawTextFitted(target, m_slotPreviews[index].summary(),
                                   {centerX, kTop + 54.0f}, 12, sf::Color(255, 255, 255),
                                   kPanelHalfW * 2.0f - 44.0f, true);
        UiRenderer::drawText(target, "THIS CANNOT BE UNDONE", {centerX, kTop + 78.0f}, 10,
                             sf::Color(200, 200, 200), true);

        std::vector<UiMenuItem> rows;
        rows.emplace_back("KEEP IT");
        rows.emplace_back("DELETE");
        UiRenderer::drawMenuItems(target, rows, m_deleteConfirmSelected,
                                  {centerX - 90.0f, kTop + 108.0f}, 32.0f, 14,
                                  0.0f, m_elapsed, centerX + kPanelHalfW);

        UiRenderer::drawShadowedText(target, "ENTER  CONFIRM      ESC  CANCEL",
                                     {centerX, kTop + kPanelHeight + 18.0f}, 11,
                                     sf::Color(220, 220, 220), true);
        return;
    }

    // --- Procedural generator submenu ---
    std::vector<UiMenuItem> rows;
    rows.emplace_back("THEME",       kThemes[m_selectedThemeIdx]);
    rows.emplace_back("DIFFICULTY",  kDifficulties[m_selectedDifficultyIdx]);
    rows.emplace_back("PITS",        percent(m_generatorConfig.pitProbability));
    rows.emplace_back("PIPES",       percent(m_generatorConfig.pipeFrequency));
    rows.emplace_back("ENEMIES",     percent(m_generatorConfig.enemySpawnRate));
    rows.emplace_back("COINS",       percent(m_generatorConfig.coinClusterRate));
    rows.emplace_back("GENERATE & PLAY");
    rows.emplace_back("GENERATE & EDIT");
    rows.emplace_back("PLAY ENDLESS");
    rows.emplace_back("BACK");

    // Height derived from the row count, like the other three pages. This was
    // the last page still hardcoding 400px: ten rows of 36px from y=262 end at
    // 599 against a panel bottom of 600 — one pixel of clearance — and the hint
    // line at y=570 was drawn *inside* the frame, between rows 9 and 10. Adding
    // an eleventh generator row would have repeated the main-menu overflow that
    // the comment at the top of this function records.
    constexpr float kTop = 200.0f;
    constexpr float kPanelHalfW = 280.0f;
    constexpr float kRowHeight = 36.0f;
    const float rowsTop = kTop + 62.0f;
    const float panelHeight =
        (rowsTop + kRowHeight * static_cast<float>(rows.size()) + 26.0f) - kTop;

    UiRenderer::drawPanel(target, {centerX - kPanelHalfW, kTop},
                          {kPanelHalfW * 2.0f, panelHeight}, sf::Color(0, 0, 0, 200));
    UiRenderer::drawText(target, "PROCEDURAL GENERATOR", {centerX, kTop + 20.0f}, 14,
                         sf::Color(120, 200, 255), true);
    UiRenderer::drawMenuItems(target, rows, m_genSelected,
                              {centerX - 210.0f, rowsTop}, kRowHeight, 13,
                              centerX + 110.0f, m_elapsed, centerX + kPanelHalfW);
    UiRenderer::drawShadowedText(target, "LEFT/RIGHT  ADJUST      ESC  BACK",
                                 {centerX, kTop + panelHeight + 18.0f}, 11,
                                 sf::Color(220, 220, 220), true);
}

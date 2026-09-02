#include "Core/DebugConsole.hpp"
#include "Core/Game.hpp"
#include "Core/EventBus.hpp"
#include "Core/GameSnapshot.hpp"
#include "Core/PlayingState.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/Player.hpp"
#include "Utils/CampaignProgress.hpp"
#include "Utils/LevelCatalog.hpp"
#include "Utils/MetaGame.hpp"
#include "Core/ReplayRecorder.hpp"
#include "Utils/SerializationUtils.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstring>
#include <sstream>

namespace {

// Small helpers shared by the commands.
Player* player() { return Game::getInstance().getPlayer(); }

std::string requirePlayer() { return "no active player - start a level first"; }

int parseInt(const std::string& text, int fallback) {
    try {
        return std::stoi(text);
    } catch (...) {
        return fallback;
    }
}

float parseFloat(const std::string& text, float fallback) {
    try {
        return std::stof(text);
    } catch (...) {
        return fallback;
    }
}

// --- The commands -----------------------------------------------------------

class HelpCommand : public IConsoleCommand {
public:
    std::string name() const override { return "help"; }
    std::string help() const override { return "help - list every command"; }
    std::string execute(const std::vector<std::string>&) override {
        std::ostringstream out;
        out << "commands:";
        for (const std::string& command : DebugConsole::getInstance().commandNames()) {
            out << " " << command;
        }
        return out.str();
    }
};

class GiveCommand : public IConsoleCommand {
public:
    std::string name() const override { return "give"; }
    std::string help() const override {
        return "give <mushroom|fire|cape|mini|star|mega> - apply a power-up";
    }
    std::string execute(const std::vector<std::string>& args) override {
        if (!player()) return requirePlayer();
        if (args.empty()) return help();

        // Indices match Player::powerUp's itemType switch.
        static const std::vector<std::pair<std::string, int>> kItems = {
            {"mushroom", 0}, {"fire", 1}, {"cape", 2},
            {"mini", 3}, {"star", 4}, {"mega", 5}
        };
        for (const auto& [itemName, itemType] : kItems) {
            if (args[0] == itemName) {
                player()->powerUp(itemType);
                return "gave " + itemName;
            }
        }
        return "unknown power-up \"" + args[0] + "\"";
    }
};

class LivesCommand : public IConsoleCommand {
public:
    std::string name() const override { return "lives"; }
    std::string help() const override { return "lives <n> - set the life count"; }
    std::string execute(const std::vector<std::string>& args) override {
        if (!player()) return requirePlayer();
        if (args.empty()) return "lives: " + std::to_string(player()->getLives());

        const int wanted = std::max(1, parseInt(args[0], player()->getLives()));
        // restoreStats is the silent path — addCoins/loseLife would publish
        // events and inflate the statistics and achievements (audit A-3).
        player()->restoreStats(wanted, player()->getCoins(), player()->getScore());
        return "lives set to " + std::to_string(wanted);
    }
};

class TeleportCommand : public IConsoleCommand {
public:
    std::string name() const override { return "tp"; }
    std::string help() const override { return "tp <x> <y> - move the player, in pixels"; }
    std::string execute(const std::vector<std::string>& args) override {
        if (!player()) return requirePlayer();
        if (args.size() < 2) return help();

        const sf::Vector2f target{parseFloat(args[0], player()->getPosition().x),
                                  parseFloat(args[1], player()->getPosition().y)};
        player()->setPosition(target);
        player()->setVelocity({0.0f, 0.0f});
        return "teleported to " + args[0] + ", " + args[1];
    }
};

class GodCommand : public IConsoleCommand {
public:
    std::string name() const override { return "god"; }
    std::string help() const override {
        return "god - toggle Debug > Cheats' IMMORTAL + INVINCIBLE together";
    }
    std::string execute(const std::vector<std::string>&) override {
        // This used to set invincibilityTimer to 100000, which is not a second
        // source of truth so much as a second, worse answer: it guarded only
        // Player::takeDamage's i-frame check, so a pit, the level clock and a
        // crush all still ended the run, and the same field drives the hurt
        // animation. It now flips the very switches the Cheats panel does, so
        // the panel and the console cannot disagree about whether god mode is
        // on — and toggling one visibly moves the other's checkbox.
        DebugCheats& cheats = Game::getInstance().debugCheats();
        if (!cheats.isArmed()) {
            return "cheats are off - turn on OPTIONS > DEBUG MODE first";
        }

        const bool wasOn = cheats.rescueInsteadOfKill() && cheats.blocksDamage();
        cheats.set(DebugCheats::Cheat::Immortal, !wasOn);
        cheats.set(DebugCheats::Cheat::Invincible, !wasOn);
        return wasOn ? "god mode off" : "god mode on (immortal + invincible)";
    }
};

class SpawnCommand : public IConsoleCommand {
public:
    std::string name() const override { return "spawn"; }
    std::string help() const override { return "spawn <type> - spawn an entity on the player"; }
    std::string execute(const std::vector<std::string>& args) override {
        if (!player()) return requirePlayer();
        if (args.empty()) return help();

        const EntityType type = SerializationUtils::parseEntityTypeName(args[0]);
        // Goomba is parseEntityTypeName's fallback, so an unrecognised name would
        // otherwise silently spawn one.
        if (type == EntityType::Goomba && args[0] != "goomba") {
            return "unknown entity \"" + args[0] + "\"";
        }

        // Routed through the same request the game's own spawners use, so the
        // console needs no handle on the entity list.
        EntitySpawnRequest request;
        request.type = static_cast<int>(type);
        request.position = player()->getPosition() + sf::Vector2f(64.0f, -32.0f);
        EventBus::getInstance().publish({EventType::EntitySpawnRequested, request});
        return "spawned " + args[0];
    }
};

class DifficultyCommand : public IConsoleCommand {
public:
    std::string name() const override { return "difficulty"; }
    std::string help() const override { return "difficulty [easy|normal|hard] - get or set"; }
    std::string execute(const std::vector<std::string>& args) override {
        Game& game = Game::getInstance();
        if (args.empty()) return "difficulty: " + game.getDifficulty();

        game.setDifficulty(args[0]);
        // setDifficulty falls back to normal for anything unrecognised, so
        // report what actually took effect rather than what was asked for.
        return "difficulty is now " + game.difficulty().getId();
    }
};

class LevelCommand : public IConsoleCommand {
public:
    std::string name() const override { return "level"; }
    std::string help() const override { return "level <0-6> - jump straight to a campaign level"; }
    std::string execute(const std::vector<std::string>& args) override {
        if (args.empty()) {
            std::ostringstream out;
            out << "levels:";
            for (int i = 0; i < LevelCatalog::count(); ++i) {
                out << " " << i << "=" << LevelCatalog::nameFor(i);
            }
            return out.str();
        }

        const int index = parseInt(args[0], -1);
        if (!LevelCatalog::isValidIndex(index)) return "no such level: " + args[0];

        Game::getInstance().changeState(std::make_unique<PlayingState>(
            false, false, MapGeneratorConfig(), 0, index));
        return "loading " + LevelCatalog::nameFor(index);
    }
};

class ProgressCommand : public IConsoleCommand {
public:
    std::string name() const override { return "progress"; }
    std::string help() const override { return "progress - campaign, star coins and New Game+"; }
    std::string execute(const std::vector<std::string>&) override {
        const std::vector<LevelProgress> progress = CampaignProgress::load();
        std::ostringstream out;
        out << "NG+" << MetaGame::newGamePlusLevel()
            << "  stars " << CampaignProgress::totalStarCoins()
            << "/" << LevelCatalog::count() * 3 << "  cleared:";
        for (std::size_t i = 0; i < progress.size(); ++i) {
            if (progress[i].completed) out << " " << LevelCatalog::nameFor(static_cast<int>(i));
        }
        return out.str();
    }
};

class ReplayCommand : public IConsoleCommand {
public:
    std::string name() const override { return "replay"; }
    std::string help() const override {
        return "replay <rec|stop|save NAME|load NAME|play|list> - record and watch a run";
    }
    std::string execute(const std::vector<std::string>& args) override {
        ReplayRecorder& replay = ReplayRecorder::getInstance();
        if (args.empty()) {
            return "frames: " + std::to_string(replay.frameCount()) +
                   (replay.isRecording() ? " (recording)" : "") +
                   (replay.isPlaying() ? " (playing)" : "");
        }

        const std::string& verb = args[0];
        if (verb == "rec") {
            replay.startRecording(args.size() > 1 ? args[1] : "manual");
            return "recording";
        }
        if (verb == "stop") {
            replay.stopRecording();
            replay.stopPlayback();
            return "stopped at " + std::to_string(replay.frameCount()) + " frames";
        }
        if (verb == "save") {
            if (args.size() < 2) return "replay save <name>";
            return replay.save(args[1]) ? "saved " + args[1] : "nothing to save";
        }
        if (verb == "load") {
            if (args.size() < 2) return "replay load <name>";
            return replay.load(args[1])
                ? "loaded " + std::to_string(replay.frameCount()) + " frames"
                : "no replay called " + args[1];
        }
        if (verb == "play") {
            return replay.startPlayback()
                ? "playing back " + std::to_string(replay.frameCount()) + " frames"
                : "nothing recorded or loaded";
        }
        if (verb == "list") {
            const std::vector<std::string> names = ReplayRecorder::list();
            if (names.empty()) return "no saved replays";
            std::ostringstream out;
            out << "replays:";
            for (const std::string& n : names) out << " " << n;
            return out.str();
        }
        return help();
    }
};

class ClearCommand : public IConsoleCommand {
public:
    std::string name() const override { return "clear"; }
    std::string help() const override { return "clear - empty the console output"; }
    std::string execute(const std::vector<std::string>&) override {
        DebugConsole::getInstance().clearOutput();
        return "";
    }
};

// Tab-completion for the input box (task 10.4 / R7 audit — the command parser
// and commandNames() already existed; nothing in draw() ever called Tab a
// completion request). ImGui's InputText callback wants a plain function
// pointer, so this lives outside the class rather than as a member; it only
// needs commandNames() and the printNote() passthrough, both public.
//
// Only the command name (the first word) is completed. Once a space appears
// the rest is argument text with no fixed vocabulary — "give star", "tp 100
// 200" — so completing past the first word would have to guess.
int DebugConsoleCompletionCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag != ImGuiInputTextFlags_CallbackCompletion) return 0;

    const std::string current(data->Buf, static_cast<std::size_t>(data->BufTextLen));
    if (current.empty() || current.find(' ') != std::string::npos) return 0;

    std::vector<std::string> matches;
    for (const std::string& name : DebugConsole::getInstance().commandNames()) {
        if (name.compare(0, current.size(), current) == 0) matches.push_back(name);
    }
    if (matches.empty()) return 0;

    std::string completion = matches.front();
    if (matches.size() > 1) {
        // Multiple candidates ("s" -> "spawn", "stage"...): complete only as
        // far as every match agrees (the longest common prefix), and list the
        // rest rather than silently guessing between them.
        for (const std::string& match : matches) {
            std::size_t i = 0;
            while (i < completion.size() && i < match.size() && completion[i] == match[i]) ++i;
            completion.resize(i);
        }
        std::string listing = "matches:";
        for (const std::string& match : matches) listing += " " + match;
        DebugConsole::getInstance().printNote(listing);
    } else {
        completion += ' '; // A single match completes straight to its arguments.
    }

    if (completion.size() > current.size()) {
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, completion.c_str());
    }
    return 0;
}

} // namespace

DebugConsole& DebugConsole::getInstance() {
    static DebugConsole instance;
    return instance;
}

void DebugConsole::registerCommand(std::unique_ptr<IConsoleCommand> command) {
    if (command) m_commands.push_back(std::move(command));
}

void DebugConsole::init() {
    if (m_initialised) return;
    m_initialised = true;

    registerCommand(std::make_unique<HelpCommand>());
    registerCommand(std::make_unique<GiveCommand>());
    registerCommand(std::make_unique<LivesCommand>());
    registerCommand(std::make_unique<TeleportCommand>());
    registerCommand(std::make_unique<GodCommand>());
    registerCommand(std::make_unique<SpawnCommand>());
    registerCommand(std::make_unique<DifficultyCommand>());
    registerCommand(std::make_unique<LevelCommand>());
    registerCommand(std::make_unique<ProgressCommand>());
    registerCommand(std::make_unique<ReplayCommand>());
    registerCommand(std::make_unique<ClearCommand>());

    print("debug console ready - type help");
}

std::vector<std::string> DebugConsole::commandNames() const {
    std::vector<std::string> names;
    names.reserve(m_commands.size());
    for (const auto& command : m_commands) names.push_back(command->name());
    std::sort(names.begin(), names.end());
    return names;
}

void DebugConsole::print(const std::string& line) {
    if (line.empty()) return;
    m_output.push_back(line);
    // Bounded: a console that grows forever is a leak with a scrollbar.
    if (m_output.size() > 200) {
        m_output.erase(m_output.begin(), m_output.begin() + 50);
    }
}

std::string DebugConsole::submit(const std::string& line) {
    if (line.empty()) return "";

    m_history.push_back(line);
    m_historyCursor = -1;
    print("> " + line);

    std::istringstream stream(line);
    std::string name;
    stream >> name;

    std::vector<std::string> args;
    std::string arg;
    while (stream >> arg) args.push_back(arg);

    for (const auto& command : m_commands) {
        if (command->name() == name) {
            const std::string result = command->execute(args);
            print(result);
            return result;
        }
    }

    const std::string unknown = "unknown command \"" + name + "\" - try help";
    print(unknown);
    return unknown;
}

void DebugConsole::draw() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(640.0f, 320.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug Console", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::BeginChild("output", ImVec2(0.0f, -30.0f), true);
    for (const std::string& line : m_output) {
        ImGui::TextUnformatted(line.c_str());
    }
    // Follow the tail, so the newest line is the one you can see.
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    static char input[256] = {0};
    ImGui::PushItemWidth(-1.0f);
    if (ImGui::InputText("##console_input", input, sizeof(input),
                         ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion,
                         &DebugConsoleCompletionCallback)) {
        submit(input);
        input[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }
    ImGui::PopItemWidth();

    ImGui::End();
}

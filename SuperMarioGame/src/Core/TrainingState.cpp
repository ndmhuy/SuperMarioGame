#include "Core/TrainingState.hpp"

#include "Core/Game.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/BorrowedPolicy.hpp"
#include "Entities/HeuristicPolicy.hpp"
#include "Entities/Mario.hpp"
#include "Entities/NeuralPolicy.hpp"
#include "Entities/Player.hpp"
#include "Graphics/UiRenderer.hpp"
#include "Utils/Constants.hpp"
#include "Utils/LevelCatalog.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>

namespace {
// Checkpoints go beside the save data, not into assets/: they are produced by
// running the game, not shipped with it.
const char* kCheckpointPath = "saves/ai/policy.ckpt";
constexpr float kStuckSeconds = 4.0f;
}

TrainingState::TrainingState(int levelIndex) {
    m_levelPath = LevelCatalog::isValidIndex(levelIndex)
                      ? LevelCatalog::pathFor(levelIndex)
                      : LevelCatalog::pathFor(0);
}

TrainingState::~TrainingState() = default;

void TrainingState::enter() {
    // Silent by default: a training run is dozens of episodes a minute, and
    // every stomp and coin firing its sound effect is unusable to sit next to.
    SoundManager::getInstance().setSFXVolume(0.0f);
    SoundManager::getInstance().setMusicVolume(0.0f);

    m_policy = std::make_unique<NeuralPolicy>();
    // Resume if a previous run left a checkpoint. Training that silently starts
    // from scratch every launch is how hours get thrown away.
    if (m_policy->load(kCheckpointPath)) {
        std::cout << "[Training] Resumed from " << kCheckpointPath << std::endl;
    }
    m_trainer = std::make_unique<PolicyTrainer>(*m_policy);
    m_trainer->openLog("saves/ai/training_log.csv");
    m_teacherPolicy = std::make_unique<HeuristicPolicy>(AIArchetype::Speedrunner);

    m_camera.setLookahead(140.0f);
    startEpisode();
}

void TrainingState::exit() {
    // The directory is not in the repo — checkpoints are produced by running,
    // not shipped — so it has to be created before the first save rather than
    // failing on it.
    std::error_code ignored;
    std::filesystem::create_directories("saves/ai", ignored);
    if (m_policy && m_policy->saveCheckpoint(kCheckpointPath)) {
        std::cout << "[Training] Saved " << kCheckpointPath << " after "
                  << (m_trainer ? m_trainer->episodes() : 0) << " episodes."
                  << std::endl;
    }
    Game::getInstance().setPlayer(nullptr);
    Game::getInstance().setTileMap(nullptr);
    m_entities.clear();
}

void TrainingState::startEpisode() {
    m_entities.clear();
    m_levelData = LevelData{};
    if (!m_loader.loadLevel(m_levelPath, m_tileMap, m_levelData)) {
        std::cerr << "[Training] Could not load " << m_levelPath << std::endl;
        return;
    }
    m_entities = std::move(m_levelData.entities);

    auto owned = std::make_unique<Mario>(m_levelData.spawnPoint);
    m_player = owned.get();
    m_entities.insert(m_entities.begin(), std::move(owned));

    // Non-optional: seventeen entity and strategy files reach for
    // Game::getInstance().getNearestPlayer() or getTileMap() from inside their
    // update. Without these, every enemy stands still and the agent trains
    // against a world with no threats in it.
    Game::getInstance().setPlayer(m_player);
    Game::getInstance().setTileMap(&m_tileMap);

    m_agent = std::make_unique<AIController>(*m_player, AIDifficulty::Hard,
                                             AIArchetype::Speedrunner);
    // Who drives this episode. Early on the teacher does, so the learner sees
    // competent play; as beta decays the learner drives and the teacher is
    // demoted to labelling whatever states the learner blunders into — which is
    // the entire point of DAgger over plain behavioural cloning.
    m_teacherDriving = m_trainer->teacherDrives();
    IAIPolicy& driver = m_teacherDriving
                            ? static_cast<IAIPolicy&>(*m_teacherPolicy)
                            : static_cast<IAIPolicy&>(*m_policy);
    m_agent->setPolicy(std::make_unique<BorrowedPolicy>(driver));
    m_teacherPolicy->reset();

    m_camera.setBounds(AABB{0.0f, 0.0f,
                            m_tileMap.getWidth() * Constants::TILE_SIZE,
                            m_tileMap.getHeight() * Constants::TILE_SIZE});
    m_camera.snapTo(m_player->getPosition());

    m_episodeTime = 0.0f;
    m_furthestX = m_player->getPosition().x;
    m_stallTime = 0.0f;
}

void TrainingState::finishEpisode(const char* reason) {
    m_lastOutcome = reason;
    m_trainer->endEpisode(reason);
    // Checkpoint periodically, not only on exit: a run left going for an hour
    // should not lose everything to a crash or a force-quit.
    if (m_trainer->episodes() % 25 == 0) {
        std::error_code ignored;
        std::filesystem::create_directories("saves/ai", ignored);
        m_policy->saveCheckpoint(kCheckpointPath);
    }
    startEpisode();
}

void TrainingState::update(float dt) {
    m_blinkPhase += dt;
    if (m_paused || !m_player || !m_agent) return;

    for (int step = 0; step < m_stepsPerFrame; ++step) {
        const float h = Constants::FIXED_TIMESTEP;

        m_agent->update(h, nullptr, m_tileMap, m_entities);

        // The supervision label for the state the agent just saw. When the
        // teacher is driving, the controller already computed exactly this, and
        // asking the heuristic again would advance its commit and escape
        // counters a second time — changing the very behaviour being copied.
        const AIObservation& observation = m_agent->lastObservation();
        const AIAction teacherAction = m_teacherDriving
                                           ? m_agent->lastAction()
                                           : m_teacherPolicy->decide(observation);
        m_trainer->learn(observation, teacherAction);

        for (auto& entity : m_entities) {
            if (entity && entity->isActive()) entity->update(h);
        }
        m_physics.update(m_entities, m_tileMap, h);
        m_entities.erase(
            std::remove_if(m_entities.begin(), m_entities.end(),
                           [this](const std::unique_ptr<Entity>& e) {
                               return !e || (e.get() != m_player && !e->isActive());
                           }),
            m_entities.end());

        m_episodeTime += h;

        const sf::Vector2f position = m_player->getPosition();
        if (position.x > m_furthestX) {
            m_furthestX = position.x;
            m_stallTime = 0.0f;
        } else {
            m_stallTime += h;
        }

        const float bottomVoid = m_tileMap.getHeight() * Constants::TILE_SIZE + 32.0f;
        if (position.y > bottomVoid || m_player->isDying() || m_player->getLives() <= 0) {
            finishEpisode("died");
            return;
        }
        if (m_episodeTime >= m_maxEpisodeSeconds) { finishEpisode("timeout"); return; }
        if (m_stallTime >= kStuckSeconds)          { finishEpisode("stuck");   return; }
        if (position.x >= (m_tileMap.getWidth() - 4) * Constants::TILE_SIZE) {
            ++m_completions;
            finishEpisode("reached the end");
            return;
        }
    }

    m_camera.follow(m_player->getPosition(), Constants::FIXED_TIMESTEP);
    m_camera.update(Constants::FIXED_TIMESTEP);
}

void TrainingState::handleInput(const sf::Event& event) {
    const auto* key = event.getIf<sf::Event::KeyPressed>();
    if (!key) return;

    switch (key->code) {
        case sf::Keyboard::Key::Escape:
            // Checkpoint lands in exit().
            Game::getInstance().popState();
            break;
        case sf::Keyboard::Key::Space:
            m_paused = !m_paused;
            break;
        case sf::Keyboard::Key::Equal:
        case sf::Keyboard::Key::Add:
            // Trade watchability for throughput.
            m_stepsPerFrame = std::min(m_stepsPerFrame * 2, 64);
            break;
        case sf::Keyboard::Key::Hyphen:
        case sf::Keyboard::Key::Subtract:
            m_stepsPerFrame = std::max(m_stepsPerFrame / 2, 1);
            break;
        default:
            break;
    }
}

void TrainingState::render(sf::RenderTarget& target) {
    target.setView(m_camera.getView());
    m_tileMap.render(target, m_camera);
    for (const auto& entity : m_entities) {
        if (entity && entity->isActive()) entity->render(target);
    }

    target.setView(target.getDefaultView());
    renderOverlay(target);
}

void TrainingState::renderCurve(sf::RenderTarget& target, const std::vector<float>& series,
                                sf::Vector2f topLeft, sf::Vector2f size, sf::Color color,
                                float fixedMax) const {
    sf::RectangleShape frame(size);
    frame.setPosition(topLeft);
    frame.setFillColor(sf::Color(0, 0, 0, 120));
    frame.setOutlineColor(sf::Color(255, 255, 255, 60));
    frame.setOutlineThickness(1.0f);
    target.draw(frame);

    if (series.size() < 2) return;

    // Autoscale unless the series has a natural ceiling (agreement is a
    // fraction, so it is pinned to 1.0 — autoscaling it would make 3% noise
    // look like dramatic progress).
    float maxValue = fixedMax;
    if (maxValue <= 0.0f) {
        maxValue = *std::max_element(series.begin(), series.end());
        if (maxValue <= 0.0f) maxValue = 1.0f;
    }

    // One column per sample, oldest at the left. Drawn as thin bars rather than
    // a polyline because SFML has no primitive line width and a 1px line is
    // hard to read against the level behind it.
    const std::size_t count = series.size();
    const float columnWidth = size.x / static_cast<float>(count);
    for (std::size_t i = 0; i < count; ++i) {
        const float value = std::clamp(series[i] / maxValue, 0.0f, 1.0f);
        const float height = value * (size.y - 2.0f);
        sf::RectangleShape bar(sf::Vector2f(std::max(columnWidth, 1.0f), height));
        bar.setPosition({topLeft.x + static_cast<float>(i) * columnWidth,
                         topLeft.y + size.y - height - 1.0f});
        bar.setFillColor(color);
        target.draw(bar);
    }
}

void TrainingState::renderOverlay(sf::RenderTarget& target) {
    if (!m_trainer) return;

    const float panelWidth = 300.0f;
    const float x = Constants::WINDOW_WIDTH - panelWidth - 12.0f;
    UiRenderer::drawPanel(target, {x, 12.0f}, {panelWidth, 430.0f});

    const float left = x + 14.0f;
    float y = 24.0f;

    UiRenderer::drawText(target, "TRAINING", {left, y}, 16, sf::Color::Yellow);
    y += 26.0f;

    const char* driver = m_teacherDriving ? "TEACHER" : "LEARNER";
    UiRenderer::drawText(target, std::string("driving: ") + driver, {left, y}, 11,
                         m_teacherDriving ? sf::Color(150, 200, 255)
                                          : sf::Color(150, 255, 150));
    y += 18.0f;

    char line[128];
    std::snprintf(line, sizeof(line), "episode %d   beta %.2f",
                  m_trainer->episodes(), m_trainer->beta());
    UiRenderer::drawText(target, line, {left, y}, 11, sf::Color::White);
    y += 18.0f;

    std::snprintf(line, sizeof(line), "samples %zu", m_trainer->samples());
    UiRenderer::drawText(target, line, {left, y}, 11, sf::Color::White);
    y += 18.0f;

    std::snprintf(line, sizeof(line), "loss %.4f", m_trainer->lastLoss());
    UiRenderer::drawText(target, line, {left, y}, 11, sf::Color(255, 180, 120));
    y += 18.0f;

    std::snprintf(line, sizeof(line), "agreement %.1f%%",
                  m_trainer->episodeAgreement() * 100.0f);
    UiRenderer::drawText(target, line, {left, y}, 11, sf::Color(150, 255, 150));
    y += 22.0f;

    // Agreement is the metric that answers "is it learning"; loss only answers
    // "is the optimiser healthy". Both are shown because they fail separately.
    UiRenderer::drawText(target, "agreement / episode", {left, y}, 9,
                         sf::Color(180, 180, 180));
    y += 12.0f;
    renderCurve(target, m_trainer->agreementHistory(), {left, y},
                {panelWidth - 28.0f, 60.0f}, sf::Color(120, 220, 120), 1.0f);
    y += 68.0f;

    UiRenderer::drawText(target, "loss / episode", {left, y}, 9,
                         sf::Color(180, 180, 180));
    y += 12.0f;
    renderCurve(target, m_trainer->lossHistory(), {left, y},
                {panelWidth - 28.0f, 60.0f}, sf::Color(230, 150, 90), 0.0f);
    y += 70.0f;

    // Per-button outputs. This is where a network that will not jump shows a
    // 0.49 where a 1.0 belongs — invisible in the loss, obvious here.
    static const char* kButtons[] = {"L", "R", "JMP", "RUN", "DWN", "FIR", "GP"};
    UiRenderer::drawText(target, "button outputs", {left, y}, 9,
                         sf::Color(180, 180, 180));
    y += 14.0f;
    const std::vector<float>& prediction = m_trainer->lastPrediction();
    const std::vector<float> perButton = m_trainer->buttonAgreement();
    for (std::size_t i = 0; i < prediction.size() && i < 7; ++i) {
        const float value = std::clamp(prediction[i], 0.0f, 1.0f);
        // Label carries this button's own agreement. One aggregate number hid a
        // total jump failure behind six well-learned buttons for 219 episodes.
        char label[24];
        std::snprintf(label, sizeof(label), "%-3s %2.0f%%", kButtons[i],
                      (i < perButton.size() ? perButton[i] : 0.0f) * 100.0f);
        UiRenderer::drawText(target, label, {left, y + static_cast<float>(i) * 14.0f},
                             9, sf::Color(200, 200, 200));

        sf::RectangleShape track({panelWidth - 110.0f, 8.0f});
        track.setPosition({left + 60.0f, y + static_cast<float>(i) * 14.0f + 2.0f});
        track.setFillColor(sf::Color(255, 255, 255, 30));
        target.draw(track);

        sf::RectangleShape fill({(panelWidth - 110.0f) * value, 8.0f});
        fill.setPosition({left + 60.0f, y + static_cast<float>(i) * 14.0f + 2.0f});
        // Coloured by the decision the threshold will actually make, so the
        // bar reads as "this button is pressed" rather than as a raw number.
        fill.setFillColor(value > 0.5f ? sf::Color(120, 220, 120)
                                       : sf::Color(200, 120, 120));
        target.draw(fill);
    }
    y += 7 * 14.0f + 6.0f;

    std::snprintf(line, sizeof(line), "last: %s   speed x%d", m_lastOutcome,
                  m_stepsPerFrame);
    UiRenderer::drawText(target, line, {left, y}, 9, sf::Color(180, 180, 180));

    if (m_paused) {
        UiRenderer::drawText(target, "PAUSED", {Constants::WINDOW_WIDTH * 0.5f, 60.0f},
                             24, sf::Color::Yellow, true);
    }

    UiRenderer::drawText(target,
                         "SPACE pause   +/- speed   ESC save & exit",
                         {Constants::WINDOW_WIDTH * 0.5f, Constants::WINDOW_HEIGHT - 26.0f},
                         10, sf::Color(200, 200, 200), true);
}

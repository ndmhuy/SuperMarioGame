#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <SFML/System/Vector2.hpp>
#include "Graphics/SpriteSheet.hpp"
#include "Graphics/Animation.hpp"
#include "Graphics/Animator.hpp"
#include "Core/ResourceManager.hpp"

// We define a helper macro for our assertions to provide clean test reporting
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[FAIL] Assert failed: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            std::exit(1); \
        } \
    } while (0)

int main() {
    std::cout << "[TEST] Starting Graphics & Animation Verification Suite..." << std::endl;

    // Determine the relative path to assets/spriteSheet/test directory
    std::string assetPath = "assets/spriteSheet/test";
    if (!std::filesystem::exists(assetPath + "/test.json")) {
        assetPath = "../assets/spriteSheet/test";
    }
    if (!std::filesystem::exists(assetPath + "/test.json")) {
        assetPath = "../../assets/spriteSheet/test";
    }
    if (!std::filesystem::exists(assetPath + "/test.json")) {
        assetPath = "SuperMarioGame/assets/spriteSheet/test";
    }
    if (!std::filesystem::exists(assetPath + "/test.json")) {
        assetPath = "../SuperMarioGame/assets/spriteSheet/test";
    }

    std::cout << "[TEST] Using asset path: " << assetPath << std::endl;
    TEST_ASSERT(std::filesystem::exists(assetPath + "/test.json"), "test.json must exist in asset path");
    TEST_ASSERT(std::filesystem::exists(assetPath + "/test.png"), "test.png must exist in asset path");

    // -------------------------------------------------------------
    // Test 1: Load SpriteSheet
    // -------------------------------------------------------------
    std::cout << "[TEST] Testing SpriteSheet loading..." << std::endl;
    SpriteSheet sheet(assetPath);

    // Get specific frames and verify their coordinates
    // "frame_00": x=1, y=1, w=32, h=48
    sf::Sprite sprite0 = sheet.getSprite("frame_00");
    sf::IntRect rect0 = sprite0.getTextureRect();
    std::cout << "frame_00: pos=(" << rect0.position.x << "," << rect0.position.y 
              << "), size=(" << rect0.size.x << "," << rect0.size.y << ")" << std::endl;
    TEST_ASSERT(rect0.position == sf::Vector2i(1, 1), "frame_00 x/y should be (1, 1)");
    TEST_ASSERT(rect0.size == sf::Vector2i(32, 48), "frame_00 size should be 32x48");

    // "frame_02": x=67, y=1, w=32, h=48
    sf::Sprite sprite2 = sheet.getSprite("frame_02");
    sf::IntRect rect2 = sprite2.getTextureRect();
    TEST_ASSERT(rect2.position == sf::Vector2i(67, 1), "frame_02 x/y should be (67, 1)");

    // "frame_04": x=34, y=50, w=32, h=48
    sf::Sprite sprite4 = sheet.getSprite("frame_04");
    sf::IntRect rect4 = sprite4.getTextureRect();
    TEST_ASSERT(rect4.position == sf::Vector2i(34, 50), "frame_04 x/y should be (34, 50)");

    // Query non-existent frame should return an empty rectangle
    sf::Sprite spriteNone = sheet.getSprite("non_existent_frame");
    TEST_ASSERT(spriteNone.getTextureRect().size == sf::Vector2i(0, 0), "non_existent_frame should have size 0x0");

    // -------------------------------------------------------------
    // Test 2: Animation & Looping Animator
    // -------------------------------------------------------------
    std::cout << "[TEST] Testing Looping Animator..." << std::endl;
    Animation loopingAnim("walk");
    loopingAnim.isLooping = true;
    loopingAnim.frameList.push_back({"frame_00", 0.1f});
    loopingAnim.frameList.push_back({"frame_01", 0.1f});
    loopingAnim.frameList.push_back({"frame_02", 0.2f});

    Animator animator(&sheet);
    animator.play(&loopingAnim);

    // Initial frame
    TEST_ASSERT(animator.getSprite().getTextureRect().position == sf::Vector2i(1, 1), "Initial frame should be frame_00");
    TEST_ASSERT(!animator.isDone(), "Looping animation is never done");

    // Advance time within first frame's duration (0.05s / 0.1s)
    animator.update(0.05f);
    TEST_ASSERT(animator.getSprite().getTextureRect().position == sf::Vector2i(1, 1), "Frame should still be frame_00");

    // Advance time past first frame's duration (additional 0.06s, total 0.11s) -> frame_01
    animator.update(0.06f);
    TEST_ASSERT(animator.getSprite().getTextureRect().position == sf::Vector2i(34, 1), "Frame should now be frame_01");

    // Advance time past second frame's duration (additional 0.1s, total 0.21s) -> frame_02
    animator.update(0.1f);
    TEST_ASSERT(animator.getSprite().getTextureRect().position == sf::Vector2i(67, 1), "Frame should now be frame_02");

    // Advance time past third frame's duration (additional 0.2f, total 0.41s) -> loops back to frame_00
    animator.update(0.2f);
    TEST_ASSERT(animator.getSprite().getTextureRect().position == sf::Vector2i(1, 1), "Frame should loop back to frame_00");

    // -------------------------------------------------------------
    // Test 3: Non-Looping Animator
    // -------------------------------------------------------------
    std::cout << "[TEST] Testing Non-Looping Animator..." << std::endl;
    Animation nonLoopingAnim("die");
    nonLoopingAnim.isLooping = false;
    nonLoopingAnim.frameList.push_back({"frame_00", 0.1f});
    nonLoopingAnim.frameList.push_back({"frame_01", 0.1f});

    animator.play(&nonLoopingAnim);
    TEST_ASSERT(!animator.isDone(), "Animation should not be done initially");
    TEST_ASSERT(animator.getSprite().getTextureRect().position == sf::Vector2i(1, 1), "Initial frame should be frame_00");

    // Advance to frame_01
    animator.update(0.15f);
    TEST_ASSERT(animator.getSprite().getTextureRect().position == sf::Vector2i(34, 1), "Frame should be frame_01");
    TEST_ASSERT(!animator.isDone(), "Animation should not be done yet");

    // Advance past end of animation
    animator.update(0.1f);
    TEST_ASSERT(animator.isDone(), "Animation should now be completed");
    TEST_ASSERT(animator.getSprite().getTextureRect().position == sf::Vector2i(34, 1), "Should lock on last frame when done");

    std::cout << "[SUCCESS] Graphics & Animation Verification Suite passed successfully!" << std::endl;
    return 0;
}

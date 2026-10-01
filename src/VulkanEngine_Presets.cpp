#include "VulkanEngine.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <array>
#include <memory>
#include <filesystem>
#include <algorithm>

using json = nlohmann::json;

#define VK_CHECK(x)                                                 \
    do {                                                            \
        VkResult err = x;                                           \
        if (err) {                                                  \
            std::cerr << "Detected Vulkan error: " << err << "\n";  \
            abort();                                                \
        }                                                           \
    } while (0)

// Applies a predefined particle simulation preset
void VulkanEngine::applyGlobalPreset(int presetIndex) {
    pushConstants.reset = 1;
    particleTypes.clear();
    
    // 23 Presets
    if (presetIndex == 0) { // Matrix
        pushConstants.timeSpeed = 1.0f;
        pushConstants.gravity = glm::vec2(0.0f, -0.5f);
        pushConstants.globalMovement = 1; // Downward
        ParticleTypeDef pt1; pt1.name = "Matrix Code 1"; pt1.color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f); pt1.shape = 1; pt1.size = 3.0f; pt1.lifeDuration = 4.0f; pt1.velocityProfile = 2; pt1.count = 30000;
        ParticleTypeDef pt2; pt2.name = "Matrix Code 2"; pt2.color = glm::vec4(0.0f, 0.8f, 0.0f, 1.0f); pt2.shape = 1; pt2.size = 2.0f; pt2.lifeDuration = 3.5f; pt2.velocityProfile = 2; pt2.count = 20000;
        ParticleTypeDef pt3; pt3.name = "Matrix Glitch"; pt3.color = glm::vec4(0.5f, 1.0f, 0.5f, 1.0f); pt3.shape = 1; pt3.size = 4.0f; pt3.lifeDuration = 2.0f; pt3.velocityProfile = 2; pt3.count = 5000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 1) { // Nature
        pushConstants.timeSpeed = 0.8f;
        pushConstants.gravity = glm::vec2(0.2f, -0.1f);
        pushConstants.globalMovement = 9; // Turbulence
        ParticleTypeDef pt1; pt1.name = "Leaves Green"; pt1.color = glm::vec4(0.2f, 0.8f, 0.2f, 1.0f); pt1.shape = 5; pt1.size = 4.0f; pt1.lifeDuration = 6.0f; pt1.velocityProfile = 0; pt1.count = 20000;
        ParticleTypeDef pt2; pt2.name = "Leaves Yellow"; pt2.color = glm::vec4(0.8f, 0.8f, 0.1f, 1.0f); pt2.shape = 5; pt2.size = 3.5f; pt2.lifeDuration = 5.0f; pt2.velocityProfile = 0; pt2.count = 10000;
        ParticleTypeDef pt3; pt3.name = "Pollen"; pt3.color = glm::vec4(1.0f, 1.0f, 0.5f, 0.6f); pt3.shape = 0; pt3.size = 1.0f; pt3.lifeDuration = 8.0f; pt3.velocityProfile = 0; pt3.count = 20000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 2) { // Tempete de neige
        pushConstants.timeSpeed = 1.5f;
        pushConstants.gravity = glm::vec2(0.5f, -0.8f);
        pushConstants.globalMovement = 9; // Turbulence
        ParticleTypeDef pt1; pt1.name = "Big Snowflake"; pt1.color = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f); pt1.shape = 0; pt1.size = 3.0f; pt1.lifeDuration = 6.0f; pt1.velocityProfile = 0; pt1.count = 20000;
        ParticleTypeDef pt2; pt2.name = "Medium Snowflake"; pt2.color = glm::vec4(0.9f, 0.9f, 1.0f, 0.6f); pt2.shape = 0; pt2.size = 2.0f; pt2.lifeDuration = 5.0f; pt2.velocityProfile = 0; pt2.count = 50000;
        ParticleTypeDef pt3; pt3.name = "Blizzard Dust"; pt3.color = glm::vec4(0.8f, 0.8f, 1.0f, 0.4f); pt3.shape = 0; pt3.size = 1.0f; pt3.lifeDuration = 4.0f; pt3.velocityProfile = 0; pt3.count = 80000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 3) { // Pluie d'acide
        pushConstants.timeSpeed = 2.0f;
        pushConstants.gravity = glm::vec2(0.1f, -1.5f);
        pushConstants.globalMovement = 1; // Downward
        ParticleTypeDef pt1; pt1.name = "Acid Drop"; pt1.color = glm::vec4(0.5f, 1.0f, 0.0f, 0.9f); pt1.shape = 1; pt1.size = 2.0f; pt1.lifeDuration = 3.0f; pt1.velocityProfile = 2; pt1.count = 50000;
        ParticleTypeDef pt2; pt2.name = "Toxic Mist"; pt2.color = glm::vec4(0.2f, 0.8f, 0.1f, 0.3f); pt2.shape = 0; pt2.size = 6.0f; pt2.lifeDuration = 4.0f; pt2.velocityProfile = 0; pt2.count = 20000;
        ParticleTypeDef pt3; pt3.name = "Heavy Drop"; pt3.color = glm::vec4(0.7f, 1.0f, 0.2f, 1.0f); pt3.shape = 1; pt3.size = 3.0f; pt3.lifeDuration = 2.5f; pt3.velocityProfile = 2; pt3.count = 10000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 4) { // Cyberpunk
        pushConstants.timeSpeed = 1.0f;
        pushConstants.gravity = glm::vec2(0.0f, 0.0f);
        pushConstants.globalMovement = 7; // Spiral
        ParticleTypeDef pt1; pt1.name = "Neon Pink"; pt1.color = glm::vec4(1.0f, 0.0f, 0.5f, 1.0f); pt1.shape = 2; pt1.size = 3.0f; pt1.lifeDuration = 4.0f; pt1.velocityProfile = 4; pt1.count = 30000;
        ParticleTypeDef pt2; pt2.name = "Cyan"; pt2.color = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); pt2.shape = 0; pt2.size = 2.0f; pt2.lifeDuration = 5.0f; pt2.velocityProfile = 0; pt2.count = 30000;
        ParticleTypeDef pt3; pt3.name = "Yellow Glitch"; pt3.color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); pt3.shape = 1; pt3.size = 1.5f; pt3.lifeDuration = 2.0f; pt3.velocityProfile = 4; pt3.count = 20000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 5) { // Blade runner
        pushConstants.timeSpeed = 0.5f;
        pushConstants.gravity = glm::vec2(0.0f, -0.2f);
        pushConstants.globalMovement = 8; // Wave
        ParticleTypeDef pt1; pt1.name = "Smog Orange"; pt1.color = glm::vec4(0.8f, 0.4f, 0.0f, 0.3f); pt1.shape = 0; pt1.size = 10.0f; pt1.lifeDuration = 8.0f; pt1.velocityProfile = 1; pt1.count = 30000;
        ParticleTypeDef pt2; pt2.name = "Smog Gray"; pt2.color = glm::vec4(0.3f, 0.3f, 0.3f, 0.2f); pt2.shape = 0; pt2.size = 15.0f; pt2.lifeDuration = 10.0f; pt2.velocityProfile = 1; pt2.count = 20000;
        ParticleTypeDef pt3; pt3.name = "Rain"; pt3.color = glm::vec4(0.7f, 0.8f, 1.0f, 0.5f); pt3.shape = 1; pt3.size = 2.0f; pt3.lifeDuration = 3.0f; pt3.velocityProfile = 2; pt3.count = 10000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 6) { // Sorcelerie
        pushConstants.timeSpeed = 1.2f;
        pushConstants.gravity = glm::vec2(0.0f, 0.5f);
        pushConstants.globalMovement = 2; // Vortex
        ParticleTypeDef pt1; pt1.name = "Purple Dust"; pt1.color = glm::vec4(0.5f, 0.0f, 0.8f, 1.0f); pt1.shape = 3; pt1.size = 4.0f; pt1.lifeDuration = 4.0f; pt1.velocityProfile = 4; pt1.count = 30000;
        ParticleTypeDef pt2; pt2.name = "Green Magic"; pt2.color = glm::vec4(0.0f, 0.9f, 0.3f, 0.8f); pt2.shape = 2; pt2.size = 3.0f; pt2.lifeDuration = 5.0f; pt2.velocityProfile = 4; pt2.count = 20000;
        ParticleTypeDef pt3; pt3.name = "Dark Energy"; pt3.color = glm::vec4(0.1f, 0.0f, 0.2f, 0.5f); pt3.shape = 0; pt3.size = 8.0f; pt3.lifeDuration = 6.0f; pt3.velocityProfile = 0; pt3.count = 15000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 7) { // Magie
        pushConstants.timeSpeed = 1.0f;
        pushConstants.gravity = glm::vec2(0.0f, 0.0f);
        pushConstants.globalMovement = 11; // Dipole
        ParticleTypeDef pt1; pt1.name = "Gold Sparkle"; pt1.color = glm::vec4(1.0f, 0.8f, 0.0f, 1.0f); pt1.shape = 3; pt1.size = 5.0f; pt1.lifeDuration = 3.0f; pt1.velocityProfile = 0; pt1.count = 20000;
        ParticleTypeDef pt2; pt2.name = "Silver Sparkle"; pt2.color = glm::vec4(0.9f, 0.9f, 1.0f, 1.0f); pt2.shape = 3; pt2.size = 4.0f; pt2.lifeDuration = 2.5f; pt2.velocityProfile = 0; pt2.count = 15000;
        ParticleTypeDef pt3; pt3.name = "Aura"; pt3.color = glm::vec4(0.4f, 0.6f, 1.0f, 0.3f); pt3.shape = 0; pt3.size = 12.0f; pt3.lifeDuration = 5.0f; pt3.velocityProfile = 0; pt3.count = 5000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 8) { // Dark fantasy
        pushConstants.timeSpeed = 0.7f;
        pushConstants.gravity = glm::vec2(0.0f, -0.1f);
        pushConstants.globalMovement = 3; // Black hole
        ParticleTypeDef pt1; pt1.name = "Deep Shadow"; pt1.color = glm::vec4(0.05f, 0.0f, 0.05f, 0.8f); pt1.shape = 0; pt1.size = 10.0f; pt1.lifeDuration = 7.0f; pt1.velocityProfile = 3; pt1.count = 50000;
        ParticleTypeDef pt2; pt2.name = "Crimson Blood"; pt2.color = glm::vec4(0.6f, 0.0f, 0.0f, 0.9f); pt2.shape = 7; pt2.size = 3.0f; pt2.lifeDuration = 4.0f; pt2.velocityProfile = 0; pt2.count = 10000;
        ParticleTypeDef pt3; pt3.name = "Ash"; pt3.color = glm::vec4(0.4f, 0.4f, 0.4f, 0.6f); pt3.shape = 1; pt3.size = 2.0f; pt3.lifeDuration = 5.0f; pt3.velocityProfile = 3; pt3.count = 20000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 9) { // Space opera
        pushConstants.timeSpeed = 1.5f;
        pushConstants.gravity = glm::vec2(0.0f, 0.0f);
        pushConstants.globalMovement = 14; // Pulsar
        ParticleTypeDef pt1; pt1.name = "Blue Laser"; pt1.color = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f); pt1.shape = 6; pt1.size = 4.0f; pt1.lifeDuration = 3.0f; pt1.velocityProfile = 4; pt1.count = 25000;
        ParticleTypeDef pt2; pt2.name = "Red Laser"; pt2.color = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f); pt2.shape = 6; pt2.size = 4.0f; pt2.lifeDuration = 3.0f; pt2.velocityProfile = 4; pt2.count = 15000;
        ParticleTypeDef pt3; pt3.name = "Engine Trail"; pt3.color = glm::vec4(1.0f, 0.8f, 0.2f, 0.5f); pt3.shape = 0; pt3.size = 6.0f; pt3.lifeDuration = 2.0f; pt3.velocityProfile = 1; pt3.count = 20000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 10) { // Future
        pushConstants.timeSpeed = 2.0f;
        pushConstants.gravity = glm::vec2(0.0f, 0.0f);
        pushConstants.globalMovement = 12; // Orbit
        ParticleTypeDef pt1; pt1.name = "Data Node Cyan"; pt1.color = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f); pt1.shape = 4; pt1.size = 3.0f; pt1.lifeDuration = 6.0f; pt1.velocityProfile = 0; pt1.count = 30000;
        ParticleTypeDef pt2; pt2.name = "Data Node White"; pt2.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); pt2.shape = 4; pt2.size = 2.0f; pt2.lifeDuration = 5.0f; pt2.velocityProfile = 0; pt2.count = 15000;
        ParticleTypeDef pt3; pt3.name = "Hologram"; pt3.color = glm::vec4(0.2f, 0.5f, 1.0f, 0.2f); pt3.shape = 0; pt3.size = 8.0f; pt3.lifeDuration = 4.0f; pt3.velocityProfile = 0; pt3.count = 10000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 11) { // Feux d'artifices
        pushConstants.timeSpeed = 1.0f;
        pushConstants.gravity = glm::vec2(0.0f, -0.5f);
        pushConstants.globalMovement = 4; // Explosion
        ParticleTypeDef pt1; pt1.name = "Red Core"; pt1.color = glm::vec4(1.0f, 0.1f, 0.1f, 1.0f); pt1.shape = 0; pt1.size = 4.0f; pt1.lifeDuration = 3.0f; pt1.velocityProfile = 4; pt1.count = 15000;
        ParticleTypeDef pt2; pt2.name = "Gold Sparks"; pt2.color = glm::vec4(1.0f, 0.8f, 0.1f, 1.0f); pt2.shape = 3; pt2.size = 3.0f; pt2.lifeDuration = 4.0f; pt2.velocityProfile = 4; pt2.count = 20000;
        ParticleTypeDef pt3; pt3.name = "White Flash"; pt3.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); pt3.shape = 0; pt3.size = 6.0f; pt3.lifeDuration = 1.5f; pt3.velocityProfile = 4; pt3.count = 5000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 12) { // Explosion
        pushConstants.timeSpeed = 1.5f;
        pushConstants.gravity = glm::vec2(0.0f, 0.0f);
        pushConstants.globalMovement = 4; // Explosion
        ParticleTypeDef pt1; pt1.name = "Fire Core"; pt1.color = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f); pt1.shape = 0; pt1.size = 12.0f; pt1.lifeDuration = 2.0f; pt1.velocityProfile = 4; pt1.count = 20000;
        ParticleTypeDef pt2; pt2.name = "Flame"; pt2.color = glm::vec4(1.0f, 0.3f, 0.0f, 0.8f); pt2.shape = 0; pt2.size = 8.0f; pt2.lifeDuration = 3.0f; pt2.velocityProfile = 4; pt2.count = 40000;
        ParticleTypeDef pt3; pt3.name = "Smoke"; pt3.color = glm::vec4(0.2f, 0.2f, 0.2f, 0.5f); pt3.shape = 0; pt3.size = 15.0f; pt3.lifeDuration = 5.0f; pt3.velocityProfile = 4; pt3.count = 20000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 13) { // Galaxie
        pushConstants.timeSpeed = 0.2f;
        pushConstants.gravity = glm::vec2(0.0f, 0.0f);
        pushConstants.globalMovement = 7; // Spiral
        ParticleTypeDef pt1; pt1.name = "Bright Star"; pt1.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); pt1.shape = 0; pt1.size = 2.0f; pt1.lifeDuration = 20.0f; pt1.velocityProfile = 0; pt1.count = 50000;
        ParticleTypeDef pt2; pt2.name = "Purple Nebula"; pt2.color = glm::vec4(0.3f, 0.1f, 0.6f, 0.2f); pt2.shape = 0; pt2.size = 15.0f; pt2.lifeDuration = 20.0f; pt2.velocityProfile = 0; pt2.count = 20000;
        ParticleTypeDef pt3; pt3.name = "Blue Dust"; pt3.color = glm::vec4(0.1f, 0.4f, 0.8f, 0.3f); pt3.shape = 0; pt3.size = 10.0f; pt3.lifeDuration = 15.0f; pt3.velocityProfile = 0; pt3.count = 30000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 14) { // Etoile
        pushConstants.timeSpeed = 0.5f;
        pushConstants.gravity = glm::vec2(0.0f, 0.0f);
        pushConstants.globalMovement = 14; // Pulsar
        ParticleTypeDef pt1; pt1.name = "Plasma Yellow"; pt1.color = glm::vec4(1.0f, 0.9f, 0.2f, 0.9f); pt1.shape = 0; pt1.size = 8.0f; pt1.lifeDuration = 4.0f; pt1.velocityProfile = 4; pt1.count = 40000;
        ParticleTypeDef pt2; pt2.name = "Plasma Orange"; pt2.color = glm::vec4(1.0f, 0.5f, 0.0f, 0.7f); pt2.shape = 0; pt2.size = 10.0f; pt2.lifeDuration = 5.0f; pt2.velocityProfile = 4; pt2.count = 30000;
        ParticleTypeDef pt3; pt3.name = "Solar Flare"; pt3.color = glm::vec4(1.0f, 0.2f, 0.0f, 0.8f); pt3.shape = 3; pt3.size = 5.0f; pt3.lifeDuration = 3.0f; pt3.velocityProfile = 4; pt3.count = 10000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 15) { // Planete
        pushConstants.timeSpeed = 0.1f;
        pushConstants.gravity = glm::vec2(0.0f, 0.0f);
        pushConstants.globalMovement = 12; // Orbit
        ParticleTypeDef pt1; pt1.name = "Crust Rock"; pt1.color = glm::vec4(0.5f, 0.3f, 0.1f, 1.0f); pt1.shape = 0; pt1.size = 4.0f; pt1.lifeDuration = 25.0f; pt1.velocityProfile = 0; pt1.count = 30000;
        ParticleTypeDef pt2; pt2.name = "Mantle"; pt2.color = glm::vec4(0.8f, 0.2f, 0.0f, 1.0f); pt2.shape = 0; pt2.size = 3.0f; pt2.lifeDuration = 20.0f; pt2.velocityProfile = 0; pt2.count = 15000;
        ParticleTypeDef pt3; pt3.name = "Atmosphere"; pt3.color = glm::vec4(0.2f, 0.5f, 0.9f, 0.1f); pt3.shape = 0; pt3.size = 15.0f; pt3.lifeDuration = 30.0f; pt3.velocityProfile = 0; pt3.count = 5000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 16) { // Tornade
        pushConstants.timeSpeed = 2.0f;
        pushConstants.gravity = glm::vec2(0.0f, 0.0f);
        pushConstants.globalMovement = 13; // Tornado
        ParticleTypeDef pt1; pt1.name = "Dust"; pt1.color = glm::vec4(0.6f, 0.5f, 0.4f, 0.6f); pt1.shape = 0; pt1.size = 5.0f; pt1.lifeDuration = 6.0f; pt1.velocityProfile = 0; pt1.count = 40000;
        ParticleTypeDef pt2; pt2.name = "Debris"; pt2.color = glm::vec4(0.2f, 0.2f, 0.2f, 0.9f); pt2.shape = 1; pt2.size = 3.0f; pt2.lifeDuration = 5.0f; pt2.velocityProfile = 0; pt2.count = 15000;
        ParticleTypeDef pt3; pt3.name = "Wind Line"; pt3.color = glm::vec4(0.8f, 0.8f, 0.8f, 0.3f); pt3.shape = 1; pt3.size = 8.0f; pt3.lifeDuration = 4.0f; pt3.velocityProfile = 0; pt3.count = 10000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 17) { // Champ de fluide
        pushConstants.timeSpeed = 0.8f;
        pushConstants.gravity = glm::vec2(0.0f, -0.1f);
        pushConstants.globalMovement = 9; // Turbulence
        ParticleTypeDef pt1; pt1.name = "Water Deep"; pt1.color = glm::vec4(0.0f, 0.2f, 0.6f, 0.8f); pt1.shape = 0; pt1.size = 6.0f; pt1.lifeDuration = 8.0f; pt1.velocityProfile = 0; pt1.count = 40000;
        ParticleTypeDef pt2; pt2.name = "Water Light"; pt2.color = glm::vec4(0.0f, 0.6f, 0.9f, 0.6f); pt2.shape = 0; pt2.size = 4.0f; pt2.lifeDuration = 7.0f; pt2.velocityProfile = 0; pt2.count = 30000;
        ParticleTypeDef pt3; pt3.name = "Bubble"; pt3.color = glm::vec4(0.8f, 0.9f, 1.0f, 0.4f); pt3.shape = 2; pt3.size = 3.0f; pt3.lifeDuration = 4.0f; pt3.velocityProfile = 1; pt3.count = 5000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 18) { // Magnetisme
        pushConstants.timeSpeed = 1.0f;
        pushConstants.gravity = glm::vec2(0.0f, 0.0f);
        pushConstants.globalMovement = 11; // Dipole
        ParticleTypeDef pt1; pt1.name = "North Plus"; pt1.color = glm::vec4(1.0f, 0.1f, 0.1f, 1.0f); pt1.shape = 0; pt1.size = 4.0f; pt1.lifeDuration = 7.0f; pt1.velocityProfile = 0; pt1.count = 30000;
        ParticleTypeDef pt2; pt2.name = "South Minus"; pt2.color = glm::vec4(0.1f, 0.3f, 1.0f, 1.0f); pt2.shape = 0; pt2.size = 4.0f; pt2.lifeDuration = 7.0f; pt2.velocityProfile = 0; pt2.count = 30000;
        ParticleTypeDef pt3; pt3.name = "Flux Line"; pt3.color = glm::vec4(0.5f, 0.0f, 0.8f, 0.3f); pt3.shape = 1; pt3.size = 2.0f; pt3.lifeDuration = 5.0f; pt3.velocityProfile = 0; pt3.count = 15000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 19) { // Hyper-Espace
        pushConstants.timeSpeed = 5.0f;
        pushConstants.gravity = glm::vec2(0.0f, 0.0f);
        pushConstants.globalMovement = 4; // Explosion
        ParticleTypeDef pt1; pt1.name = "Star Line Bright"; pt1.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); pt1.shape = 1; pt1.size = 2.0f; pt1.lifeDuration = 10.0f; pt1.velocityProfile = 4; pt1.count = 40000;
        ParticleTypeDef pt2; pt2.name = "Star Line Blue"; pt2.color = glm::vec4(0.5f, 0.8f, 1.0f, 0.8f); pt2.shape = 1; pt2.size = 1.5f; pt2.lifeDuration = 12.0f; pt2.velocityProfile = 4; pt2.count = 30000;
        ParticleTypeDef pt3; pt3.name = "Hyper Dust"; pt3.color = glm::vec4(0.8f, 0.4f, 1.0f, 0.5f); pt3.shape = 0; pt3.size = 4.0f; pt3.lifeDuration = 8.0f; pt3.velocityProfile = 4; pt3.count = 20000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 20) { // Pulsar
        pushConstants.timeSpeed = 2.0f;
        pushConstants.gravity = glm::vec2(0.0f, 0.0f);
        pushConstants.globalMovement = 14; // Pulsar
        ParticleTypeDef pt1; pt1.name = "Energy Core"; pt1.color = glm::vec4(0.5f, 0.0f, 1.0f, 0.9f); pt1.shape = 2; pt1.size = 6.0f; pt1.lifeDuration = 4.0f; pt1.velocityProfile = 4; pt1.count = 30000;
        ParticleTypeDef pt2; pt2.name = "Radiation"; pt2.color = glm::vec4(0.0f, 0.8f, 1.0f, 0.6f); pt2.shape = 0; pt2.size = 4.0f; pt2.lifeDuration = 5.0f; pt2.velocityProfile = 4; pt2.count = 25000;
        ParticleTypeDef pt3; pt3.name = "Neutron Pulse"; pt3.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); pt3.shape = 6; pt3.size = 3.0f; pt3.lifeDuration = 2.0f; pt3.velocityProfile = 4; pt3.count = 10000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 21) { // Geyser
        pushConstants.timeSpeed = 1.5f;
        pushConstants.gravity = glm::vec2(0.0f, -1.0f);
        pushConstants.globalMovement = 16; // Geyser
        ParticleTypeDef pt1; pt1.name = "Steam Base"; pt1.color = glm::vec4(0.8f, 0.8f, 0.9f, 0.5f); pt1.shape = 0; pt1.size = 8.0f; pt1.lifeDuration = 6.0f; pt1.velocityProfile = 1; pt1.count = 30000;
        ParticleTypeDef pt2; pt2.name = "Boiling Water"; pt2.color = glm::vec4(0.6f, 0.9f, 1.0f, 0.8f); pt2.shape = 0; pt2.size = 5.0f; pt2.lifeDuration = 5.0f; pt2.velocityProfile = 1; pt2.count = 20000;
        ParticleTypeDef pt3; pt3.name = "Fine Mist"; pt3.color = glm::vec4(0.9f, 0.9f, 0.9f, 0.2f); pt3.shape = 0; pt3.size = 12.0f; pt3.lifeDuration = 8.0f; pt3.velocityProfile = 1; pt3.count = 15000;
        particleTypes = {pt1, pt2, pt3};
    } else if (presetIndex == 22) { // Fontaine
        pushConstants.timeSpeed = 1.0f;
        pushConstants.gravity = glm::vec2(0.0f, 0.0f);
        pushConstants.globalMovement = 18; // Fountain
        ParticleTypeDef pt1; pt1.name = "Main Stream"; pt1.color = glm::vec4(0.2f, 0.6f, 1.0f, 0.8f); pt1.shape = 0; pt1.size = 4.0f; pt1.lifeDuration = 7.0f; pt1.velocityProfile = 1; pt1.count = 40000;
        ParticleTypeDef pt2; pt2.name = "Splash"; pt2.color = glm::vec4(0.7f, 0.9f, 1.0f, 0.9f); pt2.shape = 0; pt2.size = 2.0f; pt2.lifeDuration = 4.0f; pt2.velocityProfile = 0; pt2.count = 15000;
        ParticleTypeDef pt3; pt3.name = "Droplet"; pt3.color = glm::vec4(1.0f, 1.0f, 1.0f, 0.6f); pt3.shape = 2; pt3.size = 3.0f; pt3.lifeDuration = 6.0f; pt3.velocityProfile = 1; pt3.count = 10000;
        particleTypes = {pt1, pt2, pt3};
    }
    
    needsBufferReallocation = true;
    needsTypeBufferUpdate = true;
}

// Saves the current simulation configuration to a JSON file
void VulkanEngine::savePreset(const std::string& filename) {
    json configJson; // json object for entire configuration
    configJson["timeSpeed"] = pushConstants.timeSpeed;
    configJson["gravity"] = {pushConstants.gravity.x, pushConstants.gravity.y};
    configJson["globalMovement"] = pushConstants.globalMovement;
    configJson["infiniteLoop"] = infiniteLoop;
    configJson["animationDuration"] = animationDuration;
    
    json particleTypesArray = json::array();
    for (const auto& particleType : particleTypes) {
        json particleTypeJson; // json object for particle type
        particleTypeJson["name"] = particleType.name;
        particleTypeJson["color"] = {particleType.color.x, particleType.color.y, particleType.color.z, particleType.color.w};
        particleTypeJson["size"] = particleType.size;
        particleTypeJson["opacity"] = particleType.opacity;
        particleTypeJson["lifeDuration"] = particleType.lifeDuration;
        particleTypeJson["shape"] = particleType.shape;
        particleTypeJson["fadeType"] = particleType.fadeType;
        particleTypeJson["velocityProfile"] = particleType.velocityProfile;
        particleTypeJson["count"] = particleType.count;
        particleTypesArray.push_back(particleTypeJson);
    }
    configJson["particleTypes"] = particleTypesArray;
    
    std::ofstream outputFile(filename);
    outputFile << std::setw(4) << configJson << std::endl;
}

// Loads a simulation configuration from a JSON file
void VulkanEngine::loadPreset(const std::string& filename) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) return;
    json configJson; // json object for entire configuration
    inputFile >> configJson;
    
    pushConstants.timeSpeed = configJson.value("timeSpeed", 1.0f);
    if(configJson.contains("gravity")) {
        pushConstants.gravity.x = configJson["gravity"][0];
        pushConstants.gravity.y = configJson["gravity"][1];
    }
    pushConstants.globalMovement = configJson.value("globalMovement", 0);
    infiniteLoop = configJson.value("infiniteLoop", true);
    animationDuration = configJson.value("animationDuration", 10.0f);
    
    if (configJson.contains("particleTypes")) {
        particleTypes.clear();
        for (const auto& particleTypeJson : configJson["particleTypes"]) {
            ParticleTypeDef particleType;
            particleType.name = particleTypeJson.value("name", "Unknown");
            if(particleTypeJson.contains("color")) particleType.color = glm::vec4(particleTypeJson["color"][0], particleTypeJson["color"][1], particleTypeJson["color"][2], particleTypeJson["color"][3]);
            particleType.size = particleTypeJson.value("size", 5.0f);
            particleType.opacity = particleTypeJson.value("opacity", 1.0f);
            particleType.lifeDuration = particleTypeJson.value("lifeDuration", 5.0f);
            particleType.shape = particleTypeJson.value("shape", 0);
            particleType.fadeType = particleTypeJson.value("fadeType", 1);
            particleType.velocityProfile = particleTypeJson.value("velocityProfile", 0);
            particleType.count = particleTypeJson.value("count", 10000);
            particleTypes.push_back(particleType);
        }
    }
    
    needsBufferReallocation = true;
    needsTypeBufferUpdate = true;
    pushConstants.reset = 1;
}

// Saves a specific particle type definition to a JSON file
void VulkanEngine::saveParticleType(const std::string& filename, const ParticleTypeDef& particleType) {
    json particleTypeJson; // json object for particle type
    particleTypeJson["name"] = particleType.name;
    particleTypeJson["color"] = {particleType.color.x, particleType.color.y, particleType.color.z, particleType.color.w};
    particleTypeJson["size"] = particleType.size;
    particleTypeJson["opacity"] = particleType.opacity;
    particleTypeJson["lifeDuration"] = particleType.lifeDuration;
    particleTypeJson["shape"] = particleType.shape;
    particleTypeJson["fadeType"] = particleType.fadeType;
    particleTypeJson["velocityProfile"] = particleType.velocityProfile;
    particleTypeJson["count"] = particleType.count;
    
    std::ofstream outputFile(filename);
    outputFile << std::setw(4) << particleTypeJson << std::endl;
}

// Loads a specific particle type definition from a JSON file
void VulkanEngine::loadParticleType(const std::string& filename) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) return;
    json particleTypeJson; // json object for particle type
    inputFile >> particleTypeJson;
    
    ParticleTypeDef particleType;
    particleType.name = particleTypeJson.value("name", "Imported");
    if(particleTypeJson.contains("color")) particleType.color = glm::vec4(particleTypeJson["color"][0], particleTypeJson["color"][1], particleTypeJson["color"][2], particleTypeJson["color"][3]);
    particleType.size = particleTypeJson.value("size", 5.0f);
    particleType.opacity = particleTypeJson.value("opacity", 1.0f);
    particleType.lifeDuration = particleTypeJson.value("lifeDuration", 5.0f);
    particleType.shape = particleTypeJson.value("shape", 0);
    particleType.fadeType = particleTypeJson.value("fadeType", 1);
    particleType.velocityProfile = particleTypeJson.value("velocityProfile", 0);
    particleType.count = particleTypeJson.value("count", 10000);
    
    particleTypes.push_back(particleType);
    needsBufferReallocation = true;
    needsTypeBufferUpdate = true;
}


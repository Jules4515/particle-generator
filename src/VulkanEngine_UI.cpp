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
#include <ctime>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

#define VK_CHECK(x)                                                 \
    do {                                                            \
        VkResult err = x;                                           \
        if (err) {                                                  \
            std::cerr << "Detected Vulkan error: " << err << "\n";  \
            abort();                                                \
        }                                                           \
    } while (0)

// Helper function to open a file dialog using zenity
std::string openFileDialog(bool save, const std::string& filter) {
    std::string cmd = save ? "zenity --file-selection --save --confirm-overwrite --file-filter=\"" + filter + "\"" 
                           : "zenity --file-selection --file-filter=\"" + filter + "\"";
    std::array<char, 256> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    if (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}


// Renders the ImGui user interface
void VulkanEngine::renderUI() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    ImGui::SetNextWindowSizeConstraints(ImVec2(450, 400), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin("Particle Generator");
    float sliderWidth = ImGui::GetWindowWidth() * 0.5f;
    ImGui::Text("Performance: %.1f FPS", ImGui::GetIO().Framerate);
    if (ImGui::Button("Replay")) {
        pushConstants.reset = 1;
        currentAnimationTime = 0.0f;
    }
    
    ImGui::Separator();
    const char* presets[] = { 
        "Matrix", "Nature", "Snowstorm", "Acid Rain", "Cyberpunk", "Blade runner", "Witchcraft", 
        "Magic", "Dark fantasy", "Space opera", "Future", "Fireworks", "Explosion", "Galaxy", 
        "Star", "Planet", "Tornado", "Fluid field", "Magnetism", "Hyper-Space", "Pulsar", 
        "Geyser", "Fountain" 
    };
    static int currentPreset = 0;
    if (ImGui::Combo("Global Presets", &currentPreset, presets, IM_ARRAYSIZE(presets))) {
        applyGlobalPreset(currentPreset);
    }
    
    if (ImGui::Button("Save Simulation (JSON)")) {
        std::string file = openFileDialog(true, "*.json");
        if (!file.empty()) {
            if (file.find(".json") == std::string::npos) file += ".json";
            savePreset(file);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Simulation (JSON)")) {
        std::string file = openFileDialog(false, "*.json");
        if (!file.empty()) loadPreset(file);
    }
    
    ImGui::Separator();
    ImGui::Text("Simulation Parameters");
    ImGui::PushItemWidth(sliderWidth); ImGui::SliderFloat("##ts_s", &pushConstants.timeSpeed, 0.0f, 5.0f); ImGui::PopItemWidth();
    ImGui::SameLine(); ImGui::PushItemWidth(60); ImGui::InputFloat("##ts_i", &pushConstants.timeSpeed); ImGui::PopItemWidth();
    ImGui::SameLine(); ImGui::Text("Time Speed");
    
    ImGui::PushItemWidth(sliderWidth); ImGui::SliderFloat2("##gr_s", &pushConstants.gravity.x, -5.0f, 5.0f); ImGui::PopItemWidth();
    ImGui::SameLine(); ImGui::PushItemWidth(120); ImGui::InputFloat2("##gr_i", &pushConstants.gravity.x); ImGui::PopItemWidth();
    ImGui::SameLine(); ImGui::Text("Gravity");
    
    const char* gmModes[] = {
        "None", "Downward", "Vortex", "Black Hole", "Explosion", "Wind Right", "Wind Left",
        "Spiral", "Wave", "Turbulence", "Anti-Gravity", "Magnetic Poles", "Orbit", "Tornado",
        "Pulsar", "Whirlpool", "Geyser", "Repulsion", "Fountain", "Zigzag"
    };
    ImGui::Combo("Global Movement", &pushConstants.globalMovement, gmModes, IM_ARRAYSIZE(gmModes));
    
    ImGui::PushItemWidth(sliderWidth); ImGui::SliderFloat("##dur_s", &animationDuration, 0.1f, 1000.0f); ImGui::PopItemWidth();
    ImGui::SameLine(); ImGui::PushItemWidth(60); ImGui::InputFloat("##dur_i", &animationDuration); ImGui::PopItemWidth();
    ImGui::SameLine(); ImGui::Text("Duration (s)");
    
    ImGui::Checkbox("Infinite Loop", &infiniteLoop);
    
    if (isExporting) {
        ImGui::ProgressBar(currentAnimationTime / animationDuration, ImVec2(-1.0f, 0.0f), "Exporting...");
    }
    
    if (ImGui::Button("Export MP4 Video")) {
        if (!isExporting) {
            isExporting = true;
            currentAnimationTime = 0.0f;
            pushConstants.reset = 1;
            auto t = std::time(nullptr);
            auto tm = *std::localtime(&t);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%d-%m-%Y_%H:%M:%S");
            std::string timestamp = oss.str();
            
            std::string filepath = "../videos/animation_" + timestamp + ".mp4";
            std::string cmd = "mkdir -p ../videos && ffmpeg -y -f rawvideo -pix_fmt bgra -s " + std::to_string(swapchainExtent.width) + "x" + std::to_string(swapchainExtent.height) + " -r 60 -i - -c:v libx264 -preset fast -crf 18 -pix_fmt yuv420p " + filepath;
            ffmpegPipe = popen(cmd.c_str(), "w");
        }
    }
    
    ImGui::Separator();
    ImGui::Text("Particle Types List");
    if (ImGui::Button("Add a type")) {
        ParticleTypeDef t; t.name = "New Type";
        particleTypes.push_back(t);
        needsBufferReallocation = true;
    }
    
    int toDelete = -1;
    for (size_t i = 0; i < particleTypes.size(); ++i) {
        auto& t = particleTypes[i];
        if (ImGui::TreeNode((void*)(intptr_t)i, "%s (%d)", t.name.c_str(), t.count)) {
            char nameBuf[128];
            strncpy(nameBuf, t.name.c_str(), sizeof(nameBuf));
            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) t.name = nameBuf;
            
            ImGui::PushItemWidth(sliderWidth); if (ImGui::SliderInt("##ct_s", &t.count, 1, 1000000)) needsBufferReallocation = true; ImGui::PopItemWidth();
            ImGui::SameLine(); ImGui::PushItemWidth(80); if (ImGui::InputInt("##ct_i", &t.count)) needsBufferReallocation = true; ImGui::PopItemWidth();
            ImGui::SameLine(); ImGui::Text("Count");
            
            if (ImGui::ColorEdit4("Color", &t.color.x)) needsTypeBufferUpdate = true;
            
            ImGui::PushItemWidth(sliderWidth); if (ImGui::SliderFloat("##sz_s", &t.size, 0.1f, 50.0f)) needsTypeBufferUpdate = true; ImGui::PopItemWidth();
            ImGui::SameLine(); ImGui::PushItemWidth(60); if (ImGui::InputFloat("##sz_i", &t.size)) needsTypeBufferUpdate = true; ImGui::PopItemWidth();
            ImGui::SameLine(); ImGui::Text("Size");
            
            ImGui::PushItemWidth(sliderWidth); if (ImGui::SliderFloat("##op_s", &t.opacity, 0.0f, 1.0f)) needsTypeBufferUpdate = true; ImGui::PopItemWidth();
            ImGui::SameLine(); ImGui::PushItemWidth(60); if (ImGui::InputFloat("##op_i", &t.opacity)) needsTypeBufferUpdate = true; ImGui::PopItemWidth();
            ImGui::SameLine(); ImGui::Text("Opacity");
            
            ImGui::PushItemWidth(sliderWidth); if (ImGui::SliderFloat("##ld_s", &t.lifeDuration, 0.1f, 20.0f)) needsTypeBufferUpdate = true; ImGui::PopItemWidth();
            ImGui::SameLine(); ImGui::PushItemWidth(60); if (ImGui::InputFloat("##ld_i", &t.lifeDuration)) needsTypeBufferUpdate = true; ImGui::PopItemWidth();
            ImGui::SameLine(); ImGui::Text("Life Duration");
            
            const char* shapes[] = { "Circle", "Square", "Ring", "Star", "Hexagon", "Triangle", "Cross", "Heart" };
            if (ImGui::Combo("Shape", &t.shape, shapes, IM_ARRAYSIZE(shapes))) needsTypeBufferUpdate = true;
            
            const char* fades[] = { "Instant", "Fade Out" };
            if (ImGui::Combo("Fade Type", &t.fadeType, fades, IM_ARRAYSIZE(fades))) needsTypeBufferUpdate = true;
            
            const char* vels[] = { "Random", "Upward", "Downward", "Horizontal", "Explosion" };
            if (ImGui::Combo("Velocity Profile", &t.velocityProfile, vels, IM_ARRAYSIZE(vels))) needsTypeBufferUpdate = true;
            
            if (ImGui::Button("Save Type")) {
                std::string file = openFileDialog(true, "*.json");
                if (!file.empty()) {
                    if (file.find(".json") == std::string::npos) file += ".json";
                    saveParticleType(file, t);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Type")) {
                std::string file = openFileDialog(false, "*.json");
                if (!file.empty()) loadParticleType(file);
            }
            if (ImGui::Button("Delete this type")) {
                toDelete = i;
            }
            
            ImGui::TreePop();
        }
    }
    
    if (toDelete >= 0) {
        particleTypes.erase(particleTypes.begin() + toDelete);
        needsBufferReallocation = true;
    }
    
    ImGui::End();
    ImGui::Render();
}


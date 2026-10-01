# Vulkan Particle Simulator

A high-performance particle simulator built with Vulkan, Compute Shaders, and ImGui. This application allows users to simulate hundreds of thousands of particles in real-time, apply complex global movements, customize particle types, and export the resulting animation to an MP4 video using FFmpeg.

## Features

- **Vulkan Compute Shaders**: Harness the power of the GPU to compute physics and movements for massive amounts of particles efficiently.
- **Customizable Particle Types**: Create, edit, save, and load specific particle types (color, size, life duration, shape, opacity, etc.).
- **Global Movements**: Apply global physics modifiers such as Gravity, Vortex, Black Hole, Explosion, Wind, Turbulence, Tornado, Pulsar, and more.
- **Rich UI (ImGui)**: A compact and clear user interface to tweak parameters on the fly, with precise numerical inputs and sliders.
- **23 Global Presets**: Instantly switch between entirely different simulation atmospheres like Matrix, Snowstorm, Cyberpunk, Magic, Galaxy, etc.
- **Video Export**: Seamlessly record the simulation and export it directly to an MP4 video using a raw frame pipe to FFmpeg.

## Build Instructions

### Prerequisites
- Vulkan SDK
- CMake (>= 3.20)
- FFmpeg (for video export)
- A modern C++ compiler (C++20)

### Compiling
1. Create a build directory:
   ```bash
   mkdir build && cd build
   ```
2. Generate the build files and compile:
   ```bash
   cmake ..
   make
   ```
3. Run the application:
   ```bash
   ./ParticlesVulkan
   ```

## Usage
- Modify parameters in real-time using the ImGui interface.
- Check "Infinite Loop (Preview)" to loop the simulation indefinitely.
- Press "Export MP4 Video" to render out a perfectly smooth 60 FPS video of the set duration.

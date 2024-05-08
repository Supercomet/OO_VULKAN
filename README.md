# Vulkan Rendering Engine

Physically based renderer using the Vulkan API written in C++.

A test environment for me to experiment with graphics features.

### Features

- Physically based rendering
- IBL environment map
- Adaptive exposure
- Bindless textures
- Particle system
- Multi-threaded CPU octree culling
- Shadow casting point lights
- Area lights
- Auto-partitioning Shadowmap atlas
- Multi-threaded command list recording
- [MSDF](https://github.com/Chlumsky/msdfgen) font atlas
- Post processing stack
- [XeGTAO](https://github.com/GameTechDev/XeGTAO) ambient occlusion
- FidelityFX [Single Pass Downsampler](https://gpuopen.com/fidelityfx-spd/)
- FidelityFX [FSR2](https://gpuopen.com/fidelityfx-superresolution-2/) and Nvidia [DLSS](https://www.nvidia.com/en-sg/geforce/technologies/dlss/)

### Prerequisites

- [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/) (C++20)
- [Vulkan SDK](https://vulkan.lunarg.com/) (v1.3+)

### Dependencies

The dependencies are bundled with the project

- [glm](https://github.com/g-truc/glm)
- [assimp](https://github.com/assimp/assimp)
- [freetype](https://gitlab.freedesktop.org/freetype)
- [imgui](https://github.com/ocornut/imgui)
- [msdfgen](https://github.com/Chlumsky/msdfgen)
- [Nvidia DLSS](https://github.com/NVIDIA/DLSS)
- [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)

### Controls

**WASD keys** — move around
**Right click and drag** — look around

Interact with the UI for more details

# rt-voxel-engine
“Real-time voxel engine with GPU ray tracing (Vulkan) — master’s thesis prototype.”  
Visual presentation of the engine’s capabilities: https://www.youtube.com/watch?v=RHJrIp4YQ4c

## Build and Setup Instructions

The codebase is provided together with Visual Studio project files for convenience. Creating a proper CMake build system is worth considering, but was not necessary for my personal needs.

After cloning the repository, a few additional steps are required to build the project:

1. **Install the Vulkan SDK**  
   The engine requires Vulkan. The simplest setup is to install the Vulkan SDK and ensure that the `VK_SDK_PATH` environment variable is correctly configured. This is usually handled automatically during installation.

2. **Update the Visual Studio toolset**  
   After opening the solution, update the toolset to the most recent version available on your system:  
   *Project → Properties → General → Platform Toolset*

3. **Header-only changes require a full rebuild**  
   For reasons currently unknown, changes made to standalone header files (e.g., `config.h` or other headers not directly associated with a `.cpp` file) may not trigger recompilation. In such cases, a full solution rebuild is required. This should be addressed in the future, and restructuring the project may be sufficient to resolve the issue.

## Current Limitations and Future Work

The code would benefit from refactoring, as many assumptions evolved during development. After refactoring, the chunk manager should be reimplemented and completed to function properly. Memory usage can also be improved, and implementing a proper material system remains an important milestone. Whether these improvements will be completed depends on available time and actual need.

#pragma once

#include "VulkanRTCore.h"
#include "game/chunks/ChunkManager.h"
#include "math/Camera.h"
#include "Window.h"

class ChunkManager;

class RendererRT
{
public:
    RendererRT();
    ~RendererRT();
    void Init(int width, int height);
    void SetWindow(Window* window);
    void SetCamera(const Camera* camera);
    BottomLevelAccelerationStructure BuildBlas(const std::vector<float>& vertices, const  std::vector<unsigned int>& indices);
    bool IsBlasBuilded(BottomLevelAccelerationStructure acceleration_structure);
    void FreeBlas(BottomLevelAccelerationStructure acceleration_structure);
    void Render(const ChunkManager& chunk_manager);
    void WindowSizeChanged(int width, int height);
    void ForceRebuild() { rebuild_required_ = true; };

private:
    VulkanRTCore vulkan_;
    const Camera* camera_;
    bool rebuild_required_;
};

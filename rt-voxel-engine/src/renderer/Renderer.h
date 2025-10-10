#ifdef OPENGL
#pragma once

#include "Shader.h"
#include "Texture.h"
#include "game/chunks/ChunkManager.h"
#include "math/Camera.h"
#include <glm/glm.hpp>

class Renderer 
{
public:
    Renderer();
    void Init(int width, int height);
    void SetCamera(const Camera* camera);
    void Render(const ChunkManager& chunk_manager);
    void ToggleWireframeMode();
    void WindowSizeChanged(int width, int height);

private:
    Shader shader_; 	//TODO meaningfull names
    Texture texture_;	//TODO meaningfull names
    const Camera* camera_;
    bool wire_frame_;
};
#endif
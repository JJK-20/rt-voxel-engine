#pragma once

#include <glm/glm.hpp>
#include <array>

// each texture for atlas is expected to be provided in format, where around each 'single' texture there is one pixel size border, 
// where each pixel of border is of same value as closest pixel of original texture, so we can avoid texture bleeding
class TextureAtlas 
{
public:
    TextureAtlas(float texture_size, int quad_count_x, int quad_count_y);
    //return array of 4 vec2 uv's: bottom-left, bottom-right, top-right, top-left
    std::array<glm::vec2, 4> getUVs(int index) const;
    inline float GetQuadWidth() const { return quad_width_ - border_width_; }
    inline float GetQuadHeight() const { return quad_height_ - border_width_; }

private:
    //The size of each original quad without the border
    float texture_size_;
    //The number of quads horizontally and vertically
    int quad_count_x_;
    int quad_count_y_;
    //The UV dimensions of each quad, including borders
    float quad_width_;  
    float quad_height_;
    //The size of the borders in UV space
    float border_width_;
    float border_height_;
};
#include "TextureAtlas.h"

TextureAtlas::TextureAtlas(float texture_size, int quad_count_x, int quad_count_y)
	: texture_size_(texture_size), quad_count_x_(quad_count_x), quad_count_y_(quad_count_y)
{
	quad_width_ = 1.0f / quad_count_x_;
	quad_height_ = 1.0f / quad_count_y_;
	border_width_ = quad_width_ / (texture_size_ + 2.0f);
	border_height_ = quad_height_ / (texture_size_ + 2.0f);
}

std::array<glm::vec2, 4> TextureAtlas::getUVs(int index) const
{
	int row = index / quad_count_x_;
	int col = index % quad_count_x_;

	float u = col * quad_width_ + border_width_;
	float v = 1.0f - row  * quad_height_ - border_height_;
	float u2 = u + quad_width_ - 2 * border_width_;
	float v2 = v - quad_height_ + 2 * border_height_;

	return {
		 glm::vec2(u, v2),  // bottom-left
		 glm::vec2(u2, v2), // bottom-right
		 glm::vec2(u2, v),  // top-right
		 glm::vec2(u, v)    // top-left
	};
}

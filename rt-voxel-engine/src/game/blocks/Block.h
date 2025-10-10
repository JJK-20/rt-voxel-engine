#pragma once

#include "BlockId.h"
#include "math/TextureAtlas.h"
#include <array>

class Block
{
private:
	BlockId id_;

	bool transparent_;
	bool collidable_;

	std::array<glm::vec2, 4> texture_uv_top_;
	std::array<glm::vec2, 4> texture_uv_sides_;
	std::array<glm::vec2, 4> texture_uv_bottom_;
public:
	Block() {}
	Block(BlockId id, bool transparent, bool collidable, TextureAtlas& atlas, int tex_top_index, int tex_sides_index, int tex_bottom_index)
		: id_(id), transparent_(transparent), collidable_(collidable)
	{
		texture_uv_top_ = atlas.getUVs(tex_top_index);
		texture_uv_sides_ = atlas.getUVs(tex_sides_index);
		texture_uv_bottom_ = atlas.getUVs(tex_bottom_index);
	};
	inline BlockId getId() const { return id_; }
	inline bool isTransparent() const { return transparent_; }
	inline bool isCollidable() const { return collidable_; }
	inline const std::array<glm::vec2, 4>& getUVTop() const { return texture_uv_top_; }
	inline const std::array<glm::vec2, 4>& getUVSides() const { return texture_uv_sides_; }
	inline const std::array<glm::vec2, 4>& getUVBottom() const { return texture_uv_bottom_; }
};
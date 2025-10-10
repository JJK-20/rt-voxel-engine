#pragma once

#include "Block.h"
#include "math/TextureAtlas.h"
#include <array>

enum class Face : unsigned char
{
	FRONT_FACE = 0,
	BACK_FACE,
	LEFT_FACE,
	RIGHT_FACE,
	TOP_FACE,
	BOTTOM_FACE,

	NUM_FACES
};

class BlockDatabase
{
private:
	std::array<Block, static_cast<int>(BlockId::NUM_TYPES)> blocks_;
	std::array<std::array<glm::vec3, 4>, static_cast<int>(Face::NUM_FACES)> faces_;
	std::array<float, static_cast<int>(Face::NUM_FACES)> faces_lighting_;

	const std::array<glm::vec3, 4> FRONT_FACE
	{
		glm::vec3(0, 0, 1),	//BOTTOM LEFT
		glm::vec3(1, 0, 1),	//BOTTOM RIGHT
		glm::vec3(1, 1, 1),	//TOP RIGHT
		glm::vec3(0, 1, 1),	//TOP LEFT
	};
	const std::array<glm::vec3, 4> BACK_FACE{ glm::vec3(1, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec3(1, 1, 0) };
	const std::array<glm::vec3, 4> LEFT_FACE{ glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 1), glm::vec3(0, 1, 0) };
	const std::array<glm::vec3, 4> RIGHT_FACE{ glm::vec3(1, 0, 1), glm::vec3(1, 0, 0), glm::vec3(1, 1, 0), glm::vec3(1, 1, 1) };
	const std::array<glm::vec3, 4> TOP_FACE{ glm::vec3(0, 1, 1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 0), glm::vec3(0, 1, 0) };
	const std::array<glm::vec3, 4> BOTTOM_FACE{ glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 0, 1), glm::vec3(0, 0, 1) };
public:
	BlockDatabase()
	{
		TextureAtlas atlas(16.0, 16, 16);
		blocks_[static_cast<int>(BlockId::Air)] = Block(BlockId::Air, true, false, atlas, -1, -1, -1);
		blocks_[static_cast<int>(BlockId::Grass)] = Block(BlockId::Grass, false, true, atlas, 0, 1, 2);
		blocks_[static_cast<int>(BlockId::Dirt)] = Block(BlockId::Dirt, false, true, atlas, 2, 2, 2);
		blocks_[static_cast<int>(BlockId::Stone)] = Block(BlockId::Stone, false, true, atlas, 3, 3, 3);
		blocks_[static_cast<int>(BlockId::Sand)] = Block(BlockId::Sand, false, true, atlas, 16, 16, 16);
		blocks_[static_cast<int>(BlockId::Water)] = Block(BlockId::Water, true, false, atlas, 32, 32, 32);
		blocks_[static_cast<int>(BlockId::Wood)] = Block(BlockId::Wood, false, true, atlas, 19, 18, 19);
		blocks_[static_cast<int>(BlockId::Leaves)] = Block(BlockId::Leaves, true, true, atlas, 20, 20, 20);

		faces_ = { FRONT_FACE, BACK_FACE, LEFT_FACE, RIGHT_FACE, TOP_FACE, BOTTOM_FACE };
		faces_lighting_ = { 0.84f, 0.76f, 0.68f, 0.92f, 1.0f, 0.60f };	//temporary simple lighting
	}

	inline const Block& GetBlockData(BlockId block_id) const
	{
		return blocks_[static_cast<int>(block_id)];
	}

	inline const std::array<glm::vec3, 4>& GetFaceVertices(Face face) const
	{
		return faces_[static_cast<int>(face)];
	}
	//temporary simple lighting
	inline const float GetFaceLighting(Face face) const
	{
		return faces_lighting_[static_cast<int>(face)];
	}
};
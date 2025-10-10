#pragma once

#include <glm/glm.hpp>
#include "game/blocks/BlockDatabase.h"
#ifdef OPENGL
#include "renderer/Mesh.h"
#endif
#ifdef VULKAN
#include "renderer-vulkan-rt/RendererRT.h"
#endif
#include "ChunkManager.h"
#include <vector>

class ChunkManager;

const int CHUNK_SIZE_X = 16;
const int CHUNK_SIZE_Y = 256;
const int CHUNK_SIZE_Z = 16;
const int CHUNK_VOLUME = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

struct AdjacentBlockPositions
{
	inline void update(int x, int y, int z)
	{
		up =	{ x,		y + 1,  z };
		down =	{ x,		y - 1,  z };
		left =	{ x - 1,	y,      z };
		right = { x + 1,	y,      z };
		front = { x,		y,      z + 1 };
		back =	{ x,		y,      z - 1 };
	}

	glm::ivec3 up;
	glm::ivec3 down;
	glm::ivec3 left;
	glm::ivec3 right;
	glm::ivec3 front;
	glm::ivec3 back;
};

class Chunk
{
public:
	Chunk(glm::i64vec3 global_position, ChunkManager& chunk_manager);
	void ReuseChunk(glm::i64vec3 global_position);
	void Delete();

	void SetBlock(int x, int y, int z, BlockId block);
	BlockId GetBlock(int x, int y, int z) const;
	void Generate();
	void BuildMesh();
	const bool Genereted() const { return generated_; };
	const bool Meshed() const { return meshed_; };
#ifdef OPENGL
	void Draw() const;
#endif
#ifdef VULKAN
	const bool Blased();
	const BottomLevelAccelerationStructure& GetBLAS() const { return acceleration_structure_; };
#endif
private:
	inline int GetIndex(int x, int y, int z) const;
	inline bool OutOfBounds(int x, int y, int z) const;
	void AddFace(std::vector<float>& vertices, std::vector<unsigned int>& indices, Face face, const std::array<glm::vec2, 4>& uv, const glm::vec3& offset, unsigned int& indexIndex);
	//bool IsVisible(int x, int y, int z) const;
	bool IsVisible(const Block& block, int x, int y, int z) const;

	glm::i64vec3 global_position_;	//TODO we using only x and z components in future maybe we will use y, if not think about refactor
	BlockId blocks_[CHUNK_VOLUME];
	bool generated_;
	bool meshed_;
#ifdef OPENGL
	Mesh* mesh_;
#endif
#ifdef VULKAN
	bool blased_;
	BottomLevelAccelerationStructure acceleration_structure_;
#endif
	ChunkManager& chunk_manager_; //TODO smart pointer?
};

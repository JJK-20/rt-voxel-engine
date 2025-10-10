#pragma once
#include "Chunk.h"
#include "WorldGenerator.h"
#include "renderer-vulkan-rt\RendererRT.h"
#include "game/blocks/BlockId.h"
#include "game/blocks/BlockDatabase.h"
#include <vector>

class Chunk;
class RendererRT;

class ChunkManager
{
public:
#ifdef OPENGL
	ChunkManager(int render_distance, glm::vec3 player_position, int world_generator_seed);	//TODO what about y axis
#endif
#ifdef VULKAN
	ChunkManager(int render_distance, glm::vec3 player_position, int world_generator_seed, RendererRT & renderer);	//TODO what about y axis
#endif
	~ChunkManager();
	void GenerateChunks();
	void UpdateCenter(glm::vec3 player_position);

	void SetBlock(long long int x, long long int y, long long int z, BlockId block);
	BlockId GetBlock(long long int x, long long int y, long long int z) const;
	inline int GetChunkIndex(int x, int z) const;
	Chunk& GetChunk(long long int x, long long int z);	//TODO inline it later
	inline bool OutOfBounds(long long int x, long long int y, long long int z) const;
	inline const BlockDatabase& GetBlockDatabase() const { return block_database_; };
	inline const WorldGenerator& GetWorldGenerator() const { return world_generator_; };
#ifdef VULKAN
	RendererRT& GetRenderer() const { return renderer_; };
	const std::vector<BottomLevelAccelerationStructure> GetAllBLAS() const;
#endif
#ifdef OPENGL
	void Draw() const;
#endif
private:
	BlockDatabase block_database_;
	WorldGenerator world_generator_;
#ifdef VULKAN
	RendererRT& renderer_;
#endif
	std::vector<Chunk> chunks_;
	int render_distance_;
	int generation_distance_;
	int chunks_dimension_length_;
	glm::i64vec3 chunk_offset_;
	glm::ivec3 internal_array_offset_;
};
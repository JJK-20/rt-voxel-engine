#include "ChunkManager.h"
#include <chrono>
#include <iostream>
using namespace std::chrono;

#ifdef OPENGL
ChunkManager::ChunkManager(int render_distance, glm::vec3 player_position, int world_generator_seed)
	:block_database_(), world_generator_(world_generator_seed)
#endif
#ifdef VULKAN
	ChunkManager::ChunkManager(int render_distance, glm::vec3 player_position, int world_generator_seed, RendererRT& renderer)
	:block_database_(), world_generator_(world_generator_seed), renderer_(renderer)
#endif
{
	render_distance_ = render_distance;
	generation_distance_ = render_distance + 1;
	chunks_dimension_length_ = 2 * generation_distance_ + 1;
	chunks_.reserve(chunks_dimension_length_ * chunks_dimension_length_);
	chunk_offset_ = { floor(player_position.x / (float)CHUNK_SIZE_X), 0, floor(player_position.z / (float)CHUNK_SIZE_Z) };
	internal_array_offset_ = glm::ivec3(0, 0, 0);
}

ChunkManager::~ChunkManager()
{
	for (int z = -generation_distance_; z <= generation_distance_; ++z)
		for (int x = -generation_distance_; x <= generation_distance_; ++x)
			if (chunks_[GetChunkIndex(x, z)].Meshed())
				chunks_[GetChunkIndex(x, z)].Delete();
}

void ChunkManager::GenerateChunks()
{
	for (int z = -generation_distance_ + chunk_offset_.z; z <= generation_distance_ + chunk_offset_.z; ++z)
		for (int x = -generation_distance_ + chunk_offset_.x; x <= generation_distance_ + chunk_offset_.x; ++x)
			chunks_.emplace_back(Chunk(glm::i64vec3(x, 0.0, z), *this));

	auto start = high_resolution_clock::now();

#ifndef _DEBUG
#pragma omp parallel for collapse(2)
#endif
	for (int z = -generation_distance_; z <= generation_distance_; ++z)
		for (int x = -generation_distance_; x <= generation_distance_; ++x)
			chunks_[GetChunkIndex(x, z)].Generate();

	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(stop - start);
	std::cout << "Generation time: " << duration.count() << std::endl;


	std::cout.setstate(std::ios_base::failbit);
	start = high_resolution_clock::now();

	// TODO instead of this we should have func which build one(and more but not all at one iteration) chunk,
	// closest to player, is it determined here or by the caller its another problem
#ifndef _DEBUG
#ifdef VULKAN
#pragma omp parallel for collapse(2)	// opengl and multithreading :)
#endif
#endif
	for (int z = -render_distance_; z <= render_distance_; ++z)
		for (int x = -render_distance_; x <= render_distance_; ++x)
			chunks_[GetChunkIndex(x, z)].BuildMesh();

	stop = high_resolution_clock::now();
	duration = duration_cast<milliseconds>(stop - start);
	std::cout.clear();
	std::cout << "Building mesh time (parallel, host only): " << duration.count() << std::endl;
}

void ChunkManager::UpdateCenter(glm::vec3 player_position)
{
	glm::i64vec3 new_chunk_offset = { floor(player_position.x / (float)CHUNK_SIZE_X), 0, floor(player_position.z / (float)CHUNK_SIZE_Z) };
	if (new_chunk_offset != chunk_offset_)
	{
		glm::ivec3 direction = new_chunk_offset - chunk_offset_;
		chunk_offset_ = new_chunk_offset;
		internal_array_offset_ = (internal_array_offset_ + direction + chunks_dimension_length_) % chunks_dimension_length_;

		int x_dir = (0 < direction.x) - (direction.x < 0);
		int x_range = direction.x;
		int z_dir = (0 < direction.z) - (direction.z < 0);
		int z_range = direction.z;

		if (x_dir)
			for (int x = x_dir * generation_distance_; x != x_dir * generation_distance_ - x_range; x = x - x_dir)
				for (int z = -generation_distance_; z <= generation_distance_; ++z)
				{
					if (chunks_[GetChunkIndex(x, z)].Meshed())
						chunks_[GetChunkIndex(x, z)].Delete();
					chunks_[GetChunkIndex(x, z)].ReuseChunk(glm::i64vec3(x + chunk_offset_.x, 0, z + chunk_offset_.z));
					chunks_[GetChunkIndex(x, z)].Generate();
				}
		if (z_dir)
			for (int z = z_dir * generation_distance_; z != z_dir * generation_distance_ - z_range; z = z - z_dir)
				for (int x = -generation_distance_; x <= generation_distance_; ++x)
				{
					if (chunks_[GetChunkIndex(x, z)].Meshed())
						chunks_[GetChunkIndex(x, z)].Delete();
					chunks_[GetChunkIndex(x, z)].ReuseChunk(glm::i64vec3(x + chunk_offset_.x, 0, z + chunk_offset_.z));
					chunks_[GetChunkIndex(x, z)].Generate();
				}

		if (x_dir)
			for (int x = x_dir * render_distance_; x != x_dir * render_distance_ - x_range; x = x - x_dir)
				for (int z = -render_distance_; z <= render_distance_; ++z)
					if (chunks_[GetChunkIndex(x, z)].Meshed() == false)
						chunks_[GetChunkIndex(x, z)].BuildMesh();
		if (z_dir)
			for (int z = z_dir * render_distance_; z != z_dir * render_distance_ - z_range; z = z - z_dir)
				for (int x = -render_distance_; x <= render_distance_; ++x)
					if (chunks_[GetChunkIndex(x, z)].Meshed() == false)
						chunks_[GetChunkIndex(x, z)].BuildMesh();

#ifdef VULKAN
		renderer_.ForceRebuild();
#endif
	}
}

void ChunkManager::SetBlock(long long int x, long long int y, long long int z, BlockId block)
{
	if (OutOfBounds(x, y, z))
	{
		//TODO for constant world size we cannot set block out of bounds, 
		//for infinite, we need to store in some list of not set blocks and set them in future once chunks are loaded
		assert(false);
		return;
	}
	int x_chunk = floor((double)x / (double)CHUNK_SIZE_X) - chunk_offset_.x;
	int z_chunk = floor((double)z / (double)CHUNK_SIZE_Z) - chunk_offset_.z;
	int x_local = ((x % CHUNK_SIZE_X) + CHUNK_SIZE_X) % CHUNK_SIZE_X;
	int z_local = ((z % CHUNK_SIZE_Z) + CHUNK_SIZE_Z) % CHUNK_SIZE_Z;

	chunks_[GetChunkIndex(x_chunk, z_chunk)].SetBlock(x_local, y, z_local, block);
}

BlockId ChunkManager::GetBlock(long long int x, long long int y, long long int z) const
{
	if (OutOfBounds(x, y, z))
		return	y >= CHUNK_SIZE_Y ? BlockId::Air : BlockId::Stone;
	int x_chunk = floor((double)x / (double)CHUNK_SIZE_X) - chunk_offset_.x;
	int z_chunk = floor((double)z / (double)CHUNK_SIZE_Z) - chunk_offset_.z;
	int x_local = ((x % CHUNK_SIZE_X) + CHUNK_SIZE_X) % CHUNK_SIZE_X;
	int z_local = ((z % CHUNK_SIZE_Z) + CHUNK_SIZE_Z) % CHUNK_SIZE_Z;

	return chunks_[GetChunkIndex(x_chunk, z_chunk)].GetBlock(x_local, y, z_local);
}

inline int ChunkManager::GetChunkIndex(int x, int z) const
{
	return (x + internal_array_offset_.x + generation_distance_ + chunks_dimension_length_) % chunks_dimension_length_ +
		(z + internal_array_offset_.z + generation_distance_ + chunks_dimension_length_) % chunks_dimension_length_ * chunks_dimension_length_;
}

Chunk& ChunkManager::GetChunk(long long int x, long long int z)
{
	return chunks_[GetChunkIndex(x, z)];
}

inline bool ChunkManager::OutOfBounds(long long int x, long long int y, long long int z) const
{
	/*	return  x >= CHUNK_SIZE_X * (1 + generation_distance_ + chunk_offset_.x) || x < CHUNK_SIZE_X * (-generation_distance_ + chunk_offset_.x) ||
			y >= CHUNK_SIZE_Y || y < 0 ||
			z >= CHUNK_SIZE_Z * (1 + generation_distance_ + chunk_offset_.z) || z < CHUNK_SIZE_Z * (-generation_distance_ + chunk_offset_.z);*/
	if (x >= CHUNK_SIZE_X * (1 + generation_distance_ + chunk_offset_.x) || x < CHUNK_SIZE_X * (-generation_distance_ + chunk_offset_.x) ||
		z >= CHUNK_SIZE_Z * (1 + generation_distance_ + chunk_offset_.z) || z < CHUNK_SIZE_Z * (-generation_distance_ + chunk_offset_.z))
		assert(false);	//TODO remove later
	return  y >= CHUNK_SIZE_Y || y < 0;
}

#ifdef VULKAN
const std::vector<BottomLevelAccelerationStructure> ChunkManager::GetAllBLAS() const
{
	std::vector<BottomLevelAccelerationStructure> blases;
	blases.reserve(chunks_dimension_length_ * chunks_dimension_length_);
	for (const auto& chunk : chunks_)
	{
		if (chunk.Meshed())
			blases.push_back(chunk.GetBLAS());
	}
	return blases;
}
#endif

#ifdef OPENGL
void ChunkManager::Draw() const
{
	for (int z = -generation_distance_; z <= generation_distance_; ++z)
		for (int x = -generation_distance_; x <= generation_distance_; ++x)
			if (chunks_[GetChunkIndex(x, z)].Meshed())
				chunks_[GetChunkIndex(x, z)].Draw();
}
#endif
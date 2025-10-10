#include "Chunk.h"
#include <FastNoiseLite/FastNoiseLite.h>
#include <chrono>
#include <iostream>
using namespace std::chrono;

Chunk::Chunk(glm::i64vec3 global_position, ChunkManager& chunk_manager)
	:global_position_(global_position), chunk_manager_(chunk_manager), generated_(false), meshed_(false)
#ifdef VULKAN
	, blased_(false)
#endif
{

}

void Chunk::ReuseChunk(glm::i64vec3 global_position)
{
	global_position_ = global_position;
	generated_ = false;
	meshed_ = false;
	#ifdef VULKAN
	blased_ = false;
	#endif
}

#ifdef VULKAN
const bool Chunk::Blased()
{
	if (!blased_)
		blased_ = chunk_manager_.GetRenderer().IsBlasBuilded(acceleration_structure_);
	return blased_;
}
#endif

void Chunk::Generate() 
{
	WorldGenerator world_generator = chunk_manager_.GetWorldGenerator();

	for (int z = 0; z < CHUNK_SIZE_Z; ++z) 
	{
		for (int x = 0; x < CHUNK_SIZE_X; ++x) 
		{
			int worldX = x + global_position_.x * CHUNK_SIZE_X;
			int worldZ = z + global_position_.z * CHUNK_SIZE_Z;

			HeightPayload height = world_generator.GenerateHeight(worldX, worldZ);

			for (int y = 0; y < CHUNK_SIZE_Y; ++y)
			{
				BlockId block = world_generator.GetBlockType(x, y, z, height);
				if (block == BlockId::Stone && y % 32 == 0)
					block = BlockId::Wood;
				blocks_[GetIndex(x, y, z)] = block;
			}
		}
	}
	generated_ = true;
}

void Chunk::Delete()
{
#ifdef OPENGL
	delete mesh_;
#endif
#ifdef VULKAN
	chunk_manager_.GetRenderer().FreeBlas(acceleration_structure_);
	blased_ = false;
#endif
	meshed_ = false;
	//std::cout << "Freeing Blas X: " << this->global_position_.x << " Y: " << this->global_position_.y << " Z: " << this->global_position_.z << std::endl;
}

void Chunk::SetBlock(int x, int y, int z, BlockId block)
{
	//TODO for world generation (tree) this could be a case, but should it be?
	if (OutOfBounds(x, y, z)) 
		assert(false);

	blocks_[GetIndex(x, y, z)] = block;
	meshed_ = false;
#ifdef VULKAN
	blased_ = false;
#endif
}

BlockId Chunk::GetBlock(int x, int y, int z) const
{
	if (OutOfBounds(x, y, z)) 
		return chunk_manager_.GetBlock(x + CHUNK_SIZE_X * global_position_.x, y, z + CHUNK_SIZE_Z * global_position_.z);

	return blocks_[GetIndex(x, y, z)];
}

inline int Chunk::GetIndex(int x, int y, int z) const
{
	return x + z * CHUNK_SIZE_X + y * (CHUNK_SIZE_X * CHUNK_SIZE_Z);
}

inline bool Chunk::OutOfBounds(int x, int y, int z) const
{
	return  x >= CHUNK_SIZE_X || x < 0 ||
		y >= CHUNK_SIZE_Y || y < 0 ||
		z >= CHUNK_SIZE_Z || z < 0;
}

// I was able to achieve another 10% speed-up by:
// - Allocating (resize()) vertex and index buffers once in SharedBufferPool for all chunks, instead of for each BuildMesh call.
// - Using [] instead of emplace_back to add new vertices and indices to the buffers 
//   (buffers need to be large enough to hold the maximum number of vertices and indices). 
//   This increases performance by avoiding bounds checks and potential reallocations. 
//   (In that case maybe just move into arrays, so it will get stack allocated and not heap allocated, cache friendliness??)
// - Reducing the number of parameters of AddFace by removing vertices, indices, and indexIndex, making them static, global, or class members.
//   The compiler might not have been able to optimize to pass all params through registers
// 
// Additionally, I need to look into optimizing GetBlock() as it was consuming around 30% of the time during development (now probably 20%), 
// since BuildMesh requires more AddFace calls for real meshes than during testing.
// 
// The most important optimization is to parallelize what can be parallelized. I will start with the "easy and fast to write" 
// ( opengl and multithreading, i changed my mind :) ) parallelization now, but in the future, I should implement it "the proper" way.
// 
// As someone smart said: "premature optimization is the root of all evil,", 
// so I will wait to optimize further until most features are added or until it becomes a real bottleneck.
// It also include using better algorithm (improving what we have) or moving into Greedy Meshing
void Chunk::BuildMesh()
{
	static auto total = high_resolution_clock::now() - high_resolution_clock::now();
	static int avg_ctr = 0;
	auto start = high_resolution_clock::now();

	const BlockDatabase& db = chunk_manager_.GetBlockDatabase();

	const float x_offset = global_position_.x * CHUNK_SIZE_X;
	const float y_offset = global_position_.y * CHUNK_SIZE_Y;
	const float z_offset = global_position_.z * CHUNK_SIZE_Z;

	std::vector<float> vertices;
	std::vector<unsigned int> indices;
	vertices.reserve(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * 4 * 5);//TODO
	indices.reserve(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z * 6);//TODO

	AdjacentBlockPositions neighbour;
	unsigned int indexIndex = 0;

	for (int y = 0; y < CHUNK_SIZE_Y; ++y)
		for (int z = 0; z < CHUNK_SIZE_Z; ++z)
			for (int x = 0; x < CHUNK_SIZE_X; ++x)
			{
				BlockId block = GetBlock(x, y, z);

				// TODO when split into solid and transparent skip when transparent
				if (block == BlockId::Air)
					continue;

				const auto& blockData = db.GetBlockData(block);

				//TODO get some data?
				neighbour.update(x, y, z);

				//ADD 6 faces if...
				//if (IsVisible(neighbour.down.x, neighbour.down.y, neighbour.down.z))
				if (IsVisible(blockData, neighbour.down.x, neighbour.down.y, neighbour.down.z))
					AddFace(vertices, indices, Face::BOTTOM_FACE, blockData.getUVBottom(), { x_offset + x, y_offset + y, z_offset + z }, indexIndex);
				//if (IsVisible(neighbour.up.x, neighbour.up.y, neighbour.up.z))
				if (IsVisible(blockData, neighbour.up.x, neighbour.up.y, neighbour.up.z))
					AddFace(vertices, indices, Face::TOP_FACE, blockData.getUVTop(), { x_offset + x, y_offset + y, z_offset + z }, indexIndex);
				//if (IsVisible(neighbour.back.x, neighbour.back.y, neighbour.back.z))
				if (IsVisible(blockData, neighbour.back.x, neighbour.back.y, neighbour.back.z))
					AddFace(vertices, indices, Face::BACK_FACE, blockData.getUVSides(), { x_offset + x, y_offset + y, z_offset + z }, indexIndex);
				//if (IsVisible(neighbour.front.x, neighbour.front.y, neighbour.front.z))
				if (IsVisible(blockData, neighbour.front.x, neighbour.front.y, neighbour.front.z))
					AddFace(vertices, indices, Face::FRONT_FACE, blockData.getUVSides(), { x_offset + x, y_offset + y, z_offset + z }, indexIndex);
				//if (IsVisible(neighbour.left.x, neighbour.left.y, neighbour.left.z))
				if (IsVisible(blockData, neighbour.left.x, neighbour.left.y, neighbour.left.z))
					AddFace(vertices, indices, Face::LEFT_FACE, blockData.getUVSides(), { x_offset + x, y_offset + y, z_offset + z }, indexIndex);
				//if (IsVisible(neighbour.right.x, neighbour.right.y, neighbour.right.z))
				if (IsVisible(blockData, neighbour.right.x, neighbour.right.y, neighbour.right.z))
					AddFace(vertices, indices, Face::RIGHT_FACE, blockData.getUVSides(), { x_offset + x, y_offset + y, z_offset + z }, indexIndex);
			}
#ifdef OPENGL
	mesh_ = new Mesh(vertices, indices);
#endif
#ifdef VULKAN
	acceleration_structure_ = chunk_manager_.GetRenderer().BuildBlas(vertices, indices);
#endif
	meshed_ = true;
	//std::cout << "Allocating Blas X: " << this->global_position_.x << " Y: " << this->global_position_.y << " Z: " << this->global_position_.z << std::endl;
	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(stop - start);
	std::cout << "current: " << duration.count() << std::endl;
	total += stop - start;
	std::cout << "total: " << duration_cast<milliseconds>(total).count() << std::endl;
	++avg_ctr;
	std::cout << "average: " << duration_cast<microseconds>(total / avg_ctr).count() << std::endl;
}

////TODO name
//bool Chunk::IsVisible(int x, int y, int z) const
//{
//	// TODO when split into solid and transparent
//	// for some reason this is slightly faster than: return GetBlock(x, y, z) == BlockId::Air;
//	if (GetBlock(x, y, z) == BlockId::Air)
//		return true;
//	return false;
//}

bool Chunk::IsVisible(const Block& block, int x, int y, int z) const
{
	const BlockDatabase& db = chunk_manager_.GetBlockDatabase();
	Block neighbour_block = db.GetBlockData(GetBlock(x, y, z));

	if (neighbour_block.isTransparent() && block.getId() != GetBlock(x, y, z))
		return true;
	return false;
}

void Chunk::AddFace(std::vector<float>& vertices, std::vector<unsigned int>& indices, Face face, const std::array<glm::vec2, 4>& uv, const glm::vec3& offset, unsigned int& indexIndex)
{
	const BlockDatabase& db = chunk_manager_.GetBlockDatabase();
	const auto& face_vert = db.GetFaceVertices(face);

	for (int i = 0; i < 4; ++i)
	{
		vertices.emplace_back(face_vert[i].x + offset.x);
		vertices.emplace_back(face_vert[i].y + offset.y);
		vertices.emplace_back(face_vert[i].z + offset.z);
		vertices.emplace_back(uv[i].x);
		vertices.emplace_back(uv[i].y);
		#ifdef OPENGL
		vertices.emplace_back(db.GetFaceLighting(face));	//temporary simple lighting
		#endif
	}

	indices.emplace_back(indexIndex);
	indices.emplace_back(indexIndex + 1);
	indices.emplace_back(indexIndex + 2);
	indices.emplace_back(indexIndex + 2);
	indices.emplace_back(indexIndex + 3);
	indices.emplace_back(indexIndex);

	indexIndex += 4;
}

#ifdef OPENGL
void Chunk::Draw() const
{
	mesh_->Draw();
}
#endif

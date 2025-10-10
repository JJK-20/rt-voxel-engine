#include "WorldGenerator.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <random>

WorldGenerator::WorldGenerator(int seed)
	: seed_(seed), octaves_(4), frequency_(0.00052137f), amplitude_(1.0f), persistance_(0.5f), base_height_(128), sea_level_(128)
{
	noise_.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	noise_.SetFrequency(1.0f);
	noise_.SetSeed(seed_);

	// Example control points, modify as needed
	continentalness_control_points_ =
	{
		{-1.0f, -68.0f},
		{-0.6f, -60.0f},
		{-0.5f, -36.0f},
		{-0.2f, -28.0f},
		{-0.1f,  0.0f},
		{ 0.0f,  3.0f},
		{ 0.25f, 24.0f},
		{ 0.55f, 48.0f},
		{ 1.0f,  80.0f},
	};

	// Example control points, modify as needed
	erosion_control_points_ =
	{
		  {-1.0f, 1.0f},
		  {-0.6f, 0.6f},
		  {-0.5f, 0.7f},
		  {-0.4f, 0.24f},
		  {-0.1f, 0.22f},
		  { 0.2f, 0.14f},
		  { 0.28f, 0.28f},
		  { 0.44f, 0.28f},
		  { 0.52f, 0.1f},
		  { 1.0f, 0.0f},
	};

	// Example control points, modify as needed
	peaks_and_valeys_control_points_ =
	{
		{-1.0f, -32.0f},
		{-0.4f, -24.0f},
		{ 0.0f,  0.0f},
		{ 0.6f,  32.0f},
		{ 0.8f,  38.0f},
		{ 1.0f,  46.0f},
	};
}

void WorldGenerator::SetSeed(int seed)
{
	seed_ = seed;
	noise_.SetSeed(seed_);
}

void WorldGenerator::SetOctaves(int octaves)
{
	octaves_ = octaves;
}

void WorldGenerator::SetFrequency(float frequency)
{
	frequency_ = frequency;
}

void WorldGenerator::SetAmplitude(float amplitude)
{
	amplitude_ = amplitude;
}

void WorldGenerator::SetPersistance(float persistance)
{
	persistance_ = persistance;
}

void WorldGenerator::SetBaseHeight(int baseHeight)
{
	base_height_ = baseHeight;
}

void WorldGenerator::SetSeaLevel(int seaLevel)
{
	sea_level_ = seaLevel;
}

HeightPayload WorldGenerator::GenerateHeight(int x, int z) const
{
	HeightPayload height;

	float continentalness = 0.0f;
	float current_frequency = frequency_;
	float current_amplitude = amplitude_;

	for (int i = 0; i < octaves_; ++i)
	{
		continentalness += noise_.GetNoise(x * current_frequency, z * current_frequency) * current_amplitude;
		current_frequency /= persistance_;
		current_amplitude *= persistance_;
	}



	float erosion = 0.0f;
	current_frequency = frequency_ * 0.63721;
	current_amplitude = amplitude_;
	int current_octaves_ = octaves_ + 2;

	for (int i = 0; i < current_octaves_; ++i)
	{
		erosion += noise_.GetNoise(x * current_frequency, z * current_frequency) * current_amplitude;
		current_frequency /= persistance_;
		current_amplitude *= persistance_;
	}



	float peaks_and_valeys = 0.0f;
	current_frequency = frequency_ * 3.21;
	current_amplitude = amplitude_;
	current_octaves_ = octaves_ * 2;

	for (int i = 0; i < current_octaves_; ++i)
	{
		peaks_and_valeys += noise_.GetNoise(x * current_frequency, z * current_frequency) * current_amplitude;
		current_frequency /= persistance_;
		current_amplitude *= persistance_;
	}
	peaks_and_valeys = 1 - abs(3 * abs(peaks_and_valeys) - 2);



	float height_add = 0.0f;
	float continentalness_add = SplineInterpolate(continentalness, continentalness_control_points_);
	float erosion_multiplayer = SplineInterpolate(erosion, erosion_control_points_);
	float peaks_and_valeys_add = SplineInterpolate(peaks_and_valeys, peaks_and_valeys_control_points_);

	if (continentalness_add > 0.0f)
		height_add += 0.6f * continentalness_add + 0.4f * continentalness_add * erosion_multiplayer;
	else
		height_add += continentalness_add;

	if (peaks_and_valeys_add > 0.0f)
		height_add += 0.3f * peaks_and_valeys_add + 0.7f * peaks_and_valeys_add * erosion_multiplayer;
	else
		height_add += peaks_and_valeys_add;

	height.height = roundf(height_add) + base_height_;
	height.continentalness = continentalness;
	height.erosion = erosion;
	height.peaks_and_valeys = peaks_and_valeys;

	if (GetBlockType(x, height.height, z, height) == BlockId::Grass && ShouldPlaceTree(x, z))
		height.should_place_tree = true;
	else
		height.should_place_tree = false;

	return height;
}

BlockId WorldGenerator::GetBlockType(int x, int y, int z, HeightPayload height) const
{
	// Landmass
	if (height.height >= sea_level_)
	{
		//Beach
		if (height.continentalness <= 0.0f)
		{
			if (y < height.height - 2) return BlockId::Stone;
			else if (y <= height.height) return BlockId::Sand;
			else return BlockId::Air;
		}
		//Inland
		if (y < height.height - 2) return BlockId::Stone;
		if (y <= height.height && height.height > base_height_ + base_height_ * 0.55) return BlockId::Stone;
		if (y <= height.height && height.height <= sea_level_ + 1) return BlockId::Sand;
		else if (y < height.height) return BlockId::Dirt;
		else if (y == height.height) return BlockId::Grass;
		else if (height.should_place_tree && y <= height.height + 4 ) return BlockId::Wood; // Instead we should do some decoration phase and place tree there
		else if (height.should_place_tree && y <= height.height + 8 ) return BlockId::Leaves; // Instead we should do some decoration phase and place tree there
		else return BlockId::Air;
	}
	// Ocean
	else
	{
		if (y < height.height - 2) return BlockId::Stone;
		else if (y <= height.height) return BlockId::Sand;
		else if (y <= sea_level_) return BlockId::Water;
		else return BlockId::Air;
	}
}

float WorldGenerator::SplineInterpolate(float x, const std::vector<std::pair<float, float>>& points) const
{
	if (x <= points.front().first)
		return points.front().second;
	if (x >= points.back().first)
		return points.back().second;

	// Linear Interpolation (Default)
	/*
	for (size_t i = 1; i < points.size(); ++i) {
		if (x < points[i].first) {
			float x0 = points[i - 1].first;
			float y0 = points[i - 1].second;
			float x1 = points[i].first;
			float y1 = points[i].second;

			float t = (x - x0) / (x1 - x0);
			return y0 + t * (y1 - y0);
		}
	}
	*/

	// Cosine Interpolation
	for (size_t i = 1; i < points.size(); ++i) {
		if (x < points[i].first) {
			float x0 = points[i - 1].first;
			float y0 = points[i - 1].second;
			float x1 = points[i].first;
			float y1 = points[i].second;

			float t = (x - x0) / (x1 - x0);
			float t2 = (1.0f - cos(t * M_PI)) / 2.0f;
			return y0 * (1.0f - t2) + y1 * t2;
		}
	}

	// Hermite Interpolation
	/*
	for (size_t i = 1; i < points.size(); ++i) {
		if (x < points[i].first) {
			float x0 = points[i - 1].first;
			float y0 = points[i - 1].second;
			float x1 = points[i].first;
			float y1 = points[i].second;

			float t = (x - x0) / (x1 - x0);
			float t2 = t * t;
			float t3 = t2 * t;

			float tension = 0.0f; // You can adjust this value (0.0 = Catmull-Rom spline)
			float h1 = 2.0f * t3 - 3.0f * t2 + 1.0f;
			float h2 = -2.0f * t3 + 3.0f * t2;
			float h3 = t3 - 2.0f * t2 + t;
			float h4 = t3 - t2;

			return y0 * h1 + y1 * h2 + tension * ((y1 - y0) * (h3 + h4));
		}
	}
	*/
}

bool WorldGenerator::ShouldPlaceTree(int x, int z) const
{
	// Determine the base grid point
	int grid_x = static_cast<int>(floor(x / tree_grid_size_));
	int grid_z = static_cast<int>(floor(z / tree_grid_size_));

	// Create a pseudo-random number generator based on grid position
	std::mt19937 rng(seed_ + grid_x * 374761393 + grid_z * 668265263);
	std::uniform_real_distribution<float> jitter_dist(-tree_jitter_amount_, tree_jitter_amount_);
	std::uniform_real_distribution<float> chance_dist(0.0f, 1.0f);

	// Calculate the exact tree position by adding jitter
	float jittered_x = grid_x * tree_grid_size_ + jitter_dist(rng);
	float jittered_z = grid_z * tree_grid_size_ + jitter_dist(rng);

	// Only place a tree if the jittered position is close to the original point
	if (std::abs(jittered_x - x) < 1.0f && std::abs(jittered_z - z) < 1.0f)
	{
		// Use noise or a random chance to decide if a tree should be placed
		float chance = chance_dist(rng);
		return chance < tree_chance_;
	}

	return false;
}
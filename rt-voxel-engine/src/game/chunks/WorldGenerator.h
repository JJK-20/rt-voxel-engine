#pragma once

#include "game/blocks/BlockDatabase.h"
#include <FastNoiseLite/FastNoiseLite.h>
#include <glm/glm.hpp>
#include <vector>
#include <utility>

struct HeightPayload
{
    int height;
    float continentalness;
    float erosion;
    float peaks_and_valeys;
    bool should_place_tree;
};

class WorldGenerator 
{
public:
    WorldGenerator(int seed = 0);

    void SetSeed(int seed);
    void SetOctaves(int octaves);
    void SetFrequency(float frequency);
    void SetAmplitude(float amplitude);
    void SetPersistance(float persistance);
    void SetBaseHeight(int baseHeight);
    void SetSeaLevel(int seaLevel);

    HeightPayload GenerateHeight(int x, int z) const;
    BlockId GetBlockType(int x, int y, int z, HeightPayload height) const;

private:
    float SplineInterpolate(float x, const std::vector<std::pair<float, float>>& points) const;

    FastNoiseLite noise_;
    int seed_;
    int octaves_;
    float frequency_;
    float amplitude_;
    float persistance_;

    int base_height_;
    int sea_level_;

    std::vector<std::pair<float, float>> continentalness_control_points_;
    std::vector<std::pair<float, float>> erosion_control_points_;
    std::vector<std::pair<float, float>> peaks_and_valeys_control_points_;

    float tree_chance_ = 0.20f; // 20% chance of tree spawning on a grass block
    float tree_grid_size_ = 4.0f;  // Distance between grid points
    float tree_jitter_amount_ = 3.0f;  // Maximum random offset from grid point

    bool ShouldPlaceTree(int x, int z) const;
};

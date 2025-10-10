#pragma once

#define RESOLUTION_WIDTH  3840
#define RESOLUTION_HEIGHT 2160

#define SEED 12345678					// Random world generator seed

#define RENDER_DISTANCE 50				// For RTX 4080, up to ~200 works fine with a static world

#define DYNAMIC_WORLD false				// Whether the world is generated dynamically around the player
										// Currently this feature is very WIP and may cause crashes
										// Works acceptable with RENDER_DISTANCE up to ~20; safe value: 10

// Player settings
#define PLAYER_START_POS glm::vec3(8.0f, 140.0f, 8.0f)

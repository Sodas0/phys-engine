#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "world.h"
#include <stdint.h>

// Minimal simulator: wraps World and provides clean API
typedef struct {
    World world;
    char scene_path[256];
    uint32_t seed;
    float dt;  // Fixed timestep (simulator-owned)
} Simulator;

// Core API (minimal, no episodes/rewards/randomization yet)
Simulator* sim_create(const char* scene_path, uint32_t seed, float dt);
void sim_destroy(Simulator* sim);
void sim_reset(Simulator* sim);
void sim_step(Simulator* sim, float action);

// Read-only access to world for rendering
World* sim_get_world(Simulator* sim);

#endif

#include "simulator.h"
#include "scene.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Helper: apply actuator pose (matches main.c logic exactly)
static void apply_actuator_pose(World *world, float angle) {
    if (world->actuator_body_index < 0) return;
    Body *beam = world_get_body(world, world->actuator_body_index);
    if (!beam || beam->shape.type != SHAPE_RECT) return;

    Body *base = world_get_body(world, 0);
    int use_fulcrum = (base && base != beam && base->shape.type == SHAPE_RECT);

    if (use_fulcrum) {
        float h_base = base->shape.rect.height;
        float h_beam = beam->shape.rect.height;
        float pivot_y = base->position.y - h_base * 0.5f;
        float beam_y = pivot_y - h_beam * 0.5f;
        beam->position.x = base->position.x;
        beam->position.y = beam_y;
    } else {
        beam->position = world->actuator_pivot;
    }
    beam->angle = angle;
    beam->velocity = VEC2_ZERO;
    beam->angular_velocity = 0.0f;
}

Simulator* sim_create(const char* scene_path, uint32_t seed, float dt) {
    Simulator* sim = (Simulator*)malloc(sizeof(Simulator));
    if (!sim) return NULL;
    
    strncpy(sim->scene_path, scene_path, sizeof(sim->scene_path) - 1);
    sim->scene_path[sizeof(sim->scene_path) - 1] = '\0';
    sim->seed = seed;
    sim->dt = dt;
    
    // Load initial scene
    if (scene_load(scene_path, &sim->world) != 0) {
        free(sim);
        return NULL;
    }
    
    // Set simulator-owned dt
    sim->world.dt = dt;
    
    // Seed the world's RNG
    world_seed(&sim->world, seed);
    
    return sim;
}

void sim_destroy(Simulator* sim) {
    if (sim) {
        free(sim);
    }
}

void sim_reset(Simulator* sim) {
    if (!sim) return;
    
    // Reload scene from JSON (deterministic, no randomization yet)
    scene_load(sim->scene_path, &sim->world);
    sim->world.dt = sim->dt;
    world_seed(&sim->world, sim->seed);
}

void sim_step(Simulator* sim, float action) {
    if (!sim) return;
    
    // Apply action before physics step
    apply_actuator_pose(&sim->world, action);
    
    // Advance physics by one timestep
    world_step(&sim->world);
    
    // Reapply actuator pose after physics step
    apply_actuator_pose(&sim->world, action);
}

World* sim_get_world(Simulator* sim) {
    return sim ? &sim->world : NULL;
}

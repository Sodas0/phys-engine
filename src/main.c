#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "world.h"
#include "scene.h"
#include "vec2.h"

// === UNIT SYSTEM QUICK REFERENCE ===
// Scale: 100 pixels = 1 meter
// - Gravity: 981.0 px/s² = 9.81 m/s² (Earth standard)
// - Position: pixels (1920×1080 screen = 19.2m × 10.8m room)
// - Velocity: pixels/s (200 px/s = 2.0 m/s jogging speed)
// - Mass: kilograms (1.0 kg typical, 0 = static)
// - Time: seconds (dt = 0.016667 = 60 Hz)
// See UNITS.md for full documentation

// Window dimensions [im using 1920x1080 cause 1440p 240hz oled makes sim look amazing]
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

// Simulator-owned fixed timestep (seconds). Scenes do not specify dt.
#define SIM_DT  (1.0f / 120.0f)   // 120 Hz

// Actuator control -- simple beam for now
#define BEAM_ANGLE_SPEED  1.5f   // radians per second
#define BEAM_ANGLE_MAX    0.5f   // max tilt in radians (~28 degrees)

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

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("2D phys-eng",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED);

    // Seed random for spawn_random
    srand((unsigned)time(NULL));

    // === Load scene ===
    World world;
    if (scene_load("scenes/fulcrum.json", &world) != 0) {
        fprintf(stderr, "Failed to load scene\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    world.dt = SIM_DT;  // Simulator owns dt; scenes do not set it

    float beam_angle = 0.0f;

    // Configure debug visualization
    world.debug.show_velocity = 1;   // See velocity vectors
    world.debug.show_contacts = 1;   // See rect-rect contact points, normals, penetration

    // Frame timing: run physics at the rate specified by world.dt
    // Convert dt (seconds) to milliseconds for SDL_Delay
    Uint32 frame_time_ms = (Uint32)(world.dt * 1000.0f);
    if (frame_time_ms < 1) frame_time_ms = 1;  // Minimum 1ms delay

    int running = 1;
    SDL_Event event;
    while (running) {
        Uint64 frame_start = SDL_GetTicks64();
        
        // kb control for scene reload
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_R) {
                if (scene_load("scenes/fulcrum.json", &world) == 0) {
                    world.dt = SIM_DT;
                    beam_angle = 0.0f;
                    world.debug.show_velocity = 0;
                    world.debug.show_contacts = 0;
                    frame_time_ms = (Uint32)(world.dt * 1000.0f);
                    if (frame_time_ms < 1) frame_time_ms = 1;
                }
            }
        }

        // kb control for actuator tilt (scale by dt for frame-rate independence)
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if (world.actuator_body_index >= 0) {
            if (keys[SDL_SCANCODE_A]) beam_angle -= BEAM_ANGLE_SPEED * world.dt;
            if (keys[SDL_SCANCODE_D]) beam_angle += BEAM_ANGLE_SPEED * world.dt;
            if (beam_angle > BEAM_ANGLE_MAX)  beam_angle = BEAM_ANGLE_MAX;
            if (beam_angle < -BEAM_ANGLE_MAX) beam_angle = -BEAM_ANGLE_MAX;
            apply_actuator_pose(&world, beam_angle);
        }

        // Physics update
        world_step(&world);

        if (world.actuator_body_index >= 0)
            apply_actuator_pose(&world, beam_angle);

        // Render
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        world_render_debug(&world, renderer);

        SDL_RenderPresent(renderer);

        // Frame timing: sleep to maintain real-time simulation
        Uint64 elapsed = SDL_GetTicks64() - frame_start;
        if (elapsed < frame_time_ms) {
            SDL_Delay((Uint32)(frame_time_ms - elapsed));
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

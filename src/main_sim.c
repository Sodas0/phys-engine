#include <SDL.h>
#include <stdio.h>
#include "simulator.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define SIM_DT (1.0f / 240.0f)  // 240 Hz fixed physics timestep
#define BEAM_ANGLE_SPEED 1.5f   // radians per second
#define BEAM_ANGLE_MAX 0.5f     // max tilt in radians

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("2D phys-eng sim mode",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED);

    // Create simulator
    Simulator* sim = sim_create("scenes/ball_pit.json", 12345, SIM_DT);
    if (!sim) {
        fprintf(stderr, "Failed to create simulator\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Configure debug visualization
    World* world = sim_get_world(sim);
    world->debug.show_velocity = 1;
    world->debug.show_contacts = 1;

    float beam_angle = 0.0f;
    
    // Accumulator for fixed timestep
    Uint64 last_time = SDL_GetPerformanceCounter();
    float accumulator = 0.0f;
    
    int running = 1;
    SDL_Event event;
    
    while (running) {
        // Calculate elapsed time since last frame
        Uint64 current_time = SDL_GetPerformanceCounter();
        float frame_time = (float)(current_time - last_time) / SDL_GetPerformanceFrequency();
        last_time = current_time;
        
        // Cap frame time to prevent spiral of death
        if (frame_time > 0.25f) frame_time = 0.25f;
        
        accumulator += frame_time;
        
        // Handle input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_R) {
                sim_reset(sim);
                beam_angle = 0.0f;
            }
        }

        // Keyboard control for actuator (scale by frame_time, not SIM_DT)
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_A]) beam_angle -= BEAM_ANGLE_SPEED * frame_time;
        if (keys[SDL_SCANCODE_D]) beam_angle += BEAM_ANGLE_SPEED * frame_time;
        if (beam_angle > BEAM_ANGLE_MAX)  beam_angle = BEAM_ANGLE_MAX;
        if (beam_angle < -BEAM_ANGLE_MAX) beam_angle = -BEAM_ANGLE_MAX;

        // Run physics steps as needed to catch up to real time
        while (accumulator >= SIM_DT) {
            sim_step(sim, beam_angle);
            accumulator -= SIM_DT;
        }
        // sim_step(sim, beam_angle);

        // Render current state (interpolation could be added here later)
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        world_render_debug(world, renderer);
        SDL_RenderPresent(renderer);
    }

    sim_destroy(sim);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

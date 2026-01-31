#include <SDL.h>
#include <stdio.h>
#include "simulator.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define SIM_DT (1.0f / 240.0f)  // 240 Hz fixed physics timestep

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
    Simulator* sim = sim_create("scenes/fulcrum.json", 12345, SIM_DT);
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
    
    // Accumulator for fixed timestep
    Uint64 last_time = SDL_GetPerformanceCounter();
    float accumulator = 0.0f;
    
    // Debug stats tracking
    int frame_count = 0;
    float debug_timer = 0.0f;
    const float DEBUG_PRINT_INTERVAL = 1.0f;  

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
            }
        }

        // Keyboard control: generate normalized action command ∈ [-1, 1]
        // NO DIRECT STATE MODIFCATION ANYMORE.
        float action = 0.0f;
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_A]) action -= 1.0f;  // Left: negative command
        if (keys[SDL_SCANCODE_D]) action += 1.0f;  // Right: positive command

        // Run physics steps as needed to catch up to real time
        while (accumulator >= SIM_DT) {
            sim_step(sim, action);
            accumulator -= SIM_DT;
        }
        
        // Debug output: print actuator stats periodically, not sure why i didnt add this earlier when debugging accumulator problems.
        debug_timer += frame_time;
        frame_count++;
        if (debug_timer >= DEBUG_PRINT_INTERVAL) {
            float fps = frame_count / debug_timer;
            printf("[Actuator Debug] FPS: %.1f | Action: %+.3f | Angle: %+.4f rad (%.1f°) | AngVel: %+.4f rad/s | max angle reached: %s\n",
                   fps,
                   action,
                   sim->actuator.angle,
                   sim->actuator.angle * 57.2958f,  // Convert to degrees for readability
                   sim->actuator.angular_velocity,
                   (sim->actuator.angle >= 0.5f || sim->actuator.angle <= -0.5f) ? "YES" : "NO");
            debug_timer = 0.0f;
            frame_count = 0;
        }

        
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

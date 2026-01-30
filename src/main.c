#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "world.h"
#include "render.h"

#define PI 3.14159265358979323846f

// Window dimensions
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

//TODO: make main function more concise
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

    // === Initialize physics world ===
    World world;
    // SANITY TEST: Disable gravity to isolate angular impulse behavior
    // world_init(&world, vec2(0, 98.1f), 1.0f / 60.0f);  // Zero gravity
    world_init(&world, vec2(0, 0.0f), 1.0f / 60.0f);  // Zero gravity
    world_set_bounds(&world, 0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // Configure debug visualization
    world.debug.show_velocity = 1;  // See velocity vectors

    Body b;
    
    // Dynamic rectangle target (centered)
    b = body_create_rect(vec2(400, 300), 120.0f, 80.0f, 5.0f, 0.5f);
    b.color = (SDL_Color){100, 150, 255, 255};
    b.restitution = 1.0f;
    world_add_body(&world, b);
    
    // TEST 1: Circle fired at CORNER of rectangle (should cause rotation)
    b = body_default(vec2(100, 246), 20.0f);
    b.color = (SDL_Color){255, 100, 100, 255};
    b.velocity = vec2(200, 0);  // Fast horizontal shot at top-left corner
    b.restitution = 1.0f;
    world_add_body(&world, b);
    
    // // TEST 2: Circle fired at CENTER of rectangle (minimal rotation)
    // b = body_default(vec2(100, 300), 20.0f);
    // b.color = (SDL_Color){100, 255, 100, 255};
    // b.velocity = vec2(200, 0);  // Same speed, aimed at center
    // b.restitution = 0.6f;
    // world_add_body(&world, b);
    
    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
        }

        // Physics update
        world_step(&world);

        // Render
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        world_render_debug(&world, renderer);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

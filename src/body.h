#ifndef BODY_H
#define BODY_H

#include "vec2.h"
#include <SDL.h>

// Shape type enum
typedef enum {
    SHAPE_CIRCLE,
    SHAPE_RECT
} ShapeType;

typedef struct {
    ShapeType type;
    union{
        struct{
            float radius;
        } circle;

        struct{
            float width;
            float height;
        } rect;
    };
} Shape;

typedef struct Body {
    Vec2 position;
    Vec2 velocity;

    float mass;
    float inv_mass;
    float restitution;

    float angle;
    float angular_velocity;
    float inv_inertia;

    Shape shape;

    SDL_Color color;
} Body;


// === Circle constructors ===

// Create a dynamic circle body with given properties (full control)
Body body_create_circle(Vec2 pos, float radius, float mass, float restitution);

// Create a circle body with sensible defaults (mass=1, restitution=0.8, white color)
// This is the preferred constructor for most use cases
Body body_default(Vec2 pos, float radius);

// Create a static (immovable) circle body
Body body_create_static(Vec2 pos, float radius);

// === Rectangle constructors ===

// Create a dynamic rectangle body with given properties (full control)
// Computes moment of inertia: I = (1/12) * mass * (width^2 + height^2)
Body body_create_rect(Vec2 pos, float width, float height, float mass, float restitution);

// Create a rectangle body with sensible defaults (mass=1, restitution=0.8, white color)
Body body_default_rect(Vec2 pos, float width, float height);

// Create a static (immovable) rectangle body
Body body_create_static_rect(Vec2 pos, float width, float height);

// === Common functions ===

// Make an existing body static (sets inv_mass = 0, inv_inertia = 0)
void body_set_static(Body *b);

// Check if body is static (inv_mass == 0)
int body_is_static(const Body *b);

#endif // BODY_H

#include "body.h"

// === Circle constructors ===

Body body_create_circle(Vec2 pos, float radius, float mass, float restitution) {
    Body b;
    b.position = pos;
    b.velocity = VEC2_ZERO;
    b.mass = mass;
    b.inv_mass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
    b.restitution = restitution;
    b.color = (SDL_Color){255, 255, 255, 255};  // Default white
    
    // Shape
    b.shape.type = SHAPE_CIRCLE;
    b.shape.circle.radius = radius;
    
    // Rotational dynamics - circles use I = (1/2) * m * r^2
    b.angle = 0.0f;
    b.angular_velocity = 0.0f;
    float inertia = (mass > 0.0f) ? (0.5f * mass * radius * radius) : 0.0f;
    b.inv_inertia = (inertia > 0.0f) ? (1.0f / inertia) : 0.0f;
    
    return b;
}

Body body_default(Vec2 pos, float radius) {
    return body_create_circle(pos, radius, 1.0f, 0.8f);
}

Body body_create_static(Vec2 pos, float radius) {
    Body b;
    b.position = pos;
    b.velocity = VEC2_ZERO;
    b.mass = 0.0f;
    b.inv_mass = 0.0f;
    b.restitution = 0.5f;
    b.color = (SDL_Color){100, 100, 100, 255};  // Gray for static
    
    // Shape
    b.shape.type = SHAPE_CIRCLE;
    b.shape.circle.radius = radius;
    
    // Static bodies don't rotate
    b.angle = 0.0f;
    b.angular_velocity = 0.0f;
    b.inv_inertia = 0.0f;
    
    return b;
}

// === Rectangle constructors ===

Body body_create_rect(Vec2 pos, float width, float height, float mass, float restitution) {
    Body b;
    b.position = pos;
    b.velocity = VEC2_ZERO;
    b.mass = mass;
    b.inv_mass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
    b.restitution = restitution;
    b.color = (SDL_Color){255, 255, 255, 255};  // Default white
    
    // Shape
    b.shape.type = SHAPE_RECT;
    b.shape.rect.width = width;
    b.shape.rect.height = height;
    
    // Rotational dynamics - rectangles use I = (1/12) * m * (w^2 + h^2)
    b.angle = 0.0f;
    b.angular_velocity = 0.0f;
    float inertia = (mass > 0.0f) ? ((1.0f / 12.0f) * mass * (width * width + height * height)) : 0.0f;
    b.inv_inertia = (inertia > 0.0f) ? (1.0f / inertia) : 0.0f;
    
    return b;
}

Body body_default_rect(Vec2 pos, float width, float height) {
    return body_create_rect(pos, width, height, 1.0f, 0.8f);
}

Body body_create_static_rect(Vec2 pos, float width, float height) {
    Body b;
    b.position = pos;
    b.velocity = VEC2_ZERO;
    b.mass = 0.0f;
    b.inv_mass = 0.0f;
    b.restitution = 0.5f;
    b.color = (SDL_Color){100, 100, 100, 255};  // Gray for static
    
    // Shape
    b.shape.type = SHAPE_RECT;
    b.shape.rect.width = width;
    b.shape.rect.height = height;
    
    // Static bodies don't rotate
    b.angle = 0.0f;
    b.angular_velocity = 0.0f;
    b.inv_inertia = 0.0f;
    
    return b;
}

// === Common functions ===

void body_set_static(Body *b) {
    b->inv_mass = 0.0f;
    b->mass = 0.0f;
    b->inv_inertia = 0.0f;
}

int body_is_static(const Body *b) {
    return b->inv_mass == 0.0f;
}

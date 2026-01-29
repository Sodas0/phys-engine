#include "collision.h"
#include <math.h>

// --- Positional Correction ---
// Pushes overlapping bodies apart to prevent sinking
static void collision_positional_correction(Body *a, Body *b, Collision *col) {
    const float PERCENT = 0.2f;   // 20% of penetration corrected per iteration
    const float SLOP = 0.01f;     // Allow small overlap to prevent jitter
    
    float inv_mass_sum = a->inv_mass + b->inv_mass;
    if (inv_mass_sum == 0.0f) return;  // Both static
    
    float correction = fmaxf(col->penetration - SLOP, 0.0f) * PERCENT / inv_mass_sum;
    Vec2 correction_vec = vec2_scale(col->normal, correction);
    
    a->position = vec2_sub(a->position, vec2_scale(correction_vec, a->inv_mass));
    b->position = vec2_add(b->position, vec2_scale(correction_vec, b->inv_mass));
}


//TODO: add collision resolution for other shapes as well.
// --- Impulse-Based Collision Resolution ---
void collision_resolve(Body *a, Body *b, Collision *col) {
    // Early exit: both bodies are static
    float inv_mass_sum = a->inv_mass + b->inv_mass;
    if (inv_mass_sum == 0.0f) return;
    
    // Calculate relative velocity (b relative to a)
    Vec2 rel_vel = vec2_sub(b->velocity, a->velocity);
    
    // Relative velocity along collision normal
    float vel_along_normal = vec2_dot(rel_vel, col->normal);
    
    // Early exit: bodies are separating (moving apart)
    if (vel_along_normal > 0.0f) {
        // Still apply positional correction if overlapping
        collision_positional_correction(a, b, col);
        return;
    }
    
    // Calculate restitution (use minimum of the two bodies)
    float e = fminf(a->restitution, b->restitution);
    
    // Calculate impulse magnitude
    // j = -(1 + e) * v_rel_n / (inv_mass_a + inv_mass_b)
    float j = -(1.0f + e) * vel_along_normal / inv_mass_sum;
    
    // Apply impulse to velocities
    Vec2 impulse = vec2_scale(col->normal, j);
    a->velocity = vec2_sub(a->velocity, vec2_scale(impulse, a->inv_mass));
    b->velocity = vec2_add(b->velocity, vec2_scale(impulse, b->inv_mass));
    
    // Apply positional correction to prevent sinking
    collision_positional_correction(a, b, col);
}

int collision_detect_circles(const Body *a, const Body *b, Collision *out) {
    // Vector from A to B
    Vec2 ab = vec2_sub(b->position, a->position);
    
    // Distance squared between centers
    float dist_sq = vec2_len_sq(ab);
    float radius_sum = a->shape.circle.radius + b->shape.circle.radius;
    
    // Check if circles are overlapping
    if (dist_sq >= radius_sum * radius_sum) {
        return 0;  // No collision
    }
    
    float dist = sqrtf(dist_sq);
    
    // Handle case where circles are at the same position
    if (dist < 1e-8f) {
        out->normal = vec2(1.0f, 0.0f);  // Arbitrary direction
        out->penetration = radius_sum;
        out->contact = a->position;
    } else {
        // Normal points from A to B
        out->normal = vec2_scale(ab, 1.0f / dist);
        out->penetration = radius_sum - dist;
        // Contact point: on the surface of A, offset toward B
        out->contact = vec2_add(a->position, vec2_scale(out->normal, a->shape.circle.radius - out->penetration * 0.5f));
    }
    
    // body_a and body_b indices are set by the caller
    out->body_a = -1;
    out->body_b = -1;
    
    return 1;  // Collision detected
}

int collision_detect_circle_rect(const Body *circle, const Body *rect, Collision *out) {
    float radius = circle->shape.circle.radius;
    
    // Compute rectangle half-extents and bounds
    float half_w = rect->shape.rect.width * 0.5f;
    float half_h = rect->shape.rect.height * 0.5f;
    
    float rect_min_x = rect->position.x - half_w;
    float rect_max_x = rect->position.x + half_w;
    float rect_min_y = rect->position.y - half_h;
    float rect_max_y = rect->position.y + half_h;
    
    // Clamp circle center to rectangle bounds to find closest point
    float closest_x = fmaxf(rect_min_x, fminf(circle->position.x, rect_max_x));
    float closest_y = fmaxf(rect_min_y, fminf(circle->position.y, rect_max_y));
    Vec2 closest = vec2(closest_x, closest_y);
    
    // Vector from closest point to circle center
    Vec2 diff = vec2_sub(circle->position, closest);
    float dist_sq = vec2_len_sq(diff);
    
    // Check if circle center is inside the rectangle
    int inside = (circle->position.x >= rect_min_x && circle->position.x <= rect_max_x &&
                  circle->position.y >= rect_min_y && circle->position.y <= rect_max_y);
    
    if (!inside) {
        // Circle center is outside rectangle
        if (dist_sq >= radius * radius) {
            return 0;  // No collision
        }
        
        float dist = sqrtf(dist_sq);
        
        // Handle edge case: closest point is exactly at circle center (shouldn't happen if outside)
        if (dist < 1e-8f) {
            out->normal = vec2(1.0f, 0.0f);  // Arbitrary direction
            out->penetration = radius;
        } else {
            // Normal points from circle (A) toward rect (B)
            // diff points from rect toward circle, so negate it
            out->normal = vec2_scale(diff, -1.0f / dist);
            out->penetration = radius - dist;
        }
        out->contact = closest;
    } else {
        // Circle center is inside rectangle - find closest edge
        float dx_left = circle->position.x - rect_min_x;
        float dx_right = rect_max_x - circle->position.x;
        float dy_top = circle->position.y - rect_min_y;
        float dy_bottom = rect_max_y - circle->position.y;
        
        // Find minimum distance to edge
        // Normal points from circle toward rect interior (opposite of escape direction)
        // This way, -normal pushes circle OUT toward the nearest edge
        float min_dist = dx_left;
        out->normal = vec2(1.0f, 0.0f);   // Escape left, normal points right
        closest = vec2(rect_min_x, circle->position.y);
        
        if (dx_right < min_dist) {
            min_dist = dx_right;
            out->normal = vec2(-1.0f, 0.0f);  // Escape right, normal points left
            closest = vec2(rect_max_x, circle->position.y);
        }
        if (dy_top < min_dist) {
            min_dist = dy_top;
            out->normal = vec2(0.0f, 1.0f);   // Escape up, normal points down
            closest = vec2(circle->position.x, rect_min_y);
        }
        if (dy_bottom < min_dist) {
            min_dist = dy_bottom;
            out->normal = vec2(0.0f, -1.0f);  // Escape down, normal points up
            closest = vec2(circle->position.x, rect_max_y);
        }
        
        // Penetration is radius + distance to edge (since center is inside)
        out->penetration = radius + min_dist;
        out->contact = closest;
    }
    
    // body_a and body_b indices are set by the caller
    out->body_a = -1;
    out->body_b = -1;
    
    return 1;  // Collision detected
}

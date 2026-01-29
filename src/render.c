#include "render.h"
#include "body.h"
#include <math.h>

void render_circle(SDL_Renderer *r, int cx, int cy, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);

    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        SDL_RenderDrawPoint(r, cx + x, cy + y);
        SDL_RenderDrawPoint(r, cx + y, cy + x);
        SDL_RenderDrawPoint(r, cx - y, cy + x);
        SDL_RenderDrawPoint(r, cx - x, cy + y);
        SDL_RenderDrawPoint(r, cx - x, cy - y);
        SDL_RenderDrawPoint(r, cx - y, cy - x);
        SDL_RenderDrawPoint(r, cx + y, cy - x);
        SDL_RenderDrawPoint(r, cx + x, cy - y);

        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x--;
            err += 1 - 2 * x;
        }
    }
}

void render_circle_filled(SDL_Renderer *r, int cx, int cy, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);

    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        SDL_RenderDrawLine(r, cx - x, cy + y, cx + x, cy + y);
        SDL_RenderDrawLine(r, cx - x, cy - y, cx + x, cy - y);
        SDL_RenderDrawLine(r, cx - y, cy + x, cx + y, cy + x);
        SDL_RenderDrawLine(r, cx - y, cy - x, cx + y, cy - x);

        y++;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x--;
            err += 1 - 2 * x;
        }
    }
}

void render_rect(SDL_Renderer *r, int cx, int cy, int width, int height, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {cx - width / 2, cy - height / 2, width, height};
    SDL_RenderDrawRect(r, &rect);
}

void render_rect_filled(SDL_Renderer *r, int cx, int cy, int width, int height, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {cx - width / 2, cy - height / 2, width, height};
    SDL_RenderFillRect(r, &rect);
}

// Helper: rotate a point (px, py) around origin by angle (radians)
static void rotate_point(float px, float py, float angle, float *out_x, float *out_y) {
    float c = cosf(angle);
    float s = sinf(angle);
    *out_x = px * c - py * s;
    *out_y = px * s + py * c;
}

// Helper: get the 4 corners of a rotated rectangle (corners are output in order: TL, TR, BR, BL)
static void get_rect_corners(float cx, float cy, float width, float height, float angle,
                             float *x0, float *y0, float *x1, float *y1,
                             float *x2, float *y2, float *x3, float *y3) {
    float hw = width / 2.0f;
    float hh = height / 2.0f;

    // Local corners relative to center (before rotation)
    float lx[4] = {-hw, hw, hw, -hw};
    float ly[4] = {-hh, -hh, hh, hh};

    // Rotate each corner and translate to world position
    float rx, ry;
    rotate_point(lx[0], ly[0], angle, &rx, &ry); *x0 = cx + rx; *y0 = cy + ry;
    rotate_point(lx[1], ly[1], angle, &rx, &ry); *x1 = cx + rx; *y1 = cy + ry;
    rotate_point(lx[2], ly[2], angle, &rx, &ry); *x2 = cx + rx; *y2 = cy + ry;
    rotate_point(lx[3], ly[3], angle, &rx, &ry); *x3 = cx + rx; *y3 = cy + ry;
}

void render_rect_rotated(SDL_Renderer *r, float cx, float cy, float width, float height, float angle, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);

    float x0, y0, x1, y1, x2, y2, x3, y3;
    get_rect_corners(cx, cy, width, height, angle, &x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3);

    // Draw 4 edges
    SDL_RenderDrawLine(r, (int)x0, (int)y0, (int)x1, (int)y1);
    SDL_RenderDrawLine(r, (int)x1, (int)y1, (int)x2, (int)y2);
    SDL_RenderDrawLine(r, (int)x2, (int)y2, (int)x3, (int)y3);
    SDL_RenderDrawLine(r, (int)x3, (int)y3, (int)x0, (int)y0);
}

void render_rect_rotated_filled(SDL_Renderer *r, float cx, float cy, float width, float height, float angle, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);

    float x0, y0, x1, y1, x2, y2, x3, y3;
    get_rect_corners(cx, cy, width, height, angle, &x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3);

    // Put corners into arrays for easier processing
    float vx[4] = {x0, x1, x2, x3};
    float vy[4] = {y0, y1, y2, y3};

    // Find bounding box
    float min_y = vy[0], max_y = vy[0];
    for (int i = 1; i < 4; i++) {
        if (vy[i] < min_y) min_y = vy[i];
        if (vy[i] > max_y) max_y = vy[i];
    }

    // Scanline fill: for each row, find intersection with polygon edges
    for (int y = (int)min_y; y <= (int)max_y; y++) {
        float x_intersections[4];
        int num_intersections = 0;

        // Check each edge for intersection with this scanline
        for (int i = 0; i < 4; i++) {
            int j = (i + 1) % 4;
            float y1_edge = vy[i];
            float y2_edge = vy[j];

            // Check if scanline crosses this edge
            if ((y1_edge <= y && y2_edge > y) || (y2_edge <= y && y1_edge > y)) {
                // Calculate x intersection using linear interpolation
                float t = (y - y1_edge) / (y2_edge - y1_edge);
                float x_int = vx[i] + t * (vx[j] - vx[i]);
                x_intersections[num_intersections++] = x_int;
            }
        }

        // Sort intersections (simple bubble sort for 2 elements typically)
        for (int i = 0; i < num_intersections - 1; i++) {
            for (int j = i + 1; j < num_intersections; j++) {
                if (x_intersections[i] > x_intersections[j]) {
                    float temp = x_intersections[i];
                    x_intersections[i] = x_intersections[j];
                    x_intersections[j] = temp;
                }
            }
        }

        // Draw horizontal lines between pairs of intersections
        for (int i = 0; i + 1 < num_intersections; i += 2) {
            SDL_RenderDrawLine(r, (int)x_intersections[i], y, (int)x_intersections[i + 1], y);
        }
    }
}

void render_line(SDL_Renderer *r, int x1, int y1, int x2, int y2, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(r, x1, y1, x2, y2);
}

void render_point(SDL_Renderer *r, int x, int y, int size, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x - size / 2, y - size / 2, size, size};
    SDL_RenderFillRect(r, &rect);
}

void render_arrow(SDL_Renderer *r, int x, int y, float vx, float vy, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);

    int ex = x + (int)vx;
    int ey = y + (int)vy;

    // Main line
    SDL_RenderDrawLine(r, x, y, ex, ey);

    // Arrowhead (two small lines)
    float len = sqrtf(vx * vx + vy * vy);
    if (len < 1.0f) return;

    float nx = vx / len;
    float ny = vy / len;
    float px = -ny;
    float py = nx;
    float head = 8.0f;

    int ax = ex - (int)(nx * head + px * head * 0.5f);
    int ay = ey - (int)(ny * head + py * head * 0.5f);
    int bx = ex - (int)(nx * head - px * head * 0.5f);
    int by = ey - (int)(ny * head - py * head * 0.5f);

    SDL_RenderDrawLine(r, ex, ey, ax, ay);
    SDL_RenderDrawLine(r, ex, ey, bx, by);
}

void render_body(SDL_Renderer *r, const Body *b) {
    int cx = (int)b->position.x;
    int cy = (int)b->position.y;
    int radius = (int)b->radius;

    // Filled circle with body color
    render_circle_filled(r, cx, cy, radius, b->color);

    // White outline for visibility
    SDL_Color outline = {255, 255, 255, 255};
    render_circle(r, cx, cy, radius, outline);
}

void render_body_debug(SDL_Renderer *r, const Body *b, int show_velocity) {
    // Draw the body itself
    render_body(r, b);
    
    if (show_velocity && !body_is_static(b)) {
        float vel_scale = 20.0f;
        SDL_Color yellow = {255, 255, 0, 255};
        render_arrow(r, (int)b->position.x, (int)b->position.y,
                     b->velocity.x * vel_scale, b->velocity.y * vel_scale, yellow);
    }

    // Draw center point
    SDL_Color center_color = {255, 255, 255, 255};
    render_point(r, (int)b->position.x, (int)b->position.y, 4, center_color);
}

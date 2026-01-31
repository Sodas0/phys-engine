# Scene Files

This directory stores physics scenes as JSON for reproducibility.

## Scene Format

Each scene is defined in JSON with the following structure:

```json
{
  "world": {
    "gravity": [x, y],
    "bounds": {
      "left": value,
      "top": value,
      "right": value,
      "bottom": value
    }
  },
  "bodies": [
    // Array of body definitions [find examples in the json files]
  ]
}
```

### World Configuration

- `gravity`: [x, y] - Gravity vector in **pixels/s²**
  - Earth gravity: `[0, 981.0]` (9.81 m/s² × 100 px/m)
  - Zero gravity: `[0, 0]` (for collision tests)
- `bounds`: World boundaries in **pixels**
  - Standard 1080p: `left: 0, top: 0, right: 1920, bottom: 1080`
  - Physical size: 19.2m × 10.8m (with 100 px/m scale)

The fixed timestep (`dt`) is owned by the simulator (see `SIM_DT` in `main.c`); scene files do not specify it.

### Body Definitions

Each body in the `bodies` array can have the following properties:

#### Required Fields

- `type`: "circle" or "rect"
- `position`: [x, y] - Position in world space

#### Circle-Specific

- `radius`: Circle radius (required for circles)

#### Rectangle-Specific

- `width`: Rectangle width (required for rects)
- `height`: Rectangle height (required for rects)

#### Optional Fields (all body types)

- `mass`: Body mass in **kilograms (kg)** (default: 1.0)
  - Small ball: 0.5-1.5 kg
  - Box: 1.0-5.0 kg
  - Heavy object: 10.0+ kg
- `restitution`: Bounciness, 0-1 (default: 0.8)
  - 0.0 = no bounce (perfectly inelastic)
  - 1.0 = perfect bounce (perfectly elastic)
- `velocity`: [x, y] - Initial velocity in **pixels/s** (default: [0, 0])
  - 150 px/s = 1.5 m/s (walking speed)
  - 400 px/s = 4.0 m/s (running speed)
- `angular_velocity`: Initial angular velocity in **radians/s** (default: 0)
- `angle`: Initial rotation in **radians** (default: 0)
  - 0.785 rad ≈ 45°
  - 1.571 rad ≈ 90°
- `color`: [r, g, b, a] - RGBA color, 0-255 (default: [255, 255, 255, 255])
- `static`: true/false - Makes body immovable (mass = 0) (default: false)
- `actuator`: true/false - Marks body as controllable actuator (default: false)
  - Only one per scene
  - A/D keys tilt when supported in main loop

## Unit System Quick Reference

**Scale: 100 pixels = 1 meter**

| Quantity | Example Value | Physical Equivalent |
|----------|---------------|---------------------|
| Position | `[960, 540]` | Center of 1920×1080 screen (9.6m, 5.4m) |
| Radius | `30` px | 0.30 m (basketball) |
| Velocity | `200` px/s | 2.0 m/s (jogging) |
| Gravity | `981.0` px/s² | 9.81 m/s² (Earth) |
| Mass | `1.5` kg | 1.5 kg (typical ball) |

For full details, see [../UNITS.md](../UNITS.md)



## Usage
Loading a scene is easy - just replace the path in main.c
```c
World world;
if (scene_load("scenes/test_collision.json", &world) != 0) {
    fprintf(stderr, "Failed to load scene\n");
    return 1;
}
```

## Creating New Scenes

1. Create a new `.json` file in this directory
2. Define the world configuration and bodies
3. Update `main.c` to load your scene file
4. Compile and run to test

## Here is my personal preference:

- Use `static: true` for ground/wall bodies
- Lower `restitution` values (0.3-0.5) for more stable stacking
- Higher `restitution` values (0.8-1.0) for bouncy collisions
- Adjust `dt` if you need different simulation speed/accuracy trade-offs

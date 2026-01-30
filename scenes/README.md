# Scene Files

This directory is intended for storing scenes as JSON for reproducibility. 

## Scene Format

Each scene is defined in JSON with the following structure:

```json
{
  "world": {
    "gravity": [x, y],
    "dt": timestep,
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

- `gravity`: [x, y] - Gravity vector (default: [0, 98.1])
- `dt`: Fixed timestep in seconds (default: 0.016667, which is 1/60)
- `bounds`: World boundaries for constraining bodies

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

- `mass`: Body mass (default: 1.0)
- `restitution`: Bounciness, 0-1 (default: 0.8)
- `velocity`: [x, y] - Initial velocity (default: [0, 0])
- `angular_velocity`: Initial angular velocity in radians/s (default: 0)
- `angle`: Initial rotation in radians (default: 0)
- `color`: [r, g, b, a] - RGBA color, 0-255 (default: [255, 255, 255, 255])
- `static`: true/false - Makes body immovable (default: false)



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

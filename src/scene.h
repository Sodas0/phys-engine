#ifndef SCENE_H
#define SCENE_H

#include "world.h"

// Load a scene from a JSON file and populate the world
// Returns 0 on success, -1 on failure
int scene_load(const char *filepath, World *world);

#endif // SCENE_H

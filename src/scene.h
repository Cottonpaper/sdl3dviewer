#ifndef SCENE_H
#define SCENE_H

#include <stdbool.h>

#include "mathx.h"

/* World geometry. Shapes drive both drawing and collision.
   Cylinders are Y-axis aligned; base is the bottom center point. */
typedef enum {
    SHAPE_BOX,
    SHAPE_CYLINDER,
    SHAPE_SPHERE,
} ShapeType;

typedef struct {
    ShapeType type;
    vec3 color;
    union {
        struct { vec3 min, max; } box;
        struct { vec3 base; float radius, height; } cyl;
        struct { vec3 center; float radius; } sphere;
    };
} Shape;

void scene_init(void);
const Shape *scene_shapes(int *count);
bool scene_add_shape(Shape s); /* false when the world is full */
void scene_remove_shape(int index); /* index into scene_shapes(); shifts the rest down */
vec3 scene_spawn(void);

/* Plain-text level format ("# sdl3dviewer level v1", one shape per line).
   scene_load replaces the current world on success; false = no/bad file. */
bool scene_load(const char *path);
bool scene_save(const char *path);

#endif

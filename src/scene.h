#ifndef SCENE_H
#define SCENE_H

#include "mathx.h"

/* Axis-aligned box: used for both drawing and collision. */
typedef struct {
    vec3 min;
    vec3 max;
    vec3 color;
} Box;

const Box *scene_boxes(int *count);
vec3 scene_spawn(void);

#endif

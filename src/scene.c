#include "scene.h"

#define ARENA 20.0f   /* half-size of the walled play area */
#define WALL_H 3.0f
#define WALL_T 0.5f

#define BOX(x0, y0, z0, x1, y1, z1, r, g, b) \
    { { x0, y0, z0 }, { x1, y1, z1 }, { r, g, b } }

static const Box boxes[] = {

    /* Staircase: each step is jumpable from the previous one */
    BOX( 4.0f, 0.0f, -8.0f,  6.0f, 0.6f, -6.0f,  0.85f, 0.45f, 0.30f),
    BOX( 6.0f, 0.0f, -8.0f,  8.0f, 1.2f, -6.0f,  0.85f, 0.55f, 0.30f),
    BOX( 8.0f, 0.0f, -8.0f, 10.0f, 1.8f, -6.0f,  0.85f, 0.65f, 0.30f),
    BOX(10.0f, 0.0f, -8.0f, 14.0f, 2.4f, -4.0f,  0.85f, 0.75f, 0.30f),

    /* Pillars */
    BOX(-6.0f, 0.0f, -6.0f, -5.0f, 4.0f, -5.0f,  0.30f, 0.60f, 0.85f),
    BOX(-6.0f, 0.0f,  5.0f, -5.0f, 4.0f,  6.0f,  0.30f, 0.60f, 0.85f),
    BOX( 5.0f, 0.0f,  5.0f,  6.0f, 4.0f,  6.0f,  0.30f, 0.60f, 0.85f),

    /* Low crates */
    BOX(-2.0f, 0.0f, -12.0f,  0.0f, 1.0f, -10.0f,  0.40f, 0.75f, 0.35f),
    BOX( 0.5f, 0.0f, -12.5f,  1.5f, 0.5f, -11.5f,  0.45f, 0.80f, 0.40f),
    BOX(-1.5f, 1.0f, -11.5f, -0.5f, 1.8f, -10.5f,  0.35f, 0.70f, 0.30f),

    /* A wall section with a gap */
    BOX(-14.0f, 0.0f, 8.0f, -9.0f, 2.5f, 9.0f,  0.75f, 0.35f, 0.55f),
    BOX( -7.0f, 0.0f, 8.0f, -2.0f, 2.5f, 9.0f,  0.75f, 0.35f, 0.55f),

    /* Floating platform: jump across the gap from the top of the staircase */
    BOX(10.0f, 2.6f, -3.0f, 14.0f, 3.0f, 1.0f,  0.90f, 0.90f, 0.90f),
};

const Box *scene_boxes(int *count)
{
    *count = (int)(sizeof(boxes) / sizeof(boxes[0]));
    return boxes;
}

vec3 scene_spawn(void)
{
    return v3(0.0f, 0.0f, 4.0f);
}

#include "scene.h"

#include <stdio.h>
#include <string.h>

#define ARENA 20.0f   /* half-size of the walled play area */
#define WALL_H 3.0f
#define WALL_T 0.5f
#define MAX_SHAPES 512 /* level shapes + shapes added in build mode */

#define BOX(x0, y0, z0, x1, y1, z1, r, g, b) \
    { SHAPE_BOX, { r, g, b }, .box = { { x0, y0, z0 }, { x1, y1, z1 } } }

static const Shape level_shapes[] = {
    /* green */
    BOX(-2.00f, 0.00f, 0.00f, -1.00f, 3.00f, 1.00f,  0.40f, 0.80f, 0.35f),
    BOX(-4.00f, 0.00f, 3.00f, -3.00f, 3.00f, 4.00f,  0.40f, 0.80f, 0.35f),
    BOX(-4.00f, 0.00f, 1.00f, -3.00f, 3.00f, 2.00f,  0.40f, 0.80f, 0.35f),
    BOX(-1.00f, 0.00f, 7.00f, 0.00f, 3.00f, 8.00f,  0.40f, 0.80f, 0.35f),
    BOX(3.00f, 0.00f, 4.00f, 4.00f, 3.00f, 5.00f,  0.40f, 0.80f, 0.35f),
    BOX(3.00f, 0.00f, 3.00f, 4.00f, 3.00f, 4.00f,  0.40f, 0.80f, 0.35f),
    BOX(3.00f, 0.00f, 2.00f, 4.00f, 3.00f, 3.00f,  0.40f, 0.80f, 0.35f),
    BOX(3.00f, 0.00f, 1.00f, 4.00f, 3.00f, 2.00f,  0.40f, 0.80f, 0.35f),
    BOX(2.00f, 0.00f, 0.00f, 3.00f, 3.00f, 1.00f,  0.40f, 0.80f, 0.35f),
    BOX(1.00f, 0.00f, 0.00f, 2.00f, 3.00f, 1.00f,  0.40f, 0.80f, 0.35f),
    BOX(-4.00f, 0.00f, 2.00f, -3.00f, 3.00f, 3.00f,  0.40f, 0.80f, 0.35f),
    BOX(-3.00f, 0.00f, 0.00f, -2.00f, 3.00f, 1.00f,  0.40f, 0.80f, 0.35f),
    BOX(-2.00f, 0.00f, 7.00f, -1.00f, 3.00f, 8.00f,  0.40f, 0.80f, 0.35f),
    BOX(-3.00f, 0.00f, 7.00f, -2.00f, 3.00f, 8.00f,  0.40f, 0.80f, 0.35f),
    BOX(-4.00f, 0.00f, 6.00f, -3.00f, 3.00f, 7.00f,  0.40f, 0.80f, 0.35f),
    BOX(-4.00f, 0.00f, 5.00f, -3.00f, 3.00f, 6.00f,  0.40f, 0.80f, 0.35f),
    BOX(-4.00f, 0.00f, 4.00f, -3.00f, 3.00f, 5.00f,  0.40f, 0.80f, 0.35f),
    BOX(3.00f, 0.00f, 5.00f, 4.00f, 3.00f, 6.00f,  0.40f, 0.80f, 0.35f),
    BOX(0.00f, 0.00f, 7.00f, 1.00f, 3.00f, 8.00f,  0.40f, 0.80f, 0.35f),
    BOX(1.00f, 0.00f, 7.00f, 2.00f, 3.00f, 8.00f,  0.40f, 0.80f, 0.35f),
    BOX(3.00f, 0.00f, 6.00f, 4.00f, 3.00f, 7.00f,  0.40f, 0.80f, 0.35f),
    BOX(2.00f, 0.00f, 7.00f, 3.00f, 3.00f, 8.00f,  0.40f, 0.80f, 0.35f),
    /* blue */
    BOX(1.00f, 3.00f, 6.00f, 2.00f, 4.00f, 7.00f,  0.30f, 0.50f, 0.90f),
    BOX(0.00f, 3.00f, 6.00f, 1.00f, 4.00f, 7.00f,  0.30f, 0.50f, 0.90f),
    BOX(-1.00f, 3.00f, 6.00f, 0.00f, 4.00f, 7.00f,  0.30f, 0.50f, 0.90f),
    BOX(-2.00f, 3.00f, 6.00f, -1.00f, 4.00f, 7.00f,  0.30f, 0.50f, 0.90f),
    BOX(-3.00f, 3.00f, 6.00f, -2.00f, 4.00f, 7.00f,  0.30f, 0.50f, 0.90f),
    BOX(-3.00f, 3.00f, 5.00f, -2.00f, 4.00f, 6.00f,  0.30f, 0.50f, 0.90f),
    BOX(-3.00f, 3.00f, 4.00f, -2.00f, 4.00f, 5.00f,  0.30f, 0.50f, 0.90f),
    BOX(-3.00f, 3.00f, 3.00f, -2.00f, 4.00f, 4.00f,  0.30f, 0.50f, 0.90f),
    BOX(-3.00f, 3.00f, 2.00f, -2.00f, 4.00f, 3.00f,  0.30f, 0.50f, 0.90f),
    BOX(-3.00f, 3.00f, 1.00f, -2.00f, 4.00f, 2.00f,  0.30f, 0.50f, 0.90f),
    BOX(2.00f, 3.00f, 6.00f, 3.00f, 4.00f, 7.00f,  0.30f, 0.50f, 0.90f),
    BOX(2.00f, 3.00f, 5.00f, 3.00f, 4.00f, 6.00f,  0.30f, 0.50f, 0.90f),
    BOX(2.00f, 3.00f, 4.00f, 3.00f, 4.00f, 5.00f,  0.30f, 0.50f, 0.90f),
    BOX(2.00f, 3.00f, 3.00f, 3.00f, 4.00f, 4.00f,  0.30f, 0.50f, 0.90f),
    BOX(2.00f, 3.00f, 2.00f, 3.00f, 4.00f, 3.00f,  0.30f, 0.50f, 0.90f),
    BOX(2.00f, 3.00f, 1.00f, 3.00f, 4.00f, 2.00f,  0.30f, 0.50f, 0.90f),
    BOX(1.00f, 3.00f, 1.00f, 2.00f, 4.00f, 2.00f,  0.30f, 0.50f, 0.90f),
    BOX(0.00f, 3.00f, 1.00f, 1.00f, 4.00f, 2.00f,  0.30f, 0.50f, 0.90f),
    BOX(0.00f, 3.00f, 0.00f, 1.00f, 4.00f, 1.00f,  0.30f, 0.50f, 0.90f),
    BOX(-1.00f, 3.00f, 0.00f, 0.00f, 4.00f, 1.00f,  0.30f, 0.50f, 0.90f),
    BOX(-1.00f, 3.00f, 1.00f, 0.00f, 4.00f, 2.00f,  0.30f, 0.50f, 0.90f),
    BOX(-2.00f, 3.00f, 1.00f, -1.00f, 4.00f, 2.00f,  0.30f, 0.50f, 0.90f),
    /* purple */
    BOX(1.00f, 4.00f, 5.00f, 2.00f, 5.00f, 6.00f,  0.60f, 0.40f, 0.85f),
    BOX(0.00f, 4.00f, 5.00f, 1.00f, 5.00f, 6.00f,  0.60f, 0.40f, 0.85f),
    BOX(-1.00f, 4.00f, 5.00f, 0.00f, 5.00f, 6.00f,  0.60f, 0.40f, 0.85f),
    BOX(-2.00f, 4.00f, 5.00f, -1.00f, 5.00f, 6.00f,  0.60f, 0.40f, 0.85f),
    BOX(-2.00f, 4.00f, 4.00f, -1.00f, 5.00f, 5.00f,  0.60f, 0.40f, 0.85f),
    BOX(-2.00f, 4.00f, 3.00f, -1.00f, 5.00f, 4.00f,  0.60f, 0.40f, 0.85f),
    BOX(-2.00f, 4.00f, 2.00f, -1.00f, 5.00f, 3.00f,  0.60f, 0.40f, 0.85f),
    BOX(-1.00f, 4.00f, 2.00f, 0.00f, 5.00f, 3.00f,  0.60f, 0.40f, 0.85f),
    BOX(0.00f, 4.00f, 2.00f, 1.00f, 5.00f, 3.00f,  0.60f, 0.40f, 0.85f),
    BOX(1.00f, 4.00f, 2.00f, 2.00f, 5.00f, 3.00f,  0.60f, 0.40f, 0.85f),
    BOX(1.00f, 4.00f, 4.00f, 2.00f, 5.00f, 5.00f,  0.60f, 0.40f, 0.85f),
    BOX(1.00f, 4.00f, 3.00f, 2.00f, 5.00f, 4.00f,  0.60f, 0.40f, 0.85f),
    /* yellow */
    BOX(-1.00f, 5.00f, 4.00f, 0.00f, 6.00f, 5.00f,  0.95f, 0.85f, 0.30f),
    BOX(0.00f, 5.00f, 4.00f, 1.00f, 6.00f, 5.00f,  0.95f, 0.85f, 0.30f),
    BOX(-1.00f, 5.00f, 3.00f, 0.00f, 6.00f, 4.00f,  0.95f, 0.85f, 0.30f),
    BOX(0.00f, 5.00f, 3.00f, 1.00f, 6.00f, 4.00f,  0.95f, 0.85f, 0.30f),
    /* green */
    BOX(-1.00f, 2.00f, 0.00f, 0.00f, 3.00f, 1.00f,  0.40f, 0.80f, 0.35f),
    BOX(0.00f, 2.00f, 0.00f, 1.00f, 3.00f, 1.00f,  0.40f, 0.80f, 0.35f),
};

#define LEVEL_SHAPE_COUNT (int)(sizeof(level_shapes) / sizeof(level_shapes[0]))

static Shape world_shapes[MAX_SHAPES];
static int world_count;

void scene_init(void)
{
    world_count = LEVEL_SHAPE_COUNT;
    memcpy(world_shapes, level_shapes, sizeof(level_shapes));
}

const Shape *scene_shapes(int *count)
{
    *count = world_count;
    return world_shapes;
}

bool scene_add_shape(Shape s)
{
    if (world_count >= MAX_SHAPES)
        return false;
    world_shapes[world_count++] = s;
    return true;
}

void scene_remove_shape(int index)
{
    if (index < 0 || index >= world_count)
        return;
    memmove(&world_shapes[index], &world_shapes[index + 1],
            (size_t)(world_count - index - 1) * sizeof(Shape));
    world_count--;
}

vec3 scene_spawn(void)
{
    return v3(0.0f, 0.0f, 4.0f);
}

/* --- level file ----------------------------------------------------------
   "# sdl3dviewer level v1" header, then one shape per line:
     box      x0 y0 z0 x1 y1 z1  r g b
     cylinder cx cy cz radius height  r g b
     sphere   cx cy cz radius  r g b
   Blank lines and # comments are ignored; unknown lines are skipped so
   future formats stay forward-compatible. */

bool scene_load(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        return false;

    Shape loaded[MAX_SHAPES];
    int count = 0;
    int version = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "# sdl3dviewer level v%d", &version) == 1)
            continue;

        char *p = line;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '\0' || *p == '\n' || *p == '\r' || *p == '#')
            continue;

        char type[16];
        if (sscanf(p, "%15s", type) != 1)
            continue;

        Shape s = { 0 };
        s.color = v3(1.0f, 1.0f, 1.0f);
        bool ok = false;

        if (strcmp(type, "box") == 0) {
            float x0, y0, z0, x1, y1, z1, r, g, b;
            if (sscanf(p, "%*s %f %f %f %f %f %f %f %f %f",
                       &x0, &y0, &z0, &x1, &y1, &z1, &r, &g, &b) == 9) {
                s.type = SHAPE_BOX;
                s.box.min = v3(x0, y0, z0);
                s.box.max = v3(x1, y1, z1);
                s.color = v3(r, g, b);
                ok = true;
            }
        } else if (strcmp(type, "cylinder") == 0) {
            float cx, cy, cz, rad, h, r, g, b;
            if (sscanf(p, "%*s %f %f %f %f %f %f %f %f",
                       &cx, &cy, &cz, &rad, &h, &r, &g, &b) == 8) {
                s.type = SHAPE_CYLINDER;
                s.cyl.base = v3(cx, cy, cz);
                s.cyl.radius = rad;
                s.cyl.height = h;
                s.color = v3(r, g, b);
                ok = true;
            }
        } else if (strcmp(type, "sphere") == 0) {
            float cx, cy, cz, rad, r, g, b;
            if (sscanf(p, "%*s %f %f %f %f %f %f %f",
                       &cx, &cy, &cz, &rad, &r, &g, &b) == 7) {
                s.type = SHAPE_SPHERE;
                s.sphere.center = v3(cx, cy, cz);
                s.sphere.radius = rad;
                s.color = v3(r, g, b);
                ok = true;
            }
        }
        if (!ok)
            continue; /* unknown type or bad line: skip */

        if (count >= MAX_SHAPES)
            break;
        loaded[count++] = s;
    }
    fclose(fp);

    if (count == 0 && version == 0)
        return false; /* not a level file */
    if (version > 1)
        return false; /* written by a newer version */

    world_count = count;
    memcpy(world_shapes, loaded, (size_t)count * sizeof(Shape));
    return true;
}

bool scene_save(const char *path)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
        return false;

    fprintf(fp, "# sdl3dviewer level v1\n");
    for (int i = 0; i < world_count; i++) {
        const Shape *s = &world_shapes[i];
        switch (s->type) {
        case SHAPE_BOX:
            fprintf(fp, "box %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g\n",
                    s->box.min.x, s->box.min.y, s->box.min.z,
                    s->box.max.x, s->box.max.y, s->box.max.z,
                    s->color.x, s->color.y, s->color.z);
            break;
        case SHAPE_CYLINDER:
            fprintf(fp, "cylinder %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g\n",
                    s->cyl.base.x, s->cyl.base.y, s->cyl.base.z,
                    s->cyl.radius, s->cyl.height,
                    s->color.x, s->color.y, s->color.z);
            break;
        case SHAPE_SPHERE:
            fprintf(fp, "sphere %.9g %.9g %.9g %.9g %.9g %.9g %.9g\n",
                    s->sphere.center.x, s->sphere.center.y, s->sphere.center.z,
                    s->sphere.radius,
                    s->color.x, s->color.y, s->color.z);
            break;
        }
    }
    fclose(fp);
    return true;
}

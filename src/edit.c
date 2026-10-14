#include "edit.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "scene.h"

#define PLACE_RANGE 25.0f
#define PLACE_HOVER 8.0f /* aiming at the sky: ghost floats this far ahead */
#define SNAP        0.5f
#define FIELD_MIN   0.25f
#define FIELD_MAX   8.0f
#define FIELD_MAXLEN 15

#define PLACE_R 0.95f
#define PLACE_G 0.60f
#define PLACE_B 0.20f

typedef struct {
    const char *name;
    const char *labels[3];
    float defaults[3];
    int field_count;
} ShapeDef;

static const ShapeDef shape_defs[EDIT_SHAPE_COUNT] = {
    { "BOX",      { "LENGTH", "WIDTH", "HEIGHT" }, { 2.0f, 2.0f, 1.0f }, 3 },
    { "CYLINDER", { "RADIUS", "HEIGHT" },          { 1.0f, 2.0f, 0.0f }, 2 },
    { "SPHERE",   { "RADIUS" },                    { 1.0f, 0.0f, 0.0f }, 1 },
};

const char *edit_shape_name(int shape)
{
    return shape_defs[shape].name;
}

int edit_field_count(int shape)
{
    return shape_defs[shape].field_count;
}

const char *edit_field_label(int shape, int field)
{
    return shape_defs[shape].labels[field];
}

const char *edit_field_text(const Editor *ed, int field)
{
    return ed->fields[ed->shape][field].text;
}

const char *edit_field_placeholder(const Editor *ed, int field)
{
    static char buf[3][16];
    snprintf(buf[field], sizeof(buf[field]), "%g", ed->fields[ed->shape][field].value);
    return buf[field];
}

void edit_init(Editor *ed)
{
    memset(ed, 0, sizeof(*ed));
    ed->mode = EDIT_PLAY;
    ed->shape = SHAPE_BOX;
    ed->focused = -1;
    ed->remove_idx = -1;
    ed->view_w = 1280.0f;
    ed->view_h = 720.0f;
    for (int s = 0; s < EDIT_SHAPE_COUNT; s++)
        for (int f = 0; f < 3; f++)
            ed->fields[s][f].value = shape_defs[s].defaults[f];
}

void edit_set_view_size(Editor *ed, int w, int h)
{
    if (w > 0) ed->view_w = (float)w;
    if (h > 0) ed->view_h = (float)h;
}

void edit_cancel(Editor *ed)
{
    if (ed->mode != EDIT_PLAY)
        ed->mode = EDIT_PLAY;
}

/* --- menu fields ------------------------------------------------------- */

static void field_reparse(EditField *f)
{
    char *end;
    float v = strtof(f->text, &end);
    if (end != f->text) { /* empty / invalid input keeps the last value */
        f->value = mx_clampf(v, FIELD_MIN, FIELD_MAX);
        f->has_value = true;
    }
}

static void field_push(EditField *f, const char *s)
{
    for (; *s; s++) {
        char c = *s;
        bool digit = c >= '0' && c <= '9';
        bool dot = c == '.' && !strchr(f->text, '.');
        size_t len = strlen(f->text);
        if ((digit || dot) && len < FIELD_MAXLEN) {
            f->text[len] = c;
            f->text[len + 1] = '\0';
        }
    }
    field_reparse(f);
}

static void field_backspace(EditField *f)
{
    size_t len = strlen(f->text);
    if (len)
        f->text[len - 1] = '\0';
    field_reparse(f);
}

static EditField *focused_field(Editor *ed)
{
    int n = edit_field_count(ed->shape);
    if (ed->focused < 0 || ed->focused >= n)
        return NULL;
    return &ed->fields[ed->shape][ed->focused];
}

static void confirm(Editor *ed)
{
    int n = edit_field_count(ed->shape);
    for (int f = 0; f < n; f++) { /* empty fields fall back to their default */
        if (!ed->fields[ed->shape][f].has_value) {
            ed->fields[ed->shape][f].value = shape_defs[ed->shape].defaults[f];
            ed->fields[ed->shape][f].has_value = true;
        }
    }
    ed->mode = EDIT_PLACE;
}

/* --- input ------------------------------------------------------------- */

void edit_key(Editor *ed, int key)
{
    switch (ed->mode) {
    case EDIT_PLAY:
        if (key == (int)SDL_SCANCODE_E)
            ed->mode = EDIT_MENU, ed->focused = 0;
        else if (key == (int)SDL_SCANCODE_R)
            ed->mode = EDIT_REMOVE, ed->remove_idx = -1;
        break;

    case EDIT_MENU:
        if (key == (int)SDL_SCANCODE_TAB) {
            ed->focused = (ed->focused + 1) % edit_field_count(ed->shape);
        } else if (key == (int)SDL_SCANCODE_BACKSPACE) {
            EditField *f = focused_field(ed);
            if (f)
                field_backspace(f);
        } else if (key == (int)SDL_SCANCODE_RETURN) {
            confirm(ed);
        } else if (key == (int)SDL_SCANCODE_ESCAPE) {
            ed->mode = EDIT_PLAY;
        }
        break;

    case EDIT_PLACE:
        if (key == (int)SDL_SCANCODE_E)
            ed->mode = EDIT_MENU, ed->focused = 0;
        break;

    case EDIT_REMOVE:
        if (key == (int)SDL_SCANCODE_R)
            ed->mode = EDIT_PLAY, ed->remove_idx = -1;
        break;
    }
}

void edit_text_input(Editor *ed, const char *text)
{
    if (ed->mode != EDIT_MENU)
        return;
    EditField *f = focused_field(ed);
    if (f)
        field_push(f, text);
}

void edit_mouse_move(Editor *ed, float x, float y)
{
    ed->mx = x;
    ed->my = y;
}

static bool in_rect(const EditRect *r, float x, float y)
{
    return x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

void edit_mouse_click(Editor *ed, int button, float x, float y)
{
    if (ed->mode != EDIT_MENU)
        return;
    if (button == SDL_BUTTON_RIGHT) {
        ed->mode = EDIT_PLAY;
        return;
    }
    if (button != SDL_BUTTON_LEFT)
        return;

    EditMenuLayout L;
    edit_menu_layout(ed, &L);

    for (int i = 0; i < EDIT_SHAPE_COUNT; i++)
        if (in_rect(&L.shape_btns[i], x, y)) {
            ed->shape = i;
            ed->focused = 0;
            return;
        }
    for (int f = 0; f < edit_field_count(ed->shape); f++)
        if (in_rect(&L.fields[f], x, y)) {
            ed->focused = f;
            return;
        }
    if (in_rect(&L.ok, x, y))
        confirm(ed);
}

/* --- menu layout (shared by drawing and hit-testing) ------------------- */

void edit_menu_layout(const Editor *ed, EditMenuLayout *out)
{
    const float pad = 18.0f;
    const float title_h = 24.0f;
    const float btn_w = 120.0f, btn_h = 40.0f, btn_gap = 12.0f;
    const float fld_w = 150.0f, fld_h = 36.0f, fld_gap = 16.0f;
    const float label_h = 16.0f, label_gap = 6.0f;
    const float ok_w = 120.0f, ok_h = 40.0f;
    const float hint_h = 20.0f;

    float inner_w = fmaxf(3 * btn_w + 2 * btn_gap, 3 * fld_w + 2 * fld_gap);
    float w = inner_w + pad * 2.0f;
    float y = 0.0f;

    float title_y = y;          y += title_h + 12.0f;
    float shape_y = y;          y += btn_h + 16.0f;
    float label_y = y;          y += label_h + label_gap;
    float fld_y = y;            y += fld_h + 18.0f;
    float ok_y = y;             y += ok_h + 12.0f;
    float hint_y = y;

    float h = y + hint_h + pad;
    float x0 = (ed->view_w - w) * 0.5f;
    float y0 = (ed->view_h - h) * 0.5f;

    out->panel = (EditRect){ x0, y0, w, h };
    out->title = (EditRect){ x0, y0 + title_y, w, title_h };
    for (int i = 0; i < EDIT_SHAPE_COUNT; i++)
        out->shape_btns[i] = (EditRect){ x0 + pad + i * (btn_w + btn_gap), y0 + shape_y, btn_w, btn_h };
    for (int i = 0; i < 3; i++) {
        out->field_labels[i] = (EditRect){ x0 + pad + i * (fld_w + fld_gap), y0 + label_y, fld_w, label_h };
        out->fields[i] = (EditRect){ x0 + pad + i * (fld_w + fld_gap), y0 + fld_y, fld_w, fld_h };
    }
    out->ok = (EditRect){ x0 + w - pad - ok_w, y0 + ok_y, ok_w, ok_h };
    out->hint = (EditRect){ x0, y0 + hint_y, w, hint_h };
}

/* --- ghost placement ---------------------------------------------------- */

static bool ray_shape(vec3 o, vec3 d, const Shape *s, float tmax, float *t_out)
{
    switch (s->type) {
    case SHAPE_BOX: {
        float oa[3] = { o.x, o.y, o.z };
        float da[3] = { d.x, d.y, d.z };
        float lo[3] = { s->box.min.x, s->box.min.y, s->box.min.z };
        float hi[3] = { s->box.max.x, s->box.max.y, s->box.max.z };
        float tmin = 0.0f;
        for (int a = 0; a < 3; a++) {
            if (fabsf(da[a]) < 1e-8f) {
                if (oa[a] < lo[a] || oa[a] > hi[a])
                    return false;
                continue;
            }
            float t1 = (lo[a] - oa[a]) / da[a];
            float t2 = (hi[a] - oa[a]) / da[a];
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax)
                return false;
        }
        *t_out = tmin;
        return tmax >= 0.0f;
    }

    case SHAPE_CYLINDER: {
        float ox = o.x - s->cyl.base.x, oz = o.z - s->cyl.base.z;
        float a = d.x * d.x + d.z * d.z;
        float best = -1.0f;
        if (a > 1e-8f) {
            float b = 2.0f * (ox * d.x + oz * d.z);
            float c = ox * ox + oz * oz - s->cyl.radius * s->cyl.radius;
            float disc = b * b - 4.0f * a * c;
            if (disc >= 0.0f) {
                float sq = sqrtf(disc);
                float cand[2] = { (-b - sq) / (2.0f * a), (-b + sq) / (2.0f * a) };
                for (int i = 0; i < 2; i++) {
                    float t = cand[i];
                    float y = o.y + t * d.y;
                    if (t > 1e-4f && t < tmax && y >= s->cyl.base.y && y <= s->cyl.base.y + s->cyl.height) {
                        best = t;
                        break;
                    }
                }
            }
        } else if (ox * ox + oz * oz < s->cyl.radius * s->cyl.radius && fabsf(d.y) > 1e-8f) {
            /* vertical ray inside the circle: hits a cap */
            float t = (s->cyl.base.y - o.y) / d.y;
            float t2 = (s->cyl.base.y + s->cyl.height - o.y) / d.y;
            if (t > t2) { float tmp = t; t = t2; t2 = tmp; }
            if (t > 1e-4f && t < tmax)
                best = t;
        }
        if (best < 0.0f)
            return false;
        *t_out = best;
        return true;
    }

    case SHAPE_SPHERE: {
        vec3 oc = v3_sub(o, s->sphere.center);
        float b = 2.0f * v3_dot(oc, d);
        float c = v3_dot(oc, oc) - s->sphere.radius * s->sphere.radius;
        float disc = b * b - 4.0f * c;
        if (disc < 0.0f)
            return false;
        float sq = sqrtf(disc);
        float cand[2] = { (-b - sq) / 2.0f, (-b + sq) / 2.0f };
        for (int i = 0; i < 2; i++) {
            if (cand[i] > 1e-4f && cand[i] < tmax) {
                *t_out = cand[i];
                return true;
            }
        }
        return false;
    }
    }
    return false;
}

/* Highest surface of a shape directly under (x, z). */
static float shape_ground_y(float x, float z, const Shape *s)
{
    switch (s->type) {
    case SHAPE_BOX:
        if (x >= s->box.min.x && x <= s->box.max.x && z >= s->box.min.z && z <= s->box.max.z)
            return s->box.max.y;
        return 0.0f;
    case SHAPE_CYLINDER: {
        float dx = x - s->cyl.base.x, dz = z - s->cyl.base.z;
        if (dx * dx + dz * dz <= s->cyl.radius * s->cyl.radius)
            return s->cyl.base.y + s->cyl.height;
        return 0.0f;
    }
    case SHAPE_SPHERE: {
        float dx = x - s->sphere.center.x, dz = z - s->sphere.center.z;
        float d2 = dx * dx + dz * dz;
        if (d2 <= s->sphere.radius * s->sphere.radius)
            return s->sphere.center.y + sqrtf(s->sphere.radius * s->sphere.radius - d2);
        return 0.0f;
    }
    }
    return 0.0f;
}

/* Highest surface under (x, z): the floor or the top of any shape. */
static float ground_y(float x, float z, const Shape *shapes, int shape_count)
{
    float g = 0.0f;
    for (int i = 0; i < shape_count; i++) {
        float top = shape_ground_y(x, z, &shapes[i]);
        if (top > g)
            g = top;
    }
    return g;
}

static float snap(float v)
{
    return floorf(v / SNAP + 0.5f) * SNAP;
}

/* Build the ghost shape with correct dimensions for the current selection;
   position is filled in by edit_update_ghost. */
static Shape pending_shape(const Editor *ed)
{
    Shape s = { 0 };
    s.type = (ShapeType)ed->shape;
    s.color = v3(PLACE_R, PLACE_G, PLACE_B);

    switch (s.type) {
    case SHAPE_BOX: {
        float len = ed->fields[ed->shape][0].value;
        float wid = ed->fields[ed->shape][1].value;
        float hgt = ed->fields[ed->shape][2].value;
        s.box.min = v3(-len * 0.5f, 0.0f, -wid * 0.5f);
        s.box.max = v3(len * 0.5f, hgt, wid * 0.5f);
        break;
    }
    case SHAPE_CYLINDER:
        s.cyl.base = v3(0.0f, 0.0f, 0.0f);
        s.cyl.radius = ed->fields[ed->shape][0].value;
        s.cyl.height = ed->fields[ed->shape][1].value;
        break;
    case SHAPE_SPHERE:
        s.sphere.center = v3(0.0f, 0.0f, 0.0f);
        s.sphere.radius = ed->fields[ed->shape][0].value;
        break;
    }
    return s;
}

void edit_update_ghost(Editor *ed, const Player *p, const Shape *shapes, int shape_count)
{
    ed->ghost_valid = false;
    if (ed->mode != EDIT_PLACE)
        return;

    vec3 o = player_eye(p);
    vec3 d = player_forward(p);
    if (v3_len(d) < 1e-6f)
        return;

    float t = PLACE_HOVER;
    if (d.y < -1e-6f) { /* floor plane y = 0 */
        float tf = -o.y / d.y;
        if (tf > 0.0f && tf < t)
            t = tf;
    }
    for (int i = 0; i < shape_count; i++) {
        float ts;
        if (ray_shape(o, d, &shapes[i], t, &ts) && ts > 0.0f && ts < t)
            t = ts;
    }

    vec3 hp = v3_add(o, v3_scale(d, t));
    float cx = snap(hp.x), cz = snap(hp.z);
    float gy = ground_y(cx, cz, shapes, shape_count);

    ed->ghost = pending_shape(ed);
    switch (ed->ghost.type) {
    case SHAPE_BOX:
        ed->ghost.box.min = v3(cx + ed->ghost.box.min.x, gy, cz + ed->ghost.box.min.z);
        ed->ghost.box.max = v3(cx + ed->ghost.box.max.x, gy + ed->ghost.box.max.y, cz + ed->ghost.box.max.z);
        break;
    case SHAPE_CYLINDER:
        ed->ghost.cyl.base = v3(cx, gy, cz);
        break;
    case SHAPE_SPHERE:
        ed->ghost.sphere.center = v3(cx, gy + ed->ghost.sphere.radius, cz);
        break;
    }
    ed->ghost_valid = true;
}

bool edit_place(Editor *ed)
{
    if (ed->mode != EDIT_PLACE || !ed->ghost_valid)
        return false;
    return scene_add_shape(ed->ghost);
}

/* --- remove mode -------------------------------------------------------- */

void edit_update_remove(Editor *ed, const Player *p, const Shape *shapes, int shape_count)
{
    ed->remove_idx = -1;
    if (ed->mode != EDIT_REMOVE)
        return;

    vec3 o = player_eye(p);
    vec3 d = player_forward(p);
    if (v3_len(d) < 1e-6f)
        return;

    float best = FLT_MAX;
    for (int i = 0; i < shape_count; i++) {
        float t;
        if (ray_shape(o, d, &shapes[i], best, &t) && t > 0.0f && t < best) {
            best = t;
            ed->remove_idx = i;
        }
    }
}

bool edit_remove(Editor *ed)
{
    if (ed->mode != EDIT_REMOVE || ed->remove_idx < 0)
        return false;
    scene_remove_shape(ed->remove_idx);
    ed->remove_idx = -1;
    return true;
}

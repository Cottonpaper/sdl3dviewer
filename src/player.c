#include "player.h"

#define PLAYER_RADIUS   0.3f
#define PLAYER_HEIGHT   1.8f
#define EYE_HEIGHT      1.65f
#define WALK_SPEED      5.0f
#define SPRINT_SPEED    9.0f
#define JUMP_SPEED      7.5f
#define GRAVITY         20.0f
#define MAX_FALL_SPEED  50.0f
#define MOUSE_SENS      0.0025f
#define SKIN            0.001f

void player_init(Player *p, vec3 spawn)
{
    p->pos = spawn;
    p->vel = v3(0.0f, 0.0f, 0.0f);
    p->yaw = mx_radians(-90.0f); /* look down -Z */
    p->pitch = 0.0f;
    p->grounded = false;
}

void player_look(Player *p, float dx, float dy)
{
    const float limit = mx_radians(89.0f);
    p->yaw += dx * MOUSE_SENS;
    p->pitch = mx_clampf(p->pitch - dy * MOUSE_SENS, -limit, limit);

    if (p->yaw > MX_PI) p->yaw -= 2.0f * MX_PI;
    if (p->yaw < -MX_PI) p->yaw += 2.0f * MX_PI;
}

vec3 player_eye(const Player *p)
{
    return v3(p->pos.x, p->pos.y + EYE_HEIGHT, p->pos.z);
}

vec3 player_forward(const Player *p)
{
    float cp = cosf(p->pitch);
    return v3(cp * cosf(p->yaw), sinf(p->pitch), cp * sinf(p->yaw));
}

/* Closest point of the player's AABB (from feet position) to c. */
static vec3 closest_on_player(vec3 c, vec3 pos)
{
    return v3(mx_clampf(c.x, pos.x - PLAYER_RADIUS, pos.x + PLAYER_RADIUS),
              mx_clampf(c.y, pos.y, pos.y + PLAYER_HEIGHT),
              mx_clampf(c.z, pos.z - PLAYER_RADIUS, pos.z + PLAYER_RADIUS));
}

static bool box_overlaps(const Shape *s, vec3 pos)
{
    return pos.x + PLAYER_RADIUS > s->box.min.x && pos.x - PLAYER_RADIUS < s->box.max.x &&
           pos.z + PLAYER_RADIUS > s->box.min.z && pos.z - PLAYER_RADIUS < s->box.max.z &&
           pos.y + PLAYER_HEIGHT > s->box.min.y && pos.y < s->box.max.y;
}

/* Player treated as a vertical cylinder: horizontal circle overlap. */
static bool cyl_overlaps(const Shape *s, vec3 pos)
{
    float dx = pos.x - s->cyl.base.x;
    float dz = pos.z - s->cyl.base.z;
    float rr = s->cyl.radius + PLAYER_RADIUS;
    return pos.y < s->cyl.base.y + s->cyl.height &&
           pos.y + PLAYER_HEIGHT > s->cyl.base.y &&
           dx * dx + dz * dz < rr * rr;
}

static bool sphere_overlaps(const Shape *s, vec3 pos)
{
    vec3 d = v3_sub(s->sphere.center, closest_on_player(s->sphere.center, pos));
    return v3_dot(d, d) < s->sphere.radius * s->sphere.radius;
}

static bool overlaps_player(const Shape *s, vec3 pos)
{
    switch (s->type) {
    case SHAPE_BOX:      return box_overlaps(s, pos);
    case SHAPE_CYLINDER: return cyl_overlaps(s, pos);
    case SHAPE_SPHERE:   return sphere_overlaps(s, pos);
    }
    return false;
}

/* Move along one axis, then push out of any shape we ended up inside. */
static void move_axis(Player *p, int axis, float delta, const Shape *shapes, int count)
{
    float *pos = axis == 0 ? &p->pos.x : axis == 1 ? &p->pos.y : &p->pos.z;
    *pos += delta;

    if (axis == 1) {
        p->grounded = false;
        if (p->pos.y < 0.0f) {
            p->pos.y = 0.0f;
            p->vel.y = 0.0f;
            p->grounded = true;
        }
    }

    for (int i = 0; i < count; i++) {
        const Shape *s = &shapes[i];
        if (!overlaps_player(s, p->pos))
            continue;

        switch (s->type) {
        case SHAPE_BOX:
            switch (axis) {
            case 0:
                p->pos.x = delta > 0.0f ? s->box.min.x - PLAYER_RADIUS - SKIN : s->box.max.x + PLAYER_RADIUS + SKIN;
                break;
            case 2:
                p->pos.z = delta > 0.0f ? s->box.min.z - PLAYER_RADIUS - SKIN : s->box.max.z + PLAYER_RADIUS + SKIN;
                break;
            case 1:
                if (delta <= 0.0f) {
                    p->pos.y = s->box.max.y;
                    p->grounded = true;
                } else {
                    p->pos.y = s->box.min.y - PLAYER_HEIGHT - SKIN;
                }
                p->vel.y = 0.0f;
                break;
            }
            break;

        case SHAPE_CYLINDER:
            if (axis == 1) {
                /* land on the flat top / bump the flat bottom */
                if (delta <= 0.0f) {
                    p->pos.y = s->cyl.base.y + s->cyl.height;
                    p->grounded = true;
                } else {
                    p->pos.y = s->cyl.base.y - PLAYER_HEIGHT - SKIN;
                }
                p->vel.y = 0.0f;
            } else {
                /* push out to the circular wall */
                float c = axis == 0 ? s->cyl.base.x : s->cyl.base.z;
                float other = axis == 0 ? p->pos.z : p->pos.x;
                float other_c = axis == 0 ? s->cyl.base.z : s->cyl.base.x;
                float d = other - other_c;
                float rr = s->cyl.radius + PLAYER_RADIUS + SKIN;
                float t = rr * rr - d * d;
                if (t > 0.0f) {
                    float off = sqrtf(t);
                    float *pa = axis == 0 ? &p->pos.x : &p->pos.z;
                    *pa = delta > 0.0f ? c + off : c - off;
                }
            }
            break;

        case SHAPE_SPHERE: {
            vec3 c = s->sphere.center;
            float r = s->sphere.radius;
            if (axis == 1) {
                /* land on the curved surface directly under / above us */
                float d = sqrtf((p->pos.x - c.x) * (p->pos.x - c.x) +
                                (p->pos.z - c.z) * (p->pos.z - c.z));
                if (d < r) {
                    float h = sqrtf(r * r - d * d);
                    if (delta <= 0.0f) {
                        p->pos.y = c.y + h;
                        p->grounded = true;
                    } else {
                        p->pos.y = c.y - h - PLAYER_HEIGHT - SKIN;
                    }
                    p->vel.y = 0.0f;
                }
                /* beside the sphere: horizontal pushes handle it */
            } else {
                /* axis-aligned push against the bounding box; the radial
                   pass after all axis moves rounds this off */
                float lo = (axis == 0 ? c.x : c.z) - r;
                float hi = (axis == 0 ? c.x : c.z) + r;
                *pos = delta > 0.0f ? lo - PLAYER_RADIUS - SKIN : hi + PLAYER_RADIUS + SKIN;
            }
            break;
        }
        }
    }
}

/* Round out sphere collisions: push the player out along the actual surface. */
static void sphere_push_out(Player *p, const Shape *s)
{
    vec3 c = s->sphere.center;
    vec3 diff = v3_sub(closest_on_player(c, p->pos), c);
    float dist = v3_len(diff);
    float target = s->sphere.radius + SKIN;
    if (dist >= target)
        return;

    vec3 n = dist > 1e-6f ? v3_scale(diff, 1.0f / dist) : v3(0.0f, 1.0f, 0.0f);
    p->pos = v3_add(p->pos, v3_scale(n, target - dist));
    if (n.y > 0.3f) { /* pushed mostly upward: standing on the sphere */
        p->grounded = true;
        p->vel.y = 0.0f;
    }
}

void player_update(Player *p, const PlayerInput *in, float dt, const Shape *shapes, int shape_count)
{
    vec3 fwd = v3(cosf(p->yaw), 0.0f, sinf(p->yaw));
    vec3 right = v3(-fwd.z, 0.0f, fwd.x);
    vec3 wish = v3(0.0f, 0.0f, 0.0f);

    if (in->forward) wish = v3_add(wish, fwd);
    if (in->back)    wish = v3_sub(wish, fwd);
    if (in->right)   wish = v3_add(wish, right);
    if (in->left)    wish = v3_sub(wish, right);
    wish = v3_scale(v3_norm(wish), in->sprint ? SPRINT_SPEED : WALK_SPEED);

    p->vel.x = wish.x;
    p->vel.z = wish.z;

    if (in->jump && p->grounded) {
        p->vel.y = JUMP_SPEED;
        p->grounded = false;
    }
    p->vel.y -= GRAVITY * dt;
    if (p->vel.y < -MAX_FALL_SPEED)
        p->vel.y = -MAX_FALL_SPEED;

    move_axis(p, 0, p->vel.x * dt, shapes, shape_count);
    move_axis(p, 2, p->vel.z * dt, shapes, shape_count);
    move_axis(p, 1, p->vel.y * dt, shapes, shape_count);

    for (int i = 0; i < shape_count; i++)
        if (shapes[i].type == SHAPE_SPHERE)
            sphere_push_out(p, &shapes[i]);
}

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

static bool overlaps(vec3 pos, const Box *b)
{
    return pos.x + PLAYER_RADIUS > b->min.x && pos.x - PLAYER_RADIUS < b->max.x &&
           pos.z + PLAYER_RADIUS > b->min.z && pos.z - PLAYER_RADIUS < b->max.z &&
           pos.y + PLAYER_HEIGHT > b->min.y && pos.y < b->max.y;
}

/* Move along one axis, then push out of any box we ended up inside. */
static void move_axis(Player *p, int axis, float delta, const Box *boxes, int box_count)
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

    for (int i = 0; i < box_count; i++) {
        const Box *b = &boxes[i];
        if (!overlaps(p->pos, b))
            continue;

        switch (axis) {
        case 0:
            p->pos.x = delta > 0.0f ? b->min.x - PLAYER_RADIUS - SKIN : b->max.x + PLAYER_RADIUS + SKIN;
            break;
        case 2:
            p->pos.z = delta > 0.0f ? b->min.z - PLAYER_RADIUS - SKIN : b->max.z + PLAYER_RADIUS + SKIN;
            break;
        case 1:
            if (delta <= 0.0f) {
                p->pos.y = b->max.y;
                p->grounded = true;
            } else {
                p->pos.y = b->min.y - PLAYER_HEIGHT - SKIN;
            }
            p->vel.y = 0.0f;
            break;
        }
    }
}

void player_update(Player *p, const PlayerInput *in, float dt, const Box *boxes, int box_count)
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

    move_axis(p, 0, p->vel.x * dt, boxes, box_count);
    move_axis(p, 2, p->vel.z * dt, boxes, box_count);
    move_axis(p, 1, p->vel.y * dt, boxes, box_count);
}

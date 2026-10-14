#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

#include "mathx.h"
#include "scene.h"

typedef struct {
    bool forward, back, left, right;
    bool jump, sprint;
} PlayerInput;

typedef struct {
    vec3 pos;      /* feet position */
    vec3 vel;
    float yaw;     /* radians, 0 = +X */
    float pitch;   /* radians, clamped to about +/-89 degrees */
    bool grounded;
} Player;

void player_init(Player *p, vec3 spawn);
void player_look(Player *p, float dx, float dy);
void player_update(Player *p, const PlayerInput *in, float dt, const Box *boxes, int box_count);

vec3 player_eye(const Player *p);
vec3 player_forward(const Player *p);

#endif

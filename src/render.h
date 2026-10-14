#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>

#include "player.h"
#include "scene.h"

bool render_init(void);
void render_resize(int width, int height);
void render_frame(const Player *player, const Box *boxes, int box_count);
void render_shutdown(void);

#endif

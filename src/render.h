#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>

#include "edit.h"
#include "menu.h"
#include "player.h"
#include "scene.h"

bool render_init(void);
void render_resize(int width, int height);
void render_set_ui_size(int width, int height); /* UI space in window coords */
void render_frame(const Player *player, const Shape *shapes, int shape_count,
                  const Editor *ed, const MenuState *menu);
void render_shutdown(void);

#endif

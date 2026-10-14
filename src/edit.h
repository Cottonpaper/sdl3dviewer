#ifndef EDIT_H
#define EDIT_H

#include <stdbool.h>

#include "player.h"

#define EDIT_SHAPE_COUNT 3
#define EDIT_COLOR_COUNT 9

/* Build-mode editor.

   EDIT_PLAY   normal play
   EDIT_MENU   mouse-driven build menu (mouse released, text input active);
               pick a shape, type its dimensions, Enter / OK confirms
   EDIT_PLACE  ghost of the shape follows the crosshair; left-click places;
               E reopens the menu; Esc cancels build mode entirely
   EDIT_REMOVE outline highlights the shape under the crosshair; left-click
               removes it; R or Esc returns to play */

typedef enum { EDIT_PLAY, EDIT_MENU, EDIT_PLACE, EDIT_REMOVE } EditMode;

typedef struct {
    char text[16]; /* raw typed digits (empty = fall back to value) */
    float value;   /* last valid parsed value */
    bool has_value;
} EditField;

typedef struct {
    EditMode mode;
    int shape;     /* SHAPE_* being placed */
    int color;     /* palette index for the placed shape */
    int focused;   /* menu field index, -1 = none */
    float mx, my;  /* menu cursor position (window coords) */
    EditField fields[EDIT_SHAPE_COUNT][3]; /* per shape, per field */
    Shape ghost;   /* box that would be placed (valid when ghost_valid) */
    bool ghost_valid;
    int remove_idx; /* shape under the crosshair in remove mode, -1 = none */
    float view_w, view_h;
} Editor;

typedef struct { float x, y, w, h; } EditRect;

typedef struct {
    EditRect panel, title, ok, hint;
    EditRect shape_btns[EDIT_SHAPE_COUNT];
    EditRect field_labels[3];
    EditRect fields[3];
    EditRect color_label;
    EditRect swatches[EDIT_COLOR_COUNT];
} EditMenuLayout;

void edit_init(Editor *ed);
void edit_set_view_size(Editor *ed, int w, int h);

void edit_key(Editor *ed, int key); /* non-repeat SDL_Scancode */
void edit_cancel(Editor *ed);
void edit_text_input(Editor *ed, const char *text);
void edit_mouse_move(Editor *ed, float x, float y);
void edit_mouse_click(Editor *ed, int button, float x, float y);

void edit_update_ghost(Editor *ed, const Player *p, const Shape *shapes, int shape_count);
bool edit_place(Editor *ed);
void edit_update_remove(Editor *ed, const Player *p, const Shape *shapes, int shape_count);
bool edit_remove(Editor *ed);

/* Menu geometry and data for the renderer; hit-testing and drawing share these. */
void edit_menu_layout(const Editor *ed, EditMenuLayout *out);
const char *edit_shape_name(int shape);
int edit_field_count(int shape);
const char *edit_field_label(int shape, int field);
const char *edit_field_text(const Editor *ed, int field);
const char *edit_field_placeholder(const Editor *ed, int field);
const char *edit_color_name(int color);
vec3 edit_color_rgb(int color);

#endif

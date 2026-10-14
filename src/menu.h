#ifndef MENU_H
#define MENU_H

#include <stdbool.h>

/* Pause menu, opened with Esc during normal play. Overlays the running
   game: the world keeps rendering behind the panel.

   MENU_MAIN     Return to game / Settings / Exit
   MENU_SETTINGS mouse sensitivity, move speed and sprint multiplier
                 steppers + back */

typedef enum { MENU_CLOSED, MENU_MAIN, MENU_SETTINGS } MenuMode;

typedef enum { MENU_ACT_NONE, MENU_ACT_RESUME, MENU_ACT_QUIT } MenuAction;

#define MENU_SETTING_COUNT 3 /* rows in the settings menu */

typedef struct { float x, y, w, h; } MenuRect;

typedef struct {
    MenuMode mode;
    float mx, my;    /* cursor position (window coords) */
    float sens;      /* mouse sensitivity multiplier, 1.0 = default */
    float speed;     /* walk speed, units/s */
    float sprint_mult; /* sprint speed multiplier */
    float view_w, view_h;
} MenuState;

typedef struct {
    MenuRect panel, title;
    MenuRect buttons[3]; /* main menu: return / settings / exit */
    MenuRect row_label[MENU_SETTING_COUNT];   /* settings rows */
    MenuRect row_minus[MENU_SETTING_COUNT];
    MenuRect row_value[MENU_SETTING_COUNT];
    MenuRect row_plus[MENU_SETTING_COUNT];
    MenuRect back;
} MenuLayout;

void menu_init(MenuState *m);
void menu_set_view_size(MenuState *m, int w, int h);
void menu_mouse_move(MenuState *m, float x, float y);

/* Clamp/snap all setting values into their valid ranges (after loading). */
void menu_sanitize(MenuState *m);

/* True if any setting differs (compared with a small tolerance, since the
   snapped values carry float rounding). */
bool menu_differs(const MenuState *a, const MenuState *b);

/* Handles one click; returns an action for main.c to act on (RESUME / QUIT).
   Setting changes are applied to m directly. */
MenuAction menu_click(MenuState *m, int button, float x, float y);

/* Menu geometry shared by hit-testing and drawing. Zero-sized rects are
   unused for the current mode. */
void menu_layout(const MenuState *m, MenuLayout *out);
const char *menu_setting_label(int row);
const char *menu_setting_text(const MenuState *m, int row);

#endif

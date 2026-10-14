#include "menu.h"

#include <math.h>
#include <stdio.h>

#include <SDL3/SDL.h>

/* Settings rows: label, step, min, max, display format.
   Order matches the layout rows top to bottom. */
typedef struct {
    const char *label;
    float step, min, max;
    const char *fmt;
} SettingDef;

static const SettingDef setting_defs[MENU_SETTING_COUNT] = {
    { "MOUSE SENSITIVITY", 0.25f, 0.25f, 4.0f,  "%.2fx" },
    { "MOVE SPEED",        0.50f, 1.0f,  20.0f, "%.1f"  },
    { "SPRINT MULTIPLIER", 0.10f, 1.0f,  3.0f,  "%.1fx" },
};

static float setting_get(const MenuState *m, int row)
{
    switch (row) {
    case 0: return m->sens;
    case 1: return m->speed;
    case 2: return m->sprint_mult;
    }
    return 0.0f;
}

static void setting_set(MenuState *m, int row, float v)
{
    switch (row) {
    case 0: m->sens = v; break;
    case 1: m->speed = v; break;
    case 2: m->sprint_mult = v; break;
    }
}

void menu_init(MenuState *m)
{
    m->mode = MENU_CLOSED;
    m->mx = 0.0f;
    m->my = 0.0f;
    m->sens = 1.0f;
    m->speed = 5.0f;
    m->sprint_mult = 1.8f;
    m->view_w = 1280.0f;
    m->view_h = 720.0f;
    menu_sanitize(m);
}

void menu_set_view_size(MenuState *m, int w, int h)
{
    if (w > 0) m->view_w = (float)w;
    if (h > 0) m->view_h = (float)h;
}

void menu_mouse_move(MenuState *m, float x, float y)
{
    m->mx = x;
    m->my = y;
}

void menu_sanitize(MenuState *m)
{
    for (int i = 0; i < MENU_SETTING_COUNT; i++) {
        const SettingDef *d = &setting_defs[i];
        float v = setting_get(m, i);
        if (v < d->min)
            v = d->min;
        if (v > d->max)
            v = d->max;
        v = roundf(v / d->step) * d->step;
        if (v < d->min)
            v = d->min;
        if (v > d->max)
            v = d->max;
        setting_set(m, i, v);
    }
}

bool menu_differs(const MenuState *a, const MenuState *b)
{
    for (int i = 0; i < MENU_SETTING_COUNT; i++) {
        float d = setting_get(a, i) - setting_get(b, i);
        if (d < -1e-4f || d > 1e-4f)
            return true;
    }
    return false;
}

static bool in_rect(const MenuRect *r, float x, float y)
{
    return x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

MenuAction menu_click(MenuState *m, int button, float x, float y)
{
    if (m->mode == MENU_CLOSED)
        return MENU_ACT_NONE;

    if (button == SDL_BUTTON_RIGHT) {
        if (m->mode == MENU_SETTINGS)
            m->mode = MENU_MAIN; /* right click backs out of settings */
        else
            return MENU_ACT_RESUME;
        return MENU_ACT_NONE;
    }
    if (button != SDL_BUTTON_LEFT)
        return MENU_ACT_NONE;

    MenuLayout L;
    menu_layout(m, &L);

    if (m->mode == MENU_MAIN) {
        if (in_rect(&L.buttons[0], x, y))
            return MENU_ACT_RESUME;
        if (in_rect(&L.buttons[1], x, y)) {
            m->mode = MENU_SETTINGS;
            return MENU_ACT_NONE;
        }
        if (in_rect(&L.buttons[2], x, y))
            return MENU_ACT_QUIT;
        if (in_rect(&L.reset, x, y))
            return MENU_ACT_RESET_WORLD;
        return MENU_ACT_NONE;
    }

    /* settings: steppers on every row */
    for (int i = 0; i < MENU_SETTING_COUNT; i++) {
        const SettingDef *d = &setting_defs[i];
        if (in_rect(&L.row_minus[i], x, y))
            setting_set(m, i, setting_get(m, i) - d->step);
        else if (in_rect(&L.row_plus[i], x, y))
            setting_set(m, i, setting_get(m, i) + d->step);
        else
            continue;
        menu_sanitize(m);
        break;
    }
    if (in_rect(&L.back, x, y))
        m->mode = MENU_MAIN;
    return MENU_ACT_NONE;
}

void menu_layout(const MenuState *m, MenuLayout *out)
{
    const float pad = 18.0f;
    const float title_h = 24.0f;
    const float btn_h = 40.0f, btn_gap = 12.0f;
    const float label_h = 16.0f, label_gap = 6.0f;
    const float row_gap = 14.0f;

    float w = 280.0f;
    float btn_w = w - pad * 2.0f;
    float x0 = (m->view_w - w) * 0.5f;
    float y = 0.0f;

    float title_y = y;
    y += title_h + 12.0f;

    MenuRect zero = { 0, 0, 0, 0 };
    for (int i = 0; i < 3; i++)
        out->buttons[i] = zero;
    for (int i = 0; i < MENU_SETTING_COUNT; i++)
        out->row_label[i] = out->row_minus[i] = out->row_value[i] = out->row_plus[i] = zero;
    out->back = zero;
    out->reset = zero;

    float h;
    if (m->mode == MENU_MAIN) {
        float btn_y[3];
        for (int i = 0; i < 3; i++) {
            btn_y[i] = y;
            y += btn_h + (i < 2 ? btn_gap : 0.0f);
        }
        float reset_y = y + btn_gap; /* reset button sits at the panel's bottom left */
        y = reset_y + btn_h;
        h = y + pad;
        float y0 = (m->view_h - h) * 0.5f;
        out->panel = (MenuRect){ x0, y0, w, h };
        out->title = (MenuRect){ x0, y0 + title_y, w, title_h };
        for (int i = 0; i < 3; i++)
            out->buttons[i] = (MenuRect){ x0 + pad, y0 + btn_y[i], btn_w, btn_h };
        out->reset = (MenuRect){ x0 + pad, y0 + reset_y, (btn_w - 12.0f) * 0.5f, btn_h };
    } else {
        float label_y[MENU_SETTING_COUNT], row_y[MENU_SETTING_COUNT], back_y;
        for (int i = 0; i < MENU_SETTING_COUNT; i++) {
            label_y[i] = y;
            y += label_h + label_gap;
            row_y[i] = y;
            y += btn_h + (i < MENU_SETTING_COUNT - 1 ? row_gap : 18.0f);
        }
        back_y = y;
        y += btn_h;
        h = y + pad;
        float y0 = (m->view_h - h) * 0.5f;

        out->panel = (MenuRect){ x0, y0, w, h };
        out->title = (MenuRect){ x0, y0 + title_y, w, title_h };

        const float ste_w = 40.0f, ste_gap = 12.0f;
        float val_w = btn_w - 2.0f * ste_w - 2.0f * ste_gap;
        for (int i = 0; i < MENU_SETTING_COUNT; i++) {
            out->row_label[i] = (MenuRect){ x0 + pad, y0 + label_y[i], btn_w, label_h };
            out->row_minus[i] = (MenuRect){ x0 + pad, y0 + row_y[i], ste_w, btn_h };
            out->row_value[i] = (MenuRect){ x0 + pad + ste_w + ste_gap, y0 + row_y[i], val_w, btn_h };
            out->row_plus[i] = (MenuRect){
                x0 + pad + ste_w + ste_gap + val_w + ste_gap, y0 + row_y[i], ste_w, btn_h
            };
        }
        out->back = (MenuRect){ x0 + pad, y0 + back_y, btn_w, btn_h };
    }
}

const char *menu_setting_label(int row)
{
    return setting_defs[row].label;
}

const char *menu_setting_text(const MenuState *m, int row)
{
    static char buf[MENU_SETTING_COUNT][16];
    snprintf(buf[row], sizeof(buf[row]), setting_defs[row].fmt, setting_get(m, row));
    return buf[row];
}

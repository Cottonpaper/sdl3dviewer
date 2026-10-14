#include <stdbool.h>
#include <stdio.h>

#include <glad/gl.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "edit.h"
#include "player.h"
#include "render.h"
#include "scene.h"

#define WINDOW_TITLE "sdl3dviewer"
#define WINDOW_W 1280
#define WINDOW_H 720
#define MAX_DT 0.05f

static GLADapiproc gl_loader(const char *name)
{
    return (GLADapiproc)SDL_GL_GetProcAddress(name);
}

static void set_mouse_captured(SDL_Window *window, bool *captured, bool on)
{
    *captured = on;
    SDL_SetWindowRelativeMouseMode(window, on);
}

/* Keep mouse capture and text input matching the editor mode: the menu needs
   a free cursor and typed digits, everything else follows the player's last
   look preference. */
static void sync_modes(SDL_Window *window, bool *captured, bool *text_active, const Editor *ed, bool want_look)
{
    bool want_cap = want_look && ed->mode != EDIT_MENU;
    if (want_cap != *captured)
        set_mouse_captured(window, captured, want_cap);

    bool want_text = ed->mode == EDIT_MENU;
    if (want_text != *text_active) {
        if (want_text)
            SDL_StartTextInput(window);
        else
            SDL_StopTextInput(window);
        *text_active = want_text;
    }
}

static void update_view_size(SDL_Window *window, Editor *ed)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    edit_set_view_size(ed, w, h);
    render_set_ui_size(w, h);
}

/* Level file lives in SDL's per-user pref directory so it works no matter
   how the game is started. */
static void level_file_path(char *buf, size_t n)
{
    buf[0] = '\0';
    char *dir = SDL_GetPrefPath("", "sdl3dviewer");
    if (!dir) {
        SDL_Log("Failed to get pref path: %s", SDL_GetError());
        return;
    }
    snprintf(buf, n, "%slevel.txt", dir);
    SDL_free(dir);
}

static void save_level(const char *path)
{
    if (!path[0]) {
        SDL_Log("No level file path; cannot save");
        return;
    }
    if (scene_save(path))
        SDL_Log("Level saved to %s", path);
    else
        SDL_Log("Failed to save level to %s: %s", path, SDL_GetError());
}

static PlayerInput read_input(void)
{
    const bool *keys = SDL_GetKeyboardState(NULL);
    PlayerInput in;
    in.forward = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    in.back = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
    in.left = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    in.right = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    in.jump = keys[SDL_SCANCODE_SPACE];
    in.sprint = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    return in;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window *window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_W, WINDOW_H,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl = SDL_GL_CreateContext(window);
    if (!gl) {
        SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!gladLoadGL(gl_loader)) {
        SDL_Log("Failed to load OpenGL functions");
        SDL_GL_DestroyContext(gl);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_Log("OpenGL %s, %s", (const char *)glGetString(GL_VERSION), (const char *)glGetString(GL_RENDERER));

    SDL_GL_SetSwapInterval(1);

    if (!render_init()) {
        SDL_GL_DestroyContext(gl);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int pw, ph;
    SDL_GetWindowSizeInPixels(window, &pw, &ph);
    render_resize(pw, ph);

    scene_init();

    char lvl_path[512];
    level_file_path(lvl_path, sizeof(lvl_path));
    SDL_Log("Level file: %s", lvl_path);
    if (scene_load(lvl_path)) {
        int n;
        scene_shapes(&n);
        SDL_Log("Loaded saved level (%d shapes)", n);
    } else
        SDL_Log("Using built-in level");

    Player player;
    player_init(&player, scene_spawn());

    Editor editor;
    edit_init(&editor);
    update_view_size(window, &editor);

    bool captured = false;
    bool text_active = false;
    bool want_look = true;
    set_mouse_captured(window, &captured, true);

    Uint64 last = SDL_GetTicksNS();
    Uint64 fps_timer = last;
    int fps_frames = 0;
    bool running = true;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (!e.key.repeat) {
                    if (e.key.scancode == SDL_SCANCODE_F5) {
                        save_level(lvl_path);
                    } else if (e.key.scancode == SDL_SCANCODE_ESCAPE) {
                        if (editor.mode != EDIT_PLAY)
                            edit_cancel(&editor); /* close menu / exit build mode */
                        else if (captured) {
                            want_look = false;
                            set_mouse_captured(window, &captured, false);
                        } else
                            running = false;
                    } else {
                        edit_key(&editor, (int)e.key.scancode);
                    }
                    sync_modes(window, &captured, &text_active, &editor, want_look);
                }
                break;
            case SDL_EVENT_TEXT_INPUT:
                edit_text_input(&editor, e.text.text);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (editor.mode == EDIT_MENU) {
                    edit_mouse_click(&editor, e.button.button, e.button.x, e.button.y);
                } else if (editor.mode == EDIT_PLACE) {
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        int shape_count;
                        const Shape *shapes = scene_shapes(&shape_count);
                        edit_update_ghost(&editor, &player, shapes, shape_count);
                        edit_place(&editor);
                    } else if (e.button.button == SDL_BUTTON_RIGHT) {
                        edit_cancel(&editor);
                    }
                } else if (editor.mode == EDIT_REMOVE) {
                    if (e.button.button == SDL_BUTTON_LEFT)
                        edit_remove(&editor);
                    else if (e.button.button == SDL_BUTTON_RIGHT)
                        edit_cancel(&editor);
                } else if (!captured) {
                    want_look = true;
                    set_mouse_captured(window, &captured, true);
                }
                sync_modes(window, &captured, &text_active, &editor, want_look);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (editor.mode == EDIT_MENU)
                    edit_mouse_move(&editor, e.motion.x, e.motion.y);
                else if (captured)
                    player_look(&player, e.motion.xrel, e.motion.yrel);
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                render_resize(e.window.data1, e.window.data2);
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                update_view_size(window, &editor);
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                set_mouse_captured(window, &captured, false);
                break;
            default:
                break;
            }
        }

        Uint64 now = SDL_GetTicksNS();
        float dt = (float)(now - last) / 1e9f;
        last = now;
        if (dt > MAX_DT)
            dt = MAX_DT;

        PlayerInput in = { 0 };
        if (captured)
            in = read_input();

        int shape_count;
        const Shape *shapes = scene_shapes(&shape_count); /* includes placed shapes */
        player_update(&player, &in, dt, shapes, shape_count);

        if (editor.mode == EDIT_PLACE)
            edit_update_ghost(&editor, &player, shapes, shape_count);
        else if (editor.mode == EDIT_REMOVE)
            edit_update_remove(&editor, &player, shapes, shape_count);

        render_frame(&player, shapes, shape_count, &editor);
        SDL_GL_SwapWindow(window);

        fps_frames++;
        if (now - fps_timer >= SDL_NS_PER_SECOND) {
            char title[64];
            snprintf(title, sizeof(title), "%s - %d FPS", WINDOW_TITLE, fps_frames);
            SDL_SetWindowTitle(window, title);
            fps_frames = 0;
            fps_timer = now;
        }
    }

    /* Autosave the world (built-in level + everything placed) on exit. */
    save_level(lvl_path);

    render_shutdown();
    SDL_GL_DestroyContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

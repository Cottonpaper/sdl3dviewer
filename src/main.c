#include <stdbool.h>
#include <stdio.h>

#include <glad/gl.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

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

    int box_count;
    const Box *boxes = scene_boxes(&box_count);

    Player player;
    player_init(&player, scene_spawn());

    bool captured = false;
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
                if (e.key.scancode == SDL_SCANCODE_ESCAPE && !e.key.repeat) {
                    if (captured)
                        set_mouse_captured(window, &captured, false);
                    else
                        running = false;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (!captured)
                    set_mouse_captured(window, &captured, true);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (captured)
                    player_look(&player, e.motion.xrel, e.motion.yrel);
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                render_resize(e.window.data1, e.window.data2);
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
        player_update(&player, &in, dt, boxes, box_count);

        render_frame(&player, boxes, box_count);
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

    render_shutdown();
    SDL_GL_DestroyContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

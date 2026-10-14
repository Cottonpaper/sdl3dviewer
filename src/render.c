#include "render.h"

#include <glad/gl.h>
#include <SDL3/SDL.h>

#define FOG_R 0.62f
#define FOG_G 0.74f
#define FOG_B 0.86f
#define FLOOR_SIZE 400.0f

static const char *vertex_src =
    "#version 330 core\n"
    "layout(location = 0) in vec3 a_pos;\n"
    "layout(location = 1) in vec3 a_normal;\n"
    "uniform mat4 u_model;\n"
    "uniform mat4 u_view_proj;\n"
    "out vec3 v_world;\n"
    "out vec3 v_normal;\n"
    "void main() {\n"
    "    vec4 world = u_model * vec4(a_pos, 1.0);\n"
    "    v_world = world.xyz;\n"
    "    v_normal = mat3(u_model) * a_normal;\n"
    "    gl_Position = u_view_proj * world;\n"
    "}\n";

static const char *fragment_src =
    "#version 330 core\n"
    "in vec3 v_world;\n"
    "in vec3 v_normal;\n"
    "uniform vec3 u_color;\n"
    "uniform vec3 u_eye;\n"
    "uniform vec3 u_fog_color;\n"
    "uniform int u_checker;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    vec3 n = normalize(v_normal);\n"
    "    vec3 base = u_color;\n"
    "    if (u_checker == 1) {\n"
    "        float c = mod(floor(v_world.x) + floor(v_world.z), 2.0);\n"
    "        base *= mix(0.8, 1.0, c);\n"
    "    }\n"
    "    vec3 light_dir = normalize(vec3(0.4, 1.0, 0.3));\n"
    "    float diffuse = max(dot(n, light_dir), 0.0);\n"
    "    vec3 color = base * (0.35 + 0.65 * diffuse);\n"
    "    float dist = length(v_world - u_eye);\n"
    "    float fog = clamp((dist - 20.0) / 60.0, 0.0, 1.0);\n"
    "    frag_color = vec4(mix(color, u_fog_color, fog), 1.0);\n"
    "}\n";

/* Unit cube centred on the origin: 6 faces * 2 triangles, position + normal. */
static const float cube_vertices[] = {
    /* +X */
     0.5f, -0.5f, -0.5f,  1, 0, 0,   0.5f,  0.5f, -0.5f,  1, 0, 0,   0.5f,  0.5f,  0.5f,  1, 0, 0,
     0.5f, -0.5f, -0.5f,  1, 0, 0,   0.5f,  0.5f,  0.5f,  1, 0, 0,   0.5f, -0.5f,  0.5f,  1, 0, 0,
    /* -X */
    -0.5f, -0.5f,  0.5f, -1, 0, 0,  -0.5f,  0.5f,  0.5f, -1, 0, 0,  -0.5f,  0.5f, -0.5f, -1, 0, 0,
    -0.5f, -0.5f,  0.5f, -1, 0, 0,  -0.5f,  0.5f, -0.5f, -1, 0, 0,  -0.5f, -0.5f, -0.5f, -1, 0, 0,
    /* +Y */
    -0.5f,  0.5f, -0.5f,  0, 1, 0,  -0.5f,  0.5f,  0.5f,  0, 1, 0,   0.5f,  0.5f,  0.5f,  0, 1, 0,
    -0.5f,  0.5f, -0.5f,  0, 1, 0,   0.5f,  0.5f,  0.5f,  0, 1, 0,   0.5f,  0.5f, -0.5f,  0, 1, 0,
    /* -Y */
    -0.5f, -0.5f,  0.5f,  0,-1, 0,  -0.5f, -0.5f, -0.5f,  0,-1, 0,   0.5f, -0.5f, -0.5f,  0,-1, 0,
    -0.5f, -0.5f,  0.5f,  0,-1, 0,   0.5f, -0.5f, -0.5f,  0,-1, 0,   0.5f, -0.5f,  0.5f,  0,-1, 0,
    /* +Z */
    -0.5f, -0.5f,  0.5f,  0, 0, 1,   0.5f, -0.5f,  0.5f,  0, 0, 1,   0.5f,  0.5f,  0.5f,  0, 0, 1,
    -0.5f, -0.5f,  0.5f,  0, 0, 1,   0.5f,  0.5f,  0.5f,  0, 0, 1,  -0.5f,  0.5f,  0.5f,  0, 0, 1,
    /* -Z */
     0.5f, -0.5f, -0.5f,  0, 0,-1,  -0.5f, -0.5f, -0.5f,  0, 0,-1,  -0.5f,  0.5f, -0.5f,  0, 0,-1,
     0.5f, -0.5f, -0.5f,  0, 0,-1,  -0.5f,  0.5f, -0.5f,  0, 0,-1,   0.5f,  0.5f, -0.5f,  0, 0,-1,
};

static GLuint program, vao, vbo;
static GLint loc_model, loc_view_proj, loc_color, loc_eye, loc_fog_color, loc_checker;
static float aspect = 16.0f / 9.0f;

static GLuint compile_shader(GLenum type, const char *src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        SDL_Log("Shader compile error: %s", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint link_program(const char *vs_src, const char *fs_src)
{
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    if (!vs || !fs) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        SDL_Log("Shader link error: %s", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

bool render_init(void)
{
    program = link_program(vertex_src, fragment_src);
    if (!program)
        return false;

    loc_model = glGetUniformLocation(program, "u_model");
    loc_view_proj = glGetUniformLocation(program, "u_view_proj");
    loc_color = glGetUniformLocation(program, "u_color");
    loc_eye = glGetUniformLocation(program, "u_eye");
    loc_fog_color = glGetUniformLocation(program, "u_fog_color");
    loc_checker = glGetUniformLocation(program, "u_checker");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(FOG_R, FOG_G, FOG_B, 1.0f);
    return true;
}

void render_resize(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;
    glViewport(0, 0, width, height);
    aspect = (float)width / (float)height;
}

static void draw_box(vec3 min, vec3 max, vec3 color, int checker)
{
    vec3 size = v3_sub(max, min);
    vec3 center = v3_scale(v3_add(min, max), 0.5f);
    mat4 model = mat4_mul(mat4_translate(center), mat4_scale(size));

    glUniformMatrix4fv(loc_model, 1, GL_FALSE, model.m);
    glUniform3f(loc_color, color.x, color.y, color.z);
    glUniform1i(loc_checker, checker);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void render_frame(const Player *player, const Box *boxes, int box_count)
{
    vec3 eye = player_eye(player);
    mat4 view = mat4_look_at(eye, v3_add(eye, player_forward(player)), v3(0.0f, 1.0f, 0.0f));
    mat4 proj = mat4_perspective(mx_radians(75.0f), aspect, 0.05f, 200.0f);
    mat4 view_proj = mat4_mul(proj, view);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniformMatrix4fv(loc_view_proj, 1, GL_FALSE, view_proj.m);
    glUniform3f(loc_eye, eye.x, eye.y, eye.z);
    glUniform3f(loc_fog_color, FOG_R, FOG_G, FOG_B);

    /* Floor: a large thin slab whose top face sits at y = 0. */
    draw_box(v3(-FLOOR_SIZE, -1.0f, -FLOOR_SIZE), v3(FLOOR_SIZE, 0.0f, FLOOR_SIZE),
             v3(0.45f, 0.50f, 0.45f), 1);

    for (int i = 0; i < box_count; i++)
        draw_box(boxes[i].min, boxes[i].max, boxes[i].color, 0);
}

void render_shutdown(void)
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
}

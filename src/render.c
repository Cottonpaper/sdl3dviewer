#include "render.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glad/gl.h>
#include <SDL3/SDL.h>

#include "edit.h"
#include "font.h"
#include "menu.h"

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

/* Wireframe outline (placement ghost): line edges of the unit cube. */
static const char *line_vertex_src =
    "#version 330 core\n"
    "layout(location = 0) in vec3 a_pos;\n"
    "uniform mat4 u_model;\n"
    "uniform mat4 u_view_proj;\n"
    "void main() {\n"
    "    gl_Position = u_view_proj * u_model * vec4(a_pos, 1.0);\n"
    "}\n";

static const char *line_fragment_src =
    "#version 330 core\n"
    "uniform vec4 u_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = u_color;\n"
    "}\n";

static const float line_vertices[] = {
    /* bottom ring (y = -0.5) */
    -0.5f,-0.5f,-0.5f,   0.5f,-0.5f,-0.5f,
     0.5f,-0.5f,-0.5f,   0.5f,-0.5f, 0.5f,
     0.5f,-0.5f, 0.5f,  -0.5f,-0.5f, 0.5f,
    -0.5f,-0.5f, 0.5f,  -0.5f,-0.5f,-0.5f,
    /* top ring (y = +0.5) */
    -0.5f, 0.5f,-0.5f,   0.5f, 0.5f,-0.5f,
     0.5f, 0.5f,-0.5f,   0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,  -0.5f, 0.5f,-0.5f,
    /* verticals */
    -0.5f,-0.5f,-0.5f,  -0.5f, 0.5f,-0.5f,
     0.5f,-0.5f,-0.5f,   0.5f, 0.5f,-0.5f,
     0.5f,-0.5f, 0.5f,   0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,
};

/* --- generated meshes ----------------------------------------------------
   Cylinder: Y-axis, unit radius 0.5, height 1, base at y = -0.5.
   Sphere: unit radius 0.5, centered on the origin. */

#define CYL_SEG 24
#define CYL_VERTS (CYL_SEG * 12) /* 6 side + 3 top cap + 3 bottom cap per segment */
static float cyl_verts[CYL_VERTS * 6];
static int cyl_vert_count = CYL_VERTS;

#define SPH_SEG 24
#define SPH_ROWS 16
#define SPH_VERTS (SPH_SEG * SPH_ROWS * 6)
static float sph_verts[SPH_VERTS * 6];
static int sph_vert_count = SPH_VERTS;

static void mesh_vert(float **out, vec3 p, vec3 n)
{
    float *v = *out;
    v[0] = p.x; v[1] = p.y; v[2] = p.z;
    v[3] = n.x; v[4] = n.y; v[5] = n.z;
    *out += 6;
}

static void build_cylinder(void)
{
    float *v = cyl_verts;
    for (int i = 0; i < CYL_SEG; i++) {
        float a0 = (float)i / CYL_SEG * 2.0f * MX_PI;
        float a1 = (float)(i + 1) / CYL_SEG * 2.0f * MX_PI;
        vec3 n0 = v3(cosf(a0), 0.0f, sinf(a0));
        vec3 n1 = v3(cosf(a1), 0.0f, sinf(a1));
        vec3 b0 = v3(0.5f * n0.x, -0.5f, 0.5f * n0.z);
        vec3 b1 = v3(0.5f * n1.x, -0.5f, 0.5f * n1.z);
        vec3 t0 = v3(0.5f * n0.x, 0.5f, 0.5f * n0.z);
        vec3 t1 = v3(0.5f * n1.x, 0.5f, 0.5f * n1.z);
        /* side (winding outward) */
        mesh_vert(&v, b0, n0); mesh_vert(&v, t0, n0); mesh_vert(&v, t1, n1);
        mesh_vert(&v, b0, n0); mesh_vert(&v, t1, n1); mesh_vert(&v, b1, n1);
        /* caps as fans around the centers */
        mesh_vert(&v, v3(0.0f, 0.5f, 0.0f), v3(0, 1, 0));
        mesh_vert(&v, t1, v3(0, 1, 0)); mesh_vert(&v, t0, v3(0, 1, 0));
        mesh_vert(&v, v3(0.0f, -0.5f, 0.0f), v3(0, -1, 0));
        mesh_vert(&v, b0, v3(0, -1, 0)); mesh_vert(&v, b1, v3(0, -1, 0));
    }
}

static vec3 sph_pt(float t, float f)
{
    return v3(0.5f * sinf(t) * cosf(f), 0.5f * cosf(t), 0.5f * sinf(t) * sinf(f));
}

static vec3 sph_n(float t, float f)
{
    return v3(sinf(t) * cosf(f), cosf(t), sinf(t) * sinf(f));
}

static void build_sphere(void)
{
    float *v = sph_verts;
    for (int r = 0; r < SPH_ROWS; r++) {
        float t0 = (float)r / SPH_ROWS * MX_PI;
        float t1 = (float)(r + 1) / SPH_ROWS * MX_PI;
        for (int s = 0; s < SPH_SEG; s++) {
            float f0 = (float)s / SPH_SEG * 2.0f * MX_PI;
            float f1 = (float)(s + 1) / SPH_SEG * 2.0f * MX_PI;
            vec3 A = sph_pt(t0, f0), B = sph_pt(t1, f0), C = sph_pt(t1, f1), D = sph_pt(t0, f1);
            vec3 nA = sph_n(t0, f0), nB = sph_n(t1, f0), nC = sph_n(t1, f1), nD = sph_n(t0, f1);
            mesh_vert(&v, A, nA); mesh_vert(&v, D, nD); mesh_vert(&v, C, nC);
            mesh_vert(&v, A, nA); mesh_vert(&v, C, nC); mesh_vert(&v, B, nB);
        }
    }
}

/* --- wireframe outlines (unit size, matching the meshes above) ---------- */

#define LINE_CYL_VERTS (CYL_SEG * 4 + (CYL_SEG / 6) * 2)
static float line_cyl_verts[LINE_CYL_VERTS * 3];
static int line_cyl_vert_count = LINE_CYL_VERTS;

#define LINE_SPH_VERTS (3 * CYL_SEG * 2 + 2 * CYL_SEG * 2)
static float line_sph_verts[LINE_SPH_VERTS * 3];
static int line_sph_vert_count = LINE_SPH_VERTS;

static void line_seg(float **out, vec3 a, vec3 b)
{
    float *v = *out;
    v[0] = a.x; v[1] = a.y; v[2] = a.z;
    v[3] = b.x; v[4] = b.y; v[5] = b.z;
    *out += 6;
}

static void build_line_cylinder(void)
{
    float *v = line_cyl_verts;
    for (int i = 0; i < CYL_SEG; i++) {
        float a0 = (float)i / CYL_SEG * 2.0f * MX_PI;
        float a1 = (float)(i + 1) / CYL_SEG * 2.0f * MX_PI;
        vec3 p0a = v3(0.5f * cosf(a0), -0.5f, 0.5f * sinf(a0));
        vec3 p1a = v3(0.5f * cosf(a1), -0.5f, 0.5f * sinf(a1));
        vec3 p0b = v3(0.5f * cosf(a0), 0.5f, 0.5f * sinf(a0));
        vec3 p1b = v3(0.5f * cosf(a1), 0.5f, 0.5f * sinf(a1));
        line_seg(&v, p0a, p1a); /* bottom ring */
        line_seg(&v, p0b, p1b); /* top ring */
    }
    for (int i = 0; i < CYL_SEG; i += 6) {
        float a = (float)i / CYL_SEG * 2.0f * MX_PI;
        line_seg(&v, v3(0.5f * cosf(a), -0.5f, 0.5f * sinf(a)),
                     v3(0.5f * cosf(a), 0.5f, 0.5f * sinf(a)));
    }
}

static void build_line_sphere(void)
{
    float *v = line_sph_verts;
    /* horizontal rings */
    for (int k = 0; k < 3; k++) {
        float t = (k + 1) / 4.0f * MX_PI; /* 45, 90, 135 degrees */
        for (int i = 0; i < CYL_SEG; i++) {
            float f0 = (float)i / CYL_SEG * 2.0f * MX_PI;
            float f1 = (float)(i + 1) / CYL_SEG * 2.0f * MX_PI;
            line_seg(&v, sph_pt(t, f0), sph_pt(t, f1));
        }
    }
    /* two vertical great circles (XY and ZY planes) */
    for (int k = 0; k < 2; k++)
        for (int i = 0; i < CYL_SEG; i++) {
            float t0 = (float)i / CYL_SEG * 2.0f * MX_PI;
            float t1 = (float)(i + 1) / CYL_SEG * 2.0f * MX_PI;
            vec3 a = k == 0 ? v3(0.5f * sinf(t0), 0.5f * cosf(t0), 0.0f)
                            : v3(0.0f, 0.5f * cosf(t0), 0.5f * sinf(t0));
            vec3 b = k == 0 ? v3(0.5f * sinf(t1), 0.5f * cosf(t1), 0.0f)
                            : v3(0.0f, 0.5f * cosf(t1), 0.5f * sinf(t1));
            line_seg(&v, a, b);
        }
}

/* 2D overlay (editor menu / hints): pixel-space positions, font atlas UVs,
   per-vertex color. One texture holds the font glyphs plus a white texel
   at (0, 0) that solid rectangles sample. */
static const char *ui_vertex_src =
    "#version 330 core\n"
    "layout(location = 0) in vec2 a_pos;\n"
    "layout(location = 1) in vec2 a_uv;\n"
    "layout(location = 2) in vec4 a_color;\n"
    "uniform vec2 u_res;\n"
    "out vec2 v_uv;\n"
    "out vec4 v_color;\n"
    "void main() {\n"
    "    vec2 clip = a_pos / u_res * 2.0 - 1.0;\n"
    "    gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);\n"
    "    v_uv = a_uv;\n"
    "    v_color = a_color;\n"
    "}\n";

static const char *ui_fragment_src =
    "#version 330 core\n"
    "in vec2 v_uv;\n"
    "in vec4 v_color;\n"
    "uniform sampler2D u_tex;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = v_color * texture(u_tex, v_uv);\n"
    "}\n";

#define ATLAS_W (1 + 95 * FONT_ADVANCE)
#define ATLAS_H FONT_GLYPH_H

typedef struct { float x, y, u, v, r, g, b, a; } UiVertex;
#define UI_MAX_VERTS 4096
static UiVertex ui_verts[UI_MAX_VERTS];
static int ui_count;

static GLuint program, vao, vbo;
static GLint loc_model, loc_view_proj, loc_color, loc_eye, loc_fog_color, loc_checker;
static float aspect = 16.0f / 9.0f;

static GLuint cyl_vao, cyl_vbo;
static GLuint sph_vao, sph_vbo;

static GLuint line_program, line_vao, line_vbo;             /* box edges */
static GLuint line_cyl_vao, line_cyl_vbo;
static GLuint line_sph_vao, line_sph_vbo;
static GLint lloc_model, lloc_view_proj, lloc_color;

static GLuint ui_program, ui_vao, ui_vbo, font_tex;
static GLint uiloc_res, uiloc_tex;
static float view_w = 1280.0f, view_h = 720.0f; /* UI space (window coords) */
static mat4 cur_view_proj;

static void mesh_vao(GLuint *vao_out, GLuint *vbo_out, const void *data, size_t bytes, bool normals)
{
    glGenVertexArrays(1, vao_out);
    glGenBuffers(1, vbo_out);
    glBindVertexArray(*vao_out);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo_out);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)bytes, data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (normals ? 6 : 3) * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    if (normals) {
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
    glBindVertexArray(0);
}

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

/* Font atlas: 95 printable glyphs, 1 px apart, plus a white texel at x = 0. */
static GLuint build_font_texture(void)
{
    unsigned char *px = calloc((size_t)ATLAS_W * ATLAS_H * 4, 1);
    if (!px)
        return 0;

    for (int y = 0; y < ATLAS_H; y++) { /* white sample texel for solid rects */
        unsigned char *p = &px[((size_t)y * ATLAS_W + 0) * 4];
        p[0] = p[1] = p[2] = p[3] = 255;
    }
    for (int i = 0; i < 95; i++) {
        const unsigned char *g = font_glyph((char)(32 + i));
        for (int col = 0; col < FONT_GLYPH_W; col++)
            for (int row = 0; row < FONT_GLYPH_H; row++)
                if (g[col] & (1u << row)) {
                    unsigned char *p = &px[((size_t)row * ATLAS_W + 1 + i * FONT_ADVANCE + col) * 4];
                    p[0] = p[1] = p[2] = p[3] = 255;
                }
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ATLAS_W, ATLAS_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    free(px);
    return tex;
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

    line_program = link_program(line_vertex_src, line_fragment_src);
    if (!line_program)
        return false;
    lloc_model = glGetUniformLocation(line_program, "u_model");
    lloc_view_proj = glGetUniformLocation(line_program, "u_view_proj");
    lloc_color = glGetUniformLocation(line_program, "u_color");

    build_cylinder();
    build_sphere();
    build_line_cylinder();
    build_line_sphere();
    mesh_vao(&cyl_vao, &cyl_vbo, cyl_verts, sizeof(cyl_verts), true);
    mesh_vao(&sph_vao, &sph_vbo, sph_verts, sizeof(sph_verts), true);
    mesh_vao(&line_vao, &line_vbo, line_vertices, sizeof(line_vertices), false);
    mesh_vao(&line_cyl_vao, &line_cyl_vbo, line_cyl_verts, sizeof(line_cyl_verts), false);
    mesh_vao(&line_sph_vao, &line_sph_vbo, line_sph_verts, sizeof(line_sph_verts), false);

    ui_program = link_program(ui_vertex_src, ui_fragment_src);
    if (!ui_program)
        return false;
    uiloc_res = glGetUniformLocation(ui_program, "u_res");
    uiloc_tex = glGetUniformLocation(ui_program, "u_tex");

    glGenVertexArrays(1, &ui_vao);
    glGenBuffers(1, &ui_vbo);
    glBindVertexArray(ui_vao);
    glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UiVertex), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(UiVertex), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(UiVertex), (void *)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    font_tex = build_font_texture();
    if (!font_tex) {
        SDL_Log("Failed to build font texture");
        return false;
    }

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

/* UI space in window coords (points), so menu hit-testing and drawing agree. */
void render_set_ui_size(int width, int height)
{
    if (width > 0) view_w = (float)width;
    if (height > 0) view_h = (float)height;
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

static void draw_shape(const Shape *s)
{
    mat4 model;

    switch (s->type) {
    case SHAPE_BOX: {
        vec3 size = v3_sub(s->box.max, s->box.min);
        vec3 center = v3_scale(v3_add(s->box.min, s->box.max), 0.5f);
        model = mat4_mul(mat4_translate(center), mat4_scale(size));
        glBindVertexArray(vao);
        break;
    }
    case SHAPE_CYLINDER: {
        vec3 c = s->cyl.base;
        model = mat4_mul(mat4_translate(v3(c.x, c.y + s->cyl.height * 0.5f, c.z)),
                         mat4_scale(v3(2.0f * s->cyl.radius, s->cyl.height, 2.0f * s->cyl.radius)));
        glBindVertexArray(cyl_vao);
        break;
    }
    case SHAPE_SPHERE:
        model = mat4_mul(mat4_translate(s->sphere.center),
                         mat4_scale(v3(2.0f * s->sphere.radius, 2.0f * s->sphere.radius, 2.0f * s->sphere.radius)));
        glBindVertexArray(sph_vao);
        break;
    default:
        return;
    }

    glUniformMatrix4fv(loc_model, 1, GL_FALSE, model.m);
    glUniform3f(loc_color, s->color.x, s->color.y, s->color.z);
    glUniform1i(loc_checker, 0);
    switch (s->type) {
    case SHAPE_BOX:      glDrawArrays(GL_TRIANGLES, 0, 36); break;
    case SHAPE_CYLINDER: glDrawArrays(GL_TRIANGLES, 0, cyl_vert_count); break;
    case SHAPE_SPHERE:   glDrawArrays(GL_TRIANGLES, 0, sph_vert_count); break;
    default: break;
    }
}

/* --- editor overlay drawing ------------------------------------------- */

static void ui_push(float x0, float y0, float x1, float y1,
                    float u0, float v0, float u1, float v1,
                    float r, float g, float b, float a)
{
    if (ui_count + 6 > UI_MAX_VERTS)
        return;
    UiVertex *v = &ui_verts[ui_count];
    v[0] = (UiVertex){ x0, y0, u0, v0, r, g, b, a };
    v[1] = (UiVertex){ x0, y1, u0, v1, r, g, b, a };
    v[2] = (UiVertex){ x1, y1, u1, v1, r, g, b, a };
    v[3] = (UiVertex){ x0, y0, u0, v0, r, g, b, a };
    v[4] = (UiVertex){ x1, y1, u1, v1, r, g, b, a };
    v[5] = (UiVertex){ x1, y0, u1, v0, r, g, b, a };
    ui_count += 6;
}

static void ui_rect(float x, float y, float w, float h, float r, float g, float b, float a)
{
    float u = 0.5f / ATLAS_W, v = 0.5f / ATLAS_H; /* white texel */
    ui_push(x, y, x + w, y + h, u, v, u, v, r, g, b, a);
}

static void ui_text(float x, float y, const char *s, float scale, float r, float g, float b, float a)
{
    for (; *s; s++) {
        int gi = (unsigned char)*s - 32;
        if (*s != ' ' && gi >= 0 && gi < 95) {
            float u0 = (1 + gi * FONT_ADVANCE) / (float)ATLAS_W;
            float u1 = (1 + gi * FONT_ADVANCE + FONT_GLYPH_W) / (float)ATLAS_W;
            ui_push(x, y, x + FONT_GLYPH_W * scale, y + FONT_GLYPH_H * scale,
                    u0, 0.0f, u1, 1.0f, r, g, b, a);
        }
        x += FONT_ADVANCE * scale;
    }
}

static float ui_text_width(const char *s, float scale)
{
    return *s ? (float)strlen(s) * FONT_ADVANCE * scale - scale : 0.0f;
}

static void ui_flush(void)
{
    if (!ui_count)
        return;

    glUseProgram(ui_program);
    glUniform2f(uiloc_res, view_w, view_h);
    glUniform1i(uiloc_tex, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font_tex);
    glBindVertexArray(ui_vao);
    glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)ui_count * sizeof(UiVertex), ui_verts, GL_STREAM_DRAW);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, ui_count);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    ui_count = 0;
}

static void draw_outline(const Shape *s, float r, float g, float b)
{
    mat4 model;
    GLuint lva;
    int count;

    switch (s->type) {
    case SHAPE_BOX: {
        vec3 size = v3_sub(s->box.max, s->box.min);
        vec3 center = v3_scale(v3_add(s->box.min, s->box.max), 0.5f);
        model = mat4_mul(mat4_translate(center), mat4_scale(v3_scale(size, 1.02f)));
        lva = line_vao;
        count = 24;
        break;
    }
    case SHAPE_CYLINDER: {
        vec3 c = s->cyl.base;
        model = mat4_mul(
            mat4_translate(v3(c.x, c.y + s->cyl.height * 0.5f, c.z)),
            mat4_scale(v3(2.0f * s->cyl.radius * 1.02f, s->cyl.height * 1.02f, 2.0f * s->cyl.radius * 1.02f)));
        lva = line_cyl_vao;
        count = line_cyl_vert_count;
        break;
    }
    case SHAPE_SPHERE:
        model = mat4_mul(mat4_translate(s->sphere.center),
                         mat4_scale(v3(2.0f * s->sphere.radius * 1.02f, 2.0f * s->sphere.radius * 1.02f,
                                       2.0f * s->sphere.radius * 1.02f)));
        lva = line_sph_vao;
        count = line_sph_vert_count;
        break;
    default:
        return;
    }

    glUseProgram(line_program);
    glUniformMatrix4fv(lloc_model, 1, GL_FALSE, model.m);
    glUniformMatrix4fv(lloc_view_proj, 1, GL_FALSE, cur_view_proj.m);
    glUniform4f(lloc_color, r, g, b, 1.0f);
    glBindVertexArray(lva);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, count);
}

static void ui_text_centered(const EditRect *r, const char *s, float scale, float r_, float g_, float b_, float a_)
{
    ui_text(r->x + (r->w - ui_text_width(s, scale)) * 0.5f,
            r->y + (r->h - FONT_GLYPH_H * scale) * 0.5f, s, scale, r_, g_, b_, a_);
}

static void ui_text_centered_at(float x, float y, float w, float h,
                                const char *s, float scale, float r_, float g_, float b_, float a_)
{
    ui_text_centered(&(EditRect){ x, y, w, h }, s, scale, r_, g_, b_, a_);
}

static void draw_menu(const Editor *ed)
{
    EditMenuLayout L;
    edit_menu_layout(ed, &L);

    ui_count = 0;
    ui_rect(L.panel.x, L.panel.y, L.panel.w, L.panel.h, 0.06f, 0.07f, 0.09f, 0.90f);
    ui_text_centered(&L.title, "ADD SHAPE", 2.0f, 0.95f, 0.95f, 0.95f, 1.0f);

    /* shape buttons */
    for (int i = 0; i < EDIT_SHAPE_COUNT; i++) {
        bool hover = ed->mx >= L.shape_btns[i].x && ed->mx < L.shape_btns[i].x + L.shape_btns[i].w &&
                     ed->my >= L.shape_btns[i].y && ed->my < L.shape_btns[i].y + L.shape_btns[i].h;
        float r, g, b;
        if (i == ed->shape) {
            r = 0.24f; g = 0.42f; b = 0.80f;
        } else if (hover) {
            r = 0.16f; g = 0.18f; b = 0.23f;
        } else {
            r = 0.11f; g = 0.12f; b = 0.16f;
        }
        ui_rect(L.shape_btns[i].x, L.shape_btns[i].y, L.shape_btns[i].w, L.shape_btns[i].h, r, g, b, 1.0f);
        ui_text_centered(&L.shape_btns[i], edit_shape_name(i), 2.0f, 0.95f, 0.95f, 0.95f, 1.0f);
    }

    /* dimension fields */
    for (int f = 0; f < edit_field_count(ed->shape); f++) {
        bool focused = f == ed->focused;
        bool hover = ed->mx >= L.fields[f].x && ed->mx < L.fields[f].x + L.fields[f].w &&
                     ed->my >= L.fields[f].y && ed->my < L.fields[f].y + L.fields[f].h;

        ui_text(L.field_labels[f].x, L.field_labels[f].y + 2.0f, edit_field_label(ed->shape, f),
                1.5f, 0.65f, 0.70f, 0.75f, 1.0f);

        /* border then interior */
        float br, bg, bb;
        if (focused) {
            br = 1.0f; bg = 0.85f; bb = 0.20f;
        } else if (hover) {
            br = 0.45f; bg = 0.50f; bb = 0.58f;
        } else {
            br = 0.30f; bg = 0.33f; bb = 0.38f;
        }
        ui_rect(L.fields[f].x - 2.0f, L.fields[f].y - 2.0f, L.fields[f].w + 4.0f, L.fields[f].h + 4.0f,
                br, bg, bb, 1.0f);
        ui_rect(L.fields[f].x, L.fields[f].y, L.fields[f].w, L.fields[f].h, 0.10f, 0.11f, 0.14f, 1.0f);

        const char *text = edit_field_text(ed, f);
        const char *shown = *text ? text : edit_field_placeholder(ed, f);
        float a = *text ? 1.0f : 0.45f;
        float tx = L.fields[f].x + 8.0f;
        float ty = L.fields[f].y + (L.fields[f].h - FONT_GLYPH_H * 2.0f) * 0.5f;
        ui_text(tx, ty, shown, 2.0f, 0.95f, 0.95f, 0.95f, a);
        if (focused) /* caret */
            ui_rect(tx + ui_text_width(shown, 2.0f) + 3.0f, ty, 2.0f, FONT_GLYPH_H * 2.0f,
                    0.95f, 0.95f, 0.95f, 1.0f);
    }

    /* OK button */
    bool hover = ed->mx >= L.ok.x && ed->mx < L.ok.x + L.ok.w && ed->my >= L.ok.y && ed->my < L.ok.y + L.ok.h;
    ui_rect(L.ok.x, L.ok.y, L.ok.w, L.ok.h, hover ? 0.30f : 0.20f, hover ? 0.55f : 0.45f, 0.25f, 1.0f);
    ui_text_centered(&L.ok, "OK", 2.0f, 0.95f, 0.95f, 0.95f, 1.0f);

    ui_text_centered(&L.hint, "TYPE SIZE   TAB NEXT FIELD   ENTER OK   ESC CANCEL",
                     1.5f, 0.60f, 0.65f, 0.70f, 1.0f);
    ui_flush();
}

static void draw_hud(const Editor *ed)
{
    const char *msg = NULL;
    if (ed->mode == EDIT_PLACE)
        msg = "BUILD MODE   CLICK PLACE   E EDIT   ESC EXIT";
    else if (ed->mode == EDIT_REMOVE)
        msg = "REMOVE MODE   CLICK REMOVE   R/ESC EXIT";
    if (!msg)
        return;

    const float scale = 2.0f;
    float w = ui_text_width(msg, scale);
    float x = (view_w - w) * 0.5f;

    ui_count = 0;
    ui_rect(x - 10.0f, 8.0f, w + 20.0f, (FONT_GLYPH_H + 4) * scale, 0.06f, 0.07f, 0.09f, 0.70f);
    ui_text(x, 12.0f, msg, scale, 0.95f, 0.95f, 0.95f, 1.0f);
    ui_flush();
}

static void draw_pause_menu(const MenuState *m)
{
    MenuLayout L;
    menu_layout(m, &L);

    ui_count = 0;
    /* dim the world slightly so the panel reads; the level stays visible */
    ui_rect(0.0f, 0.0f, view_w, view_h, 0.0f, 0.0f, 0.0f, 0.35f);
    ui_rect(L.panel.x, L.panel.y, L.panel.w, L.panel.h, 0.06f, 0.07f, 0.09f, 0.92f);

    if (m->mode == MENU_MAIN) {
        static const char *labels[3] = { "RETURN TO GAME", "SETTINGS", "EXIT" };
        ui_text_centered_at(L.title.x, L.title.y, L.title.w, L.title.h,
                            "PAUSED", 2.0f, 0.95f, 0.95f, 0.95f, 1.0f);
        for (int i = 0; i < 3; i++) {
            const MenuRect *r = &L.buttons[i];
            bool hover = m->mx >= r->x && m->mx < r->x + r->w &&
                         m->my >= r->y && m->my < r->y + r->h;
            ui_rect(r->x, r->y, r->w, r->h, hover ? 0.16f : 0.11f, hover ? 0.18f : 0.12f,
                    hover ? 0.23f : 0.16f, 1.0f);
            ui_text_centered_at(r->x, r->y, r->w, r->h, labels[i], 2.0f, 0.95f, 0.95f, 0.95f, 1.0f);
        }
    } else {
        ui_text_centered_at(L.title.x, L.title.y, L.title.w, L.title.h,
                            "SETTINGS", 2.0f, 0.95f, 0.95f, 0.95f, 1.0f);

        for (int i = 0; i < MENU_SETTING_COUNT; i++) {
            ui_text(L.row_label[i].x, L.row_label[i].y + 2.0f, menu_setting_label(i),
                    1.5f, 0.65f, 0.70f, 0.75f, 1.0f);

            const MenuRect *mi = &L.row_minus[i], *va = &L.row_value[i], *pl = &L.row_plus[i];
            bool hover_mi = m->mx >= mi->x && m->mx < mi->x + mi->w && m->my >= mi->y && m->my < mi->y + mi->h;
            bool hover_pl = m->mx >= pl->x && m->mx < pl->x + pl->w && m->my >= pl->y && m->my < pl->y + pl->h;
            ui_rect(mi->x, mi->y, mi->w, mi->h, hover_mi ? 0.16f : 0.11f, hover_mi ? 0.18f : 0.12f,
                    hover_mi ? 0.23f : 0.16f, 1.0f);
            ui_rect(pl->x, pl->y, pl->w, pl->h, hover_pl ? 0.16f : 0.11f, hover_pl ? 0.18f : 0.12f,
                    hover_pl ? 0.23f : 0.16f, 1.0f);
            ui_text_centered_at(mi->x, mi->y, mi->w, mi->h, "<", 2.0f, 0.95f, 0.95f, 0.95f, 1.0f);
            ui_text_centered_at(pl->x, pl->y, pl->w, pl->h, ">", 2.0f, 0.95f, 0.95f, 0.95f, 1.0f);

            /* value box, styled like the build menu's fields */
            ui_rect(va->x - 2.0f, va->y - 2.0f, va->w + 4.0f, va->h + 4.0f, 0.30f, 0.33f, 0.38f, 1.0f);
            ui_rect(va->x, va->y, va->w, va->h, 0.10f, 0.11f, 0.14f, 1.0f);
            ui_text_centered_at(va->x, va->y, va->w, va->h, menu_setting_text(m, i), 2.0f,
                                0.95f, 0.95f, 0.95f, 1.0f);
        }

        const MenuRect *bk = &L.back;
        bool hover_bk = m->mx >= bk->x && m->mx < bk->x + bk->w && m->my >= bk->y && m->my < bk->y + bk->h;
        ui_rect(bk->x, bk->y, bk->w, bk->h, hover_bk ? 0.16f : 0.11f, hover_bk ? 0.18f : 0.12f,
                hover_bk ? 0.23f : 0.16f, 1.0f);
        ui_text_centered_at(bk->x, bk->y, bk->w, bk->h, "BACK", 2.0f, 0.95f, 0.95f, 0.95f, 1.0f);
    }
    ui_flush();
}

void render_frame(const Player *player, const Shape *shapes, int shape_count,
                  const Editor *ed, const MenuState *menu)
{
    vec3 eye = player_eye(player);
    mat4 view = mat4_look_at(eye, v3_add(eye, player_forward(player)), v3(0.0f, 1.0f, 0.0f));
    mat4 proj = mat4_perspective(mx_radians(75.0f), aspect, 0.05f, 200.0f);
    mat4 view_proj = mat4_mul(proj, view);
    cur_view_proj = view_proj;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
    glBindVertexArray(vao);
    glUniformMatrix4fv(loc_view_proj, 1, GL_FALSE, view_proj.m);
    glUniform3f(loc_eye, eye.x, eye.y, eye.z);
    glUniform3f(loc_fog_color, FOG_R, FOG_G, FOG_B);

    /* Floor: a large thin slab whose top face sits at y = 0. */
    draw_box(v3(-FLOOR_SIZE, -1.0f, -FLOOR_SIZE), v3(FLOOR_SIZE, 0.0f, FLOOR_SIZE),
             v3(0.45f, 0.50f, 0.45f), 1);

    for (int i = 0; i < shape_count; i++)
        draw_shape(&shapes[i]);

    if (ed) {
        if (ed->mode == EDIT_PLACE && ed->ghost_valid)
            draw_outline(&ed->ghost, 1.0f, 0.85f, 0.20f);
        if (ed->mode == EDIT_REMOVE && ed->remove_idx >= 0 && ed->remove_idx < shape_count)
            draw_outline(&shapes[ed->remove_idx], 1.0f, 0.25f, 0.20f);
        if (ed->mode == EDIT_MENU)
            draw_menu(ed);
        else
            draw_hud(ed);
    }

    if (menu && menu->mode != MENU_CLOSED)
        draw_pause_menu(menu);
}

void render_shutdown(void)
{
    glDeleteTextures(1, &font_tex);
    glDeleteBuffers(1, &ui_vbo);
    glDeleteVertexArrays(1, &ui_vao);
    glDeleteProgram(ui_program);
    glDeleteBuffers(1, &line_sph_vbo);
    glDeleteVertexArrays(1, &line_sph_vao);
    glDeleteBuffers(1, &line_cyl_vbo);
    glDeleteVertexArrays(1, &line_cyl_vao);
    glDeleteBuffers(1, &line_vbo);
    glDeleteVertexArrays(1, &line_vao);
    glDeleteProgram(line_program);
    glDeleteBuffers(1, &sph_vbo);
    glDeleteVertexArrays(1, &sph_vao);
    glDeleteBuffers(1, &cyl_vbo);
    glDeleteVertexArrays(1, &cyl_vao);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
}

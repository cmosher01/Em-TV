#include <stdio.h>
#include <math.h>
#include <time.h>
#include <GL/gl3w.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "rngs.h"

extern void glRasterPos2f(float, float);
extern void glColor3f(float, float, float);



static GLuint program;
static GLint attribute_coord2d;
static GLint u_offset_x;
static GLint u_scale_x;
static GLuint vbo;

static int t;
static int prevf;
static int f;

static float offset_x = 0.0;
static float scale_x = 0.8;



/*
                 u        p
t 0  1  2  3  4  5  6  7 [8]
                    2  1  0 /3
  0 .2 .4 .6 .8  1 .7 .3 [0]
*/

struct deflector {
    int period;
    int uptime;
    int curr_t;
    int flyback;
};

double deflector_step(struct deflector *d) {
    const double u = (double)d->uptime;
    const double p = (double)d->period;
    const int t = d->curr_t;

    d->flyback = (u < t);
    double y;
    if (!d->flyback) {
        y = t/u;
    } else {
        y = (p-t)/(p-u);
    }

    ++d->curr_t;
    d->curr_t %= d->period;

    return y;
}



struct deflector H;
struct deflector V;



struct vtx {
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

static double video_static() {
    const double scale = 0.85-0.1;
    return 0.1 + Random()*scale;
}

#define C_H 910
#define C_FIELD (C_H*80)

static void idle() {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    struct vtx graph[C_FIELD];
    for (int i = 0; i < C_FIELD; ++i) {
        graph[i].x = deflector_step(&H)*2-1;
        graph[i].y = -(deflector_step(&V)*2-1);
        graph[i].z = 0.0;
        graph[i].w = V.flyback ? 3.0 : H.flyback ? 2.0 : video_static();
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(graph), graph, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glutPostRedisplay();
}

static void init() {
    H.period = C_H;
    H.uptime = 754;
    V.period = C_FIELD;
    V.uptime = C_FIELD-3*H.period;

    H.curr_t = 0;
    V.curr_t = 0;

    glEnable(GL_MULTISAMPLE);

    GLint compile_ok = GL_FALSE, link_ok = GL_FALSE;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const char *vs_source =

        "#version 120 \n"

        "uniform float offset_x; \n"
        "uniform float scale_x; \n"
        "attribute vec4 vtx; \n"
        "varying vec4 f_color; \n"

        "mat4 XYZ2RGB = mat4("
            "+1.8464881, -0.5521299, -0.2766458, +0.0000000,"
            "-0.9826630, +2.0044755, -0.0690396, +0.0000000,"
            "+0.0736477, -0.1453020, +1.3018376, +0.0000000,"
            "+0.0000000, +0.0000000, +0.0000000, +1.0000000); \n"

        "void main(void) { \n"
        "    gl_Position = vec4((vtx.x + offset_x) * scale_x, (vtx.y + offset_x) * scale_x, vtx.z, 1.0); \n"

        "    if (vtx.w >= 2.9) { \n"
        "        f_color = vec4(0.4,0.0,0.4,0.7); \n"
        // "        f_color = vec4(0.0,0.0,0.0,0.0); \n"
        "    } else if (vtx.w >= 1.9) { \n"
        "        f_color = vec4(0.6,0.6,0.2,0.4); \n"
        // "        f_color = vec4(0.0,0.0,0.0,0.0); \n"
        "    } else { \n"
        "        f_color = XYZ2RGB * vec4(vtx.w*0.265/0.285, vtx.w, vtx.w*(1.0-0.265-0.285)/0.285, 0.8); \n"
        "    } \n"

        "    gl_PointSize = 3.0; \n"
        "} \n";

    glShaderSource(vs, 1, &vs_source, NULL);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &compile_ok);
    if (!compile_ok) {
        GLint maxLength = 0;
        glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &maxLength);
        char *errorLog = (char*)malloc(maxLength);
        glGetShaderInfoLog(vs, maxLength, &maxLength, errorLog);
        fprintf(stderr, "%s\n", errorLog);
        glDeleteShader(vs); // Don't leak the shader.
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fs_source =

        "#version 120 \n"  // OpenGL 2.1

        "varying vec4 f_color; \n"

        "void main(void) { \n"
        "    gl_FragColor = f_color; \n"
        "} \n";

    glShaderSource(fs, 1, &fs_source, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &compile_ok);
    if (!compile_ok) {
        GLint maxLength = 0;
        glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &maxLength);
        char *errorLog = (char*)malloc(maxLength);
        glGetShaderInfoLog(fs, maxLength, &maxLength, errorLog);
        fprintf(stderr, "%s\n", errorLog);
        glDeleteShader(fs); // Don't leak the shader.
    }

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if (!link_ok) {
        fprintf(stderr, "link error\n");
    }


    attribute_coord2d = glGetAttribLocation(program, "vtx");
    if (attribute_coord2d == -1) {
        fprintf(stderr, "Could not bind attribute vtx\n");
    }

    u_offset_x = glGetUniformLocation(program, "offset_x");
    u_scale_x = glGetUniformLocation(program, "scale_x");

    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

    glGenBuffers(1, &vbo);
    idle();
}


static void display() {
    glEnable(GL_MULTISAMPLE);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);

    glUniform1f(u_offset_x, offset_x);
    glUniform1f(u_scale_x, scale_x);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(attribute_coord2d);

    glVertexAttribPointer(
        attribute_coord2d, // attribute
        sizeof(struct vtx)/sizeof(GLfloat),                 // number of elements per vertex
        GL_FLOAT,          // the type of each element
        GL_FALSE,          // take our values as-is
        0,                 // no extra data between each position
        0); // pointer to the C array (NULL = use vbo)


    glDrawArrays(GL_POINTS, 0, C_FIELD);

    glDisableVertexAttribArray(attribute_coord2d);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);


    int now = glutGet(GLUT_ELAPSED_TIME);
    ++f;
    if (now-t >= 1000) {
        prevf = f;
        f = 0;
        t = now;
    }

    glRasterPos2f(-0.95, -0.95);
    glColor3f(0.7f, 0.7f, 0.7f);
    unsigned char s[500];
    sprintf((char*)s, "%6.2f ms/fr", 1000.0/prevf);
    glutBitmapString(GLUT_BITMAP_8_BY_13, s);


    glutSwapBuffers();

}

static void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 17: // control-Q
        case 27: // escape
            glutLeaveMainLoop();
        break;
        default:
        break;
    }
}

int main(int argc, char **argv) {
    PlantSeeds(-1);

    glutInit(&argc, argv);
    t = glutGet(GLUT_ELAPSED_TIME);
    f = 0;
    glutInitContextFlags(GLUT_DEBUG);

    glutInitContextVersion(2, 1);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL | GLUT_MULTISAMPLE);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
    glutInitWindowSize(640, 480);
    int id_win = glutCreateWindow(argv[0]);
    if (!id_win) {
        fprintf(stderr, "Error creating window\n");
        return 1;
    }

    // glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);
    // glutSpecialFunc(special);
    // glutMouseFunc(mouse);
    // glutMotionFunc(motion);

    gl3wInit();
    printf("%s, OpenGL %s, GLSL %s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

    init();
    glutMainLoop();

    printf("Exit program\n");
    return 0;
}

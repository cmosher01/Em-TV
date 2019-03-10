#include <stdio.h>
#include <math.h>
#include <GL/gl3w.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "rngs.h"

extern void glRasterPos2f(float, float);
extern void glColor3f(float, float, float);



static GLuint program;
static GLint attribute_coord2d;
static GLint u_hilite_flyback;
static GLint u_brightness;
static GLint u_contrast;
static GLuint vbo;

static int t;
static int prevf;
static int f;

static int hilite_flyback = 0;
static float brightness = 0.0;
static float contrast = 1.0;


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
    const double u = d->uptime;
    const int t = d->curr_t;

    d->flyback = (u < t);

    const double p = d->flyback ? d->period : 0.0;
    const double y = (t-p)/(u-p);

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
    return 0.05+Random()*0.90;
}

static void idle() {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    struct vtx graph[(int)(V.period+1)];
    for (int i = 0; i < V.period; ++i) {
        graph[i].x = deflector_step(&H)*2-1;
        graph[i].y = -(deflector_step(&V)*2-1);
        graph[i].z = -0.87;
        graph[i].w = video_static();
        if (V.flyback || H.flyback) {
            graph[i].w *= -1.0;
        }
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(graph), graph, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glutPostRedisplay();
}

static int C_H;
static int C_H_VIS;
static float C_V;
static int C_V_BNK;
static int V_OFF;

static void init_deflectors() {
    const float C_FIELD = C_H*C_V;

    H.period = C_H;
    H.uptime = C_H_VIS;
    V.period = C_FIELD;
    V.uptime = C_FIELD-C_V_BNK*H.period;

    H.curr_t = 0;
    V.curr_t = V_OFF;
}

static void init() {
    C_H = 910;
    C_H_VIS = 754;
    C_V = 37.5;
    C_V_BNK = 3;
    V_OFF = (C_H-C_H_VIS)/2;
    init_deflectors();

    glEnable(GL_MULTISAMPLE);

    GLint compile_ok = GL_FALSE, link_ok = GL_FALSE;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const char *vs_source =

        "#version 120 \n"

        "uniform int hilite_flyback; \n"
        "uniform float brightness; \n"
        "uniform float contrast; \n"
        "attribute vec4 vtx; \n"
        "varying vec4 f_color; \n"

        "mat4 XYZ2RGB = mat4("
            "+1.8464881, -0.5521299, -0.2766458, +0.0000000,"
            "-0.9826630, +2.0044755, -0.0690396, +0.0000000,"
            "+0.0736477, -0.1453020, +1.3018376, +0.0000000,"
            "+0.0000000, +0.0000000, +0.0000000, +1.0000000); \n"

        "vec4 barrel(vec3 v) { \n"
        "    vec4 p = vec4(v.x, v.y, v.z, 1.0); \n"
        "    float rxy = length(p.xy) * 0.5; \n"
        "    if (rxy > 0) { \n"
        "        float phi = atan(rxy, -p.z); \n"
        "        float r = phi / (3.1415927/2.0); \n"
        "        p.xy *= r/rxy; \n"
        "    } \n"
        "    return p; \n"
        "} \n"

        "void main(void) { \n"
        "    vec3 p1 = vtx.xyz; \n"
        "    vec4 p = barrel(p1); \n"
        "    p.xy *= 1.317; \n"
        "    gl_Position = p; \n"

        "    if (vtx.w < -0.001 && hilite_flyback != 0) { \n"
        "        f_color = vec4(0.6,0.6,0.2,(0.5 + brightness) * contrast); \n"
        "    } else { \n"
        "        float Y = (abs(vtx.w) + brightness) * contrast; \n"
        "        f_color = XYZ2RGB * vec4(Y*0.265/0.285, Y, Y*(1.0-0.265-0.285)/0.285, .8); \n"
        "    } \n"

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
        glDeleteShader(vs);
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fs_source =

        "#version 120 \n"

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
        glDeleteShader(fs);
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

    u_hilite_flyback = glGetUniformLocation(program, "hilite_flyback");
    u_brightness = glGetUniformLocation(program, "brightness");
    u_contrast = glGetUniformLocation(program, "contrast");

    glDisable(GL_DEPTH_TEST);

    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.0);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glGenBuffers(1, &vbo);
    idle();
}


static void display() {
    glClearColor(0.01012, 0.02052, 0.03986, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);



    glUseProgram(program);

    glUniform1i(u_hilite_flyback, hilite_flyback);
    glUniform1f(u_brightness, brightness);
    glUniform1f(u_contrast, contrast);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(attribute_coord2d);
    glVertexAttribPointer(attribute_coord2d, sizeof(struct vtx)/sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_LINE_STRIP, 0, V.period);
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
    sprintf((char*)s,
        "%6.2f ms/fr  br %+5.2f  cn %4.2f  H %d  V %5.1f (%+d)",
        1000.0/prevf, brightness, contrast, C_H, C_V, V_OFF);
    glutBitmapString(GLUT_BITMAP_8_BY_13, s);


    glutSwapBuffers();

}

static void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 17: // control-Q
        case 27: // escape
            glutLeaveMainLoop();
        break;
        case 'f':
            hilite_flyback = !hilite_flyback;
        break;
        case 'b':
            if (brightness <= 0.95)
                brightness += 0.05;
        break;
        case 'd':
            if (brightness >= -0.95)
                brightness -= 0.05;
        break;
        case 'c':
            if (contrast <= 3.95)
                contrast += 0.05;
        break;
        case 'a':
            if (contrast >= 0.05)
                contrast -= 0.05;
        break;
        case '[':
            if (C_V > C_V_BNK) {
                C_V -= 0.5;
                init_deflectors();
            }
        break;
        case ']':
            if (C_V < 700) {
                C_V += 0.5;
                init_deflectors();
            }
        break;
        case ',':
            V_OFF = fmax(V_OFF-10,0);
            init_deflectors();
        break;
        case '.':
            V_OFF = fmin(V_OFF+10,C_H);
            init_deflectors();
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

    glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE | GLUT_MULTISAMPLE);
    glutSetOption(GLUT_MULTISAMPLE, 16);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
    glutInitWindowSize(640, 480);
    int id_win = glutCreateWindow(argv[0]);
    if (!id_win) {
        fprintf(stderr, "Error creating window\n");
        return 1;
    }

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);

    gl3wInit();
    printf("%s, OpenGL %s, GLSL %s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

    init();
    glutMainLoop();

    printf("Exit program\n");
    return 0;
}

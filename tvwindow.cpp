#include "tvwindow.h"
#include "tvdeflector.h"
#include <QOpenGLFunctions>
#include <QDebug>
#include <QString>
#include <QSettings>
#include <QSize>
#include <QPoint>
#include <QWidget>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QRandomGenerator>
#include <vector>
#include <algorithm>
#include <QBasicTimer>

TvWindow::TvWindow():
    QOpenGLWindow (QOpenGLWindow::UpdateBehavior::NoPartialUpdate),
    vbo(QOpenGLBuffer::Type::VertexBuffer) {
}

TvWindow::~TvWindow() {
}

static QSurfaceFormat getSurfaceFormat() {
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::RenderableType::OpenGL);
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
    format.setVersion(3,3);
    format.setSwapBehavior(QSurfaceFormat::SwapBehavior::SingleBuffer);
    format.setSwapInterval(0);
    return format;
}

void TvWindow::initialize() {
    setFormat(getSurfaceFormat());
    resize(getSavedWindowSize());
    setPosition(getSavedWindowPosition());
    connect(this, &TvWindow::frameSwapped, this, qOverload<>(&TvWindow::update));
}

static constexpr auto SET_WIN_SIZE = "tv/window/size";

QSize TvWindow::getSavedWindowSize() {
    return QSettings().value(SET_WIN_SIZE, QSize(800, 600)).toSize();
}

void TvWindow::saveWindowSize(const QSize& size) {
    QSettings().setValue(SET_WIN_SIZE, size);
}

static constexpr auto SET_WIN_POS = "tv/window/position";

QPoint TvWindow::getSavedWindowPosition() {
    return QSettings().value(SET_WIN_POS, QPoint(200, 200)).toPoint();
}

void TvWindow::saveWindowPosition(const QPoint& pos) {
    QSettings().setValue(SET_WIN_POS, pos);
}

void TvWindow::initializeGL() {
    QOpenGLFunctions gl(QOpenGLContext::currentContext());
    gl.initializeOpenGLFunctions();
    printContextInformation();

    createShaders();
    createGeometry();

    this->timer = std::make_unique<QBasicTimer>();
    this->timer->start(17, this); // ~60 Hz
}

void TvWindow::resizeGL(int width, int height) {
    QOpenGLFunctions gl(QOpenGLContext::currentContext());
    gl.glViewport(0, 0, width, height);
    update();
}

static float video_static() {
    return (0.05f+static_cast<float>(QRandomGenerator::global()->generateDouble())*0.90f);
}

static float y_jitter() {
    return (static_cast<float>(QRandomGenerator::global()->generateDouble())*2.0f-1.0f)*0.002f;
}

struct vtx {
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};

void TvWindow::fill() {
    this->vbo.bind();
    vtx* pvtx = static_cast<vtx*>(this->vbo.map(QOpenGLBuffer::Access::WriteOnly));
    if (pvtx != nullptr) {
        std::vector<vtx> graph;
        for (int i = 0; i < this->V->get_period(); ++i) {
            vtx v;
            v.x = this->H->step()*2-1;
            v.y = y_jitter()-(this->V->step()*2-1);
            v.z = -0.87f;
            v.w = video_static();
            if (this->V->is_flyback() || this->H->is_flyback()) {
                v.w *= -1.0f;
            }
            graph.push_back(v);
        }
        std::copy(graph.begin(), graph.end(), pvtx);
        this->vbo.unmap();
    }
    this->vbo.release();
}

void TvWindow::timerEvent(QTimerEvent *event) {
    static_cast<void>(event);
    makeCurrent();
    fill();
    doneCurrent();
}

void TvWindow::paintGL() {
    QOpenGLFunctions gl(QOpenGLContext::currentContext());

    gl.glClear(GL_COLOR_BUFFER_BIT);

    this->shader.bind();
    this->shader.setUniformValue("hilite_flyback", 0);
    this->shader.setUniformValue("brightness", this->brightness);
    this->shader.setUniformValue("contrast", this->contrast);
    this->vao.bind();
    gl.glDrawArrays(GL_LINE_STRIP, 0, this->V->get_period());
    this->vao.release();
    this->shader.release();
}

bool TvWindow::event(QEvent *event) {
    if (event->type() == QEvent::Close) {
        emit closing();
        return true;
    }
    return QOpenGLWindow::event(event);
}

void TvWindow::moveEvent(QMoveEvent *event) {
    saveWindowPosition(event->pos());
}

void TvWindow::resizeEvent(QResizeEvent *event) {
    saveWindowSize(event->size());
}

void TvWindow::printContextInformation() {
    QOpenGLFunctions gl(QOpenGLContext::currentContext());

    auto glType = (context()->isOpenGLES()) ? "OpenGL ES" : "OpenGL";
    auto glVersion = reinterpret_cast<const char*>(gl.glGetString(GL_VERSION));

    QString glProfile;
#define CASE(c) case QSurfaceFormat::c: glProfile = #c; break
    switch (format().profile()) {
      CASE(NoProfile);
      CASE(CoreProfile);
      CASE(CompatibilityProfile);
    }
#undef CASE

    qDebug() << qPrintable(glType) << qPrintable(glVersion) << "(" << qPrintable(glProfile) << ")";
}

void TvWindow::createShaders() {
    if (!this->shader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vertex.glsl")) {
        qDebug() << "Error in vertex shader:" << this->shader.log();
        exit(1);
    }
    if (!this->shader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fragment.glsl")) {
        qDebug() << "Error in fragment shader:" << this->shader.log();
        exit(1);
    }
    if (!this->shader.link()) {
        qDebug() << "Error linking shader program:" << this->shader.log();
        exit(1);
    }
}

void TvWindow::createGeometry() {
    QOpenGLFunctions gl(QOpenGLContext::currentContext());

    C_H = 910;
    C_H_BNK = 156;
    C_V = 262.5;
//    C_V = 32.5;
    C_V_BNK = 3;
    V_OFF = C_H_BNK/2;
    const float C_FIELD = C_V*C_H;
    const float C_FIELD_BNK = C_V_BNK*C_H;

    this->H = std::make_unique<TvDeflector>(C_H, C_H-C_H_BNK);
    this->V = std::make_unique<TvDeflector>(C_FIELD, C_FIELD-C_FIELD_BNK, V_OFF);



    this->shader.bind();

    this->vbo.create();
    this->vbo.bind();
    this->vbo.setUsagePattern(QOpenGLBuffer::UsagePattern::DynamicDraw);
    this->vbo.allocate(this->V->get_period() * static_cast<int>(sizeof(vtx)));

    this->vao.create();
    this->vao.bind();

    this->shader.enableAttributeArray("vtx");
    this->shader.setAttributeBuffer("vtx", GL_FLOAT, 0, 4, sizeof(vtx));

    gl.glEnable(GL_MULTISAMPLE);
    gl.glDisable(GL_DEPTH_TEST);

    gl.glEnable(GL_LINE_SMOOTH);
    gl.glLineWidth(1);
    gl.glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    gl.glClearColor(0.01012f, 0.02052f, 0.03986f, 1.0f);

    this->vao.release();
    this->vbo.release();
    this->shader.release();
}

void TvWindow::set_brightness(float brightness) {
    this->brightness = brightness;
    makeCurrent();
    update();
    doneCurrent();
}

void TvWindow::set_contrast(float contrast) {
    this->contrast = contrast;
    makeCurrent();
    update();
    doneCurrent();
}

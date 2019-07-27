#ifndef TVWINDOW_H
#define TVWINDOW_H

#include <QOpenGLWindow>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <memory>

class TvDeflector;
class QSize;
class QPoint;
class QMoveEvent;
class QResizeEvent;
class QCloseEvent;
class QBasicTimer;

class TvWindow : public QOpenGLWindow {
    Q_OBJECT

public:
    template<class T> static std::unique_ptr<T> create() {
        auto p = std::make_unique<T>();
        p->initialize();
        p->show();
        return p;
    }

    TvWindow();
    virtual ~TvWindow() override;
    explicit TvWindow(const TvWindow& that) = default;

    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void timerEvent(QTimerEvent *event) override;

    void set_brightness(float brightness);
    void set_contrast(float contrast);

signals:
    void closing();

protected:
    void initialize();
    bool event(QEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void printContextInformation();
    void createShaders();
    void createGeometry();
    void fill();

    static QSize getSavedWindowSize();
    static void saveWindowSize(const QSize& size);
    static QPoint getSavedWindowPosition();
    static void saveWindowPosition(const QPoint& pos);

    QOpenGLShaderProgram shader;
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;

    std::unique_ptr<QBasicTimer> timer;

    int C_H;
    int C_H_BNK;
    float C_V;
    int C_V_BNK;
    int V_OFF;

    std::unique_ptr<TvDeflector> H;
    std::unique_ptr<TvDeflector> V;

    float brightness;
    float contrast;
};

#endif // TVWINDOW_H

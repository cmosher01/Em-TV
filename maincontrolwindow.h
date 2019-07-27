#ifndef MAINCONTROLWINDOW_H
#define MAINCONTROLWINDOW_H

#include <QMainWindow>
#include <memory>

class TvWindow;

namespace Ui
{
    class MainControlWindow;
}

class MainControlWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainControlWindow(QWidget *parent = nullptr);
    ~MainControlWindow();

public slots:
    void tv_closing();

private slots:
    void on_actionQuit_triggered();
    void on_power_stateChanged(int state);

    void on_brightness_valueChanged(int value);

    void on_contrast_valueChanged(int value);

private:
    Ui::MainControlWindow *ui;
    std::unique_ptr<TvWindow> tv;
};

#endif // MAINCONTROLWINDOW_H

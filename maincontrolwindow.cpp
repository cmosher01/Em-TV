#include "maincontrolwindow.h"
#include "ui_maincontrolwindow.h"
#include "tvwindow.h"
#include <QtDebug>

MainControlWindow::MainControlWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainControlWindow),
    tv(nullptr)
{
    ui->setupUi(this);
}

MainControlWindow::~MainControlWindow()
{
    delete ui;
}

void MainControlWindow::on_actionQuit_triggered()
{
    QCoreApplication::instance()->quit();
}

void MainControlWindow::on_power_stateChanged(int state)
{
    const auto should_be_on = (state != Qt::Unchecked);
    const auto is_on = (this->tv != nullptr);
    if (is_on != should_be_on) {
        if (should_be_on) {
            this->tv = TvWindow::create<TvWindow>();
            connect(this->tv.get(), &TvWindow::closing, this, &MainControlWindow::tv_closing);
            on_brightness_valueChanged(this->ui->brightness->value());
            on_contrast_valueChanged(this->ui->contrast->value());
            on_lines_valueChanged(this->ui->lines->value());
            on_jitter_valueChanged(this->ui->jitter->value());
        } else {
            disconnect(this->tv.get(), &TvWindow::closing, this, &MainControlWindow::tv_closing);
            this->tv = nullptr;
        }
    }
}

void MainControlWindow::tv_closing() {
    this->ui->power->setCheckState(Qt::Unchecked);
}

void MainControlWindow::on_brightness_valueChanged(int value)
{
    if (this->tv) {
        this->tv->set_brightness(value/100.0f);
    }
}

void MainControlWindow::on_contrast_valueChanged(int value)
{
    if (this->tv) {
        this->tv->set_contrast(value/100.0f);
    }
}

void MainControlWindow::on_lines_valueChanged(int value)
{
    if (this->tv) {
        this->tv->set_lines((value/5*5)/10.0f);
    }
}

void MainControlWindow::on_jitter_valueChanged(int value)
{
    if (this->tv) {
        this->tv->set_jitter(value/1000.0f);
    }
}

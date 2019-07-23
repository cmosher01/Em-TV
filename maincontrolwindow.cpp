#include "maincontrolwindow.h"
#include "ui_maincontrolwindow.h"
#include "tvwindow.h"
#include <QtDebug>

MainControlWindow::MainControlWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainControlWindow),
    tv(nullptr)
{
    qDebug() << "MainControlWindow::MainControlWindow";
    ui->setupUi(this);
}

MainControlWindow::~MainControlWindow()
{
    qDebug() << "MainControlWindow::~MainControlWindow";
    delete ui;
}

void MainControlWindow::on_actionQuit_triggered()
{
    qDebug() << "MainControlWindow::quit";
    QCoreApplication::instance()->quit();
}

void MainControlWindow::on_cbTv_stateChanged(int state)
{
    qDebug() << "MainControlWindow::on_cbTv_stateChanged";
    const auto should_be_on = state != Qt::Unchecked;
    const auto is_on = (this->tv != nullptr);
    if (is_on != should_be_on) {
        if (should_be_on) {
            this->tv = TvWindow::create<TvWindow>();
            connect(this->tv.get(), &TvWindow::closing, this, &MainControlWindow::tv_closing);
        } else {
            disconnect(this->tv.get(), &TvWindow::closing, this, &MainControlWindow::tv_closing);
            this->tv = nullptr;
        }
    }
}

void MainControlWindow::tv_closing() {
    qDebug() << "MainControlWindow::tv_closing";
    this->ui->cbTv->setCheckState(Qt::Unchecked);
}

#include <QtGui/QtEvents>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>

#include "gui/animeWidget.h"
#include "drivers/coreManager.h"


namespace {
    constexpr int frame = 40;
    constexpr int fps = 1000/frame;
}

AnimeWidget::AnimeWidget(QWidget *parent)
    : QOpenGLWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);

    this->makeCurrent();
    this->startTimer(fps);
}

AnimeWidget::~AnimeWidget() {}

void AnimeWidget::initializeGL() {
    CoreManager::GetInstance()->Initialize(this);
    CoreManager::GetInstance()->resize(this->width(),this->height());
}

void AnimeWidget::paintGL() {
    CoreManager::GetInstance()->update();
}

void AnimeWidget::resizeGL(int width, int height) {
    CoreManager::GetInstance()->resize(width,height);
}

void AnimeWidget::mousePressEvent(QMouseEvent *event) {
    int x = event->x();
    int y = event->y();
    CoreManager::GetInstance()->mousePressEvent(x,y);
}

void AnimeWidget::mouseReleaseEvent(QMouseEvent *event) {
    int x = event->x();
    int y = event->y();
    CoreManager::GetInstance()->mouseReleaseEvent(x,y);
}

void AnimeWidget::mouseMoveEvent(QMouseEvent *event) {
    int x = event->x();
    int y = event->y();
    CoreManager::GetInstance()->mouseMoveEvent(x,y);
}

void AnimeWidget::timerEvent(QTimerEvent*) {
    this->update();
}

void AnimeWidget::closeEvent(QCloseEvent * e) {
    QApplication::sendEvent(this->parent(), e);
}

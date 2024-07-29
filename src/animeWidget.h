/**
 * @file animeWidget.h
 * @brief This file defines a widget rendering models based on the QOpenGLWidget.
 * 
 * @author SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once

#include <AppOpenGLWrapper.hpp>
#include <QtWidgets/QOpenGLWidget>

/**
 * @class AnimeWidget
 * @brief A QOpenGLWidget based model rendering widget.
 */
class AnimeWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    AnimeWidget(QWidget *parent = 0);
    ~AnimeWidget();
protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
    void closeEvent(QCloseEvent * e) override;
};

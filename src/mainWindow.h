/**
 * @file mainWindow.h
 * @brief Main window for the application.
 * 
 * @author SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once

#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSystemTrayIcon>

#include "animeWidget.h"

#include "ui_mainWindow.h"

class mainWindow : public QMainWindow, public Ui::mainWindow {
    //Q_OBJECT
public:

    mainWindow(QApplication* mapp = nullptr);
    ~mainWindow();

protected:
    void closeEvent(QCloseEvent * e) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void exitApp();
    void switchClicked(QAction* curAct);
    void geoEditMode(bool enter);
    void config();
    void chatBegin();

    void aboutAuthor();
private:
    void writeSettings();
    void loadSettings();
    void loadStyleSheet();
    
    void loadModels();
    void setupTray();

    QSystemTrayIcon* systemTray;
    QMenu *mainMenu;

    QMenu *switchMenu;
    QList<QAction*> modelList;
    QActionGroup *switchActionGroup;
    QAction *exitAction;
    QAction *geoEditAction;
    QAction *settingsAction;
    QAction *chatAction;
    QAction *aboutAction;
    
    QApplication* app;
};
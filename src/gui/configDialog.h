/**
 * @file configDialog.h
 * @brief Configurations dialog for the application.
 * 
 * @author SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once

#include <QtWidgets/QDialog>

#include "ui_configDialog.h"

class ConfigDialog : public QDialog, public Ui::configDialog {
    Q_OBJECT
public:
    ConfigDialog(QWidget* parent = nullptr);
    ~ConfigDialog();

public slots:
    void setCurrentModel(const QString& modelName);

private slots:
    void on_expressionAddBtn_clicked();
    void on_motionAddBtn_clicked();
    void on_expressionRmBtn_clicked();
    void on_motionRmBtn_clicked();

private:
    void reload();
    void saveConfig();

    bool parseExpressions(void* fref, void* exprNode);
    bool parseMotions(void* fref, void* motionNode);

    void addExpression(const char* fn);
    void addMotion(const char* fn, bool idle);

    void rmExpression(const char* fn);
    void rmMotion(const char* fn, bool idle);

    QString curResDir;
    QString curConfigFn;
    void* jsonRoot;
};

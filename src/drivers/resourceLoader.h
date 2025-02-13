/**
 * @file resourceLoader.h
 * @brief A source file defining the resouce loader of the application.
 * 
 * @author SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once

#include <QtCore/QString>

#include "utils/consts.h"

class resourceLoader {
public:
    static resourceLoader& get_instance() {
        static resourceLoader ins;
        return ins;
    }
    bool initialize();
    void release();

    const QVector<QString>& getModelList();
    QString                 getTrayIconPath();
    QString                 getCurrentModelName();
    int                     getCurrentModelIndex();

    bool                    setCurrentModel(int idx);
    

private:
    QVector<QString>    modelList;
    
    QString             trayIconPath;

    QString             currentModelName;
    int                 currentModelIndex;

    void*               jsonRoot;
    bool                isInit;

    bool saveConfig(char* new_config);

    resourceLoader() { isInit = false; }
    ~resourceLoader();
};

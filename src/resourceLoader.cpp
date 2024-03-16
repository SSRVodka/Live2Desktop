#include <stdio.h>
#include <string.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include "cJSON.h"
#include "logger.h"
#include "resourceLoader.h"


bool resourceLoader::initialize() {
    if(isInit == true) {
        stdLogger.Warning("Resource loader has already initialized.");
        return true;
    }

    QFile config(CONFIG_FILE_PATH);
    if (!config.open(QIODevice::ReadOnly | QIODevice::Text)) {
        stdLogger.Exception(
            QString("Failed to read: %1")
            .arg(CONFIG_FILE_PATH)
            .toStdString().c_str()
        );
        return false;
    }
    QTextStream stream(&config);
    QString buffer = stream.readAll();
    cJSON* root = cJSON_Parse(buffer.toStdString().c_str());
    config.close();

    if (root == NULL) {
        stdLogger.Exception(
            QString("Failed to parse JSON: %1")
            .arg(buffer)
            .toStdString().c_str()
        );
        return false;
    }
    cJSON* node = cJSON_GetObjectItem(root, "systemtray");

    if (node == NULL || !cJSON_IsString(node)) {
        stdLogger.Exception("Missing JSON key: systemtray");
        cJSON_Delete(root);
        return false;
    }
    trayIconPath = QString(RESOURCE_ROOT_DIR"%1").arg(
        cJSON_GetStringValue(node)
    );

    node = cJSON_GetObjectItem(root, "model");
    if (node == NULL || !cJSON_IsArray(node)) {
        cJSON_Delete(root);
        stdLogger.Exception("Missing JSON key: model");
        return false;
    }

    for (int i = 0; i < cJSON_GetArraySize(node); i++) {
        cJSON* model_ptr = cJSON_GetArrayItem(node,i);
        QString curModelName;
        if (model_ptr == NULL || !cJSON_IsString(model_ptr))
            continue;
        curModelName = cJSON_GetStringValue(model_ptr);
        modelList.push_back(curModelName);
    }

    if (modelList.size() == 0) {
        cJSON_Delete(root);
        stdLogger.Exception("No model found");
        return false;
    }

    currentModelName = modelList[0];
    currentModelIndex = 0;
    node = cJSON_GetObjectItem(root, "userdata");
    
    if (node != NULL && cJSON_IsObject(node)) {
        cJSON* tmpCMNode = cJSON_GetObjectItem(node, "current");
        if (tmpCMNode != NULL && cJSON_IsString(tmpCMNode)) {
            currentModelName = cJSON_GetStringValue(tmpCMNode);
            currentModelIndex = modelList.indexOf(currentModelName);
            if (currentModelIndex < 0) {
                stdLogger.Exception(
                    QString("No model named: %1")
                    .arg(currentModelName)
                    .toStdString().c_str()
                );
            }
        }
    }

    jsonRoot = (void*)root;
    isInit = true;
    return true;
}

void resourceLoader::release() {
    if (isInit == false)
        return;

    char* newConfig = cJSON_Print((cJSON*)jsonRoot);
    saveConfig(newConfig);
    cJSON_Delete((cJSON*)jsonRoot);
    stdLogger.Info("Resource loader released.");
    isInit = false;
}

const QVector<QString>& resourceLoader::getModelList() {
    return modelList;
}

QString resourceLoader::getTrayIconPath() {
    return trayIconPath;
}

QString resourceLoader::getCurrentModelName() {
    return currentModelName;
}

int resourceLoader::getCurrentModelIndex() {
    return currentModelIndex;
}

bool resourceLoader::setCurrentModel(int idx) {
    if (idx >= modelList.length() || idx < 0) {
        stdLogger.Warning(
            QString("Current model index out of range: %1")
            .arg(idx)
            .toStdString().c_str()
        );
        return false;
    }
    currentModelIndex = idx;
    currentModelName = modelList[currentModelIndex];

    cJSON* curNode = cJSON_GetObjectItem((cJSON*)jsonRoot, "userdata");
    char* cur = new char[currentModelName.length() + 1] {0};
    snprintf(cur, currentModelName.length() + 1, currentModelName.toUtf8().constData());
    cJSON_ReplaceItemInObject(curNode, "current", cJSON_CreateString(cur));
    delete[] cur;

    return true;
}

bool resourceLoader::saveConfig(char* newConfig) {
    if (newConfig == NULL) return false;

    QFile currentConfig(CONFIG_FILE_PATH);
    if (!currentConfig.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        stdLogger.Exception(
            QString("Failed to access: %1.")
            .arg(CONFIG_FILE_PATH)
            .toStdString().c_str()
        );
        return false;
    }
    currentConfig.write(newConfig);
    currentConfig.close();
    cJSON_free(newConfig);
    return true;
}

resourceLoader::~resourceLoader() {
    release();
}

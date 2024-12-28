#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include "configDialog.h"
#include "consts.h"
#include "cJSON.h"
#include "logger.h"

/* Caller should always release the allocated space. */
static char* QString2char(const QString& str) {
    // Note: get UTF8 length (Chinese -> 2 bytes; English -> 1 byte)
    int utf8len = str.toLocal8Bit().length();
    char* ans = new char[utf8len + 1] {0};
    snprintf(ans, utf8len + 1, "%s", str.toUtf8().constData());
    return ans;
}


ConfigDialog::ConfigDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi(this);

    jsonRoot = NULL;

    expressionList->setViewMode(QListView::ListMode);
    motionList->setViewMode(QListView::ListMode);

    reload();
}

ConfigDialog::~ConfigDialog() {
    saveConfig();
    if (jsonRoot) cJSON_Delete((cJSON*)jsonRoot);
}

void ConfigDialog::setCurrentModel(const QString& modelName) {
    currentModelName->setText(modelName);
    reload();
}

void ConfigDialog::reload() {
    expressionList->clear();
    motionList->clear();

    saveConfig();
    
    QString curName = currentModelName->text();
    if (curName.isEmpty()) return;

    curResDir = QString("%1%2/")
                .arg(RESOURCE_ROOT_DIR)
                .arg(curName);
    curConfigFn = QString("%1%2.model3.json")
                .arg(curResDir)
                .arg(curName);
    QFile configFile(curConfigFn);
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString msg = QString("Failed to load model configurations: %1")
                        .arg(curConfigFn);
        stdLogger.Exception(msg.toStdString().c_str());
        QMessageBox::critical(this, appName, msg);
        return;
    }
    QTextStream stream(&configFile);
    QString cont = stream.readAll();
    cJSON* root = cJSON_Parse(cont.toStdString().c_str());
    configFile.close();

    if (root == NULL) {
        stdLogger.Exception(
            QString("Failed to parse JSON: %1")
            .arg(cont)
            .toStdString().c_str()
        );
        return;
    }

    cJSON* fref = cJSON_GetObjectItem(root, "FileReferences");
    if (fref == NULL || !cJSON_IsObject(fref)) {
        stdLogger.Exception(
            QString("Missing JSON key: FileReferences")
            .toStdString().c_str()
        );
        cJSON_Delete(root);
        return;
    }

    cJSON* motionNode = cJSON_GetObjectItem(fref, "Motions");
    cJSON* expressionNode = cJSON_GetObjectItem(fref, "Expressions");

    if (jsonRoot) cJSON_Delete((cJSON*)jsonRoot);

    if (parseMotions(fref, motionNode) && parseExpressions(fref, expressionNode)) {
        jsonRoot = root;
    } else {
        cJSON_Delete(root);
        jsonRoot = nullptr;
    }
}

void ConfigDialog::saveConfig() {
    if (jsonRoot == NULL) return;
    QFile configFile(curConfigFn);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        stdLogger.Exception(
            QString("Failed to access: %1")
            .arg(curConfigFn)
            .toStdString().c_str()
        );
        return;
    }
    
    char* conf = cJSON_Print((cJSON*)jsonRoot);
    configFile.write(conf);
    configFile.close();
    cJSON_free(conf);
}

bool ConfigDialog::parseMotions(void* fref, void* motionNode) {
    if (motionNode == NULL) {
        cJSON* motions = cJSON_CreateObject();
        cJSON_AddItemToObject((cJSON*)fref, "Motions", motions);
        motionNode = motions;
    } else if (!cJSON_IsObject((cJSON*)motionNode)) {
        stdLogger.Warning("Invalid JSON key type (in Motions).");
        return false;
    }
    cJSON* idleNode = cJSON_GetObjectItem((cJSON*)motionNode, "Idle");
    if (idleNode == NULL) {
        idleNode = cJSON_AddArrayToObject((cJSON*)motionNode, "Idle");
    } else if (!cJSON_IsArray(idleNode)) {
        stdLogger.Warning("Invalid JSON key type (in Idle).");
        return false;
    }
    cJSON* tapNode = cJSON_GetObjectItem((cJSON*)motionNode, "TapBody");
    if (tapNode == NULL) {
        tapNode = cJSON_AddArrayToObject((cJSON*)motionNode, "TapBody");
    } else if (!cJSON_IsArray(tapNode)) {
        stdLogger.Warning("Invalid JSON key type (in TapBody).");
        return false;
    }
    int idleCnt = cJSON_GetArraySize(idleNode);
    int tapCnt = cJSON_GetArraySize(tapNode);
    for (int i = 0; i < idleCnt; ++i) {
        cJSON* idleObj = cJSON_GetArrayItem(idleNode, i);
        cJSON* fNode = cJSON_GetObjectItem(idleObj, "File");
        if (idleObj == NULL || fNode == NULL) {
            stdLogger.Warning("Invalid JSON struture (in Idle).");
            return false;
        }
        const char* curF = cJSON_GetStringValue(fNode);
        motionList->addItem(QString(curF) + IDLE_SUFFIX);
    }
    for (int i = 0; i < tapCnt; ++i) {
        cJSON* tapObj = cJSON_GetArrayItem(tapNode, i);
        cJSON* fNode = cJSON_GetObjectItem(tapObj, "File");
        if (tapObj == NULL || fNode == NULL) {
            stdLogger.Warning("Invalid JSON struture (in TapBody).");
            return false;
        }
        const char* curF = cJSON_GetStringValue(fNode);
        motionList->addItem(curF);
    }
    return true;
}

bool ConfigDialog::parseExpressions(void* fref, void* exprNode) {
    if (exprNode == NULL) {
        exprNode = cJSON_AddArrayToObject((cJSON*)fref, "Expressions");
    } else if (!cJSON_IsArray((cJSON*)exprNode)) {
        stdLogger.Warning("Invalid JSON key type (in Expressions)");
        return false;
    }
    int exprCnt = cJSON_GetArraySize((cJSON*)exprNode);
    for (int i = 0; i < exprCnt; ++i) {
        cJSON* exprObj = cJSON_GetArrayItem((cJSON*)exprNode, i);
        cJSON* nNode = cJSON_GetObjectItem(exprObj, "Name");
        if (exprObj == NULL || nNode == NULL) {
            stdLogger.Warning("Invalid JSON struture (in Expressions).");
            return false;
        }
        const char* curN = cJSON_GetStringValue(nNode);
        expressionList->addItem(curN);
    }
    return true;
}

void ConfigDialog::addExpression(const char* fn) {
    if (jsonRoot == NULL) {
        QMessageBox::critical(
            this, appName,
            "Failed to parse model configurations.\n" \
            "Search log / Reopen the dialog / Issue on Github for help."
        );
        return;
    }
    /* Valid for sure. */
    cJSON* fref = cJSON_GetObjectItem((cJSON*)jsonRoot, "FileReferences");
    cJSON* exprArr = cJSON_GetObjectItem(fref, "Expressions");
    cJSON* obj = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, "Name", cJSON_CreateString(fn));
    cJSON_AddItemToObject(obj, "File", cJSON_CreateString(fn));
    cJSON_AddItemToArray(exprArr, obj);

    reload();
}

void ConfigDialog::addMotion(const char* fn, bool idle) {
    if (jsonRoot == NULL) {
        QMessageBox::critical(
            this, appName,
            "Failed to parse model configurations.\n" \
            "Search log / Reopen the dialog / Issue on Github for help."
        );
        return;
    }
    /* Valid for sure. */
    cJSON* fref = cJSON_GetObjectItem((cJSON*)jsonRoot, "FileReferences");
    cJSON* mot = cJSON_GetObjectItem(fref, "Motions");
    cJSON* idleArr = cJSON_GetObjectItem(mot, "Idle");
    cJSON* tapArr = cJSON_GetObjectItem(mot, "TapBody");
    cJSON* obj = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, "File", cJSON_CreateString(fn));
    cJSON_AddItemToObject(obj, "FadeInTime", cJSON_CreateNumber(DEFAULT_FADE_IN_TIME));
    cJSON_AddItemToObject(obj, "FadeOutTime", cJSON_CreateNumber(DEFAULT_FADE_OUT_TIME));

    if (idle) cJSON_AddItemToArray(idleArr, obj);
    else cJSON_AddItemToArray(tapArr, obj);

    reload();
}

void ConfigDialog::rmExpression(const char* fn) {
    if (jsonRoot == NULL) {
        QMessageBox::critical(
            this, appName,
            "Failed to parse model configurations.\n" \
            "Search log / Reopen the dialog / Issue on Github for help."
        );
        return;
    }
    /* Valid for sure. */
    cJSON* fref = cJSON_GetObjectItem((cJSON*)jsonRoot, "FileReferences");
    cJSON* exprArr = cJSON_GetObjectItem(fref, "Expressions");
    cJSON* entry = NULL;
    int exprCnt = cJSON_GetArraySize(exprArr);
    for (int i = 0; i < exprCnt; ++i) {
        entry = cJSON_GetArrayItem(exprArr, i);
        cJSON *entryFile = cJSON_GetObjectItem(entry, "File");
        if (strcmp(fn, cJSON_GetStringValue(entryFile)) == 0) {
            cJSON_DeleteItemFromArray(exprArr, i);

            reload();
            return;
        }
    }
    stdLogger.Warning(
        QString("Invalid expression entry: %1")
        .arg(fn)
        .toStdString().c_str()
    );
}

void ConfigDialog::rmMotion(const char* fn, bool idle) {
    if (jsonRoot == NULL) {
        QMessageBox::critical(
            this, appName,
            "Failed to parse model configurations.\n" \
            "Search log / Reopen the dialog / Issue on Github for help."
        );
        return;
    }
    /* Valid for sure. */
    cJSON* fref = cJSON_GetObjectItem((cJSON*)jsonRoot, "FileReferences");
    cJSON* mot = cJSON_GetObjectItem(fref, "Motions");
    cJSON* idleArr = cJSON_GetObjectItem(mot, "Idle");
    cJSON* tapArr = cJSON_GetObjectItem(mot, "TapBody");
    cJSON* entry = NULL;
    int idleCnt = cJSON_GetArraySize(idleArr);
    int tapCnt = cJSON_GetArraySize(tapArr);
    if (idle) {
        for (int i = 0; i < idleCnt; ++i) {
            entry = cJSON_GetArrayItem(idleArr, i);
            cJSON *entryFile = cJSON_GetObjectItem(entry, "File");
            if (strcmp(fn, cJSON_GetStringValue(entryFile)) == 0) {
                cJSON_DeleteItemFromArray(idleArr, i);

                reload();
                return;
            }
        }
    } else {
        for (int i = 0; i < tapCnt; ++i) {
            entry = cJSON_GetArrayItem(tapArr, i);
            cJSON *entryFile = cJSON_GetObjectItem(entry, "File");
            if (strcmp(fn, cJSON_GetStringValue(entryFile)) == 0) {
                cJSON_DeleteItemFromArray(tapArr, i);

                reload();
                return;
            }
        }
    }
    stdLogger.Warning(
        QString("Invalid motion entry: %1")
        .arg(fn)
        .toStdString().c_str()
    );
}

void ConfigDialog::on_expressionAddBtn_clicked() {
    QStringList ans = QFileDialog::getOpenFileNames(
        this, appName, curResDir,
        "Model Expression Configurations (*.exp3.json)"
    );
    QStringList parts;
    QString curFn;
    char* tmp = NULL;
    if (ans.isEmpty()) return;
    foreach (QString full, ans) {
        parts = full.split(curResDir);
        curFn = parts[parts.length() - 1];
        tmp = QString2char(curFn);
        addExpression(tmp);
        delete[] tmp;
    }
}

void ConfigDialog::on_expressionRmBtn_clicked() {
    auto selected = expressionList->selectedItems();
    QString raw, dst;
    char* curFn;
    foreach (QListWidgetItem* item, selected) {
        raw = item->text();
        curFn = QString2char(raw);
        rmExpression(curFn);
        delete[] curFn;
    }
}

void ConfigDialog::on_motionAddBtn_clicked() {
    QStringList ans = QFileDialog::getOpenFileNames(
        this, appName, curResDir,
        "Model Motion Configurations (*.motion3.json)"
    );
    QStringList parts;
    QString curFn;
    char* tmp = NULL;
    if (ans.isEmpty()) return;
    foreach (QString full, ans) {
        parts = full.split(curResDir);
        curFn = parts[parts.length() - 1];
        tmp = QString2char(curFn);
        addMotion(tmp, idleCheckBox->isChecked());
        delete[] tmp;
    }
}

void ConfigDialog::on_motionRmBtn_clicked() {
    auto selected = motionList->selectedItems();
    QString raw, dst;
    char* curFn;
    foreach (QListWidgetItem* item, selected) {
        raw = item->text();
        if (raw.endsWith(IDLE_SUFFIX)) {
            dst = raw.mid(0, raw.length()-QString(IDLE_SUFFIX).length());
            curFn = QString2char(dst);
            rmMotion(curFn, true);
        } else {
            curFn = QString2char(raw);
            rmMotion(curFn, false);
        }
        delete[] curFn;
    }
}


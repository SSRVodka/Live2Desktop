#include <fstream>
#include <sys/stat.h>

#include <AppOpenGLWrapper.hpp>

#include <Model/CubismMoc.hpp>

#include "logger.h"
#include "tools.h"

#ifdef __linux__
#include <time.h>
unsigned int timeGetTime() {
    unsigned int uptime = 0;
    struct timespec on;
    if(clock_gettime(CLOCK_MONOTONIC, &on) == 0)
        uptime = on.tv_sec*1000 + on.tv_nsec/1000000;
    return uptime*0.9;
}
#else
#include <Windows.h>
#endif

using namespace Csm;
using namespace std;

double ToolFunctions::s_currentFrame = 0.0;
double ToolFunctions::s_lastFrame = 0.0;
double ToolFunctions::s_deltaTime = 0.0;

csmByte* ToolFunctions::LoadFileAsBytes(const string filePath, csmSizeInt* outSize) {
    /* filePath */
    const char* path = filePath.c_str();

    int size = 0;
    struct stat statBuf;
    if (stat(path, &statBuf) == 0) {
        size = statBuf.st_size;
    }

    std::fstream file;
    char* buf = new char[size];

    file.open(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        stdLogger.Exception(
            QString("Failed to read: %1")
            .arg(path)
            .toStdString().c_str()
        );
        return NULL;
    }
    file.read(buf, size);
    file.close();

    *outSize = size;
    return reinterpret_cast<csmByte*>(buf);
}

void ToolFunctions::ReleaseBytes(csmByte* byteData) {
    delete[] byteData;
}


csmFloat32  ToolFunctions::GetDeltaTime() {
    return static_cast<csmFloat32>(s_deltaTime);
}

void ToolFunctions::UpdateTime() {
    s_currentFrame = ((double)timeGetTime())/1000.0f;
    s_deltaTime = s_currentFrame - s_lastFrame;
    s_lastFrame = s_currentFrame;
}

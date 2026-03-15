#include <fstream>
#include <sys/stat.h>

#include <AppOpenGLWrapper.hpp>
#include <QtCore/QString>

#include <Model/CubismMoc.hpp>

#include "drivers/tools.h"
#include "utils/logger.h"

#ifdef _WIN32
#include <Windows.h>
#pragma comment(lib, "winmm.lib")
#else
#include <time.h>
unsigned int timeGetTime() {
    unsigned int uptime = 0;
    struct timespec on;
    if(clock_gettime(CLOCK_MONOTONIC, &on) == 0)
        uptime = on.tv_sec*1000 + on.tv_nsec/1000000;
    return uptime*0.9;
}
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

void ToolFunctions::PrintLogV(const Csm::csmChar* format, va_list args) {
    csmChar buf[256];
    
    va_list argsCopy;
    va_copy(argsCopy, args);
    vsnprintf(buf, sizeof(buf), format, argsCopy);
    va_end(argsCopy);

    // skip leading spaces
    const csmChar* ptr = buf;
    while (*ptr == ' ') ++ptr;
    
    // Check if the string starts with [CSM][?] (where ? is a single character, and the prefix is 8 characters long: [CSM][X])
    // Ensure the string is long enough to prevent out-of-bounds access
    if (strncmp(ptr, "[CSM][", 6) == 0 && ptr[6] != '\0' && ptr[7] == ']') {
        csmChar level = ptr[6];
        // skip prefix "[CSM][?]"
        const csmChar* msg = ptr + 8;
        
        switch (level) {
            case 'I': stdLogger._info(msg, "CubismCore", 0); break;
            case 'W': stdLogger._warning(msg, "CubismCore", 0); break;
            case 'D': stdLogger._debug(msg, "CubismCore", 0); break;
            case 'V': stdLogger._verbose(msg, "CubismCore", 0); break;
            case 'E': stdLogger._error(msg, "CubismCore", 0); break;
            default:  stdLogger.Exception(buf); break;
        }
    } else {
        stdLogger.Exception(buf);
    }
}

void ToolFunctions::PrintLog(const Csm::csmChar* format, ...) {
    va_list args;
    va_start(args, format);
    ToolFunctions::PrintLogV(format, args);
    va_end(args);
}

void ToolFunctions::PrintLogLn(const Csm::csmChar* format, ...) {
    csmChar formatWithNewline[260];  // 256 + "\n\0"
    snprintf(formatWithNewline, sizeof(formatWithNewline), "%s\n", format);
    
    va_list args;
    va_start(args, format);
    ToolFunctions::PrintLogV(formatWithNewline, args);
    va_end(args);
}

void ToolFunctions::PrintMessage(const Csm::csmChar* message) {
    ToolFunctions::PrintLog("%s", message);
}

void ToolFunctions::PrintMessageLn(const Csm::csmChar* message) {
    ToolFunctions::PrintLogLn("%s", message);
}

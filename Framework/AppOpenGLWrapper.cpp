#include "AppOpenGLWrapper.hpp"

#ifndef USE_GLAD_GLLOADER
QOpenGLFunctions* AppOpenGLWrapper::instance = nullptr;
#endif

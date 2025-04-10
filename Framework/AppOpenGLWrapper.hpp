/**
 * @file AppOpenGLWrapper.hpp
 * @brief This file defines the wrapper component for OpenGL Environment.
 * 
 * This file defines the wrapper component (`AppOpenGLWrapper`), 
 * which is intended to replace the Live2D Framework methods 
 * that use `glew`/`glad` with `QOpenGLFunctions`.
 * 
 * <br/>
 * 
 * Advantages:
 * - Support `QOpenGLWidget` & `glad`.
 * - Manage OpenGL Context easily.
 * 
 * @author SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once

#ifdef USE_GLAD_GLLOADER

#include <glad/glad.h>

#define APP_CALL_GLFUNC

#define initializeOpenGLFunctions gladLoadGL

#else

#include <QtGui/QOpenGLFunctions>

/**
 * @class AppOpenGLWrapper
 * @brief The wrapper for Qt 5 OpenGL Environment.
 */
class AppOpenGLWrapper {
public:
    /**
     * @brief Gets the instance of the wrapper.
     * 
     * @warning User should initialize OpenGL Context
     *          with `AppOpenGLWrapper::get()->initializeOpenGLFunctions();`
     *          before calling any OpenGL function.
     * 
     * @return the `QOpenGLFunctions` instance of the wrapper.
     */
    inline static QOpenGLFunctions* get(){
        /* Singleton pattern. */
        /* In order to share a same OpenGL context. */
        if(instance == nullptr)
            instance = new QOpenGLFunctions;
        return instance;
    }
    /** @brief Releases the instance. */
    inline static void release(){
        if(instance != nullptr)
            delete instance;
        instance = nullptr;
    }
private:
    static QOpenGLFunctions* instance;  /**< The wrapper instance. */
};

#define APP_CALL_GLFUNC AppOpenGLWrapper::get()->

#endif

/**
 * @file coreManager.h
 * @brief A source file defining an application resouces manager.
 * 
 * @author SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once
#include "drivers/allocator.h"
#include "gui/animeWidget.h"

class Renderer;
class TextureManager;

/**
 * @class CoreManager
 * @brief Application resouces manager.
 * 
 * Manage Cubism SDK, ModelManager, QOpenGLWidget and so on.
 */
class CoreManager {
public:
    /**
     * @brief Return an instance (singleton) of the class.
     * 
     * If the instance has not been created, it is created internally.
     *
     * @return Instances of the class.
     */
    static CoreManager* GetInstance();

    /**
     * @brief Release an instance (singleton) of the class.
     *
     */
    static void ReleaseInstance();

    bool Initialize(AnimeWidget* window);

    void Release();

    void resize(int width,int height);
    void update();

    void mousePressEvent(int x, int y);
    void mouseReleaseEvent(int x, int y);
    void mouseMoveEvent(int x, int y);

    /**
     * @brief Initiate lip-sync actively
     * 
     * @todo TODO: make sure this will not conflict with Cubism's internal autoplay audio.
     * 
     * @return the audio is loaded successfully
     */
    bool InitiateLipSync(std::string filePath);

    /**
     * @brief Register shaders.
     */
    GLuint CreateShader();

    /**
     * @brief Get current OpenGL widget.
     */
    AnimeWidget* GetWindow() { return _window; }

    /**
     * @brief Get viewer.
     */
    Renderer* GetView() { return _view; }

    /**
     * @brief Whether to terminate the application or not.
     */
    bool GetIsEnd() { return _isEnd; }

    /**
     * @brief Exit the application.
     */
    void AppEnd() { _isEnd = true; }

    TextureManager* GetTextureManager() { return _textureManager; }

    /**
     * @brief Self-defined rule: Stop responding to tap when drag finished for one time.
     */
    bool isTapFrozen() const { return dragFreezeTap; }

private:
    CoreManager();
    ~CoreManager();

    /**
     * @brief Initialize Cubism SDK
     */
    void InitializeCubism();

    /**
     * @brief CreateShader internal function Error check
     */
    bool CheckShader(GLuint shaderId);

    Allocator _cubismAllocator;                 /**< Cubism SDK Allocator */
    Csm::CubismFramework::Option _cubismOption; /**< Cubism SDK Option */
    AnimeWidget* _window;                       /**< QOpenGLWidget in Qt */
    Renderer* _view;                            /**< Scene Viewer (renderer) */
    bool _captured;                             /**< Is mouse clicking */
    float _mouseX;
    float _mouseY;
    bool _isEnd;                                /**< Is App terminated */
    TextureManager* _textureManager;        /**< Texture Manager */

    int _windowWidth;                           /**< Window width set by Initialize function */
    int _windowHeight;                          /**< Window height set by Initialize function */

    bool dragFreezeTap;                         /**< Stop responding to tap when drag finished (1 time) */
};

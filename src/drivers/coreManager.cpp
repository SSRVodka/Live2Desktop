#include <AppOpenGLWrapper.hpp>

#include "drivers/coreManager.h"
#include "drivers/modelManager.h"
#include "drivers/renderer.h"
#include "drivers/textureManager.h"
#include "drivers/tools.h"

using namespace Csm;

namespace {
    CoreManager* s_instance = NULL;
}

CoreManager* CoreManager::GetInstance() {
    if (s_instance == NULL)
        s_instance = new CoreManager();

    return s_instance;
}

void CoreManager::ReleaseInstance() {
    if (s_instance != NULL)
        delete s_instance;

    s_instance = NULL;
}

bool CoreManager::Initialize(AnimeWidget* window) {
    /* Replaced `glewInit()` or `gladLoadGL()` */
    APP_CALL_GLFUNC initializeOpenGLFunctions();

    /* Texture sampling settings. */
    APP_CALL_GLFUNC glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    APP_CALL_GLFUNC glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    /* Transparency settings. */
    APP_CALL_GLFUNC glEnable(GL_BLEND);
    APP_CALL_GLFUNC glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    /* Register callback function & Window size memory. */
    _windowWidth = window->width();
    _windowHeight = window->height();
    _window = window;
    /* Initialize viewer. */
    _view->Initialize();

    /* Initialize Cubism SDK. */
    InitializeCubism();
    APP_CALL_GLFUNC glViewport(0, 0, _windowWidth, _windowHeight);
    return GL_TRUE;
}

void CoreManager::Release() {
    delete _textureManager;
    delete _view;

    ModelManager::ReleaseInstance();

    CubismFramework::Dispose();
}


void CoreManager::resize(int width,int height) {
    if((_windowWidth!=width || _windowHeight!=height) && width>0 && height>0) {
        _view->Initialize();
        /* Save current size. */
        _windowWidth = width;
        _windowHeight = height;

        /* Viewport Change. */
        APP_CALL_GLFUNC glViewport(0, 0, width, height);
    }
}

void CoreManager::update() {
    ToolFunctions::UpdateTime();

    /* Reinitializes OpenGL canvas every time when it updates. */
    APP_CALL_GLFUNC glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    APP_CALL_GLFUNC glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    APP_CALL_GLFUNC glClearDepthf(1.0);

    /* Update render engine. */
    _view->Render();
}

CoreManager::CoreManager():
    _cubismOption(),
    _window(NULL),
    _captured(false),
    _mouseX(0.0f),
    _mouseY(0.0f),
    _isEnd(false),
    _windowWidth(0),
    _windowHeight(0),
    dragFreezeTap(false) {
    _view = new Renderer();
    _textureManager = new TextureManager();
}

CoreManager::~CoreManager() {
    Release();
}

void CoreManager::InitializeCubism() {
    /* Setup Cubism SDK. */
    Csm::CubismFramework::StartUp(&_cubismAllocator);

    /* Initialize Cubism SDK. */
    CubismFramework::Initialize();

    /* Load model(s). */
    ModelManager::GetInstance();

    /* Default projection. */
    CubismMatrix44 projection;

    ToolFunctions::UpdateTime();
}

void CoreManager::mousePressEvent(int x, int y) {
    if (_view == NULL)
        return;
    _captured = true;
    _view->OnTouchesBegan((float)x, (float)y);
}

void CoreManager::mouseReleaseEvent(int x, int y) {
    if (_view == NULL)
        return;

    if (_captured) {
        _captured = false;
        _view->OnTouchesEnded((float)x,(float)y);
    }
    
    dragFreezeTap = false;
}

void CoreManager::mouseMoveEvent(int x, int y) {
    _mouseX = static_cast<float>(x);
    _mouseY = static_cast<float>(y);

    if (!_captured)
        return;
    if (_view == NULL)
        return;
    _view->OnTouchesMoved(x, y);
    
    dragFreezeTap = true;
}

bool CoreManager::InitiateLipSync(std::string filePath) {
    return ModelManager::GetInstance()->StartExternalLipSync(filePath.data());
}

GLuint CoreManager::CreateShader() {
    /* Compiling Vertex Shaders. */
    GLuint vertexShaderId = APP_CALL_GLFUNC glCreateShader(GL_VERTEX_SHADER);
    const char* vertexShader =
        "#version 120\n"
        "attribute vec3 position;"
        "attribute vec2 uv;"
        "varying vec2 vuv;"
        "void main(void){"
        "    gl_Position = vec4(position, 1.0);"
        "    vuv = uv;"
        "}";
    APP_CALL_GLFUNC glShaderSource(vertexShaderId, 1, &vertexShader, NULL);
    APP_CALL_GLFUNC glCompileShader(vertexShaderId);
    if(!CheckShader(vertexShaderId)) {
        return 0;
    }

    /* Compiling fragment shaders. */
    GLuint fragmentShaderId = APP_CALL_GLFUNC glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentShader =
        "#version 120\n"
        "varying vec2 vuv;"
        "uniform sampler2D texture;"
        "uniform vec4 baseColor;"
        "void main(void){"
        "    gl_FragColor = texture2D(texture, vuv) * baseColor;"
        "}";
    APP_CALL_GLFUNC glShaderSource(fragmentShaderId, 1, &fragmentShader, NULL);
    APP_CALL_GLFUNC glCompileShader(fragmentShaderId);
    if (!CheckShader(fragmentShaderId)) {
        return 0;
    }

    /* Creating Program Objects. */
    GLuint programId = APP_CALL_GLFUNC glCreateProgram();
    APP_CALL_GLFUNC glAttachShader(programId, vertexShaderId);
    APP_CALL_GLFUNC glAttachShader(programId, fragmentShaderId);

    /* Link. */
    APP_CALL_GLFUNC glLinkProgram(programId);

    APP_CALL_GLFUNC glUseProgram(programId);

    return programId;
}

bool CoreManager::CheckShader(GLuint shaderId) {
    GLint status;
    GLint logLength;
    APP_CALL_GLFUNC glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar* log = reinterpret_cast<GLchar*>(CSM_MALLOC(logLength));
        APP_CALL_GLFUNC glGetShaderInfoLog(shaderId, logLength, &logLength, log);
        CubismLogError("Shader compile log: %s", log);
        CSM_FREE(log);
    }

    APP_CALL_GLFUNC glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        APP_CALL_GLFUNC glDeleteShader(shaderId);
        return false;
    }

    return true;
}

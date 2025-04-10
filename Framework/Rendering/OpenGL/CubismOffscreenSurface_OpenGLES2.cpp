/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismOffscreenSurface_OpenGLES2.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

CubismOffscreenSurface_OpenGLES2::CubismOffscreenSurface_OpenGLES2()
    : _renderTexture(0)
    , _colorBuffer(0)
    , _oldFBO(0)
    , _bufferWidth(0)
    , _bufferHeight(0)
    , _isColorBufferInherited(false)
{
}


void CubismOffscreenSurface_OpenGLES2::BeginDraw(GLint restoreFBO)
{
    if (_renderTexture == 0)
    {
        return;
    }

    // バックバッファのサーフェイスを記憶しておく
    if (restoreFBO < 0)
    {
        APP_CALL_GLFUNC glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_oldFBO);
    }
    else
    {
        _oldFBO = restoreFBO;
    }

    // マスク用RenderTextureをactiveにセット
    APP_CALL_GLFUNC glBindFramebuffer(GL_FRAMEBUFFER, _renderTexture);
}

void CubismOffscreenSurface_OpenGLES2::EndDraw()
{
    if (_renderTexture == 0)
    {
        return;
    }

    // 描画対象を戻す
    APP_CALL_GLFUNC glBindFramebuffer(GL_FRAMEBUFFER, _oldFBO);
}

void CubismOffscreenSurface_OpenGLES2::Clear(float r, float g, float b, float a)
{
    // マスクをクリアする
    APP_CALL_GLFUNC glClearColor(r,g,b,a);
    APP_CALL_GLFUNC glClear(GL_COLOR_BUFFER_BIT);
}

csmBool CubismOffscreenSurface_OpenGLES2::CreateOffscreenSurface(csmUint32 displayBufferWidth, csmUint32 displayBufferHeight, GLuint colorBuffer)
{
    // 一旦削除
    DestroyOffscreenSurface();

    do
    {
        GLuint ret = 0;

        // 新しく生成する
        if (colorBuffer == 0)
        {
            APP_CALL_GLFUNC glGenTextures(1, &_colorBuffer);

            APP_CALL_GLFUNC glBindTexture(GL_TEXTURE_2D, _colorBuffer);
            APP_CALL_GLFUNC glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, displayBufferWidth, displayBufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            APP_CALL_GLFUNC glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            APP_CALL_GLFUNC glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            APP_CALL_GLFUNC glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            APP_CALL_GLFUNC glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            APP_CALL_GLFUNC glBindTexture(GL_TEXTURE_2D, 0);

            _isColorBufferInherited = false;
        }
        else
        {// 指定されたものを使用
            _colorBuffer = colorBuffer;

            _isColorBufferInherited = true;
        }

        GLint tmpFramebufferObject;
        APP_CALL_GLFUNC glGetIntegerv(GL_FRAMEBUFFER_BINDING, &tmpFramebufferObject);

        APP_CALL_GLFUNC glGenFramebuffers(1, &ret);
        APP_CALL_GLFUNC glBindFramebuffer(GL_FRAMEBUFFER, ret);
        APP_CALL_GLFUNC glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorBuffer, 0);
        APP_CALL_GLFUNC glBindFramebuffer(GL_FRAMEBUFFER, tmpFramebufferObject);

        _renderTexture = ret;

        _bufferWidth = displayBufferWidth;
        _bufferHeight = displayBufferHeight;

        // 成功
        return true;

    } while (0);

    // 失敗したので削除
    DestroyOffscreenSurface();

    return false;
}

void CubismOffscreenSurface_OpenGLES2::DestroyOffscreenSurface()
{
    if (!_isColorBufferInherited && (_colorBuffer != 0))
    {
        APP_CALL_GLFUNC glDeleteTextures(1, &_colorBuffer);
        _colorBuffer = 0;
    }

    if (_renderTexture!=0)
    {
        APP_CALL_GLFUNC glDeleteFramebuffers(1, &_renderTexture);
        _renderTexture = 0;
    }
}

GLuint CubismOffscreenSurface_OpenGLES2::GetRenderTexture() const
{
    return _renderTexture;
}

GLuint CubismOffscreenSurface_OpenGLES2::GetColorBuffer() const
{
    return _colorBuffer;
}

csmUint32 CubismOffscreenSurface_OpenGLES2::GetBufferWidth() const
{
    return _bufferWidth;
}

csmUint32 CubismOffscreenSurface_OpenGLES2::GetBufferHeight() const
{
    return _bufferHeight;
}

csmBool CubismOffscreenSurface_OpenGLES2::IsValid() const
{
    return _renderTexture != 0;
}

}}}}

//------------ LIVE2D NAMESPACE ------------

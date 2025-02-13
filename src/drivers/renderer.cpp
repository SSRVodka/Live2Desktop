#include <math.h>
#include <string>

#include "drivers/coreManager.h"
#include "drivers/eventHandler.h"
#include "utils/logger.h"
#include "drivers/model.h"
#include "drivers/modelManager.h"
#include "drivers/modelParameters.h"
#include "drivers/renderer.h"
#include "drivers/textureManager.h"
#include "drivers/tools.h"

using namespace std;
using namespace ModelParameters;

Renderer::Renderer():
    _programId(0),
    _renderTarget(SelectTarget_None) {
    _clearColor[0] = 1.0f;
    _clearColor[1] = 1.0f;
    _clearColor[2] = 1.0f;
    _clearColor[3] = 0.0f;

    /* Touch-related event management. */
    _touchManager = new TouchManager();

    /* To convert from device coordinates to screen coordinates. */
    _deviceToScreen = new CubismMatrix44();

    /* Matrix that transforms the scaling and movement of the screen display. */
    _viewMatrix = new CubismViewMatrix();
}

Renderer::~Renderer() {
    _renderBuffer.DestroyOffscreenSurface();
    delete _viewMatrix;
    delete _deviceToScreen;
    delete _touchManager;
}

void Renderer::Initialize() {
    int width = CoreManager::GetInstance()->GetWindow()->width();
    int height = CoreManager::GetInstance()->GetWindow()->height();
    if(width==0 || height==0)
        return;

    /* Based on vertical size. */
    float ratio = static_cast<float>(width) / static_cast<float>(height);
    float left = -ratio;
    float right = ratio;
    float bottom = ViewLogicalLeft;
    float top = ViewLogicalRight;

    /* The range of the screen corresponding to the device.
     * Left edge of X, Right edge of X, Bottom edge of Y, Top edge of Y */
    _viewMatrix->SetScreenRect(left, right, bottom, top);
    _viewMatrix->Scale(ViewScale, ViewScale);

    _deviceToScreen->LoadIdentity(); /* Must be reset when size changes, etc. */
    if (width > height) {
        float screenW = fabsf(right - left);
        _deviceToScreen->ScaleRelative(screenW / width, -screenW / width);
    }
    else {
        float screenH = fabsf(top - bottom);
        _deviceToScreen->ScaleRelative(screenH / height, -screenH / height);
    }
    _deviceToScreen->TranslateRelative(-width * 0.5f, -height * 0.5f);

    /* Display range setting. */
    _viewMatrix->SetMaxScale(ViewMaxScale); /* marginal magnification factor. */
    _viewMatrix->SetMinScale(ViewMinScale); /* marginal reduction ratio. */

    /* Maximum range that can be displayed. */
    _viewMatrix->SetMaxScreenRect(
        ViewLogicalMaxLeft,
        ViewLogicalMaxRight,
        ViewLogicalMaxBottom,
        ViewLogicalMaxTop
    );
}

void Renderer::Render() {

    ModelManager* Live2DManager = ModelManager::GetInstance();

    Live2DManager->SetViewMatrix(_viewMatrix);

    /* Cubism update and drawing. */
    Live2DManager->OnUpdate();
}

void Renderer::OnTouchesBegan(float px, float py) const {
    _touchManager->TouchesBegan(px, py);
}

void Renderer::OnTouchesMoved(float px, float py) const {
    float viewX = this->TransformViewX(_touchManager->GetX());
    float viewY = this->TransformViewY(_touchManager->GetY());

    _touchManager->TouchesMoved(px, py);

    ModelManager* Live2DManager = ModelManager::GetInstance();
    Live2DManager->OnDrag(viewX, viewY);
}

void Renderer::OnTouchesEnded(float , float ) const {
    /* touchdown. */
    ModelManager* live2DManager = ModelManager::GetInstance();
    live2DManager->OnDrag(0.0f, 0.0f);
    /* self defined rule */
    if (!CoreManager::GetInstance()->isTapFrozen()) {
        /* single-tap. */

        /* Obtain the coordinates of the logical coordinate transformation. */
        float x = _deviceToScreen->TransformX(_touchManager->GetX());
        float y = _deviceToScreen->TransformY(_touchManager->GetY());
        stdLogger.Debug(
            QString("Touches ended at (%1, %2)")
            .arg(QString::number(x, 'f', 2))
            .arg(QString::number(y, 'f', 2))
            .toStdString().c_str()
        );
        live2DManager->OnTap(x, y);
    }
}

float Renderer::TransformViewX(float deviceX) const {
    /* Obtain the coordinates of the logical coordinate transformation. */
    float screenX = _deviceToScreen->TransformX(deviceX);
    /* Value after zooming in, zooming out, or moving. */
    return _viewMatrix->InvertTransformX(screenX);
}

float Renderer::TransformViewY(float deviceY) const {
    /* Obtain the coordinates of the logical coordinate transformation. */
    float screenY = _deviceToScreen->TransformY(deviceY);
    /* Value after zooming in, zooming out, or moving. */
    return _viewMatrix->InvertTransformY(screenY);
}

float Renderer::TransformScreenX(float deviceX) const {
    return _deviceToScreen->TransformX(deviceX);
}

float Renderer::TransformScreenY(float deviceY) const {
    return _deviceToScreen->TransformY(deviceY);
}

void Renderer::PreModelDraw(Model& refModel) {
    /* Framebuffer to use when drawing to a different render target. */
    Csm::Rendering::CubismOffscreenSurface_OpenGLES2* useTarget = NULL;

    /* When drawing to a different rendering target */
    if (_renderTarget != SelectTarget_None) {
        /* Targets to be used */
        useTarget = (_renderTarget == SelectTarget_ViewFrameBuffer) ? &_renderBuffer : &refModel.GetRenderBuffer();

        /* If the internal drawing target has not been created, create it here. */
        if (!useTarget->IsValid()) {
            int width = CoreManager::GetInstance()->GetWindow()->width();
            int height = CoreManager::GetInstance()->GetWindow()->height();
            if (width != 0 && height != 0) {
                /* Model Drawing Canvas. */
                useTarget->CreateOffscreenSurface(static_cast<csmUint32>(width), static_cast<csmUint32>(height));
            }
        }

        /* Rendering start. */
        useTarget->BeginDraw();
        useTarget->Clear(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]); // 背景クリアカラー
    }
}

void Renderer::PostModelDraw(Model& refModel) {
    /* Framebuffer to use when drawing to a different render target. */
    Csm::Rendering::CubismOffscreenSurface_OpenGLES2* useTarget = NULL;

    /* When drawing to a different rendering target */
    if (_renderTarget != SelectTarget_None) {
        /* Targets to be used */
        useTarget = (_renderTarget == SelectTarget_ViewFrameBuffer) ? &_renderBuffer : &refModel.GetRenderBuffer();

        /* End of rendering. */
        useTarget->EndDraw();
    }

}

void Renderer::SwitchRenderingTarget(SelectTarget targetType) {
    _renderTarget = targetType;
}

void Renderer::SetRenderTargetClearColor(float r, float g, float b) {
    _clearColor[0] = r;
    _clearColor[1] = g;
    _clearColor[2] = b;
}


float Renderer::GetSpriteAlpha(int assign) const {
    /* Appropriately determined according to the value of `assign`. */

    /* Appropriate difference in alpha as a sample. */
    float alpha = 0.25f + static_cast<float>(assign) * 0.5f;
    if (alpha > 1.0f)
        alpha = 1.0f;
    else if (alpha < 0.1f)
        alpha = 0.1f;

    return alpha;
}

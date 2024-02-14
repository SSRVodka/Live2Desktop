/**
 * @file renderer.h
 * @brief A source file defining the renderer for the application.
 * 
 * @author SJTU-XHW
 * @date   Feb 12, 2024
 */

#pragma once

#include <AppOpenGLWrapper.hpp>
#include <CubismFramework.hpp>

#include <Math/CubismMatrix44.hpp>
#include <Math/CubismViewMatrix.hpp>
#include <Rendering/OpenGL/CubismOffscreenSurface_OpenGLES2.hpp>

class TouchManager;
class Model;

/**
 * @class Renderer
 * @brief Rendering class
 */
class Renderer {
public:

    /**
     * @brief Model rendering destination
     */
    enum SelectTarget {
        SelectTarget_None,                /**< Render to default frame buffer */
        SelectTarget_ModelFrameBuffer,    /**< Rendering to each Model's own framebuffer */
        SelectTarget_ViewFrameBuffer,     /**< Rendering to the frame buffer held by `Renderer` */
    };

    Renderer();
    ~Renderer();

    void Initialize();

    void Render();

    /**
     * @brief Called when touched.
     *
     * @param[in] pointX    Screen X-coordinate
     * @param[in] pointY    Screen Y-coordinate
     */
    void OnTouchesBegan(float pointX, float pointY) const;

    /**
    * @brief It is called if the pointer moves while touching.
    *
    * @param[in] pointX     Screen X-coordinate
    * @param[in] pointY     Screen Y-coordinate
    */
    void OnTouchesMoved(float pointX, float pointY) const;

    /**
    * @brief When the touch is finished, it is called.
    *
    * @param[in] pointX     Screen X-coordinate
    * @param[in] pointY     Screen Y-coordinate
    */
    void OnTouchesEnded(float pointX, float pointY) const;

    /**
    * @brief Convert X coordinate to View coordinate.
    *
    * @param[in] deviceX    Device X-coordinate
    */
    float TransformViewX(float deviceX) const;

    /**
    * @brief Convert Y coordinate to View coordinate.
    *
    * @param[in] deviceY    Device Y-coordinate
    */
    float TransformViewY(float deviceY) const;

    /**
    * @brief Convert X coordinate to Screen coordinate.
    *
    * @param[in] deviceX    Device X-coordinate
    */
    float TransformScreenX(float deviceX) const;

    /**
    * @brief Convert Y coordinate to Screen coordinate.
    *
    * @param[in] deviceY    Device Y-coordinate
    */
    float TransformScreenY(float deviceY) const;

    /**
     * @brief Called just before drawing a single model.
     */
    void PreModelDraw(Model& refModel);

    /**
     * @brief Called immediately after drawing one model.
     */
    void PostModelDraw(Model& refModel);

    /**
     * @brief Draws a model on a different rendering target.
     * 
     * Determine alpha when rendering.
     */
    float GetSpriteAlpha(int assign) const;

    /**
     * @brief Switch rendering destination.
     */
    void SwitchRenderingTarget(SelectTarget targetType);

    /**
     * @brief Clear background color setting when 
     *        switching to a rendering destination other than the default.
     * @param[in]   r   Red(0.0~1.0)
     * @param[in]   g   Green(0.0~1.0)
     * @param[in]   b   Blue(0.0~1.0)
     */
    void SetRenderTargetClearColor(float r, float g, float b);

private:
    TouchManager* _touchManager;            /**< Touch Manager */
    Csm::CubismMatrix44* _deviceToScreen;   /**< Device to screen matrix */
    Csm::CubismViewMatrix* _viewMatrix;     /**< viewMatrix */
    GLuint _programId;                      /**< shader ID */
    
    Csm::Rendering::CubismOffscreenFrame_OpenGLES2 _renderBuffer;   /**< Some modes render Cubism model results this way. */
    SelectTarget _renderTarget;     /**< Choice of rendering destination. */
    float _clearColor[4];           /**< Rendering target clear color. */
};

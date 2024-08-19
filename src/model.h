/**
 * @file model.h
 * @brief A source file defining the Live2D model.
 * 
 * @author SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once

#include <Model/CubismUserModel.hpp>
#include <Rendering/OpenGL/CubismOffscreenSurface_OpenGLES2.hpp>
#include <Type/csmRectF.hpp>

#include <CubismFramework.hpp>
#include <ICubismModelSetting.hpp>

#include "wavFileHandler.h"

/**
 * @class Model
 * @brief Implementation class for the model actually used by the user.
 * 
 * Model generation, functional component generation,
 * update processing and rendering calls.
 *
 */
class Model : public Csm::CubismUserModel {
    friend class ModelManager;
public:

    Model();
    virtual ~Model();

    /**
     * @brief Generates a model from the directory and file path 
     *        where model3.json is located.
     */
    bool LoadAssets(const Csm::csmChar* dir, const  Csm::csmChar* fileName);

    /**
     * @brief Rebuild the renderer.
     *
     */
    void ReloadRenderer();

    /**
     * @brief Model update process. 
     * 
     * Determines the drawing state from the model parameters.
     */
    void Update();

    /**
     * @brief The process of drawing the model.
     * 
     * Pass the View-Projection matrix of the space in which to draw the model.
     *
     * @param[in]  matrix  View-Projection Matrix
     */
    void Draw(Csm::CubismMatrix44& matrix);

    /**
     * @brief Starts playback of the motion specified by the argument.
     *
     * @param[in]   group                       The name of motion group
     * @param[in]   no                          The number of the group
     * @param[in]   priority                    The priority of the motion group
     * @param[in]   onFinishedMotionHandler     Callback function called when motion playback ends;
     *                                          if NULL, it is not called.
     * 
     * @return Returns the identification number of the motion that started.
     *         Used in the argument of `IsFinished()`,
     *         which determines whether or not an individual motion has finished.
     *         When it cannot be started, "-1" is returned.
     */
    Csm::CubismMotionQueueEntryHandle StartMotion(const Csm::csmChar* group, Csm::csmInt32 no, Csm::csmInt32 priority, Csm::ACubismMotion::FinishedMotionCallback onFinishedMotionHandler = NULL);

    /**
     * @brief Starts playback of a randomly selected motion.
     *
     * @param[in]   group                       The name of motion group
     * @param[in]   priority                    The priority of the motion group
     * @param[in]   onFinishedMotionHandler     Callback function called when motion playback ends;
     *                                          if NULL, it is not called.
     * 
     * @return Returns the identification number of the motion that started.
     *         Used in the argument of `IsFinished()`,
     *         which determines whether or not an individual motion has finished.
     *         When it cannot be started, "-1" is returned.
     */
    Csm::CubismMotionQueueEntryHandle StartRandomMotion(const Csm::csmChar* group, Csm::csmInt32 priority, Csm::ACubismMotion::FinishedMotionCallback onFinishedMotionHandler = NULL);

    /**
     * @brief Sets the facial expression motion specified by the argument.
     *
     * @param expressionID  Expression Motion ID
     */
    void SetExpression(const Csm::csmChar* expressionID);

    /**
     * @brief Set a randomly selected facial expression motion.
     */
    void SetRandomExpression();

    /**
    * @brief Receive event firing.
    */
    virtual void MotionEventFired(const Live2D::Cubism::Framework::csmString& eventValue);

    /**
     * @brief Hit Judgment Test.
     * 
     * Calculates a rectangle from the vertex list of the specified ID
     * and judges whether the coordinates are within the rectangle.
     *
     * @param[in] hitAreaName   ID of the target to test for hits
     * @param[in] x             X-coordinate to be determined
     * @param[in] y             X-coordinate to be determined
     */
    virtual Csm::csmBool HitTest(const Csm::csmChar* hitAreaName, Csm::csmFloat32 x, Csm::csmFloat32 y);

    /**
     * @brief Obtaining the buffer to be used when drawing to a different target.
     */
    Csm::Rendering::CubismOffscreenSurface_OpenGLES2& GetRenderBuffer();

protected:
    /**
     * @brief The process of drawing the model.
     * 
     * Pass the View-Projection matrix of the space in which to draw the model.
     */
    void DoDraw();

private:
    /**
     * @brief Generate models from model3.json.
     * 
     * Generate model, motion, physics and other components
     * according to the description in model3.json.
     *
     * @param[in] setting   Instance of ICubismModelSetting
     *
     */
    bool SetupModel(Csm::ICubismModelSetting* setting);

    /**
     * @brief Load textures into the OpenGL texture unit.
     */
    void SetupTextures();

    /**
     * @brief Load motion data in batches by group name.
     * 
     * Motion data names are obtained internally from ModelSetting.
     *
     * @param[in] group Motion data group name
     */
    void PreloadMotionGroup(const Csm::csmChar* group);

    /**
     * @brief Release motion data from the group name at once.
     * 
     * The name of the motion data is obtained internally from ModelSetting.
     *
     * @param[in] group Motion data group name
     */
    void ReleaseMotionGroup(const Csm::csmChar* group) const;

    /**
     * @brief Release all motion data.
     */
    void ReleaseMotions();

    /**
     * @brief Release all expression data.
     */
    void ReleaseExpressions();

    Csm::ICubismModelSetting* _modelSetting;                        /**< Model Setting Information. */
    Csm::csmString _modelHomeDir;                                   /**< Directory where model settings are located. */
    Csm::csmFloat32 _userTimeSeconds;                               /**< Totalized delta time [s]. */
    Csm::csmVector<Csm::CubismIdHandle> _eyeBlinkIds;               /**< Parameter ID for blink function set in the model. */
    Csm::csmVector<Csm::CubismIdHandle> _lipSyncIds;                /**< Parameter ID for lip-sync function set in the model. */
    Csm::csmMap<Csm::csmString, Csm::ACubismMotion*> _motions;      /**< List of loaded motions. */
    Csm::csmMap<Csm::csmString, Csm::ACubismMotion*> _expressions;  /**< List of loaded expressions. */
    Csm::csmVector<Csm::csmRectF> _hitArea;
    Csm::csmVector<Csm::csmRectF> _userArea;
    const Csm::CubismId* _idParamAngleX;        /**< Parameter ID: ParamAngleX. */
    const Csm::CubismId* _idParamAngleY;        /**< Parameter ID: ParamAngleY. */
    const Csm::CubismId* _idParamAngleZ;        /**< Parameter ID: ParamAngleZ. */
    const Csm::CubismId* _idParamBodyAngleX;    /**< Parameter ID: ParamBodyAngleX. */
    const Csm::CubismId* _idParamEyeBallX;      /**< Parameter ID: ParamEyeBallX. */
    const Csm::CubismId* _idParamEyeBallY;      /**< Parameter ID: ParamEyeBallY. */

    WavFileHandler _wavFileHandler; /**< wav file handler. */

    Csm::Rendering::CubismOffscreenSurface_OpenGLES2  _renderBuffer;  /**< Drawing destination other than frame buffer. */
};




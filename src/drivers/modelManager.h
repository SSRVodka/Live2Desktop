/**
 * @file modelManager.h
 * @brief A source file defining the ModelManager.
 * 
 * @author SSRVodka
 * @date   Feb 12, 2024
 */

#pragma once

#include <Math/CubismMatrix44.hpp>
#include <Type/csmVector.hpp>

#include <CubismFramework.hpp>

class Model;

/**
 * @class ModelManager
 * @brief Manages CubismModel in the application.

 * Performs model creation and destruction,
 * tap event handling, and model switching.
 *
 */
class ModelManager {

public:
    /**
     * @brief Return an instance (singleton) of the class.
     * 
     * If no instance has been created, an instance is created internally.
     *
     * @return the instance
     */
    static ModelManager* GetInstance();

    /**
     * @brief Release the instance.
     *
     */
    static void ReleaseInstance();

    /**
     * @brief Returns the model held in the current scene.
     *
     * @param[in] no     Index value of model list
     * @return           Returns an instance of the model.
     *                   If the index value is out of range, return NULL.
     */
    Model* GetModel(Csm::csmUint32 no) const;

    /**
     * @brief Release all models held in the current scene.
     *
     */
    void ReleaseAllModel();

    /**
    * @brief Processing when dragging the screen.
    *
    * @param[in] x  X-coordinate of screen
    * @param[in] y  Y-coordinate of screen
    */
    void OnDrag(Csm::csmFloat32 x, Csm::csmFloat32 y) const;

    /**
    * @brief Processing when the screen is tapped.
    *
    * @param[in] x X-coordinate of screen
    * @param[in] y Y-coordinate of screen
    */
    void OnTap(Csm::csmFloat32 x, Csm::csmFloat32 y);

    /**
    * @brief Processing when updating the screen.
    * 
    * Processes updating and drawing models.
    */
    void OnUpdate() const;

    /**
     * @brief Initiates lip synchronization actively.
     * 
     * @param[in] filePath 16-bit wav audio that can be loaded by `WavFileHandler`
     * 
     * @note Calls to this interface may conflict with Cubism's internal autoplay audio.
     *  The caller should take appropriate measures to avoid conflicts.
     * 
     * @return if the audio is successfully loaded
     */
    bool StartExternalLipSync(Csm::csmChar* filePath) const;

    /**
    * @brief Switching Scenes.
    * 
    * The application switches between model sets.
    */
    bool ChangeScene(Csm::csmChar* name);

    /**
     * @brief   Obtain the number of model(s).
     * @return  The number of the model(s).
     */
    Csm::csmUint32 GetModelNum() const;

    /**
     * @brief Set view matrix.
     */
    void SetViewMatrix(Live2D::Cubism::Framework::CubismMatrix44* m);

private:

    ModelManager();
    virtual ~ModelManager();

    Csm::CubismMatrix44*        _viewMatrix;    /**< View matrix used for model rendering. */
    Csm::csmVector<Model*>  _models;            /**< The container for model instances. */
};

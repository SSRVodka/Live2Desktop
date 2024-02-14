#include <string>

#include <AppOpenGLWrapper.hpp>

#include <Rendering/CubismRenderer.hpp>

#include "coreManager.h"
#include "logger.h"
#include "model.h"
#include "modelManager.h"
#include "modelParameters.h"
#include "renderer.h"
#include "resourceLoader.h"


using namespace Csm;
using namespace std;
using namespace ModelParameters;

namespace {
    ModelManager* s_instance = NULL;

    void FinishedMotion(ACubismMotion* self) {
        QString tmp;
        stdLogger.Debug(
            tmp.asprintf("Motion Finished: %8p", self)
            .toStdString().c_str()
        );
    }
}

ModelManager* ModelManager::GetInstance() {
    if (s_instance == NULL)
        s_instance = new ModelManager();

    return s_instance;
}

void ModelManager::ReleaseInstance() {
    if (s_instance != NULL)
        delete s_instance;

    s_instance = NULL;
}

ModelManager::ModelManager()
    : _viewMatrix(NULL) {
    _viewMatrix = new CubismMatrix44();

    auto m = resourceLoader::get_instance().getCurrentModelName();
    if(!ChangeScene((Csm::csmChar*)m.toStdString().c_str())) {
        stdLogger.Exception("Failed to load current model. Please check your configuration file.");
    }
}

ModelManager::~ModelManager() {
    ReleaseAllModel();
}

void ModelManager::ReleaseAllModel() {
    for (csmUint32 i = 0; i < _models.GetSize(); i++)
        delete _models[i];

    _models.Clear();
}

Model* ModelManager::GetModel(csmUint32 no) const {
    if (no < _models.GetSize())
        return _models[no];

    return NULL;
}

void ModelManager::OnDrag(csmFloat32 x, csmFloat32 y) const {
    for (csmUint32 i = 0; i < _models.GetSize(); i++) {
        Model* model = GetModel(i);

        model->SetDragging(x, y);
    }
}

void ModelManager::OnTap(csmFloat32 x, csmFloat32 y) {
    for (csmUint32 i = 0; i < _models.GetSize(); i++) {
        /* It was commented because many models do not have "hit area" designed. */

        /* if (_models[i]->HitTest(HitAreaNameHead, x, y)) {
         *     stdLogger.Debug(
         *         QString("Hit area: [%1]")
         *         .arg(HitAreaNameHead)
         *         .toStdString().c_str()
         *     );
         *     _models[i]->SetRandomExpression();
         * } else if (_models[i]->HitTest(HitAreaNameBody, x, y)) {
         *     stdLogger.Debug(
         *         QString("Hit area: [%1]")
         *         .arg(HitAreaNameBody)
         *         .toStdString().c_str()
         *     );
         *     _models[i]->StartRandomMotion(MotionGroupTapBody, PriorityNormal, FinishedMotion);
         * }
         */
        
        if (_models[i]->_expressions.GetSize() == 0) {
            _models[i]->StartRandomMotion(MotionGroupTapBody, PriorityNormal, FinishedMotion);
        } else if (_models[i]->_modelSetting->GetMotionCount(MotionGroupTapBody) == 0) {
            _models[i]->SetRandomExpression();
        } else {
            if (rand() % 2) _models[i]->SetRandomExpression();
            else _models[i]->StartRandomMotion(MotionGroupTapBody, PriorityNormal, FinishedMotion);
        }
    }
}

void ModelManager::OnUpdate() const
{
    int width = CoreManager::GetInstance()->GetWindow()->width();
    int height = CoreManager::GetInstance()->GetWindow()->height();

    CubismMatrix44 projection;
    csmUint32 modelCount = _models.GetSize();
    for (csmUint32 i = 0; i < modelCount; ++i) {
        Model* model = GetModel(i);

        if (model->GetModel()->GetCanvasWidth() > 1.0f && width < height) {
            /* Calculate scale by the horizontal size of the model 
             * when displaying a long model in a portrait window. */
            model->GetModelMatrix()->SetWidth(2.0f);
            projection.Scale(1.0f, static_cast<float>(width) / static_cast<float>(height));
        } else {
            projection.Scale(static_cast<float>(height) / static_cast<float>(width), 1.0f);
        }

        /* Multiply here if necessary. */
        if (_viewMatrix != NULL)
            projection.MultiplyByMatrix(_viewMatrix);

        /* Call before drawing a model. */
        CoreManager::GetInstance()->GetView()->PreModelDraw(*model);

        model->Update();
        /* Since the reference is passed by reference, 
         * the PROJECTION is transformed. */
        model->Draw(projection);

        /* Call after drawing a model. */
        CoreManager::GetInstance()->GetView()->PostModelDraw(*model);
    }
}

bool ModelManager::ChangeScene(Csm::csmChar* name) {
    stdLogger.Debug(
        QString("Current model index: %1")
        .arg(name)
        .toStdString().c_str()
    );
    /* From the directory name held in ModelDir[].
     * determine the path of model3.json from the directory name held in ModelDir[].
     * Make sure the directory name matches the name of model3.json. */
    char modelPath[128];
    char modelJsonName[128];
    snprintf(modelPath,128,"%s%s/",ResourcesPath,(char*)name);
    snprintf(modelJsonName,128,"%s.model3.json", (char*)name);

    ReleaseAllModel();
    _models.PushBack(new Model());
    if(_models[0]->LoadAssets(modelPath, modelJsonName)==false) {
        ReleaseAllModel();
        return false;
    }

    /*
     * Present a sample that displays model semi-transparency.
     * If USE_RENDER_TARGET and USE_MODEL_RENDER_TARGET are defined here, then
     * draw the model on another render target, and stick the drawing result as a texture on another sprite.
     */
    {
#if defined(USE_RENDER_TARGET)
        // Select this option when drawing to a target owned by Renderer.
        Renderer::SelectTarget useRenderTarget = Renderer::SelectTarget_ViewFrameBuffer;
#elif defined(USE_MODEL_RENDER_TARGET)
        // Select this option when drawing to the target of each Model.
        Renderer::SelectTarget useRenderTarget = Renderer::SelectTarget_ModelFrameBuffer;
#else
        // Render to default main frame buffer (normal)
        Renderer::SelectTarget useRenderTarget = Renderer::SelectTarget_None;
#endif

#if defined(USE_RENDER_TARGET) || defined(USE_MODEL_RENDER_TARGET)
        /* As a sample of attaching alpha to individual models,
         * create another model and shift the position slightly. */
        _models.PushBack(new Model());
        _models[1]->LoadAssets(modelPath.c_str(), modelJsonName.c_str());
        _models[1]->GetModelMatrix()->TranslateX(0.2f);
#endif

        CoreManager::GetInstance()->GetView()->SwitchRenderingTarget(useRenderTarget);

        /* Clear background color when selecting a different rendering destination. */
        float clearColor[3] = { 0.0f, 0.0f, 0.0f };
        CoreManager::GetInstance()->GetView()->SetRenderTargetClearColor(clearColor[0], clearColor[1], clearColor[2]);
    }
    return true;
}

csmUint32 ModelManager::GetModelNum() const {
    return _models.GetSize();
}

void ModelManager::SetViewMatrix(CubismMatrix44* m) {
    for (int i = 0; i < 16; i++) {
        _viewMatrix->GetArray()[i] = m->GetArray()[i];
    }
}

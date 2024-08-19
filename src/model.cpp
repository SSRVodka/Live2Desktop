#include <fstream>
#include <vector>

#include <Id/CubismIdManager.hpp>
#include <Motion/CubismMotion.hpp>
#include <Motion/CubismMotionQueueEntry.hpp>
#include <Physics/CubismPhysics.hpp>
#include <Rendering/OpenGL/CubismRenderer_OpenGLES2.hpp>
#include <Utils/CubismString.hpp>

#include <CubismDefaultParameterId.hpp>
#include <CubismModelSettingJson.hpp>


#include "coreManager.h"
#include "logger.h"
#include "model.h"
#include "modelParameters.h"
#include "textureManager.h"
#include "tools.h"

using namespace Live2D::Cubism::Framework;
using namespace Live2D::Cubism::Framework::DefaultParameterId;
using namespace ModelParameters;

namespace {
    csmByte* CreateBuffer(const csmChar* path, csmSizeInt* size) {
        stdLogger.Debug(
            QString("Create resource buffer: %1")
            .arg(path)
            .toStdString().c_str()
        );
        
        return ToolFunctions::LoadFileAsBytes(path, size);
    }

    void DeleteBuffer(csmByte* buffer, const csmChar* path = "") {
        stdLogger.Debug(
            QString("Release resource buffer: %1")
            .arg(path)
            .toStdString().c_str()
        );
        ToolFunctions::ReleaseBytes(buffer);
    }
}

Model::Model()
    : CubismUserModel()
    , _modelSetting(NULL)
    , _userTimeSeconds(0.0f) {

    _idParamAngleX = CubismFramework::GetIdManager()->GetId(ParamAngleX);
    _idParamAngleY = CubismFramework::GetIdManager()->GetId(ParamAngleY);
    _idParamAngleZ = CubismFramework::GetIdManager()->GetId(ParamAngleZ);
    _idParamBodyAngleX = CubismFramework::GetIdManager()->GetId(ParamBodyAngleX);
    _idParamEyeBallX = CubismFramework::GetIdManager()->GetId(ParamEyeBallX);
    _idParamEyeBallY = CubismFramework::GetIdManager()->GetId(ParamEyeBallY);
}

Model::~Model() {
    _renderBuffer.DestroyOffscreenSurface();

    ReleaseMotions();
    ReleaseExpressions();
    if(_modelSetting) {
        for (csmInt32 i = 0; i < _modelSetting->GetMotionGroupCount(); i++) {
            const csmChar* group = _modelSetting->GetMotionGroupName(i);
            ReleaseMotionGroup(group);
        }
        delete(_modelSetting);
    }
}

bool Model::LoadAssets(const csmChar* dir, const csmChar* fileName) {
    _modelHomeDir = dir;

    stdLogger.Debug(
        QString("Load model configuration: %1")
        .arg(fileName)
        .toStdString().c_str()
    );

    csmSizeInt size;
    const csmString path = csmString(dir) + fileName;

    csmByte* buffer = CreateBuffer(path.GetRawString(), &size);
    if(buffer == NULL)
        return false;
    
    ICubismModelSetting* setting = new CubismModelSettingJson(buffer, size);
    DeleteBuffer(buffer, path.GetRawString());

    SetupModel(setting);

    CreateRenderer();

    SetupTextures();
    return true;
}


bool Model::SetupModel(ICubismModelSetting* setting)
{
    _updating = true;
    _initialized = false;

    _modelSetting = setting;

    csmByte* buffer;
    csmSizeInt size;

    /* Cubism Model */
    if (strcmp(_modelSetting->GetModelFileName(), "") != 0) {
        csmString path = _modelSetting->GetModelFileName();
        path = _modelHomeDir + path;

        stdLogger.Debug(
            QString("Create model: %1")
            .arg(setting->GetModelFileName())
            .toStdString().c_str()
        );

        buffer = CreateBuffer(path.GetRawString(), &size);
        if(buffer==NULL)
            return false;
        
        LoadModel(buffer, size);
        DeleteBuffer(buffer, path.GetRawString());
    }

    /* Expression */
    if (_modelSetting->GetExpressionCount() > 0) {
        const csmInt32 count = _modelSetting->GetExpressionCount();
        for (csmInt32 i = 0; i < count; i++) {
            csmString name = _modelSetting->GetExpressionName(i);
            csmString path = _modelSetting->GetExpressionFileName(i);
            path = _modelHomeDir + path;

            buffer = CreateBuffer(path.GetRawString(), &size);
            if(buffer == NULL)
                return false;

            ACubismMotion* motion = LoadExpression(buffer, size, name.GetRawString());

            if (_expressions[name] != NULL) {
                ACubismMotion::Delete(_expressions[name]);
                _expressions[name] = NULL;
            }
            _expressions[name] = motion;

            DeleteBuffer(buffer, path.GetRawString());
        }
    }

    /* Physics */
    if (strcmp(_modelSetting->GetPhysicsFileName(), "") != 0) {
        csmString path = _modelSetting->GetPhysicsFileName();
        path = _modelHomeDir + path;

        buffer = CreateBuffer(path.GetRawString(), &size);
        if(buffer) {
            LoadPhysics(buffer, size);
            DeleteBuffer(buffer, path.GetRawString());
        }
    }

    /* Pose */
    if (strcmp(_modelSetting->GetPoseFileName(), "") != 0) {
        csmString path = _modelSetting->GetPoseFileName();
        path = _modelHomeDir + path;

        buffer = CreateBuffer(path.GetRawString(), &size);
        if(buffer) {
            LoadPose(buffer, size);
            DeleteBuffer(buffer, path.GetRawString());
        }
    }

    /* EyeBlink */
    if (_modelSetting->GetEyeBlinkParameterCount() > 0) {
        _eyeBlink = CubismEyeBlink::Create(_modelSetting);
    }

    /* Breath */
    {
        _breath = CubismBreath::Create();

        csmVector<CubismBreath::BreathParameterData> breathParameters;

        breathParameters.PushBack(CubismBreath::BreathParameterData(_idParamAngleX, 0.0f, 15.0f, 6.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(_idParamAngleY, 0.0f, 8.0f, 3.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(_idParamAngleZ, 0.0f, 10.0f, 5.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(_idParamBodyAngleX, 0.0f, 4.0f, 15.5345f, 0.5f));
        breathParameters.PushBack(CubismBreath::BreathParameterData(CubismFramework::GetIdManager()->GetId(ParamBreath), 0.5f, 0.5f, 3.2345f, 0.5f));

        _breath->SetParameters(breathParameters);
    }

    /* UserData */
    if (strcmp(_modelSetting->GetUserDataFile(), "") != 0) {
        csmString path = _modelSetting->GetUserDataFile();
        path = _modelHomeDir + path;
        buffer = CreateBuffer(path.GetRawString(), &size);
        if(buffer) {
            LoadUserData(buffer, size);
            DeleteBuffer(buffer, path.GetRawString());
        }
    }

    /* EyeBlinkIds */
    {
        csmInt32 eyeBlinkIdCount = _modelSetting->GetEyeBlinkParameterCount();
        for (csmInt32 i = 0; i < eyeBlinkIdCount; ++i) {
            _eyeBlinkIds.PushBack(_modelSetting->GetEyeBlinkParameterId(i));
        }
    }

    /* LipSyncIds */
    {
        csmInt32 lipSyncIdCount = _modelSetting->GetLipSyncParameterCount();
        for (csmInt32 i = 0; i < lipSyncIdCount; ++i) {
            _lipSyncIds.PushBack(_modelSetting->GetLipSyncParameterId(i));
        }
    }

    /* Layout */
    csmMap<csmString, csmFloat32> layout;
    _modelSetting->GetLayoutMap(layout);
    _modelMatrix->SetupFromLayout(layout);

    _model->SaveParameters();

    for (csmInt32 i = 0; i < _modelSetting->GetMotionGroupCount(); i++) {
        const csmChar* group = _modelSetting->GetMotionGroupName(i);
        PreloadMotionGroup(group);
    }

    _motionManager->StopAllMotions();

    _updating = false;
    _initialized = true;
    return true;
}

void Model::PreloadMotionGroup(const csmChar* group) {
    const csmInt32 count = _modelSetting->GetMotionCount(group);

    for (csmInt32 i = 0; i < count; i++) {
        //ex) idle_0
        csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, i);
        csmString path = _modelSetting->GetMotionFileName(group, i);
        path = _modelHomeDir + path;

        stdLogger.Debug(
            QString("Load motion: %1 => [%2_%3] ")
            .arg(path.GetRawString())
            .arg(group)
            .arg(i)
            .toStdString().c_str()
        );

        csmByte* buffer;
        csmSizeInt size;
        buffer = CreateBuffer(path.GetRawString(), &size);
        if(buffer) {
            CubismMotion* tmpMotion = static_cast<CubismMotion*>(LoadMotion(buffer, size, name.GetRawString()));
            csmFloat32 fadeTime = _modelSetting->GetMotionFadeInTimeValue(group, i);
            if (fadeTime >= 0.0f) {
                tmpMotion->SetFadeInTime(fadeTime);
            }

            fadeTime = _modelSetting->GetMotionFadeOutTimeValue(group, i);
            if (fadeTime >= 0.0f) {
                tmpMotion->SetFadeOutTime(fadeTime);
            }
            tmpMotion->SetEffectIds(_eyeBlinkIds, _lipSyncIds);

            if (_motions[name] != NULL) {
                ACubismMotion::Delete(_motions[name]);
            }
            _motions[name] = tmpMotion;

            DeleteBuffer(buffer, path.GetRawString());
        }
    }
}

void Model::ReleaseMotionGroup(const csmChar* group) const {
    const csmInt32 count = _modelSetting->GetMotionCount(group);
    for (csmInt32 i = 0; i < count; i++) {
        csmString voice = _modelSetting->GetMotionSoundFileName(group, i);
        if (strcmp(voice.GetRawString(), "") != 0) {
            csmString path = voice;
            path = _modelHomeDir + path;
        }
    }
}

void Model::ReleaseMotions() {
    for (csmMap<csmString, ACubismMotion*>::const_iterator iter = _motions.Begin(); iter != _motions.End(); ++iter) {
        ACubismMotion::Delete(iter->Second);
    }

    _motions.Clear();
}

void Model::ReleaseExpressions() {
    for (csmMap<csmString, ACubismMotion*>::const_iterator iter = _expressions.Begin(); iter != _expressions.End(); ++iter) {
        ACubismMotion::Delete(iter->Second);
    }

    _expressions.Clear();
}

void Model::Update() {
    const csmFloat32 deltaTimeSeconds = ToolFunctions::GetDeltaTime();
    _userTimeSeconds += deltaTimeSeconds;

    _dragManager->Update(deltaTimeSeconds);
    _dragX = _dragManager->GetX();
    _dragY = _dragManager->GetY();

    /* Parameter update by motion or not. */
    csmBool motionUpdated = false;

    /* --------------- */
    _model->LoadParameters(); /* Load last saved state. */
    if (_motionManager->IsFinished()) {
        /* If there is no motion playback, playback is performed at random 
         * from among the standby motions. */
        StartRandomMotion(MotionGroupIdle, PriorityIdle);
    } else {
        motionUpdated = _motionManager->UpdateMotion(_model, deltaTimeSeconds); // モーションを更新
    }
    _model->SaveParameters(); /* Save the state of the model. */
    /* --------------- */

    /* Blink */
    if (!motionUpdated) {
        if (_eyeBlink != NULL) {
            /* When the main motion is not updated. */
            _eyeBlink->UpdateParameters(_model, deltaTimeSeconds); /* blinking */
        }
    }

    if (_expressionManager != NULL) {
        _expressionManager->UpdateMotion(_model, deltaTimeSeconds); /* Parameter update by facial expression (relative change) */
    }

    /* Changes by dragging */
    /* Adjustment of face direction by dragging */
    _model->AddParameterValue(_idParamAngleX, _dragX * 30); /* Add a value of -30 to 30. */
    _model->AddParameterValue(_idParamAngleY, _dragY * 30);
    _model->AddParameterValue(_idParamAngleZ, _dragX * _dragY * -30);

    /* Adjusting body orientation by dragging. */
    _model->AddParameterValue(_idParamBodyAngleX, _dragX * 10); /* Add a value of -10 to 10. */

    /* Drag to adjust eye orientation. */
    _model->AddParameterValue(_idParamEyeBallX, _dragX); /* Add a value of -1 to 1. */
    _model->AddParameterValue(_idParamEyeBallY, _dragY);

    /* Breath */
    if (_breath != NULL) {
        _breath->UpdateParameters(_model, deltaTimeSeconds);
    }

    /* Physics Settings */
    if (_physics != NULL) {
        _physics->Evaluate(_model, deltaTimeSeconds);
    }

    /* Lip Sync Settings */
    if (_lipSync) {
        /* For real-time lip-sync, obtain the volume 
         * from the system and enter a value in the range of 0 to 1. */
        csmFloat32 value = 0.0f;

        /* Status update/RMS value acquisition. */
        _wavFileHandler.Update(deltaTimeSeconds);
        value = _wavFileHandler.GetRms();

        for (csmUint32 i = 0; i < _lipSyncIds.GetSize(); ++i) {
            _model->AddParameterValue(_lipSyncIds[i], value, 0.8f);
        }
    }

    /* Pose settings */
    if (_pose != NULL) {
        _pose->UpdateParameters(_model, deltaTimeSeconds);
    }

    _model->Update();

}

CubismMotionQueueEntryHandle Model::StartMotion(const csmChar* group, csmInt32 no, csmInt32 priority, ACubismMotion::FinishedMotionCallback onFinishedMotionHandler) {
    if (priority == PriorityForce) {
        _motionManager->SetReservePriority(priority);
    } else if (!_motionManager->ReserveMotion(priority)) {
        stdLogger.Exception(
            QString("Failed to start motion: %1 (%2)")
            .arg(group)
            .arg(no)
            .toStdString().c_str()
        );
        return InvalidMotionQueueEntryHandleValue;
    }

    const csmString fileName = _modelSetting->GetMotionFileName(group, no);

    //ex) idle_0
    csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, no);
    CubismMotion* motion = static_cast<CubismMotion*>(_motions[name.GetRawString()]);
    csmBool autoDelete = false;

    if (motion == NULL) {
        csmString path = fileName;
        path = _modelHomeDir + path;

        csmByte* buffer;
        csmSizeInt size;
        buffer = CreateBuffer(path.GetRawString(), &size);
        if(buffer) {
            motion = static_cast<CubismMotion*>(LoadMotion(buffer, size, NULL, onFinishedMotionHandler));
            csmFloat32 fadeTime = _modelSetting->GetMotionFadeInTimeValue(group, no);
            if (fadeTime >= 0.0f) {
                motion->SetFadeInTime(fadeTime);
            }

            fadeTime = _modelSetting->GetMotionFadeOutTimeValue(group, no);
            if (fadeTime >= 0.0f) {
                motion->SetFadeOutTime(fadeTime);
            }
            motion->SetEffectIds(_eyeBlinkIds, _lipSyncIds);
            autoDelete = true; /* Removed from memory on exit. */

            DeleteBuffer(buffer, path.GetRawString());
        }
    }
    else {
        motion->SetFinishedMotionHandler(onFinishedMotionHandler);
    }

    /* voice */
    csmString voice = _modelSetting->GetMotionSoundFileName(group, no);
    if (strcmp(voice.GetRawString(), "") != 0) {
        csmString path = voice;
        path = _modelHomeDir + path;
        _wavFileHandler.Start(path);
    }

    stdLogger.Debug(
        QString("Start motion: [%1_%2]")
        .arg(group)
        .arg(no)
        .toStdString().c_str()
    );
    return  _motionManager->StartMotionPriority(motion, autoDelete, priority);
}

CubismMotionQueueEntryHandle Model::StartRandomMotion(const csmChar* group, csmInt32 priority, ACubismMotion::FinishedMotionCallback onFinishedMotionHandler) {
    if (_modelSetting->GetMotionCount(group) == 0) {
        return InvalidMotionQueueEntryHandleValue;
    }

    csmInt32 no = rand() % _modelSetting->GetMotionCount(group);

    return StartMotion(group, no, priority, onFinishedMotionHandler);
}

void Model::DoDraw() {
    if (_model == NULL)
        return;

    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->DrawModel();
}

void Model::Draw(CubismMatrix44& matrix) {
    if (_model == NULL)
        return;

    matrix.MultiplyByMatrix(_modelMatrix);

    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->SetMvpMatrix(&matrix);

    DoDraw();
}

csmBool Model::HitTest(const csmChar* hitAreaName, csmFloat32 x, csmFloat32 y) {
    /* When transparent, there is no hit detection. */
    if (_opacity < 1)
        return false;
    
    const csmInt32 count = _modelSetting->GetHitAreasCount();
    for (csmInt32 i = 0; i < count; i++) {
        if (strcmp(_modelSetting->GetHitAreaName(i), hitAreaName) == 0) {
            const CubismIdHandle drawID = _modelSetting->GetHitAreaId(i);
            return IsHit(drawID, x, y);
        }
    }
    return false; /* false if not present. */
}

void Model::SetExpression(const csmChar* expressionID) {
    ACubismMotion* motion = _expressions[expressionID];
    stdLogger.Debug(
        QString("Expression: [%1]")
        .arg(expressionID)
        .toStdString().c_str()
    );

    if (motion != NULL) {
        _expressionManager->StartMotionPriority(motion, false, PriorityForce);
    }
    else {
        stdLogger.Warning(
            QString("Expression [%1] is null.")
            .arg(expressionID)
            .toStdString().c_str()
        );
    }
}

void Model::SetRandomExpression() {
    if (_expressions.GetSize() == 0)
        return;

    csmInt32 no = rand() % _expressions.GetSize();
    csmMap<csmString, ACubismMotion*>::const_iterator map_ite;
    csmInt32 i = 0;
    for (map_ite = _expressions.Begin(); map_ite != _expressions.End(); map_ite++) {
        if (i == no) {
            csmString name = (*map_ite).First;
            SetExpression(name.GetRawString());
            return;
        }
        i++;
    }
}

void Model::ReloadRenderer() {
    DeleteRenderer();

    CreateRenderer();

    SetupTextures();
}

void Model::SetupTextures() {
    for (csmInt32 modelTextureNumber = 0; modelTextureNumber < _modelSetting->GetTextureCount(); modelTextureNumber++) {
        /* Skip load-bind process if texture name is an empty string. */
        if (strcmp(_modelSetting->GetTextureFileName(modelTextureNumber), "") == 0)
            continue;

        /* Load textures into the OpenGL texture unit. */
        csmString texturePath = _modelSetting->GetTextureFileName(modelTextureNumber);
        texturePath = _modelHomeDir + texturePath;

        TextureManager::TextureInfo* texture = CoreManager::GetInstance()->GetTextureManager()->CreateTextureFromPngFile(texturePath.GetRawString());
        if(texture != NULL) {
            const csmInt32 glTextueNumber = texture->id;
            /* OpenGL */
            GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->BindTexture(modelTextureNumber, glTextueNumber);
        }
    }

#ifdef PREMULTIPLIED_ALPHA_ENABLE
    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->IsPremultipliedAlpha(true);
#else
    GetRenderer<Rendering::CubismRenderer_OpenGLES2>()->IsPremultipliedAlpha(false);
#endif

}

void Model::MotionEventFired(const csmString& eventValue) {
    CubismLogInfo("%s is fired on Model!!", eventValue.GetRawString());
}

Csm::Rendering::CubismOffscreenSurface_OpenGLES2& Model::GetRenderBuffer() {
    return _renderBuffer;
}

#pragma once

#include "Urho3D/Engine/Application.h"
#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/AnimationController.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/UI/Text.h"

using namespace Urho3D;

/// 播放 Urho3D .ani 骨骼动画的安卓查看器。
/// 加载一个带皮网格模型(.mdl) + 骨骼动画(.ani)，循环播放；
/// 支持单指拖拽旋转视角，可一键切换骨骼线框可视化。
class UrhoApp : public Application
{
public:
    UrhoApp(Context* context);
    virtual void Setup();
    virtual void Start();
    virtual void Stop();

private:
    void CreateScene();
    void CreateCamera();
    void CreateViewport();
    void CreateUI();

    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);
    void HandleBoneToggle(StringHash eventType, VariantMap& eventData);

    void RotateFromInput();

    SharedPtr<Scene> scene_;
    SharedPtr<Node> cameraNode_;
    SharedPtr<Node> characterNode_;
    SharedPtr<AnimationController> animCtrl_;
    SharedPtr<AnimatedModel> animModel_;
    SharedPtr<Text> boneLabel_;

    bool drawBones_;
    float yaw_;
    float pitch_;
    float distance_;
    Vector3 lookCenter_;
};
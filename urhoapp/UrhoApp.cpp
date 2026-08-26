#include "UrhoApp.h"

#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Engine/Engine.h"
#include "Urho3D/Engine/DebugHud.h"
#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/AnimationController.h"
#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Graphics/Octree.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/Graphics/Zone.h"
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Input/Input.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/UI/Button.h"
#include "Urho3D/UI/Font.h"
#include "Urho3D/UI/Text.h"
#include "Urho3D/UI/UI.h"
#include "Urho3D/UI/UIEvents.h"

URHO3D_DEFINE_APPLICATION_MAIN(UrhoApp)

UrhoApp::UrhoApp(Context* context) :
    Application(context),
    drawBones_(false),
    yaw_(45.0f),
    pitch_(-15.0f),
    distance_(6.0f),
    lookCenter_(0.0f, 1.2f, 0.0f)
{
}

void UrhoApp::Setup()
{
    engineParameters_["WindowTitle"] = "Urho3D .ani 播放器";
    engineParameters_["LogName"] = GetSubsystem<FileSystem>()->GetProgramDir() + "UrhoApp.log";
    engineParameters_["Headless"] = false;
    // 允许触摸输入（移动端默认开启），资源路径交给引擎默认处理
}

void UrhoApp::Start()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    // 需要的核心数据文件：从引擎 bin/CoreData 打包进 assets
    cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");   // 触发热加载，验证 CoreData 可用

    CreateScene();
    CreateCamera();
    CreateViewport();
    CreateUI();

    // 订阅更新与渲染后事件，用于旋转视角、绘制骨骼
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(UrhoApp, HandleUpdate));
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(UrhoApp, HandlePostRenderUpdate));
}

void UrhoApp::Stop()
{
}

void UrhoApp::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    // 环境光 + 主方向光
    Zone* zone = scene_->CreateComponent<Zone>();
    zone->SetAmbientColor(Color(0.35f, 0.42f, 0.55f));
    zone->SetFogColor(Color(0.14f, 0.16f, 0.20f));
    zone->SetFogStart(60.0f);
    zone->SetFogEnd(120.0f);

    Node* lightNode = scene_->CreateChild("Light");
    lightNode->SetDirection(Vector3(0.4f, -1.0f, 0.6f));
    Light* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor(Color(0.95f, 0.9f, 0.8f));

    // 角色：带皮骨骼模型 + 循环动画
    Model* model = cache->GetResource<Model>("Models/character.mdl");
    if (!model)
    {
        URHO3D_LOGERROR("无法加载 Models/character.mdl，请确认资源已打包");
        return;
    }

    characterNode_ = scene_->CreateChild("Character");
    characterNode_->SetScale(1.0f);
    animModel_ = characterNode_->CreateComponent<AnimatedModel>();
    animModel_->SetModel(model);

    animCtrl_ = characterNode_->CreateComponent<AnimationController>();
    // 层0，循环播放
    if (!animCtrl_->Play("Animations/walk.ani", 0, true, 0.0f))
        URHO3D_LOGERROR("无法播放 Animations/walk.ani");
}

void UrhoApp::CreateCamera()
{
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);
    camera->SetNearClip(0.1f);
}

void UrhoApp::CreateViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void UrhoApp::CreateUI()
{
    UI* ui = GetSubsystem<UI>();
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Font* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

    // 底部左侧：骨骼线框开关
    Button* boneBtn = new Button(context_);
    boneBtn->SetName("BoneToggle");
    boneBtn->SetMinSize(IntVector2(150, 44));
    boneBtn->SetPosition(12, ui->GetRoot()->GetHeight() - 60);
    boneBtn->SetStyleAuto();
    ui->GetRoot()->AddChild(boneBtn);
    SubscribeToEvent(boneBtn, E_RELEASED, URHO3D_HANDLER(UrhoApp, HandleBoneToggle));

    boneLabel_ = new Text(context_);
    boneLabel_->SetText("骨骼：关闭");
    boneLabel_->SetFont(font, 15);
    boneLabel_->SetColor(Color(0.95f, 0.95f, 0.95f));
    boneBtn->AddChild(boneLabel_);
    boneLabel_->SetAlignment(HA_CENTER, VA_CENTER);

    // 顶部提示
    Text* hint = new Text(context_);
    hint->SetText("拖动旋转视角   ·   点下方按钮看骨骼");
    hint->SetFont(font, 14);
    hint->SetColor(Color(0.85f, 0.85f, 0.85f, 0.9f));
    hint->SetPosition(12, 12);
    ui->GetRoot()->AddChild(hint);
}

void UrhoApp::HandleBoneToggle(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    if (!animModel_)
        return;
    drawBones_ = !drawBones_;
    if (boneLabel_)
        boneLabel_->SetText(drawBones_ ? "骨骼：开启" : "骨骼：关闭");

    // 开启骨骼可视化时，抬起头/根等易造成误导的换绑前两端点照常，仅控制绘制
    if (drawBones_)
    {
        // 让模型本身半透明便于看骨骼
        // （不修改材质颜色，仅切换 DebugRenderer 绘制）
    }
}

void UrhoApp::HandleUpdate(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace Update;
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    RotateFromInput();

    if (cameraNode_)
    {
        Quaternion orbit = Quaternion(pitch_, Vector3::RIGHT) * Quaternion(yaw_, Vector3::UP);
        Vector3 offset = orbit * Vector3(0.0f, 0.0f, distance_);
        cameraNode_->SetPosition(lookCenter_ + offset);
        cameraNode_->LookAt(lookCenter_);
    }
}

void UrhoApp::HandlePostRenderUpdate(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    if (drawBones_ && animModel_)
    {
        DebugRenderer* debug = scene_->GetComponent<DebugRenderer>();
        if (debug)
            // 橙色骨骼线框可视化
            debug->AddSkeleton(animModel_->GetSkeleton(), Color(1.0f, 0.6f, 0.1f));
    }
}

void UrhoApp::RotateFromInput()
{
    Input* input = GetSubsystem<Input>();

    // 移动端：单指拖动
    if (input->GetNumTouches() == 1)
    {
        TouchState* ts = input->GetTouch(0);
        yaw_ += ts->delta_.x_ * 0.35f;
        pitch_ += ts->delta_.y_ * 0.25f;
    }
    // 桌面端：鼠标拖动（便于本地验证）
    if (input->GetMouseButtonDown(MOUSEB_LEFT))
    {
        yaw_ += input->GetMouseMoveX() * 0.25f;
        pitch_ += input->GetMouseMoveY() * 0.2f;
    }

    pitch_ = Clamp(pitch_, -80.0f, 20.0f);
}
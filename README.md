# Urho3D .ani 安卓播放器

在手机上播放 Urho3D 引擎的 `.ani` 骨骼动画的 App。内置"走路"角色模型（`character.mdl`）+ 动画（`walk.ani`），打开即可循环播放，并支持**单指拖拽旋转视角 + 一键切换骨骼线框可视化**。

## 目录

| 路径 | 说明 |
|------|------|
| `urhoapp/UrhoApp.cpp|h` | 播放器 C++ 源码（加载 .mdl+.ani、循环播放、骨骼切换） |
| `bin/Data/` | 模型与动画资源（Models/Animations/Materials/Textures） |
| `.github/workflows/build-apk.yml` | 云端交叉编译出 APK 并发布到 Release |

## 构建

APK 由 GitHub Actions 在云端自动构建，编好后自动发布到本仓库的 **Releases** 页面，手机直接下载安装。

手动触发：`Actions` → `构建安卓播放器 APK` → `Run workflow`。

### 用法（App 内）
- 打开即看走路动画，循环播放
- 单指拖动：旋转视角
- 屏幕左下角按钮：切换 骨骼线框 显示
# Vulkan Real-Time PBR Renderer

[English](#english) · [中文](#中文)

---

## English

### Overview

A Windows / Vulkan learning demo that renders a physically based scene with deferred and forward paths, image-based lighting, shadows, and common post-processing. Shaders are written in GLSL and compiled offline with `glslc`.

### Demo Video

File: **`Demo/Vulkan PBR.mp4`** (~180 MB).

**Watch locally:** open the file with any player.

**Upload the video to GitHub (no external link):** Git blocks files **> 100 MB** in normal commits. Use **Git LFS** so the MP4 lives in the repo and pushes to GitHub. This repo already includes **`.gitattributes`** so `Demo/*.mp4` is tracked by LFS.

1. Install [Git LFS](https://git-lfs.com/) (Windows installer is fine).  
2. In a terminal, at your repository root (the folder that contains `.git`):

```bash
git lfs install
git add .gitattributes
git add "Demo/Vulkan PBR.mp4"
git commit -m "Add demo video (Git LFS)"
git push
```

3. Anyone who clones the repo needs Git LFS installed; otherwise they only get a small pointer file. After `git lfs install`, `git clone` / `git pull` will download the real video.

**Notes**

- [GitHub LFS](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-git-large-file-storage): free tier includes **1 GiB** storage and **1 GiB/month** bandwidth (shared); one ~180 MB video is usually fine for occasional pushes.  
- If you **already committed** the MP4 without LFS and push failed, remove it from the last commit or use `git lfs migrate`—ask for help if needed.  
- **GitHub Releases** is another way to attach a large video **without** LFS: create a Release and upload the MP4 there; it still lives on GitHub, just not under the default branch tree.

**Optional in README:** embed (may not play in the browser for very large files):

```html
<video src="Demo/Vulkan%20PBR.mp4" controls width="720"></video>
```

### Features

- Vulkan rendering pipeline, offscreen HDR, tone mapping to LDR
- Cook–Torrance PBR and IBL (irradiance, prefiltered specular, BRDF LUT)
- Directional light with shadow map (orthographic projection, PCF)
- Deferred rendering (G-Buffer MRT) with forward path toggle
- SSAO, bloom, FXAA
- Multiple point lights; FPS-style camera (WASD, mouse look, click to capture cursor, Esc to release)
- Runtime toggles (see Controls); FPS shown in the window title

### Requirements

- Windows 10 or later  
- Visual Studio 2019+ with **Desktop development with C++** and the MSVC toolset used by the solution  
- Vulkan SDK ([LunarG](https://vulkan.lunarg.com/)) — ensure `glslc` is on `PATH` for shader compilation  
- A GPU with Vulkan support

### Build & Run

1. Open `Codes.sln` in Visual Studio.  
2. Select configuration (e.g. **Release** | **x64**) and build.  
3. Run the generated executable; working directory should be the folder that contains the `Res` assets (typically the project output directory is configured to match — adjust if your paths differ).

Compile shaders when you change `.vs` / `.fs` sources, for example:

```bash
glslc -fshader-stage=fragment Res/YourShader.fs -o Res/YourShader.fsb
```

### Controls

| Key | Action |
|-----|--------|
| `W` `A` `S` `D` | Move |
| Space / Shift | Up / down |
| Mouse | Look (after click to lock cursor) |
| Esc | Release mouse capture |
| `1` | Toggle SSAO |
| `2` | Toggle bloom |
| `3` | Toggle deferred / forward |
| `4` | Toggle point lights |
| `5` | Toggle FXAA |

### Repository Layout (short)

- `Res/` — shaders (`.vs`, `.fs`, compiled `.vsb` / `.fsb`), textures, models  
- Core sources: `Scene.*`, `MyVulkan.*`, `Material.*`, `FrameBuffer.*`, `Camera.*`, `Node.*`, `main.cpp`  
- `Demo/` — demo video (e.g. `Vulkan PBR.mp4`) and other media

### License

Specify your license here (e.g. MIT) or “All rights reserved” if not open-sourcing.

---

## 中文

### 简介

基于 **Vulkan** 的实时 **PBR** 演示程序（Windows），支持延迟 / 前向切换、IBL、阴影与多种后处理；着色器为 GLSL，使用 `glslc` 离线编译。

### 演示视频

文件：**`Demo/Vulkan PBR.mp4`**（约 180 MB）。

**本机：** 用播放器直接打开即可。

**想上传到 GitHub、又不想用 B 站等外链：** 普通 Git **单文件不能超过 100 MB**，所以大视频要用 **Git LFS**。本仓库已包含 **`.gitattributes`**，会把 `Demo/*.mp4` 交给 LFS 管理。

在**仓库根目录**（包含 `.git` 的文件夹）打开终端，执行：

```bash
git lfs install
git add .gitattributes
git add "Demo/Vulkan PBR.mp4"
git commit -m "Add demo video (Git LFS)"
git push
```

克隆仓库的人也需要**安装 Git LFS**，否则只会看到小指针文件；装好后再 `git clone` / `git pull` 会拉取真实视频。

**说明**

- [GitHub 对 LFS 的说明](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-git-large-file-storage)：免费档有 **1 GiB 存储**和**每月 1 GiB 流量**（共用），单个 ~180 MB 视频一般够用。  
- 若你**已经用普通方式提交过** MP4 导致 push 失败，需要先删掉错误提交或改用 `git lfs migrate`，需要时再查文档。  
- **GitHub Releases**：也可以把视频作为 Release 附件上传，仍算「在 GitHub 上」，只是不在仓库主分支目录里。

**可选：** 在 README 里用 HTML 内嵌（网页上可能无法播放超大 MP4）：

```html
<video src="Demo/Vulkan%20PBR.mp4" controls width="720"></video>
```

### 功能概览

- Vulkan 多 Pass、离屏 HDR、色调映射到 LDR  
- Cook–Torrance PBR 与 IBL（漫反射辐照、预过滤高光、BRDF LUT）  
- 方向光 Shadow Map（正交投影、PCF）  
- 延迟渲染 G-Buffer（MRT），可与前向切换  
- SSAO、Bloom、FXAA  
- 多点光源；FPS 式操作（WASD、鼠标视角、左键锁定指针、Esc 解锁）  
- 运行时开关见下表；帧率显示在窗口标题栏  

### 环境要求

- Windows 10 及以上  
- Visual Studio 2019+，安装「使用 C++ 的桌面开发」及解决方案对应 MSVC 工具集  
- 安装 [Vulkan SDK](https://vulkan.lunarg.com/)，并将 `glslc` 加入 `PATH` 以便编译着色器  
- 支持 Vulkan 的显卡  

### 编译与运行

1. 用 Visual Studio 打开 `Codes.sln`。  
2. 选择配置（建议 **Release** | **x64**）并生成。  
3. 运行生成的可执行文件；确保工作目录能访问 `Res` 资源（若输出目录与工程不一致，请自行拷贝资源或改工作目录）。

修改 `.vs` / `.fs` 后需重新编译，例如：

```bash
glslc -fshader-stage=fragment Res/YourShader.fs -o Res/YourShader.fsb
```

### 操作说明

| 按键 | 作用 |
|------|------|
| `W` `A` `S` `D` | 移动 |
| 空格 / Shift | 上 / 下 |
| 鼠标 | 视角（点击后锁定指针） |
| Esc | 解除指针锁定 |
| `1` | 开关 SSAO |
| `2` | 开关 Bloom |
| `3` | 延迟 / 前向 |
| `4` | 点光源 |
| `5` | FXAA |

### 目录说明（节选）

- `Res/`：着色器、贴图、模型等  
- 主要代码：`Scene.*`、`MyVulkan.*`、`Material.*`、`FrameBuffer.*`、`Camera.*`、`Node.*`、`main.cpp`  
- `Demo/`：演示视频（如 `Vulkan PBR.mp4`）等素材  

### 许可证

在此填写你的许可证（如 MIT）或保留所有权利。

---

*Large demo videos: use Git LFS (see `.gitattributes` and Demo Video section).*

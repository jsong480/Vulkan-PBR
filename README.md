# Vulkan Real-Time PBR Renderer

[English](#english) · [中文](#中文)

---

## English

### Overview

A Windows / Vulkan learning demo that renders a physically based scene with deferred and forward paths, image-based lighting, shadows, and common post-processing. Shaders are written in GLSL and compiled offline with `glslc`.

### Demo Video

Place your screen recording under the project folder so it is versioned with the repo (or upload it elsewhere and link it).

| Location | Suggested path |
|----------|------------------|
| **In this repository** | `Demo/demo.mp4` (create the `Demo` folder if needed) |

**Viewing the video**

- **Locally:** open `Demo/demo.mp4` with any player.
- **On GitHub:** large binaries may use Git LFS; alternatively upload the video to [GitHub Releases](https://docs.github.com/en/repositories/releasing-projects-on-github) or host on YouTube / Bilibili and replace the link below.

You can add a clickable link in this README after you upload:

```markdown
**[Watch demo (YouTube / Bilibili)]()** — replace `()` with your URL.
```

Optional embed (works in many Markdown viewers when the file is hosted or raw-linked):

```html
<!-- Example: replace with your public video URL if not using a local path -->
<video src="Demo/demo.mp4" controls width="720"></video>
```

> **Note:** GitHub’s web UI may not inline-play very large MP4s from the repo; a short clip or an external link is often more reliable.

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
- `Demo/` — optional demo video (`demo.mp4`) and other media

### License

Specify your license here (e.g. MIT) or “All rights reserved” if not open-sourcing.

---

## 中文

### 简介

基于 **Vulkan** 的实时 **PBR** 演示程序（Windows），支持延迟 / 前向切换、IBL、阴影与多种后处理；着色器为 GLSL，使用 `glslc` 离线编译。

### 演示视频

把录屏放在仓库里（或传到网上再在下面替换链接）。

| 说明 | 建议路径 |
|------|----------|
| **放在本仓库中** | `Demo/demo.mp4`（可新建 `Demo` 文件夹） |

**观看方式**

- **本机：** 直接用播放器打开 `Demo/demo.mp4`。  
- **放到 GitHub：** 视频较大时可用 [Git LFS](https://git-lfs.com/)，或上传到 [GitHub Releases](https://docs.github.com/en/repositories/releasing-projects-on-github)、**YouTube / Bilibili**，再把下面 README 里的链接改成你的地址。

在下方自行填写公开链接：

```markdown
**[观看演示（哔哩哔哩 / YouTube）]()** — 把 `()` 里换成你的视频 URL。
```

可选 HTML（部分环境可内嵌播放，大文件在网页上可能仍建议外链）：

```html
<video src="Demo/demo.mp4" controls width="720"></video>
```

> **说明：** 在 GitHub 网页上直接内嵌仓库里很大的 MP4 有时无法播放；短视频或外链通常更稳妥。

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
- `Demo/`：可选，用于存放 `demo.mp4` 等演示素材  

### 许可证

在此填写你的许可证（如 MIT）或保留所有权利。

---

*README structure: English first, then Chinese; demo video section explains `Demo/demo.mp4` and hosting options.*

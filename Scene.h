// 对外接口：视口变化、场景初始化、每帧渲染与功能开关
#pragma once
void OnViewportChanged(int inWidth, int inHeight);
void InitScene();
void RenderOneFrame();
void ToggleSSAO();
void ToggleBloom();
bool IsSSAOEnabled();
bool IsBloomEnabled();
void ToggleDeferred();
bool IsDeferredEnabled();
void TogglePointLights();
bool IsPointLightsEnabled();
void ToggleFXAA();
bool IsFXAAEnabled();
void SetMouseDelta(float dx, float dy);
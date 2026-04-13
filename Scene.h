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
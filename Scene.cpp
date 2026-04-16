#include "Scene.h"
#include "MyVulkan.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "Node.h"
#include "Mesh.h"
#include "Captures.h"
#include <cstdlib>

// 场景与渲染主流程：离屏 Pass、延迟/前向、后处理与最终 Present

static glm::mat4 gProjectionMatrix;
static Camera gMainCamera;
static int gViewportWidth = 1280;
static int gViewportHeight = 720;

static FrameBufferEx* gHDRFBO = nullptr;
static Texture* gSkyBoxCubeMap = nullptr;
static Texture* gDiffuseIrradianceCubeMap = nullptr;
static Texture* gPrefilteredColorCubeMap = nullptr;
static Texture* gBRDFLUTTexture = nullptr;

static Node* gSkyBoxNode = nullptr;
static Node* gSphereNode = nullptr;
static Node* gHelmetNode = nullptr;
static Node* gGunNode = nullptr;
static Node* gGroundNode = nullptr;
static Node* gToneMappingNode = nullptr;
static FrameBufferEx* gLDRFBO = nullptr;
static Node* gFXAANode = nullptr;
static bool gFXAAEnabled = true;

static const uint32_t SHADOW_MAP_SIZE = 2048;
static FrameBufferEx* gShadowFBO = nullptr;
static Node* gShadowHelmetNode = nullptr;
static Node* gShadowGunNode = nullptr;
static Node* gShadowGroundNode = nullptr;
static Camera gLightCamera;
static glm::mat4 gLightProjection;
static glm::mat4 gLightVP;

static FrameBufferEx* gBloomBrightFBO = nullptr;
static FrameBufferEx* gBloomHBlurFBO = nullptr;
static FrameBufferEx* gBloomVBlurFBO = nullptr;
static Node* gBrightnessNode = nullptr;
static Node* gHBlurNode = nullptr;
static Node* gVBlurNode = nullptr;

static FrameBufferEx* gSSAOFBO = nullptr;
static FrameBufferEx* gSSAOBlurFBO = nullptr;
static Node* gSSAONode = nullptr;
static Node* gSSAOBlurNode = nullptr;
static Texture* gSSAONoiseTex = nullptr;

static bool gSSAOEnabled = true;
static bool gBloomEnabled = true;
static bool gDeferredEnabled = true;

static FrameBufferEx* gGBufferFBO = nullptr;
static Node* gGBufferHelmetNode = nullptr;
static Node* gGBufferGunNode = nullptr;
static Node* gGBufferGroundNode = nullptr;
static Node* gDeferredLightingNode = nullptr;
static VkSampler gDepthSampler = nullptr;

void ToggleSSAO() { gSSAOEnabled = !gSSAOEnabled; }
void ToggleFXAA() {
	gFXAAEnabled = !gFXAAEnabled;
	if (gFXAANode) gFXAANode->mMaterial->SetVec4(1, gFXAAEnabled ? 1.0f : 0.0f, 1.0f / float(gViewportWidth), 1.0f / float(gViewportHeight), 0.0f);
}
bool IsFXAAEnabled() { return gFXAAEnabled; }
void ToggleBloom() { gBloomEnabled = !gBloomEnabled; }
bool IsSSAOEnabled() { return gSSAOEnabled; }
bool IsBloomEnabled() { return gBloomEnabled; }
void ToggleDeferred() {
	gDeferredEnabled = !gDeferredEnabled;
	if (gSSAONode != nullptr && gDepthSampler != nullptr) {
		if (gDeferredEnabled && gGBufferFBO != nullptr) {
			gSSAONode->mMaterial->SetTexture(4, gGBufferFBO->mDepthBuffer->mImageView, gDepthSampler);
		} else if (gHDRFBO != nullptr) {
			gSSAONode->mMaterial->SetTexture(4, gHDRFBO->mDepthBuffer->mImageView, gDepthSampler);
		}
	}
}
bool IsDeferredEnabled() { return gDeferredEnabled; }

static bool gPointLightsEnabled = true;

struct PointLight {
	glm::vec3 position;
	float radius;
	glm::vec3 color;
	float intensity;
};

static const int NUM_POINT_LIGHTS = 8;
static PointLight gPointLights[NUM_POINT_LIGHTS] = {
	{ glm::vec3( 3.0f,  1.0f,  0.0f), 8.0f, glm::vec3(1.0f, 0.4f, 0.1f), 3.0f },
	{ glm::vec3(-3.0f,  1.0f,  0.0f), 8.0f, glm::vec3(0.1f, 0.4f, 1.0f), 3.0f },
	{ glm::vec3( 0.0f,  1.0f,  3.0f), 8.0f, glm::vec3(0.1f, 1.0f, 0.3f), 2.5f },
	{ glm::vec3( 0.0f,  1.0f, -3.0f), 8.0f, glm::vec3(1.0f, 0.1f, 0.3f), 2.5f },
	{ glm::vec3( 2.0f,  2.5f,  2.0f), 7.0f, glm::vec3(1.0f, 1.0f, 0.8f), 2.0f },
	{ glm::vec3(-2.0f,  2.5f, -2.0f), 7.0f, glm::vec3(0.8f, 0.6f, 1.0f), 2.0f },
	{ glm::vec3( 2.0f, -0.5f, -2.0f), 6.0f, glm::vec3(0.0f, 1.0f, 1.0f), 2.0f },
	{ glm::vec3(-2.0f, -0.5f,  2.0f), 6.0f, glm::vec3(1.0f, 1.0f, 0.0f), 2.0f },
};

static void SetPointLightsOnMaterial(Material* mat) {
	mat->SetVec4(9, float(NUM_POINT_LIGHTS), 0.0f, 0.0f, 0.0f);
	for (int i = 0; i < NUM_POINT_LIGHTS; i++) {
		mat->SetVec4(10 + i * 2,
			gPointLights[i].position.x, gPointLights[i].position.y,
			gPointLights[i].position.z, gPointLights[i].radius);
		mat->SetVec4(11 + i * 2,
			gPointLights[i].color.x, gPointLights[i].color.y,
			gPointLights[i].color.z, gPointLights[i].intensity);
	}
}

void TogglePointLights() {
	gPointLightsEnabled = !gPointLightsEnabled;
	float count = gPointLightsEnabled ? float(NUM_POINT_LIGHTS) : 0.0f;
	if (gHelmetNode) gHelmetNode->mMaterial->SetVec4(9, count, 0, 0, 0);
	if (gGunNode) gGunNode->mMaterial->SetVec4(9, count, 0, 0, 0);
	if (gGroundNode) gGroundNode->mMaterial->SetVec4(9, count, 0, 0, 0);
	if (gDeferredLightingNode) gDeferredLightingNode->mMaterial->SetVec4(9, count, 0, 0, 0);
}
bool IsPointLightsEnabled() { return gPointLightsEnabled; }

static float gMouseDx = 0.0f, gMouseDy = 0.0f;
void SetMouseDelta(float dx, float dy) { gMouseDx += dx; gMouseDy += dy; }

static void ApplyShadowViewport(Material* material) {
	if (material == nullptr || material->mPipelineStateObject == nullptr) {
		return;
	}
	material->mPipelineStateObject->mViewport = { 0.0f, float(SHADOW_MAP_SIZE), float(SHADOW_MAP_SIZE), -float(SHADOW_MAP_SIZE), 0.0f, 1.0f };
	material->mPipelineStateObject->mScissor = { {0, 0}, {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE} };
}

static void SetLightVPOnMaterial(Material* material, const glm::mat4& lightVP) {
	const float* data = glm::value_ptr(lightVP);
	material->SetVec4(1, (float*)data);
	material->SetVec4(2, (float*)(data + 4));
	material->SetVec4(3, (float*)(data + 8));
	material->SetVec4(4, (float*)(data + 12));
}

static void ApplyViewportToMaterial(Material* material, bool flipY) {
	if (material == nullptr || material->mPipelineStateObject == nullptr) {
		return;
	}
	float y = flipY ? float(gViewportHeight) : 0.0f;
	float h = flipY ? -float(gViewportHeight) : float(gViewportHeight);
	material->mPipelineStateObject->mViewport = { 0.0f, y, float(gViewportWidth), h, 0.0f, 1.0f };
	material->mPipelineStateObject->mScissor = { {0, 0}, {uint32_t(gViewportWidth), uint32_t(gViewportHeight)} };
}

static void ApplyBloomViewport(Material* material, uint32_t w, uint32_t h) {
	if (material == nullptr || material->mPipelineStateObject == nullptr) return;
	material->mPipelineStateObject->mViewport = { 0.0f, 0.0f, float(w), float(h), 0.0f, 1.0f };
	material->mPipelineStateObject->mScissor = { {0, 0}, {w, h} };
}

static void UpdateSSAOUniforms() {
	if (gSSAONode == nullptr) return;
	Material* mat = gSSAONode->mMaterial;
	mat->SetVec4(1, 0.1f, 1000.0f, float(gViewportWidth), float(gViewportHeight));
	const float* projData = glm::value_ptr(gProjectionMatrix);
	mat->SetVec4(2, (float*)projData);
	mat->SetVec4(3, (float*)(projData + 4));
	mat->SetVec4(4, (float*)(projData + 8));
	mat->SetVec4(5, (float*)(projData + 12));
	glm::mat4 invProj = glm::inverse(gProjectionMatrix);
	const float* invData = glm::value_ptr(invProj);
	mat->SetVec4(6, (float*)invData);
	mat->SetVec4(7, (float*)(invData + 4));
	mat->SetVec4(8, (float*)(invData + 8));
	mat->SetVec4(9, (float*)(invData + 12));
}

static void RecreateSSAOFBOs() {
	if (gSSAOFBO != nullptr) { delete gSSAOFBO; gSSAOFBO = nullptr; }
	gSSAOFBO = new FrameBufferEx;
	gSSAOFBO->SetSize(uint32_t(gViewportWidth), uint32_t(gViewportHeight));
	gSSAOFBO->AttachColorBuffer(VK_FORMAT_R8G8B8A8_UNORM);
	gSSAOFBO->AttachDepthBuffer();
	gSSAOFBO->Finish();
	gSSAOFBO->SetClearColor(0, 1.0f, 1.0f, 1.0f, 1.0f);

	if (gSSAOBlurFBO != nullptr) { delete gSSAOBlurFBO; gSSAOBlurFBO = nullptr; }
	gSSAOBlurFBO = new FrameBufferEx;
	gSSAOBlurFBO->SetSize(uint32_t(gViewportWidth), uint32_t(gViewportHeight));
	gSSAOBlurFBO->AttachColorBuffer(VK_FORMAT_R8G8B8A8_UNORM);
	gSSAOBlurFBO->AttachDepthBuffer();
	gSSAOBlurFBO->Finish();
	gSSAOBlurFBO->SetClearColor(0, 1.0f, 1.0f, 1.0f, 1.0f);

	if (gSSAONode != nullptr) {
		gSSAONode->mMaterial->mPipelineStateObject->mRenderPass = gSSAOFBO->mRenderPass;
		if (gDeferredEnabled && gGBufferFBO != nullptr) {
			gSSAONode->mMaterial->SetTexture(4, gGBufferFBO->mDepthBuffer->mImageView);
		} else {
			gSSAONode->mMaterial->SetTexture(4, gHDRFBO->mDepthBuffer->mImageView);
		}
		ApplyViewportToMaterial(gSSAONode->mMaterial, false);
		UpdateSSAOUniforms();
	}
	if (gSSAOBlurNode != nullptr) {
		gSSAOBlurNode->mMaterial->mPipelineStateObject->mRenderPass = gSSAOBlurFBO->mRenderPass;
		gSSAOBlurNode->mMaterial->SetTexture(4, gSSAOFBO->mAttachments[0]->mImageView);
		ApplyViewportToMaterial(gSSAOBlurNode->mMaterial, false);
	}
	if (gToneMappingNode != nullptr) {
		gToneMappingNode->mMaterial->SetTexture(6, gSSAOBlurFBO->mAttachments[0]->mImageView);
	}
}

static void RecreateGBufferFBO() {
	if (gGBufferFBO != nullptr) { delete gGBufferFBO; gGBufferFBO = nullptr; }
	gGBufferFBO = new FrameBufferEx;
	gGBufferFBO->SetSize(uint32_t(gViewportWidth), uint32_t(gViewportHeight));
	gGBufferFBO->AttachColorBuffer(VK_FORMAT_R16G16B16A16_SFLOAT);
	gGBufferFBO->AttachColorBuffer(VK_FORMAT_R16G16B16A16_SFLOAT);
	gGBufferFBO->AttachColorBuffer(VK_FORMAT_R8G8B8A8_UNORM);
	gGBufferFBO->AttachColorBuffer(VK_FORMAT_R8G8B8A8_UNORM);
	gGBufferFBO->AttachDepthBuffer();
	gGBufferFBO->Finish();
	if (gGBufferHelmetNode != nullptr) {
		gGBufferHelmetNode->mMaterial->mPipelineStateObject->mRenderPass = gGBufferFBO->mRenderPass;
		ApplyViewportToMaterial(gGBufferHelmetNode->mMaterial, true);
	}
	if (gGBufferGunNode != nullptr) {
		gGBufferGunNode->mMaterial->mPipelineStateObject->mRenderPass = gGBufferFBO->mRenderPass;
		ApplyViewportToMaterial(gGBufferGunNode->mMaterial, true);
	}
	if (gGBufferGroundNode != nullptr) {
		gGBufferGroundNode->mMaterial->mPipelineStateObject->mRenderPass = gGBufferFBO->mRenderPass;
		ApplyViewportToMaterial(gGBufferGroundNode->mMaterial, true);
	}
	if (gDeferredLightingNode != nullptr) {
		gDeferredLightingNode->mMaterial->SetTexture(4, gGBufferFBO->mAttachments[0]->mImageView);
		gDeferredLightingNode->mMaterial->SetTexture(5, gGBufferFBO->mAttachments[1]->mImageView);
		gDeferredLightingNode->mMaterial->SetTexture(6, gGBufferFBO->mAttachments[2]->mImageView);
		gDeferredLightingNode->mMaterial->SetTexture(7, gGBufferFBO->mAttachments[3]->mImageView);
		gDeferredLightingNode->mMaterial->SetTexture(8, gGBufferFBO->mDepthBuffer->mImageView);
		gDeferredLightingNode->mMaterial->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
		ApplyViewportToMaterial(gDeferredLightingNode->mMaterial, false);
	}
}

static void RecreateBloomFBOs() {
	uint32_t bloomW = uint32_t(gViewportWidth) / 2;
	uint32_t bloomH = uint32_t(gViewportHeight) / 2;
	if (bloomW == 0) bloomW = 1;
	if (bloomH == 0) bloomH = 1;

	if (gBloomBrightFBO != nullptr) { delete gBloomBrightFBO; gBloomBrightFBO = nullptr; }
	gBloomBrightFBO = new FrameBufferEx;
	gBloomBrightFBO->SetSize(bloomW, bloomH);
	gBloomBrightFBO->AttachColorBuffer(VK_FORMAT_R16G16B16A16_SFLOAT);
	gBloomBrightFBO->AttachDepthBuffer();
	gBloomBrightFBO->Finish();

	if (gBloomHBlurFBO != nullptr) { delete gBloomHBlurFBO; gBloomHBlurFBO = nullptr; }
	gBloomHBlurFBO = new FrameBufferEx;
	gBloomHBlurFBO->SetSize(bloomW, bloomH);
	gBloomHBlurFBO->AttachColorBuffer(VK_FORMAT_R16G16B16A16_SFLOAT);
	gBloomHBlurFBO->AttachDepthBuffer();
	gBloomHBlurFBO->Finish();

	if (gBloomVBlurFBO != nullptr) { delete gBloomVBlurFBO; gBloomVBlurFBO = nullptr; }
	gBloomVBlurFBO = new FrameBufferEx;
	gBloomVBlurFBO->SetSize(bloomW, bloomH);
	gBloomVBlurFBO->AttachColorBuffer(VK_FORMAT_R16G16B16A16_SFLOAT);
	gBloomVBlurFBO->AttachDepthBuffer();
	gBloomVBlurFBO->Finish();

	if (gBrightnessNode != nullptr) {
		gBrightnessNode->mMaterial->mPipelineStateObject->mRenderPass = gBloomBrightFBO->mRenderPass;
		gBrightnessNode->mMaterial->SetTexture(4, gHDRFBO->mAttachments[0]->mImageView);
		gBrightnessNode->mMaterial->SetVec4(1, 1.0f / float(gViewportWidth), 1.0f / float(gViewportHeight), 0.0f, 0.0f);
		ApplyBloomViewport(gBrightnessNode->mMaterial, bloomW, bloomH);
	}
	if (gHBlurNode != nullptr) {
		gHBlurNode->mMaterial->mPipelineStateObject->mRenderPass = gBloomHBlurFBO->mRenderPass;
		gHBlurNode->mMaterial->SetTexture(4, gBloomBrightFBO->mAttachments[0]->mImageView);
		gHBlurNode->mMaterial->SetVec4(1, 0.5f / float(bloomW), 0.0f, 0.0f, 0.0f);
		ApplyBloomViewport(gHBlurNode->mMaterial, bloomW, bloomH);
	}
	if (gVBlurNode != nullptr) {
		gVBlurNode->mMaterial->mPipelineStateObject->mRenderPass = gBloomVBlurFBO->mRenderPass;
		gVBlurNode->mMaterial->SetTexture(4, gBloomHBlurFBO->mAttachments[0]->mImageView);
		gVBlurNode->mMaterial->SetVec4(1, 0.0f, 0.5f / float(bloomH), 0.0f, 0.0f);
		ApplyBloomViewport(gVBlurNode->mMaterial, bloomW, bloomH);
	}
	if (gToneMappingNode != nullptr) {
		gToneMappingNode->mMaterial->SetTexture(5, gBloomVBlurFBO->mAttachments[0]->mImageView);
	}
}

static void RecreateHDRFBO() {
	// HDR 主颜色缓冲（线性高动态）；LDR 供 tonemap 与 FXAA 采样
	if (gHDRFBO != nullptr) {
		delete gHDRFBO;
		gHDRFBO = nullptr;
	}
	gHDRFBO = new FrameBufferEx;
	gHDRFBO->SetSize(uint32_t(gViewportWidth), uint32_t(gViewportHeight));
	gHDRFBO->AttachColorBuffer(VK_FORMAT_R32G32B32A32_SFLOAT);
	gHDRFBO->AttachDepthBuffer();
	gHDRFBO->Finish();

	if (gSkyBoxNode != nullptr) {
		gSkyBoxNode->mMaterial->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	}
	if (gSphereNode != nullptr) {
		gSphereNode->mMaterial->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	}
	if (gHelmetNode != nullptr) {
		gHelmetNode->mMaterial->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	}
	if (gGunNode != nullptr) {
		gGunNode->mMaterial->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	}
	if (gGroundNode != nullptr) {
		gGroundNode->mMaterial->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	}
	if (gLDRFBO != nullptr) { delete gLDRFBO; gLDRFBO = nullptr; }
	gLDRFBO = new FrameBufferEx;
	gLDRFBO->SetSize(uint32_t(gViewportWidth), uint32_t(gViewportHeight));
	gLDRFBO->AttachColorBuffer(VK_FORMAT_R8G8B8A8_UNORM);
	gLDRFBO->AttachDepthBuffer();
	gLDRFBO->Finish();

	if (gToneMappingNode != nullptr) {
		gToneMappingNode->mMaterial->mPipelineStateObject->mRenderPass = gLDRFBO->mRenderPass;
		gToneMappingNode->mMaterial->SetTexture(4, gHDRFBO->mAttachments[0]->mImageView);
	}
	if (gFXAANode != nullptr) {
		gFXAANode->mMaterial->mPipelineStateObject->mRenderPass = GetGlobalConfig().mSystemRenderPass;
		gFXAANode->mMaterial->SetTexture(4, gLDRFBO->mAttachments[0]->mImageView);
		gFXAANode->mMaterial->SetVec4(1, gFXAAEnabled ? 1.0f : 0.0f, 1.0f / float(gViewportWidth), 1.0f / float(gViewportHeight), 0.0f);
		ApplyViewportToMaterial(gFXAANode->mMaterial, false);
	}

	RecreateGBufferFBO();
	RecreateBloomFBOs();
	RecreateSSAOFBOs();
}

// 窗口尺寸变化：更新投影矩阵、重建 Swapchain 与所有依赖分辨率的离屏 FBO
void OnViewportChanged(int inWidth, int inHeight) {
	if (inWidth <= 0 || inHeight <= 0) {
		return;
	}
	gViewportWidth = inWidth;
	gViewportHeight = inHeight;
	gProjectionMatrix = glm::perspectiveRH_ZO((45.0f * 3.14f / 180.0f), float(inWidth) / float(inHeight), 0.1f, 1000.0f);
	OnViewportChangedVulkan(inWidth, inHeight);
	RecreateHDRFBO();
	if (gSkyBoxNode != nullptr) {
		ApplyViewportToMaterial(gSkyBoxNode->mMaterial, true);
	}
	if (gSphereNode != nullptr) {
		ApplyViewportToMaterial(gSphereNode->mMaterial, true);
	}
	if (gHelmetNode != nullptr) {
		ApplyViewportToMaterial(gHelmetNode->mMaterial, true);
	}
	if (gGunNode != nullptr) {
		ApplyViewportToMaterial(gGunNode->mMaterial, true);
	}
	if (gGroundNode != nullptr) {
		ApplyViewportToMaterial(gGroundNode->mMaterial, true);
	}
	if (gToneMappingNode != nullptr) {
		ApplyViewportToMaterial(gToneMappingNode->mMaterial, false);
	}
	if (gFXAANode != nullptr) {
		ApplyViewportToMaterial(gFXAANode->mMaterial, false);
	}
	if (gSSAONode != nullptr) {
		ApplyViewportToMaterial(gSSAONode->mMaterial, false);
	}
	if (gSSAOBlurNode != nullptr) {
		ApplyViewportToMaterial(gSSAOBlurNode->mMaterial, false);
	}
	if (gGBufferHelmetNode != nullptr) {
		ApplyViewportToMaterial(gGBufferHelmetNode->mMaterial, true);
	}
	if (gGBufferGunNode != nullptr) {
		ApplyViewportToMaterial(gGBufferGunNode->mMaterial, true);
	}
	if (gGBufferGroundNode != nullptr) {
		ApplyViewportToMaterial(gGBufferGroundNode->mMaterial, true);
	}
	if (gDeferredLightingNode != nullptr) {
		ApplyViewportToMaterial(gDeferredLightingNode->mMaterial, false);
	}
}

// 加载 IBL、创建场景节点、后处理与延迟渲染所用材质与 FBO
void InitScene() {
	GlobalConfig& globalConfig = GetGlobalConfig();
	gViewportWidth = globalConfig.mViewportWidth;
	gViewportHeight = globalConfig.mViewportHeight;
	gProjectionMatrix = glm::perspectiveRH_ZO((45.0f * 3.14f / 180.0f), float(gViewportWidth) / float(gViewportHeight), 0.1f, 1000.0f);

	// 启动时 IBL 预计算：环境 cubemap、irradiance、prefilter、BRDF LUT
	gSkyBoxCubeMap = LoadHDRICubeMapFromFile("Res/Image/1.hdr", 512, "Res/SkyBox.vsb", "Res/Texture2D2CubeMap.fsb");
	gDiffuseIrradianceCubeMap = CaptureDiffuseIrradiance(gSkyBoxCubeMap, 32, "Res/SkyBox.vsb", "Res/CaptureDiffuseIrradiance.fsb");
	gPrefilteredColorCubeMap = CapturePrefilteredColor(gSkyBoxCubeMap, 128, "Res/SkyBox.vsb", "Res/CapturePrefilteredColor.fsb");
	gBRDFLUTTexture = GenerateBRDF(512, "Res/FSQ.vsb", "Res/GenerateBRDF.fsb");

	gMainCamera.InitFPS(glm::vec3(0.0f, 1.0f, -6.0f), 0.0f, -5.0f);

	// 方向光 Shadow map：正交投影 + 深度-only FBO
	glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
	glm::vec3 lightPos = lightDir * 15.0f;
	gLightCamera.mPosition = lightPos;
	gLightCamera.mViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	gLightProjection = glm::orthoRH_ZO(-8.0f, 8.0f, -8.0f, 8.0f, 1.0f, 30.0f);
	gLightVP = gLightProjection * gLightCamera.mViewMatrix;

	gShadowFBO = new FrameBufferEx;
	gShadowFBO->SetSize(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
	gShadowFBO->AttachDepthBuffer();
	gShadowFBO->Finish();

	RecreateHDRFBO();

	gSkyBoxNode = new Node;
	gSkyBoxNode->mModelMatrix = glm::mat4(1.0f);
	gSkyBoxNode->mStaticMeshComponent = new StaticMeshComponent;
	gSkyBoxNode->mStaticMeshComponent->LoadFromFile("Res/Model/skybox.staticmesh");
	gSkyBoxNode->mMaterial = new Material("Res/SkyBox.vsb", "Res/SkyBox.fsb");
	gSkyBoxNode->mMaterial->mbEnableDepthWrite = false;
	gSkyBoxNode->mMaterial->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	gSkyBoxNode->mMaterial->SetTexture(4, gSkyBoxCubeMap->mImageView, GenCubeMapSampler());
	SetColorAttachmentCount(gSkyBoxNode->mMaterial->mPipelineStateObject, 1);
	gSkyBoxNode->mMaterial->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	gSkyBoxNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gSkyBoxNode->mMaterial, true);

	gSphereNode = new Node;
	gSphereNode->mModelMatrix = glm::mat4(1.0f);
	gSphereNode->mStaticMeshComponent = new StaticMeshComponent;
	gSphereNode->mStaticMeshComponent->LoadFromFile("Res/Model/Sphere.staticmesh");
	gSphereNode->mMaterial = new Material("Res/PBR.vsb", "Res/PBR.fsb");
	gSphereNode->mMaterial->SetTexture(4, gSkyBoxCubeMap->mImageView, GenCubeMapSampler());
	gSphereNode->mMaterial->SetTexture(5, gPrefilteredColorCubeMap->mImageView, GenCubeMapSampler());
	gSphereNode->mMaterial->SetTexture(6, gDiffuseIrradianceCubeMap->mImageView, GenCubeMapSampler());
	gSphereNode->mMaterial->SetTexture(7, gBRDFLUTTexture->mImageView);
	SetColorAttachmentCount(gSphereNode->mMaterial->mPipelineStateObject, 1);
	gSphereNode->mMaterial->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	gSphereNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gSphereNode->mMaterial, true);

	Texture* albedo = LoadTextureFromFile("Res/Image/DamagedHelmet/Albedo.jpg");
	Texture* normal = LoadTextureFromFile("Res/Image/DamagedHelmet/Normal.jpg");
	Texture* metallic = LoadTextureFromFile("Res/Image/DamagedHelmet/Metallic.png");
	Texture* roughness = LoadTextureFromFile("Res/Image/DamagedHelmet/Roughness.png");
	Texture* ao = LoadTextureFromFile("Res/Image/DamagedHelmet/AO.jpg");
	Texture* emissive = LoadTextureFromFile("Res/Image/DamagedHelmet/Emissive.jpg");
	VkSampler textureSampler = GenSampler(
		VK_FILTER_LINEAR, VK_FILTER_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);

	gHelmetNode = new Node;
	gHelmetNode->mModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
	gHelmetNode->mStaticMeshComponent = new StaticMeshComponent;
	gHelmetNode->mStaticMeshComponent->LoadFromFile2("Res/Model/DamagedHelmet.staticmesh");
	gHelmetNode->mMaterial = new Material("Res/PBR1.vsb", "Res/PBR1.fsb");
	gHelmetNode->mMaterial->SetTexture(4, gPrefilteredColorCubeMap->mImageView, GenCubeMapSampler());
	gHelmetNode->mMaterial->SetTexture(5, gDiffuseIrradianceCubeMap->mImageView, GenCubeMapSampler());
	gHelmetNode->mMaterial->SetTexture(6, gBRDFLUTTexture->mImageView, GenCubeMapSampler());
	gHelmetNode->mMaterial->SetTexture(7, albedo->mImageView, textureSampler);
	gHelmetNode->mMaterial->SetTexture(8, normal->mImageView, textureSampler);
	gHelmetNode->mMaterial->SetTexture(9, emissive->mImageView, textureSampler);
	gHelmetNode->mMaterial->SetTexture(10, ao->mImageView, textureSampler);
	gHelmetNode->mMaterial->SetTexture(11, metallic->mImageView, textureSampler);
	gHelmetNode->mMaterial->SetTexture(12, roughness->mImageView, textureSampler);
	SetColorAttachmentCount(gHelmetNode->mMaterial->mPipelineStateObject, 1);
	gHelmetNode->mMaterial->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	gHelmetNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gHelmetNode->mMaterial, true);

	Texture* gunAlbedo = LoadTextureFromFile("Res/Image/Gun/albedo.png");
	Texture* gunNormal = LoadTextureFromFile("Res/Image/Gun/normal.png");
	Texture* gunMetallic = LoadTextureFromFile("Res/Image/Gun/metallic.png");
	Texture* gunRoughness = LoadTextureFromFile("Res/Image/Gun/roughness.png");
	Texture* gunAO = LoadTextureFromFile("Res/Image/Gun/ao.png");
	float blackData[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	Texture* blackTex = new Texture(VK_FORMAT_R32G32B32A32_SFLOAT);
	GenImage(blackTex, 1, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT);
	SubmitImage2D(blackTex, 1, 1, blackData);
	blackTex->mImageView = GenImageView2D(blackTex->mImage, blackTex->mFormat, blackTex->mImageAspectFlag);

	gGunNode = new Node;
	glm::mat4 gunModel = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f));
	gunModel = glm::rotate(gunModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	gunModel = glm::scale(gunModel, glm::vec3(0.9f));
	gGunNode->mModelMatrix = gunModel;
	gGunNode->mStaticMeshComponent = new StaticMeshComponent;
	gGunNode->mStaticMeshComponent->LoadFromFile2("Res/Model/Gun.staticmesh");
	gGunNode->mMaterial = new Material("Res/PBR1.vsb", "Res/PBR1.fsb");
	gGunNode->mMaterial->SetTexture(4, gPrefilteredColorCubeMap->mImageView, GenCubeMapSampler());
	gGunNode->mMaterial->SetTexture(5, gDiffuseIrradianceCubeMap->mImageView, GenCubeMapSampler());
	gGunNode->mMaterial->SetTexture(6, gBRDFLUTTexture->mImageView, GenCubeMapSampler());
	gGunNode->mMaterial->SetTexture(7, gunAlbedo->mImageView, textureSampler);
	gGunNode->mMaterial->SetTexture(8, gunNormal->mImageView, textureSampler);
	gGunNode->mMaterial->SetTexture(9, blackTex->mImageView, textureSampler);
	gGunNode->mMaterial->SetTexture(10, gunAO->mImageView, textureSampler);
	gGunNode->mMaterial->SetTexture(11, gunMetallic->mImageView, textureSampler);
	gGunNode->mMaterial->SetTexture(12, gunRoughness->mImageView, textureSampler);
	SetColorAttachmentCount(gGunNode->mMaterial->mPipelineStateObject, 1);
	gGunNode->mMaterial->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	gGunNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gGunNode->mMaterial, true);

	gGroundNode = new Node;
	gGroundNode->mModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, 0.0f));
	gGroundNode->mStaticMeshComponent = new GroundMeshComponent;
	((GroundMeshComponent*)gGroundNode->mStaticMeshComponent)->Init(10.0f);
	gGroundNode->mMaterial = new Material("Res/PBR.vsb", "Res/Ground.fsb");
	gGroundNode->mMaterial->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	gGroundNode->mMaterial->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	gGroundNode->mMaterial->SetTexture(4, gSkyBoxCubeMap->mImageView, GenCubeMapSampler());
	gGroundNode->mMaterial->SetTexture(5, gPrefilteredColorCubeMap->mImageView, GenCubeMapSampler());
	gGroundNode->mMaterial->SetTexture(6, gDiffuseIrradianceCubeMap->mImageView, GenCubeMapSampler());
	gGroundNode->mMaterial->SetTexture(7, gBRDFLUTTexture->mImageView);
	SetColorAttachmentCount(gGroundNode->mMaterial->mPipelineStateObject, 1);
	gGroundNode->mMaterial->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	gGroundNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gGroundNode->mMaterial, true);

	// Shadow pass：复用网格，仅输出深度
	gShadowHelmetNode = new Node;
	gShadowHelmetNode->mModelMatrix = gHelmetNode->mModelMatrix;
	gShadowHelmetNode->mStaticMeshComponent = gHelmetNode->mStaticMeshComponent;
	gShadowHelmetNode->mMaterial = new Material("Res/ShadowMap.vsb", "Res/ShadowMap.fsb");
	gShadowHelmetNode->mMaterial->mCullMode = VK_CULL_MODE_NONE;
	SetColorAttachmentCount(gShadowHelmetNode->mMaterial->mPipelineStateObject, 0);
	gShadowHelmetNode->mMaterial->mPipelineStateObject->mRenderPass = gShadowFBO->mRenderPass;
	gShadowHelmetNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyShadowViewport(gShadowHelmetNode->mMaterial);

	gShadowGunNode = new Node;
	gShadowGunNode->mModelMatrix = gGunNode->mModelMatrix;
	gShadowGunNode->mStaticMeshComponent = gGunNode->mStaticMeshComponent;
	gShadowGunNode->mMaterial = new Material("Res/ShadowMap.vsb", "Res/ShadowMap.fsb");
	SetColorAttachmentCount(gShadowGunNode->mMaterial->mPipelineStateObject, 0);
	gShadowGunNode->mMaterial->mPipelineStateObject->mRenderPass = gShadowFBO->mRenderPass;
	gShadowGunNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyShadowViewport(gShadowGunNode->mMaterial);

	gShadowGroundNode = new Node;
	gShadowGroundNode->mModelMatrix = gGroundNode->mModelMatrix;
	gShadowGroundNode->mStaticMeshComponent = gGroundNode->mStaticMeshComponent;
	gShadowGroundNode->mMaterial = new Material("Res/ShadowMap.vsb", "Res/ShadowMap.fsb");
	gShadowGroundNode->mMaterial->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	gShadowGroundNode->mMaterial->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	SetColorAttachmentCount(gShadowGroundNode->mMaterial->mPipelineStateObject, 0);
	gShadowGroundNode->mMaterial->mPipelineStateObject->mRenderPass = gShadowFBO->mRenderPass;
	gShadowGroundNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyShadowViewport(gShadowGroundNode->mMaterial);

	VkSampler shadowSampler = GenSampler(
		VK_FILTER_LINEAR, VK_FILTER_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
	gHelmetNode->mMaterial->SetTexture(13, gShadowFBO->mDepthBuffer->mImageView, shadowSampler);
	gGunNode->mMaterial->SetTexture(13, gShadowFBO->mDepthBuffer->mImageView, shadowSampler);
	gGroundNode->mMaterial->SetTexture(13, gShadowFBO->mDepthBuffer->mImageView, shadowSampler);
	SetLightVPOnMaterial(gHelmetNode->mMaterial, gLightVP);
	SetLightVPOnMaterial(gGunNode->mMaterial, gLightVP);
	SetLightVPOnMaterial(gGroundNode->mMaterial, gLightVP);
	SetPointLightsOnMaterial(gHelmetNode->mMaterial);
	SetPointLightsOnMaterial(gGunNode->mMaterial);
	SetPointLightsOnMaterial(gGroundNode->mMaterial);

	uint32_t bloomW = uint32_t(gViewportWidth) / 2;
	uint32_t bloomH = uint32_t(gViewportHeight) / 2;
	if (bloomW == 0) bloomW = 1;
	if (bloomH == 0) bloomH = 1;

	FullScreenQuadMeshComponent* bloomFSQ = new FullScreenQuadMeshComponent;
	bloomFSQ->Init();

	gBrightnessNode = new Node;
	gBrightnessNode->mModelMatrix = glm::mat4(1.0f);
	gBrightnessNode->mStaticMeshComponent = bloomFSQ;
	gBrightnessNode->mMaterial = new Material("Res/FSQ.vsb", "Res/BrightnessExtract.fsb");
	gBrightnessNode->mMaterial->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	gBrightnessNode->mMaterial->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	gBrightnessNode->mMaterial->SetTexture(4, gHDRFBO->mAttachments[0]->mImageView);
	SetColorAttachmentCount(gBrightnessNode->mMaterial->mPipelineStateObject, 1);
	gBrightnessNode->mMaterial->mPipelineStateObject->mRenderPass = gBloomBrightFBO->mRenderPass;
	gBrightnessNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	gBrightnessNode->mMaterial->SetVec4(1, 1.0f / float(gViewportWidth), 1.0f / float(gViewportHeight), 0.0f, 0.0f);
	ApplyBloomViewport(gBrightnessNode->mMaterial, bloomW, bloomH);

	gHBlurNode = new Node;
	gHBlurNode->mModelMatrix = glm::mat4(1.0f);
	gHBlurNode->mStaticMeshComponent = bloomFSQ;
	gHBlurNode->mMaterial = new Material("Res/FSQ.vsb", "Res/GaussianBlur.fsb");
	gHBlurNode->mMaterial->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	gHBlurNode->mMaterial->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	gHBlurNode->mMaterial->SetTexture(4, gBloomBrightFBO->mAttachments[0]->mImageView);
	gHBlurNode->mMaterial->SetVec4(1, 0.5f / float(bloomW), 0.0f, 0.0f, 0.0f);
	SetColorAttachmentCount(gHBlurNode->mMaterial->mPipelineStateObject, 1);
	gHBlurNode->mMaterial->mPipelineStateObject->mRenderPass = gBloomHBlurFBO->mRenderPass;
	gHBlurNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyBloomViewport(gHBlurNode->mMaterial, bloomW, bloomH);

	gVBlurNode = new Node;
	gVBlurNode->mModelMatrix = glm::mat4(1.0f);
	gVBlurNode->mStaticMeshComponent = bloomFSQ;
	gVBlurNode->mMaterial = new Material("Res/FSQ.vsb", "Res/GaussianBlur.fsb");
	gVBlurNode->mMaterial->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	gVBlurNode->mMaterial->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	gVBlurNode->mMaterial->SetTexture(4, gBloomHBlurFBO->mAttachments[0]->mImageView);
	gVBlurNode->mMaterial->SetVec4(1, 0.0f, 0.5f / float(bloomH), 0.0f, 0.0f);
	SetColorAttachmentCount(gVBlurNode->mMaterial->mPipelineStateObject, 1);
	gVBlurNode->mMaterial->mPipelineStateObject->mRenderPass = gBloomVBlurFBO->mRenderPass;
	gVBlurNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyBloomViewport(gVBlurNode->mMaterial, bloomW, bloomH);

	srand(42);
	float ssaoKernel[32 * 4];
	for (int i = 0; i < 32; i++) {
		glm::vec3 s(
			((float)rand() / RAND_MAX) * 2.0f - 1.0f,
			((float)rand() / RAND_MAX) * 2.0f - 1.0f,
			(float)rand() / RAND_MAX);
		s = glm::normalize(s);
		float scale = (float)i / 32.0f;
		scale = 0.1f + scale * scale * 0.9f;
		s *= scale;
		ssaoKernel[i * 4 + 0] = s.x;
		ssaoKernel[i * 4 + 1] = s.y;
		ssaoKernel[i * 4 + 2] = s.z;
		ssaoKernel[i * 4 + 3] = 0.0f;
	}
	float noiseData[16 * 4];
	for (int i = 0; i < 16; i++) {
		float angle = ((float)rand() / RAND_MAX) * 2.0f * 3.14159265f;
		noiseData[i * 4 + 0] = cosf(angle);
		noiseData[i * 4 + 1] = sinf(angle);
		noiseData[i * 4 + 2] = 0.0f;
		noiseData[i * 4 + 3] = 0.0f;
	}
	gSSAONoiseTex = new Texture(VK_FORMAT_R32G32B32A32_SFLOAT);
	GenImage(gSSAONoiseTex, 4, 4, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT);
	SubmitImage2D(gSSAONoiseTex, 4, 4, noiseData);
	gSSAONoiseTex->mImageView = GenImageView2D(gSSAONoiseTex->mImage, gSSAONoiseTex->mFormat, gSSAONoiseTex->mImageAspectFlag);
	VkSampler noiseSampler = GenSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	gDepthSampler = GenSampler(VK_FILTER_NEAREST, VK_FILTER_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	gSSAONode = new Node;
	gSSAONode->mModelMatrix = glm::mat4(1.0f);
	gSSAONode->mStaticMeshComponent = bloomFSQ;
	gSSAONode->mMaterial = new Material("Res/FSQ.vsb", "Res/SSAO.fsb");
	gSSAONode->mMaterial->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	gSSAONode->mMaterial->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	gSSAONode->mMaterial->SetTexture(4, gDeferredEnabled ? gGBufferFBO->mDepthBuffer->mImageView : gHDRFBO->mDepthBuffer->mImageView, gDepthSampler);
	gSSAONode->mMaterial->SetTexture(5, gSSAONoiseTex->mImageView, noiseSampler);
	SetColorAttachmentCount(gSSAONode->mMaterial->mPipelineStateObject, 1);
	gSSAONode->mMaterial->mPipelineStateObject->mRenderPass = gSSAOFBO->mRenderPass;
	gSSAONode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gSSAONode->mMaterial, false);
	for (int i = 0; i < 32; i++) {
		gSSAONode->mMaterial->SetVec4(10 + i, ssaoKernel[i * 4], ssaoKernel[i * 4 + 1], ssaoKernel[i * 4 + 2], 0.0f);
	}
	UpdateSSAOUniforms();

	gSSAOBlurNode = new Node;
	gSSAOBlurNode->mModelMatrix = glm::mat4(1.0f);
	gSSAOBlurNode->mStaticMeshComponent = bloomFSQ;
	gSSAOBlurNode->mMaterial = new Material("Res/FSQ.vsb", "Res/SSAOBlur.fsb");
	gSSAOBlurNode->mMaterial->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	gSSAOBlurNode->mMaterial->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	gSSAOBlurNode->mMaterial->SetTexture(4, gSSAOFBO->mAttachments[0]->mImageView);
	SetColorAttachmentCount(gSSAOBlurNode->mMaterial->mPipelineStateObject, 1);
	gSSAOBlurNode->mMaterial->mPipelineStateObject->mRenderPass = gSSAOBlurFBO->mRenderPass;
	gSSAOBlurNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gSSAOBlurNode->mMaterial, false);

	gGBufferHelmetNode = new Node;
	gGBufferHelmetNode->mModelMatrix = gHelmetNode->mModelMatrix;
	gGBufferHelmetNode->mStaticMeshComponent = gHelmetNode->mStaticMeshComponent;
	gGBufferHelmetNode->mMaterial = new Material("Res/PBR1.vsb", "Res/GBuffer.fsb");
	gGBufferHelmetNode->mMaterial->mCullMode = VK_CULL_MODE_NONE;
	gGBufferHelmetNode->mMaterial->SetTexture(7, albedo->mImageView, textureSampler);
	gGBufferHelmetNode->mMaterial->SetTexture(8, normal->mImageView, textureSampler);
	gGBufferHelmetNode->mMaterial->SetTexture(9, emissive->mImageView, textureSampler);
	gGBufferHelmetNode->mMaterial->SetTexture(10, ao->mImageView, textureSampler);
	gGBufferHelmetNode->mMaterial->SetTexture(11, metallic->mImageView, textureSampler);
	gGBufferHelmetNode->mMaterial->SetTexture(12, roughness->mImageView, textureSampler);
	SetColorAttachmentCount(gGBufferHelmetNode->mMaterial->mPipelineStateObject, 4);
	gGBufferHelmetNode->mMaterial->mPipelineStateObject->mRenderPass = gGBufferFBO->mRenderPass;
	gGBufferHelmetNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gGBufferHelmetNode->mMaterial, true);

	gGBufferGunNode = new Node;
	gGBufferGunNode->mModelMatrix = gGunNode->mModelMatrix;
	gGBufferGunNode->mStaticMeshComponent = gGunNode->mStaticMeshComponent;
	gGBufferGunNode->mMaterial = new Material("Res/PBR1.vsb", "Res/GBuffer.fsb");
	gGBufferGunNode->mMaterial->SetTexture(7, gunAlbedo->mImageView, textureSampler);
	gGBufferGunNode->mMaterial->SetTexture(8, gunNormal->mImageView, textureSampler);
	gGBufferGunNode->mMaterial->SetTexture(9, blackTex->mImageView, textureSampler);
	gGBufferGunNode->mMaterial->SetTexture(10, gunAO->mImageView, textureSampler);
	gGBufferGunNode->mMaterial->SetTexture(11, gunMetallic->mImageView, textureSampler);
	gGBufferGunNode->mMaterial->SetTexture(12, gunRoughness->mImageView, textureSampler);
	SetColorAttachmentCount(gGBufferGunNode->mMaterial->mPipelineStateObject, 4);
	gGBufferGunNode->mMaterial->mPipelineStateObject->mRenderPass = gGBufferFBO->mRenderPass;
	gGBufferGunNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gGBufferGunNode->mMaterial, true);

	gGBufferGroundNode = new Node;
	gGBufferGroundNode->mModelMatrix = gGroundNode->mModelMatrix;
	gGBufferGroundNode->mStaticMeshComponent = gGroundNode->mStaticMeshComponent;
	gGBufferGroundNode->mMaterial = new Material("Res/PBR.vsb", "Res/GBufferGround.fsb");
	gGBufferGroundNode->mMaterial->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	gGBufferGroundNode->mMaterial->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	SetColorAttachmentCount(gGBufferGroundNode->mMaterial->mPipelineStateObject, 4);
	gGBufferGroundNode->mMaterial->mPipelineStateObject->mRenderPass = gGBufferFBO->mRenderPass;
	gGBufferGroundNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gGBufferGroundNode->mMaterial, true);

	gDeferredLightingNode = new Node;
	gDeferredLightingNode->mModelMatrix = glm::mat4(1.0f);
	gDeferredLightingNode->mStaticMeshComponent = bloomFSQ;
	gDeferredLightingNode->mMaterial = new Material("Res/FSQ.vsb", "Res/DeferredLighting.fsb");
	gDeferredLightingNode->mMaterial->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	gDeferredLightingNode->mMaterial->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	gDeferredLightingNode->mMaterial->SetTexture(4, gGBufferFBO->mAttachments[0]->mImageView);
	gDeferredLightingNode->mMaterial->SetTexture(5, gGBufferFBO->mAttachments[1]->mImageView);
	gDeferredLightingNode->mMaterial->SetTexture(6, gGBufferFBO->mAttachments[2]->mImageView);
	gDeferredLightingNode->mMaterial->SetTexture(7, gGBufferFBO->mAttachments[3]->mImageView);
	gDeferredLightingNode->mMaterial->SetTexture(8, gGBufferFBO->mDepthBuffer->mImageView, gDepthSampler);
	gDeferredLightingNode->mMaterial->SetTexture(9, gShadowFBO->mDepthBuffer->mImageView, shadowSampler);
	gDeferredLightingNode->mMaterial->SetTexture(10, gPrefilteredColorCubeMap->mImageView, GenCubeMapSampler());
	gDeferredLightingNode->mMaterial->SetTexture(11, gDiffuseIrradianceCubeMap->mImageView, GenCubeMapSampler());
	gDeferredLightingNode->mMaterial->SetTexture(12, gBRDFLUTTexture->mImageView);
	gDeferredLightingNode->mMaterial->SetTexture(13, gSkyBoxCubeMap->mImageView, GenCubeMapSampler());
	SetColorAttachmentCount(gDeferredLightingNode->mMaterial->mPipelineStateObject, 1);
	gDeferredLightingNode->mMaterial->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	gDeferredLightingNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gDeferredLightingNode->mMaterial, false);
	SetLightVPOnMaterial(gDeferredLightingNode->mMaterial, gLightVP);
	SetPointLightsOnMaterial(gDeferredLightingNode->mMaterial);

	gToneMappingNode = new Node;
	gToneMappingNode->mModelMatrix = glm::mat4(1.0f);
	gToneMappingNode->mStaticMeshComponent = new FullScreenQuadMeshComponent;
	((FullScreenQuadMeshComponent*)gToneMappingNode->mStaticMeshComponent)->Init();
	gToneMappingNode->mMaterial = new Material("Res/FSQ.vsb", "Res/ToneMapping.fsb");
	gToneMappingNode->mMaterial->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	gToneMappingNode->mMaterial->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	gToneMappingNode->mMaterial->SetTexture(4, gHDRFBO->mAttachments[0]->mImageView);
	gToneMappingNode->mMaterial->SetTexture(5, gBloomVBlurFBO->mAttachments[0]->mImageView);
	gToneMappingNode->mMaterial->SetTexture(6, gSSAOBlurFBO->mAttachments[0]->mImageView);
	SetColorAttachmentCount(gToneMappingNode->mMaterial->mPipelineStateObject, 1);
	gToneMappingNode->mMaterial->mPipelineStateObject->mRenderPass = gLDRFBO->mRenderPass;
	gToneMappingNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gToneMappingNode->mMaterial, false);

	gFXAANode = new Node;
	gFXAANode->mModelMatrix = glm::mat4(1.0f);
	gFXAANode->mStaticMeshComponent = new FullScreenQuadMeshComponent;
	((FullScreenQuadMeshComponent*)gFXAANode->mStaticMeshComponent)->Init();
	gFXAANode->mMaterial = new Material("Res/FSQ.vsb", "Res/FXAA.fsb");
	gFXAANode->mMaterial->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	gFXAANode->mMaterial->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	gFXAANode->mMaterial->SetTexture(4, gLDRFBO->mAttachments[0]->mImageView);
	gFXAANode->mMaterial->SetVec4(1, gFXAAEnabled ? 1.0f : 0.0f, 1.0f / float(gViewportWidth), 1.0f / float(gViewportHeight), 0.0f);
	SetColorAttachmentCount(gFXAANode->mMaterial->mPipelineStateObject, 1);
	gFXAANode->mMaterial->mPipelineStateObject->mRenderPass = globalConfig.mSystemRenderPass;
	gFXAANode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gFXAANode->mMaterial, false);
}

void RenderOneFrame() {
	float frameTime = GetFrameTime();
	gMainCamera.RotateFPS(gMouseDx, gMouseDy, 0.15f);
	gMouseDx = 0.0f; gMouseDy = 0.0f;
	bool kW = (GetAsyncKeyState('W') & 0x8000) != 0;
	bool kS = (GetAsyncKeyState('S') & 0x8000) != 0;
	bool kA = (GetAsyncKeyState('A') & 0x8000) != 0;
	bool kD = (GetAsyncKeyState('D') & 0x8000) != 0;
	bool kSpace = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
	bool kShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
	gMainCamera.MoveFPS(frameTime, kW, kS, kA, kD, kSpace, kShift, 5.0f);

	// Pass 0：灯光视角 Shadow map
	VkCommandBuffer commandBuffer = gShadowFBO->BeginRendering();
	gShadowHelmetNode->mModelMatrix = gHelmetNode->mModelMatrix;
	gShadowHelmetNode->Draw(commandBuffer, gLightProjection, gLightCamera);
	gShadowGunNode->mModelMatrix = gGunNode->mModelMatrix;
	gShadowGunNode->Draw(commandBuffer, gLightProjection, gLightCamera);
	gShadowGroundNode->mModelMatrix = gGroundNode->mModelMatrix;
	gShadowGroundNode->Draw(commandBuffer, gLightProjection, gLightCamera);
	vkCmdEndRenderPass(commandBuffer);

	if (gDeferredEnabled) {
		// 延迟：G-Buffer MRT，再全屏 deferred lighting → HDR
		commandBuffer = gGBufferFBO->BeginRendering(commandBuffer);
		gGBufferHelmetNode->mModelMatrix = gHelmetNode->mModelMatrix;
		gGBufferHelmetNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
		gGBufferGunNode->mModelMatrix = gGunNode->mModelMatrix;
		gGBufferGunNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
		gGBufferGroundNode->mModelMatrix = gGroundNode->mModelMatrix;
		gGBufferGroundNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
		vkCmdEndRenderPass(commandBuffer);

		glm::mat4 invVP = glm::inverse(gProjectionMatrix * gMainCamera.mViewMatrix);
		const float* invData = glm::value_ptr(invVP);
		gDeferredLightingNode->mMaterial->SetVec4(5, (float*)invData);
		gDeferredLightingNode->mMaterial->SetVec4(6, (float*)(invData + 4));
		gDeferredLightingNode->mMaterial->SetVec4(7, (float*)(invData + 8));
		gDeferredLightingNode->mMaterial->SetVec4(8, (float*)(invData + 12));

		commandBuffer = gHDRFBO->BeginRendering(commandBuffer);
		gDeferredLightingNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
		vkCmdEndRenderPass(commandBuffer);
	} else {
		// 前向：天空盒 + 物体 → HDR
		commandBuffer = gHDRFBO->BeginRendering(commandBuffer);
		gSkyBoxNode->mModelMatrix = glm::translate(glm::mat4(1.0f), gMainCamera.mPosition);
		gSkyBoxNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
		gHelmetNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
		gGunNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
		gGroundNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
		vkCmdEndRenderPass(commandBuffer);
	}

	// SSAO 与 blur（采样深度来自 G-Buffer 或 HDR）
	commandBuffer = gSSAOFBO->BeginRendering(commandBuffer);
	if (gSSAOEnabled) gSSAONode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	vkCmdEndRenderPass(commandBuffer);

	commandBuffer = gSSAOBlurFBO->BeginRendering(commandBuffer);
	if (gSSAOEnabled) gSSAOBlurNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	vkCmdEndRenderPass(commandBuffer);

	// Bloom：亮部提取 + 分离高斯模糊
	commandBuffer = gBloomBrightFBO->BeginRendering(commandBuffer);
	if (gBloomEnabled) gBrightnessNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	vkCmdEndRenderPass(commandBuffer);

	commandBuffer = gBloomHBlurFBO->BeginRendering(commandBuffer);
	if (gBloomEnabled) gHBlurNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	vkCmdEndRenderPass(commandBuffer);

	commandBuffer = gBloomVBlurFBO->BeginRendering(commandBuffer);
	if (gBloomEnabled) gVBlurNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	vkCmdEndRenderPass(commandBuffer);

	// Tonemap：HDR + Bloom + SSAO → LDR
	commandBuffer = gLDRFBO->BeginRendering(commandBuffer);
	gToneMappingNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	vkCmdEndRenderPass(commandBuffer);

	// FXAA：LDR → Swapchain（最后一道）
	gFXAANode->mMaterial->SetTexture(4, gLDRFBO->mAttachments[0]->mImageView);
	gFXAANode->mMaterial->SetVec4(1, gFXAAEnabled ? 1.0f : 0.0f, 1.0f / float(gViewportWidth), 1.0f / float(gViewportHeight), 0.0f);
	BeginRendering(commandBuffer);
	gFXAANode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	EndRendering();
	SwapBuffers();
}
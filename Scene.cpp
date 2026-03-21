#include "Scene.h"
#include "MyVulkan.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "Node.h"
#include "Mesh.h"
#include "Captures.h"



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
static Node* gToneMappingNode = nullptr;

static void ApplyViewportToMaterial(Material* material, bool flipY) {
	if (material == nullptr || material->mPipelineStateObject == nullptr) {
		return;
	}
	float y = flipY ? float(gViewportHeight) : 0.0f;
	float h = flipY ? -float(gViewportHeight) : float(gViewportHeight);
	material->mPipelineStateObject->mViewport = { 0.0f, y, float(gViewportWidth), h, 0.0f, 1.0f };
	material->mPipelineStateObject->mScissor = { {0, 0}, {uint32_t(gViewportWidth), uint32_t(gViewportHeight)} };
}

static void RecreateHDRFBO() {
	// STEP2_BEGIN: HDR render target (offscreen, high precision color)
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
	if (gToneMappingNode != nullptr) {
		Material* toneMapping = gToneMappingNode->mMaterial;
		toneMapping->mPipelineStateObject->mRenderPass = GetGlobalConfig().mSystemRenderPass;
		toneMapping->SetTexture(4, gHDRFBO->mAttachments[0]->mImageView);
	}
	// STEP2_END
}

void OnViewportChanged(int inWidth, int inHeight) {
	if (inWidth <= 0 || inHeight <= 0) {
		return;
	}
	gViewportWidth = inWidth;
	gViewportHeight = inHeight;
	gProjectionMatrix = glm::perspective((45.0f * 3.14f / 180.0f), float(inWidth) / float(inHeight), 0.1f, 1000.0f);
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
	if (gToneMappingNode != nullptr) {
		ApplyViewportToMaterial(gToneMappingNode->mMaterial, false);
	}
}

void InitScene() {
	GlobalConfig& globalConfig = GetGlobalConfig();
	gViewportWidth = globalConfig.mViewportWidth;
	gViewportHeight = globalConfig.mViewportHeight;
	gProjectionMatrix = glm::perspective((45.0f * 3.14f / 180.0f), float(gViewportWidth) / float(gViewportHeight), 0.1f, 1000.0f);

	// STEP4_BEGIN: IBL preprocessing chain (required by PBR IBL)
	gSkyBoxCubeMap = LoadHDRICubeMapFromFile("Res/Image/1.hdr", 512, "Res/SkyBox.vsb", "Res/Texture2D2CubeMap.fsb");
	gDiffuseIrradianceCubeMap = CaptureDiffuseIrradiance(gSkyBoxCubeMap, 32, "Res/SkyBox.vsb", "Res/CaptureDiffuseIrradiance.fsb");
	gPrefilteredColorCubeMap = CapturePrefilteredColor(gSkyBoxCubeMap, 128, "Res/SkyBox.vsb", "Res/CapturePrefilteredColor.fsb");
	gBRDFLUTTexture = GenerateBRDF(512, "Res/FSQ.vsb", "Res/GenerateBRDF.fsb");
	// STEP4_END

	gMainCamera.Init(glm::vec3(0.0f, 0.0f, 0.0f), 5.0f, glm::vec3(0.0f, -0.2f, 1.0f));

	RecreateHDRFBO();

	// STEP3_BEGIN: Skybox node
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
	// STEP3_END

	// STEP5_BEGIN: Simple PBR sphere (constant material, IBL-driven)
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
	// STEP5_END

	// STEP6_BEGIN: Full PBR helmet (all texture channels)
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
	gHelmetNode->mModelMatrix = glm::mat4(1.0f);
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
	// STEP6_END

	// STEP2_BEGIN: Fullscreen tone mapping pass
	gToneMappingNode = new Node;
	gToneMappingNode->mModelMatrix = glm::mat4(1.0f);
	gToneMappingNode->mStaticMeshComponent = new FullScreenQuadMeshComponent;
	((FullScreenQuadMeshComponent*)gToneMappingNode->mStaticMeshComponent)->Init();
	gToneMappingNode->mMaterial = new Material("Res/FSQ.vsb", "Res/ToneMapping.fsb");
	gToneMappingNode->mMaterial->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	gToneMappingNode->mMaterial->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	gToneMappingNode->mMaterial->SetTexture(4, gHDRFBO->mAttachments[0]->mImageView);
	SetColorAttachmentCount(gToneMappingNode->mMaterial->mPipelineStateObject, 1);
	gToneMappingNode->mMaterial->mPipelineStateObject->mRenderPass = globalConfig.mSystemRenderPass;
	gToneMappingNode->mMaterial->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	ApplyViewportToMaterial(gToneMappingNode->mMaterial, false);
	// STEP2_END
}

void RenderOneFrame() {
	float frameTime = GetFrameTime();
	gMainCamera.RoundRotate(frameTime, glm::vec3(0.0f, 0.0f, 0.0f), 5.0f, 25.0f);

	// STEP2_BEGIN: Pass 1 -> render scene to HDR offscreen target
	VkCommandBuffer commandBuffer = gHDRFBO->BeginRendering();
	gSkyBoxNode->mModelMatrix = glm::translate(glm::mat4(1.0f), gMainCamera.mPosition);
	// STEP3_DRAW_BEGIN: skybox draw (comment this line to disable skybox)
	gSkyBoxNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	// STEP3_DRAW_END
	// STEP6_DRAW_BEGIN: helmet draw (comment this line to disable full PBR)
	gHelmetNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	// STEP6_DRAW_END
	// STEP5_DRAW_BEGIN: sphere draw (useful as intermediate learning stage)
	// gSphereNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	// STEP5_DRAW_END
	vkCmdEndRenderPass(commandBuffer);
	// STEP2_END

	// STEP2_BEGIN: Pass 2 -> tone map HDR to swapchain
	BeginRendering(commandBuffer);
	gToneMappingNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	EndRendering();
	SwapBuffers();
	// STEP2_END

	// STEP0_NOTE:
	// To observe "near-original minimal state", comment out STEP3/5/6 draw blocks above.
	// Then only clear + tone-map pass path remains visible.
}
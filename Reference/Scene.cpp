#include "Scene.h"
#include "BattleFireVulkan.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "Node.h"
#include "Mesh.h"
#include "Captures.h"
// Scene-global runtime objects.
glm::mat4 gProjectionMatrix;
Camera gMainCamera;
FrameBufferEx* gHDRFBO = nullptr;
Texture* gSkyBoxCubeMap = nullptr,*gDiffuseIrradianceCubeMap=nullptr,*gPrefilteredColorCubeMap=nullptr,*gBRDFLUTTexture=nullptr;
Node* gSphereNode = nullptr, * gToneMappingNode = nullptr,*gSkyBoxNode=nullptr, * gHelmetNode = nullptr;
void OnViewportChanged(int inWidth, int inHeight) {
	// Keep projection in sync with current viewport.
	gProjectionMatrix = glm::perspective((45.0f * 3.14f / 180.0f), float(inWidth) / float(inHeight), 0.1f, 1000.0f);
	OnViewportChangedVulkan(inWidth, inHeight);
}
void InitScene() {
	// --- IBL precompute chain ---
	// 1) HDR equirectangular -> environment cubemap
	gSkyBoxCubeMap = LoadHDRICubeMapFromFile("Res/Image/1.hdr", 512, "Res/SkyBox.vsb", "Res/Texture2D2CubeMap.fsb");
	// 2) diffuse irradiance cubemap for ambient diffuse term
	gDiffuseIrradianceCubeMap = CaptureDiffuseIrradiance(gSkyBoxCubeMap, 32, "Res/SkyBox.vsb","Res/CaptureDiffuseIrradiance.fsb");
	// 3) prefiltered cubemap for ambient specular term
	gPrefilteredColorCubeMap = CapturePrefilteredColor(gSkyBoxCubeMap, 128, "Res/SkyBox.vsb", "Res/CapturePrefilteredColor.fsb");
	// 4) BRDF integration LUT for split-sum approximation
	gBRDFLUTTexture = GenerateBRDF(512, "Res/FSQ.vsb", "Res/GenerateBRDF.fsb");
	gMainCamera.Init(glm::vec3(0.0f, 0.0f, 0.0f), 5.0f, glm::vec3(0.0f,-0.2f,1.0f));

	// Load PBR texture set for the helmet.
	Texture* albedo = LoadTextureFromFile("Res/Image/DamagedHelmet/Albedo.jpg");
	Texture* normal = LoadTextureFromFile("Res/Image/DamagedHelmet/Normal.jpg");
	Texture* metallic = LoadTextureFromFile("Res/Image/DamagedHelmet/Metallic.png");
	Texture* roughness = LoadTextureFromFile("Res/Image/DamagedHelmet/Roughness.png");
	Texture* ao = LoadTextureFromFile("Res/Image/DamagedHelmet/AO.jpg");
	Texture* emissive = LoadTextureFromFile("Res/Image/DamagedHelmet/Emissive.jpg");

	GlobalConfig& globalConfig = GetGlobalConfig();
	// Offscreen HDR render target:
	// opaque scene is rendered here first, then tone-mapped to swapchain.
	gHDRFBO = new FrameBufferEx;
	gHDRFBO->SetSize(globalConfig.mViewportWidth, globalConfig.mViewportHeight);
	gHDRFBO->AttachColorBuffer(VK_FORMAT_R32G32B32A32_SFLOAT);
	gHDRFBO->AttachDepthBuffer();
	gHDRFBO->Finish();//shader resource view,sampler2d,texture2D

	gSphereNode = new Node;
	gSphereNode->mModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,0.0f,0.0f));
	Material* material = new Material("Res/PBR.vsb","Res/PBR.fsb");
	gSphereNode->mMaterial = material;
	StaticMeshComponent* sphereMesh = new StaticMeshComponent;
	sphereMesh->LoadFromFile("Res/Model/sphere.staticmesh");
	gSphereNode->mStaticMeshComponent = sphereMesh;
	material->mPipelineStateObject->mViewport = {
		0.0f,720.0f,
		1280.0f,-720.0f,
		0.0f,1.0f
	};
	material->mPipelineStateObject->mScissor = {
		{0,0},{1280,720}
	};
	material->SetTexture(4, gSkyBoxCubeMap->mImageView, GenCubeMapSampler());
	material->SetTexture(5, gPrefilteredColorCubeMap->mImageView, GenCubeMapSampler());
	material->SetTexture(6, gDiffuseIrradianceCubeMap->mImageView, GenCubeMapSampler());
	material->SetTexture(7, gBRDFLUTTexture->mImageView);
	// Sphere is also rendered into HDR FBO.
	SetColorAttachmentCount(material->mPipelineStateObject, 1);
	material->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	material->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;

	gHelmetNode = new Node;
	gHelmetNode->mModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	material = new Material("Res/PBR1.vsb", "Res/PBR1.fsb");
	gHelmetNode->mMaterial = material;
	StaticMeshComponent* helmetMesh = new StaticMeshComponent;
	helmetMesh->LoadFromFile2("Res/Model/DamagedHelmet.staticmesh");
	gHelmetNode->mStaticMeshComponent = helmetMesh;
	material->mPipelineStateObject->mViewport = {
		0.0f,720.0f,
		1280.0f,-720.0f,
		0.0f,1.0f
	};
	material->mPipelineStateObject->mScissor = {
		{0,0},{1280,720}
	};
	VkSampler textureSampler = GenSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
	
	// Bind all textures needed by full PBR material model.
	material->SetTexture(4, gPrefilteredColorCubeMap->mImageView, GenCubeMapSampler());
	material->SetTexture(5, gDiffuseIrradianceCubeMap->mImageView, GenCubeMapSampler());
	material->SetTexture(6, gBRDFLUTTexture->mImageView, GenCubeMapSampler());
	material->SetTexture(7, albedo->mImageView,textureSampler);
	material->SetTexture(8, normal->mImageView, textureSampler);
	material->SetTexture(9, emissive->mImageView, textureSampler);
	material->SetTexture(10, ao->mImageView, textureSampler);
	material->SetTexture(11, metallic->mImageView, textureSampler);
	material->SetTexture(12, roughness->mImageView, textureSampler);
	SetColorAttachmentCount(material->mPipelineStateObject, 1);
	material->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	material->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;

	gToneMappingNode = new Node;
	gToneMappingNode->mModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	material = new Material("Res/FSQ.vsb", "Res/ToneMapping.fsb");
	gToneMappingNode->mMaterial = material;
	FullScreenQuadMeshComponent* fsqMesh = new FullScreenQuadMeshComponent;
	fsqMesh->Init();
	gToneMappingNode->mStaticMeshComponent = fsqMesh;
	// Fullscreen pass: sample HDR buffer and map to LDR output.
	material->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	material->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	material->mPipelineStateObject->mViewport = {
		0.0f,0.0f,
		1280.0f,720.0f,
		0.0f,1.0f
	};
	material->mPipelineStateObject->mScissor = {
		{0,0},{1280,720}
	};
	SetColorAttachmentCount(material->mPipelineStateObject, 1);
	material->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	material->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	material->SetTexture(4, gHDRFBO->mAttachments[0]->mImageView);

	gSkyBoxNode = new Node;
	gSkyBoxNode->mModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	material = new Material("Res/SkyBox.vsb", "Res/SkyBox.fsb");
	gSkyBoxNode->mMaterial = material;
	StaticMeshComponent* skyBoxMesh = new StaticMeshComponent;
	skyBoxMesh->LoadFromFile("Res/Model/skybox.staticmesh");
	gSkyBoxNode->mStaticMeshComponent = skyBoxMesh;
	// Skybox should not write depth to avoid hiding foreground objects.
	material->mbEnableDepthWrite = false;
	material->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	material->mPipelineStateObject->mViewport = {
		0.0f,720.0f,
		1280.0f,-720.0f,
		0.0f,1.0f
	};
	material->mPipelineStateObject->mScissor = {
		{0,0},{1280,720}
	};
	material->SetTexture(4, gSkyBoxCubeMap->mImageView, GenCubeMapSampler());
	SetColorAttachmentCount(material->mPipelineStateObject, 1);
	material->mPipelineStateObject->mRenderPass = gHDRFBO->mRenderPass;
	material->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
}
void RenderOneFrame() {
	float frameTime = GetFrameTime();
	gMainCamera.RoundRotate(frameTime, glm::vec3(0.0f,0.0f,0.0f),5.0f,30.0f);
	// Pass 1: render scene into HDR offscreen framebuffer.
	VkCommandBuffer commandBuffer = gHDRFBO->BeginRendering();
	gSkyBoxNode->mModelMatrix = glm::translate(glm::mat4(1.0f), gMainCamera.mPosition);
	gSkyBoxNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	gHelmetNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	//render skybox
	//render sphere
	//render helmet
	//render gun
	vkCmdEndRenderPass(commandBuffer);
	// Pass 2: render fullscreen tone-mapping to system swapchain render pass.
	BeginRendering(commandBuffer);
	gToneMappingNode->Draw(commandBuffer, gProjectionMatrix, gMainCamera);
	EndRendering();
	SwapBuffers();//glSwapBuffers();
}
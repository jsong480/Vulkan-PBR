#include "Scene.h"
#include "MyVulkan.h"
#include "Mesh.h"
#include "Node.h"
#include "Camera.h"
#include "FrameBuffer.h"

// 将离屏 RT 拷贝到 cubemap 指定面/ mip
static bool gMatrixInited = false;
static glm::mat4 gCaptureProjectionMatrix;
static Camera gCaptureCameras[6];
void CopyRTImageToCubeMap(VkCommandBuffer inCommandBuffer, VkImage inSrcImage, int inWidth, int inHeight, VkImage inDstCubeMap, int inFace, int inMipmapLevel) {
	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT ,0,1,0,1 };
	TransferImageLayout(inCommandBuffer, inSrcImage, subresourceRange,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	VkImageCopy copyRegion = {};

	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.srcSubresource.baseArrayLayer = 0;
	copyRegion.srcSubresource.mipLevel = 0;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.srcOffset = { 0, 0, 0 };

	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.dstSubresource.baseArrayLayer = inFace;
	copyRegion.dstSubresource.mipLevel = inMipmapLevel;
	copyRegion.dstSubresource.layerCount = 1;
	copyRegion.dstOffset = { 0, 0, 0 };

	copyRegion.extent.width = static_cast<uint32_t>(inWidth);
	copyRegion.extent.height = static_cast<uint32_t>(inHeight);
	copyRegion.extent.depth = 1;

	vkCmdCopyImage(
		inCommandBuffer,
		inSrcImage,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		inDstCubeMap,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&copyRegion);
}
Texture* CreateTextureFromFile(const char* path) {
	int image_width, image_height, channel_count;
	float* pixelData = stbi_loadf(path, &image_width, &image_height, &channel_count, 0);
	float* pixel = new float[image_width*image_height*4];
	for (int i=0;i<image_width*image_height;i++){
		pixel[i * 4] = pixelData[i * 3];
		pixel[i * 4 + 1] = pixelData[i * 3 + 1];
		pixel[i * 4 + 2] = pixelData[i * 3 + 2];
		pixel[i * 4 + 3] = 1.0f;
	}
	Texture* texture = new Texture(VK_FORMAT_R32G32B32A32_SFLOAT);
	texture->mFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
	GenImage(texture, image_width, image_height, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT);
	SubmitImage2D(texture, image_width, image_height, pixel);
	texture->mImageView = GenImageView2D(texture->mImage, texture->mFormat, texture->mImageAspectFlag);
	stbi_image_free(pixelData);
	delete[]pixel;
	return texture;
}
void InitMatrices() {
	if (gMatrixInited) {
		return;
	}
	gMatrixInited = true;
	gCaptureProjectionMatrix = glm::perspective((90.0f * 3.14f / 180.0f), 1.0f, 0.1f, 100.0f);
	gCaptureCameras[0].mViewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	gCaptureCameras[1].mViewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	gCaptureCameras[2].mViewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	gCaptureCameras[3].mViewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	gCaptureCameras[4].mViewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	gCaptureCameras[5].mViewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
}
// 六面渲染 HDRI 到立方体贴图
void HDRI2CubeMap(const char* inFilePath, Texture* inoutCubeMap, int inCubeMapResolution, const char* inVSFilePath, const char* inFSFilePath) {
	InitMatrices();
	Texture*hdriTexture = CreateTextureFromFile(inFilePath);
	StaticMeshComponent* skyboxMesh = new StaticMeshComponent;
	skyboxMesh->LoadFromFile("Res/Model/skybox.staticmesh");

	FrameBufferEx* HDRI2CubeMapFBO = new FrameBufferEx;
	HDRI2CubeMapFBO->SetSize(inCubeMapResolution, inCubeMapResolution);
	HDRI2CubeMapFBO->AttachColorBuffer(VK_FORMAT_R32G32B32A32_SFLOAT);
	HDRI2CubeMapFBO->AttachDepthBuffer();
	HDRI2CubeMapFBO->Finish(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	Node* HDRI2CubeMapNode = new Node;
	HDRI2CubeMapNode->mModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	HDRI2CubeMapNode->mStaticMeshComponent = skyboxMesh;
	Material* material = new Material(inVSFilePath, inFSFilePath);
	material->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	material->mPipelineStateObject->mViewport = {
		0,float(inCubeMapResolution),
		float(inCubeMapResolution),-float(inCubeMapResolution),
		0.0f,1.0f
	};
	material->mPipelineStateObject->mScissor = {
		{0,0},{uint32_t(inCubeMapResolution),uint32_t(inCubeMapResolution)}
	};
	material->SetTexture(4, hdriTexture->mImageView);
	SetColorAttachmentCount(material->mPipelineStateObject, 1);
	material->mPipelineStateObject->mRenderPass = HDRI2CubeMapFBO->mRenderPass;
	material->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	HDRI2CubeMapNode->mMaterial = material;

	for (int i = 0; i < 6; i++) {
		VkCommandBuffer commandbuffer = HDRI2CubeMapFBO->BeginRendering();
		HDRI2CubeMapNode->Draw(commandbuffer, gCaptureProjectionMatrix, gCaptureCameras[i]);
		vkCmdEndRenderPass(commandbuffer);
		VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT ,0,1,i,1 };
		TransferImageLayout(commandbuffer, inoutCubeMap->mImage, subresourceRange,
			VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		CopyRTImageToCubeMap(commandbuffer, HDRI2CubeMapFBO->mAttachments[0]->mImage, inCubeMapResolution, inCubeMapResolution, inoutCubeMap->mImage, i, 0);
		TransferImageLayout(commandbuffer, inoutCubeMap->mImage, subresourceRange,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		EndOneTimeCommandBuffer(commandbuffer);
	}
}
Texture* LoadHDRICubeMapFromFile(const char* inFilePath, int inCubeMapResolution, const char* inVSFilePath, const char* inFSFilePath) {
	Texture*texture = new Texture(VK_FORMAT_R32G32B32A32_SFLOAT);
	texture->mFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
	GenImageCube(texture, inCubeMapResolution, inCubeMapResolution, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT);
	texture->mImageView = GenImageViewCube(texture->mImage, texture->mFormat, texture->mImageAspectFlag);
	HDRI2CubeMap(inFilePath, texture, inCubeMapResolution, inVSFilePath,inFSFilePath);
	return texture;
}
Texture* CaptureDiffuseIrradiance(Texture* inSrcCubeMap, int inCubeMapResolution, const char* inVSFilePath, const char* inFSFilePath) {
	Texture* texture = new Texture(VK_FORMAT_R32G32B32A32_SFLOAT);
	texture->mFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
	GenImageCube(texture, inCubeMapResolution, inCubeMapResolution, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT);
	texture->mImageView = GenImageViewCube(texture->mImage, texture->mFormat, texture->mImageAspectFlag);
	StaticMeshComponent* skyboxMesh = new StaticMeshComponent;
	skyboxMesh->LoadFromFile("Res/Model/skybox.staticmesh");

	FrameBufferEx* captureDiffuseIrradianceFBO = new FrameBufferEx;
	captureDiffuseIrradianceFBO->SetSize(inCubeMapResolution, inCubeMapResolution);
	captureDiffuseIrradianceFBO->AttachColorBuffer(VK_FORMAT_R32G32B32A32_SFLOAT);
	captureDiffuseIrradianceFBO->AttachDepthBuffer();
	captureDiffuseIrradianceFBO->Finish(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	gCaptureCameras[2].mViewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	gCaptureCameras[3].mViewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));

	Node* captureDiffuseIrradianceNode = new Node;
	captureDiffuseIrradianceNode->mModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	captureDiffuseIrradianceNode->mStaticMeshComponent = skyboxMesh;
	Material* material = new Material(inVSFilePath, inFSFilePath);
	material->mPipelineStateObject->mViewport = {
		0,0.0f,
		float(inCubeMapResolution),float(inCubeMapResolution),
		0.0f,1.0f
	};
	material->mPipelineStateObject->mScissor = {
		{0,0},{uint32_t(inCubeMapResolution),uint32_t(inCubeMapResolution)}
	};
	material->SetTexture(4, inSrcCubeMap->mImageView, GenCubeMapSampler());
	SetColorAttachmentCount(material->mPipelineStateObject, 1);
	material->mPipelineStateObject->mRenderPass = captureDiffuseIrradianceFBO->mRenderPass;
	material->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	captureDiffuseIrradianceNode->mMaterial = material;

	for (int i = 0; i < 6; i++) {
		VkCommandBuffer commandbuffer = captureDiffuseIrradianceFBO->BeginRendering();
		captureDiffuseIrradianceNode->Draw(commandbuffer, gCaptureProjectionMatrix, gCaptureCameras[i]);
		vkCmdEndRenderPass(commandbuffer);
		VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT ,0,1,i,1 };
		TransferImageLayout(commandbuffer, texture->mImage, subresourceRange,
			VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		CopyRTImageToCubeMap(commandbuffer, captureDiffuseIrradianceFBO->mAttachments[0]->mImage, inCubeMapResolution, inCubeMapResolution, texture->mImage, i, 0);
		TransferImageLayout(commandbuffer, texture->mImage, subresourceRange,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		EndOneTimeCommandBuffer(commandbuffer);
	}
	return texture;
}
Texture* CapturePrefilteredColor(Texture* inSrcCubeMap, int inCubeMapResolution, const char* inVSFilePath, const char* inFSFilePath) {
	Texture* texture = new Texture(VK_FORMAT_R32G32B32A32_SFLOAT);
	texture->mFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
	GenImageCube(texture, inCubeMapResolution, inCubeMapResolution, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT,5);
	texture->mImageView = GenImageViewCube(texture->mImage, texture->mFormat, texture->mImageAspectFlag,5);
	StaticMeshComponent* skyboxMesh = new StaticMeshComponent;
	skyboxMesh->LoadFromFile("Res/Model/skybox.staticmesh");

	FrameBufferEx* capturePrefilteredColorFBO = new FrameBufferEx;
	capturePrefilteredColorFBO->SetSize(inCubeMapResolution, inCubeMapResolution);
	capturePrefilteredColorFBO->AttachColorBuffer(VK_FORMAT_R32G32B32A32_SFLOAT);
	capturePrefilteredColorFBO->AttachDepthBuffer();
	capturePrefilteredColorFBO->Finish(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	Node* capturePrefilteredColorNode = new Node;
	capturePrefilteredColorNode->mModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	capturePrefilteredColorNode->mStaticMeshComponent = skyboxMesh;
	Material* material = new Material(inVSFilePath, inFSFilePath);
	material->mPipelineStateObject->mViewport = {
		0,0.0f,
		float(inCubeMapResolution),float(inCubeMapResolution),
		0.0f,1.0f
	};
	material->mPipelineStateObject->mScissor = {
		{0,0},{uint32_t(inCubeMapResolution),uint32_t(inCubeMapResolution)}
	};
	material->SetTexture(4, inSrcCubeMap->mImageView, GenCubeMapSampler());
	SetColorAttachmentCount(material->mPipelineStateObject, 1);
	material->mPipelineStateObject->mRenderPass = capturePrefilteredColorFBO->mRenderPass;
	material->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	capturePrefilteredColorNode->mMaterial = material;

	for (int mipmapLevel = 0; mipmapLevel < 5; mipmapLevel++) {
		float roughness = float(mipmapLevel) / float(4);
		for (int i = 0; i < 6; ++i) {
			float currentResolution=static_cast<float>(inCubeMapResolution * std::pow(0.5f, mipmapLevel));
			material->mPipelineStateObject->mViewport.y = 0.0f;
			material->mPipelineStateObject->mViewport.width = currentResolution;
			material->mPipelineStateObject->mViewport.height = currentResolution;
			VkCommandBuffer commandbuffer = capturePrefilteredColorFBO->BeginRendering();
			capturePrefilteredColorNode->mMaterial->SetVec4(0, roughness, 0.0f, 0.0f, 0.0f);
			capturePrefilteredColorNode->Draw(commandbuffer, gCaptureProjectionMatrix, gCaptureCameras[i]);
			vkCmdEndRenderPass(commandbuffer);
			VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT ,mipmapLevel,1,i,1 };
			TransferImageLayout(commandbuffer, texture->mImage, subresourceRange,
				VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
			CopyRTImageToCubeMap(commandbuffer, capturePrefilteredColorFBO->mAttachments[0]->mImage, currentResolution, currentResolution, texture->mImage, i, mipmapLevel);
			TransferImageLayout(commandbuffer, texture->mImage, subresourceRange,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
			EndOneTimeCommandBuffer(commandbuffer);
		}
	}
	return texture;
}
Texture* GenerateBRDF(int inResolution, const char* inVSFilePath , const char* inFSFilePath) {
	FullScreenQuadMeshComponent* fsqMesh = new FullScreenQuadMeshComponent;
	fsqMesh->Init();

	FrameBufferEx* generateBRDFFBO = new FrameBufferEx;
	generateBRDFFBO->SetSize(inResolution, inResolution);
	generateBRDFFBO->AttachColorBuffer(VK_FORMAT_R32G32_SFLOAT);
	generateBRDFFBO->AttachDepthBuffer();
	generateBRDFFBO->Finish();

	Node* generateBRDFNode = new Node;
	generateBRDFNode->mModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	generateBRDFNode->mStaticMeshComponent = fsqMesh;
	Material* material = new Material(inVSFilePath, inFSFilePath);
	material->mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	material->SetFrontFace(VK_FRONT_FACE_CLOCKWISE);
	material->mPipelineStateObject->mViewport = {
		0,0,
		(float)inResolution,(float)inResolution,
		0.0f,1.0f
	};
	material->mPipelineStateObject->mScissor = {
		{0,0},{(unsigned int)inResolution,(unsigned int)inResolution}
	};
	SetColorAttachmentCount(material->mPipelineStateObject, 1);
	material->mPipelineStateObject->mRenderPass = generateBRDFFBO->mRenderPass;
	material->mPipelineStateObject->mSampleCount = VK_SAMPLE_COUNT_1_BIT;
	generateBRDFNode->mMaterial = material;

	VkCommandBuffer commandbuffer = generateBRDFFBO->BeginRendering();
	generateBRDFNode->Draw(commandbuffer, gCaptureProjectionMatrix, gCaptureCameras[0]);
	vkCmdEndRenderPass(commandbuffer);
	EndOneTimeCommandBuffer(commandbuffer);
	return generateBRDFFBO->mAttachments[0];
}
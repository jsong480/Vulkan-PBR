#include "Material.h"

VkSampler Material::mDefaultSampler = nullptr;

Material::Material(const char* inVSPath, const char* inFSPath) {
	mGPUProgram = new GPUProgram;
	mGPUProgram->AttachShader(VK_SHADER_STAGE_VERTEX_BIT, inVSPath);
	mGPUProgram->AttachShader(VK_SHADER_STAGE_FRAGMENT_BIT, inFSPath);
	if (mDefaultSampler == nullptr) {
		mDefaultSampler= GenSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	}
	mbEnableDepthTest = true;
	mbEnableDepthWrite = true;
	mFrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	mPipelineStateObject = new PipelineStateObject;
	mbNeedUpdatePSO = true;
	mVec4s = new Vec4UniformBufferData(4096);

	mDescriptorPool = InitDescriptorPool();
	mDescriptorSet = InitDescriptorSet(mDescriptorPool);

	mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}
void Material::SetFrontFace(VkFrontFace inVkFrontFace) {
	mFrontFace = inVkFrontFace;
	mbNeedUpdatePSO = true;
}
void Material::EnableDepthTest(bool inEnable) {
	mbEnableDepthTest = inEnable;
	mbNeedUpdatePSO = true;
}
void Material::Active(VkCommandBuffer inCommandBuffer, int inVertexDataSize) {
	if (mVec4s->mUBO == nullptr) {
		mVec4s->UpdateGPUData();
		SetUBO(3, mVec4s->mUBO);
	}
	else {
		mVec4s->UpdateGPUData();
	}
	if (mbNeedUpdatePSO) {
		mbNeedUpdatePSO = false;
		CreateGraphicPipeline(mPipelineStateObject,inVertexDataSize,mGPUProgram,mbEnableDepthTest,mbEnableDepthWrite,mFrontFace, mPrimitiveTopology);
	}
	for (int i = 0; i < mWriteDescriptor.size(); ++i) {
		mWriteDescriptor[i].dstSet = mDescriptorSet;
	}
	vkUpdateDescriptorSets(GetGlobalConfig().mLogicDevice, static_cast<uint32_t>(mWriteDescriptor.size()), mWriteDescriptor.data(), 0, nullptr);

	vkCmdBindPipeline(inCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineStateObject->mPipeline);
	vkCmdBindDescriptorSets(inCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineStateObject->mPipelineLayout, 0, 1, &mDescriptorSet, 0, nullptr);
}
void Material::SetTexture(int inBindingPoint, VkImageView inVkImageView, VkSampler inVkSampler/* =nullptr */) {
	if (inVkSampler == nullptr) {
		inVkSampler = mDefaultSampler;
	}
	VkDescriptorImageInfo* imageInfo = new VkDescriptorImageInfo;
	imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo->imageView = inVkImageView;
	imageInfo->sampler = inVkSampler;

	VkWriteDescriptorSet descriptorWriter = {};
	descriptorWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWriter.dstSet = mDescriptorSet;
	descriptorWriter.dstBinding = inBindingPoint;
	descriptorWriter.dstArrayElement = 0;
	descriptorWriter.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWriter.descriptorCount = 1;
	descriptorWriter.pImageInfo = imageInfo;
	mWriteDescriptor.push_back(descriptorWriter);
}
void Material::SetUBO(int inBindingPosition, BufferObject* inUniformBufferObject) {
	VkDescriptorBufferInfo* bufferInfo = new VkDescriptorBufferInfo;
	bufferInfo->buffer = inUniformBufferObject->mBuffer;
	bufferInfo->offset = 0;
	bufferInfo->range = inUniformBufferObject->mSize;

	VkWriteDescriptorSet descriptorWriter = {};
	descriptorWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWriter.dstSet = mDescriptorSet;
	descriptorWriter.dstBinding = inBindingPosition;
	descriptorWriter.dstArrayElement = 0;
	descriptorWriter.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWriter.descriptorCount = 1;
	descriptorWriter.pBufferInfo = bufferInfo;
	mWriteDescriptor.push_back(descriptorWriter);
}
void Material::SetVec4(int inIndex, float* inVec4) {
	mVec4s->SetVec4(inIndex, inVec4);
}
void Material::SetVec4(int inIndex, float x, float y, float z, float w) {
	float v[4];
	v[0] = x;
	v[1] = y;
	v[2] = z;
	v[3] = w;
	SetVec4(inIndex, v);
}
void Material::SetCameraWorldPosition(float inX, float inY, float inZ, float inW /* = 1.0f */) {
	SetVec4(0, inX, inY, inZ, inW);
}
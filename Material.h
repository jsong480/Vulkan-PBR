#pragma once
#include "utils.h"
#include "MyVulkan.h"
class Material {
public:
	static VkSampler mDefaultSampler;
	GPUProgram* mGPUProgram;
	PipelineStateObject* mPipelineStateObject;
	VkPrimitiveTopology mPrimitiveTopology;
	std::vector<VkWriteDescriptorSet> mWriteDescriptor;
	VkDescriptorSet mDescriptorSet;
	VkDescriptorPool mDescriptorPool;
	Vec4UniformBufferData* mVec4s;
	bool mbEnableDepthTest;
	bool mbEnableDepthWrite;
	VkFrontFace mFrontFace;
	VkCullModeFlags mCullMode;
	bool mbNeedUpdatePSO;
	Material(const char *inVSPath,const char *inFSPath);
	void SetFrontFace(VkFrontFace inVkFrontFace);
	void EnableDepthTest(bool inEnable);
	void SetTexture(int inBindingPoint, VkImageView inVkImageView, VkSampler inVkSampler = nullptr);
	void SetUBO(int inBindingPosition, BufferObject* inUniformBufferObject);
	void SetVec4(int inIndex, float* v);
	void SetVec4(int inIndex, float x, float y, float z, float w = 0.0f);
	void SetCameraWorldPosition(float inX, float inY, float inZ, float inW = 1.0f);
	void Active(VkCommandBuffer inCommandBuffer,int inVertexDataSize);
};

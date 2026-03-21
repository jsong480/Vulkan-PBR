#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include <vector>
#include <array>
#include <set>
#include <map>
#include "glm/glm.hpp"
#include "glm/ext.hpp"

struct Texture {
	VkImage mImage;
	VkDeviceMemory mMemory;
	VkImageView mImageView;
	VkImageAspectFlags mImageAspectFlag;
	VkFormat mFormat;
	Texture(VkFormat inFormat,VkImageAspectFlags inImageAspectFlag = VK_IMAGE_ASPECT_COLOR_BIT);
	virtual ~Texture();
};
struct FrameBuffer {
	VkFramebuffer mFrameBuffer;
	std::vector<Texture*> mColorAttachments;
	Texture* mDepthAttachment;
	Texture* mResolveBuffer;
	VkSampleCountFlagBits mSampleCount;
	FrameBuffer();
	~FrameBuffer();
};
struct GPUProgram {
	int mShaderStageCount;
	VkPipelineShaderStageCreateInfo mShaderStage[2];
	GPUProgram();
	~GPUProgram();
	void AttachShader(VkShaderStageFlagBits inVkShaderStageFlagBits,const char *inFilePath);
};
struct BufferObject {
	VkBuffer mBuffer;
	VkDeviceMemory mMemory;
	int mSize;
	BufferObject() {
		mBuffer = nullptr;
		mMemory = nullptr;
		mSize = 0;
	}
	void Write(void*inData,int inDataSize=0);
};
struct Mat4UniformBufferData {
	int mMat4Count;
	float mData[16384];
	BufferObject* mUBO;
	bool mbNeedSyncData;
	Mat4UniformBufferData(int inMat4Count);
	void SetMat4(int inIndex, float* inMat4);
	void UpdateGPUData();
};
struct Vec4UniformBufferData {
	int mVec4Count;
	float mData[4096];
	BufferObject* mUBO;
	bool mbNeedSyncData;
	Vec4UniformBufferData(int inVec4Count);
	void SetVec4(int inIndex, float* inVec4);
	void UpdateGPUData();
};
struct UniformInputsBindings {
	static std::vector<VkDescriptorSetLayoutBinding> mDescriptorSetLayoutBindings;
	static std::vector<VkDescriptorPoolSize> mDescriptorPoolSize;
	static VkDescriptorSetLayout mDescriptorSetLayout;
	static int mDescriptorSetLayoutCount;
	static void Init();
	static void InitUniformInput(int inBindingPoint, VkShaderStageFlags inVkShaderStageFlags, VkDescriptorType inVkDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
};
struct PipelineStateObject {
public:
	static VkPipelineLayout mPipelineLayout;
	static void Init();

	VkRenderPass mRenderPass;
	VkSampleCountFlagBits mSampleCount;
	VkViewport mViewport;
	VkRect2D mScissor;
	std::vector<VkPipelineColorBlendAttachmentState> mColorBlendAttachmentStates;
	VkPipelineColorBlendStateCreateInfo mColorBlendState;
	float mDepthConstantFactor, mDepthClamp, mDepthSlopeFactor;
	VkPipeline mPipeline;
public:
	PipelineStateObject();
	~PipelineStateObject();
	void CleanUp();
};
struct GlobalConfig {
	void* mHWND;
	int mViewportWidth;
	int mViewportHeight;
	VkInstance mVulkanInstance;
	VkSurfaceKHR mVulkanSurface;
	VkPhysicalDevice mPhysicalDevice;
	VkDevice mLogicDevice;
	VkQueue mGraphicQueue, mPresentQueue;
	int mGraphicQueueFamilyIndex, mPresentQueueFamilyIndex;
	VkSampleCountFlagBits mCurrentPhysicalDeviceMaxSampleCount, mPreferedSampleCount;
	VkSurfaceCapabilitiesKHR mSurfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> mSurfaceFormats;
	std::vector<VkPresentModeKHR> mPresentModes;
	char** mEnabledLayers;
	int mEnabledLayerCount;
	VkSwapchainKHR mSwapChain;
	VkFormat mSwapChainImageFormat;
	VkExtent2D mSwapChainExtent;
	std::vector<VkImage> mSwapChainImages;
	std::vector<VkImageView> mSwapChainImageViews;
	VkRenderPass mSystemRenderPass;
	FrameBuffer* mSystemFrameBuffers;
	int mSystemFrameBufferCount; 
	VkCommandPool mCommandPool;
	VkSemaphore mReadyToRender;
	VkSemaphore mReadyToPresent;
	VkCommandBuffer mSystemRenderPassCommandBuffer;
};
void GenImage(Texture* texture, uint32_t w, uint32_t h, VkImageUsageFlags usage, VkSampleCountFlagBits sample_count, int mipmaplevel=1);
VkImageView GenImageView2D(VkImage inImage,VkFormat inFormat, VkImageAspectFlags inImageAspectFlags, int mipmap_level=1);
void SubmitImage2D(Texture* texture, int width, int height, const void* pixel);
VkSampler GenSampler(VkFilter inMinFilter, VkFilter inMagFilter, VkSamplerAddressMode inSamplerAddressModeU, VkSamplerAddressMode inSamplerAddressModeV, VkSamplerAddressMode inSamplerAddressModeW);
VkSampler GenCubeMapSampler(VkFilter inMinFilter= VK_FILTER_LINEAR, VkFilter inMagFilter= VK_FILTER_LINEAR, 
	VkSamplerAddressMode inSamplerAddressModeU= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
	VkSamplerAddressMode inSamplerAddressModeV= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 
	VkSamplerAddressMode inSamplerAddressModeW= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
void GenImageCube(Texture* texture, uint32_t w, uint32_t h, VkImageUsageFlags usage, VkSampleCountFlagBits sample_count, int mipmaplevel = 1);
VkImageView GenImageViewCube(VkImage inImage, VkFormat inFormat, VkImageAspectFlags inImageAspectFlags, int mipmap_level = 1);
void TransferImageLayout(VkCommandBuffer inCommandBuffer, VkImage inImage, VkImageSubresourceRange inSubresourceRange,
	VkImageLayout inOldLayout, VkAccessFlags inOldAccessFlags, VkPipelineStageFlags inSrcStageMask,
	VkImageLayout inNewLayout, VkAccessFlags inNewAccessFlags, VkPipelineStageFlags inDstStageMask);
GlobalConfig& GetGlobalConfig();
bool InitVulkan(void* param, int width, int height);
void OnViewportChangedVulkan(int inWidth, int inHeight);
VkResult GenCommandBuffer(VkCommandBuffer* command_buffer, int count, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
void DeleteCommandBuffer(VkCommandBuffer* command_buffer, int count);
VkCommandBuffer BeginRendering(VkCommandBuffer cmd = nullptr);
void EndRendering(); 
void SwapBuffers();
VkResult BeginOneTimeCommandBuffer(VkCommandBuffer* command_buffer);
void WaitForCommmandFinish(VkCommandBuffer commandbuffer);
VkResult EndOneTimeCommandBuffer(VkCommandBuffer command_buffer);
Texture* LoadTextureFromFile(const char* inFilePath);
VkShaderModule InitShaderWithCode(unsigned char* inCode, int inCodeLenInBytes);
VkResult GenBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
BufferObject* CreateBuffer(VkDeviceSize inVkDeviceSize,VkBufferUsageFlags inVkBufferUsageFlags, VkMemoryPropertyFlags inVkMemoryPropertyFlags);
void BufferSubData(VkBuffer buffer, VkBufferUsageFlags usage, const  void* data, VkDeviceSize size);
VkDescriptorPool InitDescriptorPool();
VkDescriptorSet InitDescriptorSet(VkDescriptorPool inVkDescriptorPool);
void SetColorAttachmentCount(PipelineStateObject* inPSO, int count);
void CreateGraphicPipeline(PipelineStateObject* inPSO, int inVertexDataSize,GPUProgram*inGPUProgram, bool inEnableDepthTest=true,bool inEnableDepthWrite=true,VkFrontFace inVkFrontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE,VkPrimitiveTopology inVkPrimitiveTopology= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
void SetDynamicState(PipelineStateObject* inPipelineStateObject, VkCommandBuffer commandbuffer);
void MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData);
void UnmapMemory(VkDeviceMemory memory);
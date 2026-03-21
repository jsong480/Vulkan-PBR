#pragma once
#include "MyVulkan.h"
class FrameBufferEx {
public:
	VkFramebuffer mFBO;
	VkRenderPass mRenderPass;
	uint32_t mWidth, mHeight;
	std::vector<Texture*>mAttachments;
	std::vector<VkClearValue> mClearValues;
	int mColorBufferCount;
	int mDepthBufferIndex;
	Texture*mDepthBuffer;
public:
	FrameBufferEx();
	~FrameBufferEx();
	void SetSize(uint32_t width, uint32_t height);
	void AttachColorBuffer(VkFormat inFormat = VK_FORMAT_R8G8B8A8_UNORM);
	void AttachDepthBuffer();
	void Finish(VkImageLayout inColorBufferFinalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	void SetClearColor(int index, float r, float g, float b, float a);
	void SetClearDepthStencil(float depth, uint32_t stencil);
	VkCommandBuffer BeginRendering(VkCommandBuffer commandbuffer = nullptr);
};
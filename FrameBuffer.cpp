#include "FrameBuffer.h"

FrameBufferEx::FrameBufferEx() {
	mFBO = nullptr;
	mRenderPass = nullptr;
	mDepthBuffer = nullptr;
}
FrameBufferEx::~FrameBufferEx() {
	for (auto iter=mAttachments.begin();iter!=mAttachments.end();++iter){
		delete *iter;
	}
	GlobalConfig& globalConfig = GetGlobalConfig();
	if (mRenderPass != nullptr) {
		vkDestroyRenderPass(globalConfig.mLogicDevice, mRenderPass, nullptr);
	}
	if (mFBO!= nullptr){
		vkDestroyFramebuffer(globalConfig.mLogicDevice, mFBO, nullptr);
	}
}
void FrameBufferEx::SetSize(uint32_t width, uint32_t height) {
	mWidth = width;
	mHeight = height;
}
void FrameBufferEx::AttachColorBuffer(VkFormat inFormat /* = VK_FORMAT_R8G8B8A8_UNORM */) {
	Texture*color_buffer = new Texture(inFormat);
	color_buffer->mFormat = inFormat;
	GenImage(color_buffer, mWidth, mHeight, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT ,VK_SAMPLE_COUNT_1_BIT);
	color_buffer->mImageView=GenImageView2D(color_buffer->mImage,color_buffer->mFormat,color_buffer->mImageAspectFlag);
	mColorBufferCount++;
	mAttachments.push_back(color_buffer);
	VkClearValue cv;
	cv.color = { 0.0f,0.0f,0.0f,0.0f };
	mClearValues.push_back(cv);
}
void FrameBufferEx::AttachDepthBuffer() {
	Texture *depth_buffer = new Texture(VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_ASPECT_DEPTH_BIT);
	depth_buffer->mFormat = VK_FORMAT_D24_UNORM_S8_UINT;
	GenImage(depth_buffer, mWidth, mHeight, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT);
	depth_buffer->mImageView = GenImageView2D(depth_buffer->mImage, depth_buffer->mFormat, depth_buffer->mImageAspectFlag);
	mDepthBufferIndex = mAttachments.size();
	mAttachments.push_back(depth_buffer);
	VkClearValue cv;
	cv.depthStencil = {1.0f,0};
	mClearValues.push_back(cv);
	mDepthBuffer = depth_buffer;
}
// 创建 RenderPass（MRT + depth）与 Framebuffer；颜色附件最终 layout 可指定（如 transfer src）
void FrameBufferEx::Finish(VkImageLayout inColorBufferFinalLayout) {
	std::vector<VkAttachmentDescription> attachments;
	std::vector<VkAttachmentReference> colorattachment_refences;
	VkAttachmentReference depthattachment_ref = {};
	std::vector<VkImageView> render_targets;
	render_targets.resize(mAttachments.size());
	attachments.resize(mAttachments.size());
	int color_buffer_count = 0;
	for (size_t i=0;i<mAttachments.size();++i){
		Texture*texture = mAttachments[i];
		if (texture->mImageAspectFlag==VK_IMAGE_ASPECT_COLOR_BIT){
			color_buffer_count++;
		}else if (texture->mImageAspectFlag==VK_IMAGE_ASPECT_DEPTH_BIT){

		}
		render_targets[i] = texture->mImageView;
	}
	colorattachment_refences.resize(color_buffer_count);
	int color_buffer_index = 0;
	int attachment_point = 0;
	for (size_t i=0;i<mAttachments.size();++i){
		Texture*texture = mAttachments[i];
		if (texture->mImageAspectFlag==VK_IMAGE_ASPECT_COLOR_BIT){
			attachments[i] = {
				0,
				texture->mFormat,
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_STORE,
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				inColorBufferFinalLayout
			};
			colorattachment_refences[color_buffer_index++] = {
				uint32_t(attachment_point++),VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};
		}else if (texture->mImageAspectFlag==VK_IMAGE_ASPECT_DEPTH_BIT){
			attachments[i] = {
				0,
				texture->mFormat,
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_STORE,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
			depthattachment_ref.attachment = attachment_point++;
			depthattachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
	}

	VkSubpassDescription subpasses = {};
	subpasses.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses.colorAttachmentCount = colorattachment_refences.size();
	subpasses.pColorAttachments = colorattachment_refences.data();
	subpasses.pDepthStencilAttachment = &depthattachment_ref;

	VkRenderPassCreateInfo rpci = {};
	rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rpci.attachmentCount = attachments.size();
	rpci.pAttachments = attachments.data();
	rpci.subpassCount = 1;
	rpci.pSubpasses = &subpasses;
	GlobalConfig& globalConfig = GetGlobalConfig();
	vkCreateRenderPass(globalConfig.mLogicDevice, &rpci, nullptr,&mRenderPass);
	VkFramebufferCreateInfo fbci = {};
	fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbci.pAttachments = render_targets.data();
	fbci.attachmentCount = render_targets.size();
	fbci.width = mWidth;
	fbci.height = mHeight;
	fbci.layers = 1;
	fbci.renderPass = mRenderPass;
	vkCreateFramebuffer(globalConfig.mLogicDevice, &fbci, nullptr, &mFBO);
}
void FrameBufferEx::SetClearColor(int index, float r, float g, float b, float a) {
	mClearValues[index].color = {r,g,b,a};
}
void FrameBufferEx::SetClearDepthStencil(float depth, uint32_t stencil) {
	mClearValues[mDepthBufferIndex].depthStencil = { depth,stencil };
}
VkCommandBuffer FrameBufferEx::BeginRendering(VkCommandBuffer commandbuffer) {
	VkCommandBuffer cmd;
	if (commandbuffer != nullptr) {
		cmd = commandbuffer;
	}
	else {
		BeginOneTimeCommandBuffer(&cmd);
	}
	VkFramebuffer render_target = mFBO;
	VkRenderPass render_pass = mRenderPass;

	VkRenderPassBeginInfo rpbi = {};
	rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpbi.framebuffer = render_target;
	rpbi.renderPass = render_pass;
	rpbi.renderArea.offset = { 0,0 };
	rpbi.renderArea.extent = { mWidth,mHeight };
	rpbi.clearValueCount = mClearValues.size();
	rpbi.pClearValues = mClearValues.data();
	vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
	return cmd;
}
// Vulkan 封装：Instance/Device、Swapchain、命令缓冲、纹理与 Graphics Pipeline
#include "MyVulkan.h"
#include "Utils.h"
#include "Mesh.h"
#pragma comment(lib,"vulkan-1.lib")
static GlobalConfig sGlobalConfig = {};
static const char* sPreferedLayers[] = {
	"VK_LAYER_LUNARG_standard_validation",
	"VK_LAYER_KHRONOS_validation"
};
static const char* sEnabledExtensions[] = {
	VK_KHR_SURFACE_EXTENSION_NAME ,
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME
};
static const char* sEnabledLogicDeviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,VK_EXT_NON_SEAMLESS_CUBE_MAP_EXTENSION_NAME };
static PFN_vkCreateDebugReportCallbackEXT __vkCreateDebugReportCallback = nullptr;
static PFN_vkDestroyDebugReportCallbackEXT __vkDestroyDebugReportCallback = nullptr;
static PFN_vkCreateWin32SurfaceKHR __vkCreateWin32SurfaceKHR = nullptr;
#define vkCreateDebugReportCallback __vkCreateDebugReportCallback
#define vkDestroyDebugReportCallback __vkDestroyDebugReportCallback
#define vkCreateWin32SurfaceKHR __vkCreateWin32SurfaceKHR
static VkDebugReportCallbackEXT sVulkanDebugger;

std::vector<VkDescriptorSetLayoutBinding> UniformInputsBindings::mDescriptorSetLayoutBindings;
std::vector<VkDescriptorPoolSize> UniformInputsBindings::mDescriptorPoolSize;
VkDescriptorSetLayout UniformInputsBindings::mDescriptorSetLayout;
int UniformInputsBindings::mDescriptorSetLayoutCount=1;

VkPipelineLayout PipelineStateObject::mPipelineLayout = nullptr;

Mat4UniformBufferData::Mat4UniformBufferData(int inMat4Count) {
	mMat4Count = inMat4Count;
	mbNeedSyncData = true;
	mUBO = 0;
}
void Mat4UniformBufferData::SetMat4(int inIndex, float* inMat4) {
	int offset = inIndex * 16;
	memcpy(mData + offset, inMat4, sizeof(float) * 16);
	mbNeedSyncData = true;
}
void Mat4UniformBufferData::UpdateGPUData() {
	if (!mbNeedSyncData) {
		return;
	}
	if (mUBO == nullptr) {
		mUBO = CreateBuffer(sizeof(mData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
	mbNeedSyncData = false;
	mUBO->Write(mData, sizeof(mData));
}
Vec4UniformBufferData::Vec4UniformBufferData(int inVec4Count) {
	mVec4Count = inVec4Count;
	mbNeedSyncData = true;
	mUBO = 0;
}
void Vec4UniformBufferData::SetVec4(int inIndex, float* inVec4) {
	int offset = inIndex * 4;
	memcpy(mData + offset, inVec4, sizeof(float) * 4);
	mbNeedSyncData = true;
}
void Vec4UniformBufferData::UpdateGPUData() {
	if (!mbNeedSyncData) {
		return;
	}
	if (mUBO == nullptr) {
		mUBO = CreateBuffer(sizeof(mData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
	mbNeedSyncData = false;
	mUBO->Write(mData, sizeof(mData));
}

// 与 shader 中 binding 对应：0~2 mat4、3 vec4 block、4~13 纹理
void UniformInputsBindings::Init() {
	InitUniformInput(0, VK_SHADER_STAGE_VERTEX_BIT);
	InitUniformInput(1, VK_SHADER_STAGE_VERTEX_BIT);
	InitUniformInput(2, VK_SHADER_STAGE_VERTEX_BIT);
	InitUniformInput(3, VK_SHADER_STAGE_FRAGMENT_BIT);
	InitUniformInput(4, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(5, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(6, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(7, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(8, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(9, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(10, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(11, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(12, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	InitUniformInput(13, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(mDescriptorSetLayoutBindings.size());
	layoutInfo.pBindings = mDescriptorSetLayoutBindings.data();
	if (vkCreateDescriptorSetLayout(sGlobalConfig.mLogicDevice, &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS) {
		printf("failed to create descriptor set layout!\n");
	}
}
void UniformInputsBindings::InitUniformInput(int inBindingPoint, VkShaderStageFlags inVkShaderStageFlags, VkDescriptorType inVkDescriptorType) {
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
	descriptorSetLayoutBinding.binding = inBindingPoint;
	descriptorSetLayoutBinding.descriptorType = inVkDescriptorType;
	descriptorSetLayoutBinding.descriptorCount = 1;
	descriptorSetLayoutBinding.stageFlags = inVkShaderStageFlags;
	descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
	mDescriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);

	VkDescriptorPoolSize descriptorPoolSize = {};
	descriptorPoolSize.type = inVkDescriptorType;
	descriptorPoolSize.descriptorCount = 1;
	mDescriptorPoolSize.push_back(descriptorPoolSize);
}
void PipelineStateObject::Init() {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pSetLayouts = &UniformInputsBindings::mDescriptorSetLayout;
	pipelineLayoutInfo.setLayoutCount = UniformInputsBindings::mDescriptorSetLayoutCount;
	if (vkCreatePipelineLayout(sGlobalConfig.mLogicDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS) {
		printf("create pipeline layout fail\n");
	}
}
uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(sGlobalConfig.mPhysicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return 0;
}

void InitSrcAccessMask(VkImageLayout oldLayout, VkImageMemoryBarrier& barrier) {
	switch (oldLayout) {
	case VK_IMAGE_LAYOUT_UNDEFINED:
		barrier.srcAccessMask = 0;
		break;
	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		printf("InitSrcAccessMask no %d\n", oldLayout);
		break;
	}
}
void InitDstAccessMask(VkImageLayout newLayout, VkImageMemoryBarrier& barrier) {
	switch (newLayout) {
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		if (barrier.srcAccessMask == 0) {
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		printf("InitDstAccessMask no %d\n", newLayout);
		break;
	}
}
void TransferImageLayout(VkCommandBuffer inCommandBuffer, VkImage inImage, VkImageSubresourceRange inSubresourceRange,
	VkImageLayout inOldLayout, VkAccessFlags inOldAccessFlags, VkPipelineStageFlags inSrcStageMask,
	VkImageLayout inNewLayout, VkAccessFlags inNewAccessFlags, VkPipelineStageFlags inDstStageMask) {
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.oldLayout = inOldLayout;
	barrier.srcAccessMask = inNewAccessFlags;
	barrier.dstAccessMask = inDstStageMask;
	barrier.newLayout = inNewLayout;
	barrier.image = inImage;
	barrier.subresourceRange = inSubresourceRange;
	vkCmdPipelineBarrier(inCommandBuffer, inSrcStageMask, inDstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}
void GenImage(Texture* texture, uint32_t w, uint32_t h, VkImageUsageFlags usage, VkSampleCountFlagBits sample_count, int mipmaplevel) {
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent = { w,h,1 };
	imageInfo.mipLevels = mipmaplevel;
	imageInfo.arrayLayers = 1;
	imageInfo.format = texture->mFormat;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = sample_count;
	if (vkCreateImage(sGlobalConfig.mLogicDevice, &imageInfo, nullptr, &texture->mImage) != VK_SUCCESS) {
		printf("failed to create image!\n");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(sGlobalConfig.mLogicDevice, texture->mImage, &memRequirements);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (vkAllocateMemory(sGlobalConfig.mLogicDevice, &allocInfo, nullptr, &texture->mMemory) != VK_SUCCESS) {
		printf("failed to allocate image memory!\n");
	}
	vkBindImageMemory(sGlobalConfig.mLogicDevice, texture->mImage, texture->mMemory, 0);
}
void SubmitBufferToImage(VkBuffer buffer, Texture* texture, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer;
	BeginOneTimeCommandBuffer(&commandBuffer);
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = texture->mImageAspectFlag;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width,height,1 };
	vkCmdCopyBufferToImage(commandBuffer, buffer, texture->mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	EndOneTimeCommandBuffer(commandBuffer);
}
void SubmitImage2D(Texture* texture, int width, int height, const void* pixel) {
	VkDeviceSize imageSize = width * height;
	if (texture->mFormat == VK_FORMAT_R8G8B8A8_UNORM) {
		imageSize *= 4;
	}
	else if (texture->mFormat == VK_FORMAT_R32G32B32A32_SFLOAT) {
		imageSize *= 16;
	}
	VkBuffer tempbuffer;
	VkDeviceMemory tempmemory;
	GenBuffer(tempbuffer, tempmemory, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void* data;
	vkMapMemory(sGlobalConfig.mLogicDevice, tempmemory, 0, imageSize, 0, &data);
	memcpy(data, pixel, static_cast<size_t>(imageSize));
	vkUnmapMemory(sGlobalConfig.mLogicDevice, tempmemory);

	VkCommandBuffer commandbuffer;
	BeginOneTimeCommandBuffer(&commandbuffer);
	VkImageSubresourceRange subresourceRange = { texture->mImageAspectFlag ,0,1,0,1 };
	TransferImageLayout(commandbuffer, texture->mImage, subresourceRange,
		VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	EndOneTimeCommandBuffer(commandbuffer);

	SubmitBufferToImage(tempbuffer, texture, width, height);

	BeginOneTimeCommandBuffer(&commandbuffer);
	TransferImageLayout(commandbuffer, texture->mImage, subresourceRange,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	EndOneTimeCommandBuffer(commandbuffer);

	vkDestroyBuffer(sGlobalConfig.mLogicDevice, tempbuffer, nullptr);
	vkFreeMemory(sGlobalConfig.mLogicDevice, tempmemory, nullptr);
}
VkImageView GenImageView2D(VkImage inImage,VkFormat inFormat, VkImageAspectFlags inImageAspectFlags, int inMipmapLevelCount) {
	VkImageView imageView;
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = inImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = inFormat;
	viewInfo.subresourceRange.aspectMask = inImageAspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = inMipmapLevelCount;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	if (vkCreateImageView(sGlobalConfig.mLogicDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		printf("failed to create texture image view!");
		return nullptr;
	}
	return imageView;
}
VkSampler GenSampler(VkFilter inMinFilter,VkFilter inMagFilter,VkSamplerAddressMode inSamplerAddressModeU, VkSamplerAddressMode inSamplerAddressModeV, VkSamplerAddressMode inSamplerAddressModeW) {
	VkSampler sampler=nullptr;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.minFilter = inMinFilter;
	samplerInfo.magFilter = inMagFilter;
	samplerInfo.addressModeU = inSamplerAddressModeU;
	samplerInfo.addressModeV = inSamplerAddressModeV;
	samplerInfo.addressModeW = inSamplerAddressModeW;
	samplerInfo.anisotropyEnable = false;
	samplerInfo.maxAnisotropy = 0.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	if (vkCreateSampler(sGlobalConfig.mLogicDevice, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
		printf("failed to create texture sampler!");
		return nullptr;
	}
	return sampler;
}

void GenImageCube(Texture* texture, uint32_t w, uint32_t h, VkImageUsageFlags usage, VkSampleCountFlagBits sample_count, int mipmaplevel) {
	VkImageCreateInfo ici = {};
	ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ici.imageType = VK_IMAGE_TYPE_2D;
	ici.extent = { w,h,1 };
	ici.mipLevels = mipmaplevel;
	ici.arrayLayers = 6;
	ici.format = texture->mFormat;
	ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ici.usage = usage;
	ici.samples = sample_count;
	ici.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	if (vkCreateImage(sGlobalConfig.mLogicDevice, &ici, nullptr, &texture->mImage) != VK_SUCCESS) {
		printf("failed to create image\n");
	}
	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(sGlobalConfig.mLogicDevice, texture->mImage, &memory_requirements);
	VkMemoryAllocateInfo mai = {};
	mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mai.allocationSize = memory_requirements.size;
	mai.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(sGlobalConfig.mLogicDevice, &mai, nullptr, &texture->mMemory);
	vkBindImageMemory(sGlobalConfig.mLogicDevice, texture->mImage, texture->mMemory, 0);
}
VkImageView GenImageViewCube(VkImage inImage, VkFormat inFormat, VkImageAspectFlags inImageAspectFlags, int mipmap_level) {
	VkImageView imageView=nullptr;
	VkImageViewCreateInfo ivci = {};
	ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ivci.image = inImage;
	ivci.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	ivci.format = inFormat;
	ivci.subresourceRange.aspectMask = inImageAspectFlags;
	ivci.subresourceRange.baseMipLevel = 0;
	ivci.subresourceRange.levelCount = mipmap_level;
	ivci.subresourceRange.baseArrayLayer = 0;
	ivci.subresourceRange.layerCount = 6;
	ivci.components = { VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_G,VK_COMPONENT_SWIZZLE_B,VK_COMPONENT_SWIZZLE_A };
	vkCreateImageView(sGlobalConfig.mLogicDevice, &ivci, nullptr, &imageView);
	return imageView;
}
VkSampler GenCubeMapSampler(VkFilter inMinFilter, VkFilter inMagFilter, VkSamplerAddressMode inSamplerAddressModeU, VkSamplerAddressMode inSamplerAddressModeV, VkSamplerAddressMode inSamplerAddressModeW) {
	VkSampler sampler = nullptr;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.minFilter = inMinFilter;
	samplerInfo.magFilter = inMagFilter;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = inSamplerAddressModeU;
	samplerInfo.addressModeV = inSamplerAddressModeV;
	samplerInfo.addressModeW = inSamplerAddressModeW;
	samplerInfo.anisotropyEnable = false;
	samplerInfo.maxAnisotropy = 0.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.flags = VK_SAMPLER_CREATE_NON_SEAMLESS_CUBE_MAP_BIT_EXT;
	if (vkCreateSampler(sGlobalConfig.mLogicDevice, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
		printf("failed to create texture sampler!");
		return nullptr;
	}
	return sampler;
}

VkShaderModule InitShaderWithCode(unsigned char* inCode, int inCodeLenInBytes) {
	VkShaderModule shader = nullptr;
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pCode = (uint32_t*)inCode;
	createInfo.codeSize = inCodeLenInBytes;
	if (vkCreateShaderModule(sGlobalConfig.mLogicDevice, &createInfo, nullptr, &shader) != VK_SUCCESS) {
		printf("create render shader module fail\n");
		return nullptr;
	}
	return shader;
}
VkResult GenBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkResult ret = vkCreateBuffer(sGlobalConfig.mLogicDevice, &bufferInfo, nullptr, &buffer);
	if (ret != VK_SUCCESS) {
		printf("failed to create buffer!\n");
		return ret;
	}
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(sGlobalConfig.mLogicDevice, buffer, &memRequirements);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
	ret = vkAllocateMemory(sGlobalConfig.mLogicDevice, &allocInfo, nullptr, &bufferMemory);
	if (ret != VK_SUCCESS) {
		printf("failed to allocate buffer memory!");
		return ret;
	}
	vkBindBufferMemory(sGlobalConfig.mLogicDevice, buffer, bufferMemory, 0);
	return VK_SUCCESS;
}
BufferObject* CreateBuffer(VkDeviceSize inVkDeviceSize, VkBufferUsageFlags inVkBufferUsageFlags, VkMemoryPropertyFlags inVkMemoryPropertyFlags) {
	BufferObject* bufferObject = new BufferObject;
	bufferObject->mSize = inVkDeviceSize;
	GenBuffer(bufferObject->mBuffer, bufferObject->mMemory, inVkDeviceSize, inVkBufferUsageFlags, inVkMemoryPropertyFlags);
	return bufferObject;
}
void CopyBuffer(VkBuffer src, VkBuffer dst, int size) {
	VkCommandBuffer commandBuffer;
	BeginOneTimeCommandBuffer(&commandBuffer);
	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);
	EndOneTimeCommandBuffer(commandBuffer);
}
void BufferSubData(VkBuffer buffer, VkBufferUsageFlags usage, const  void* data, VkDeviceSize size) {
	VkBuffer tempbuffer;
	VkDeviceMemory tempbuffer_memory;
	GenBuffer(tempbuffer, tempbuffer_memory, size, usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void* dst;
	vkMapMemory(sGlobalConfig.mLogicDevice, tempbuffer_memory, 0, size, 0, &dst);
	memcpy(dst, data, size_t(size));
	vkUnmapMemory(sGlobalConfig.mLogicDevice, tempbuffer_memory);
	CopyBuffer(tempbuffer, buffer, size);
	vkDestroyBuffer(sGlobalConfig.mLogicDevice, tempbuffer, nullptr);
	vkFreeMemory(sGlobalConfig.mLogicDevice, tempbuffer_memory, nullptr);
}

VkDescriptorSetLayout InitDescriptorSetLayout() {
	VkDescriptorSetLayout descriptorSetLayout=nullptr;
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(UniformInputsBindings::mDescriptorSetLayoutBindings.size());
	layoutInfo.pBindings = UniformInputsBindings::mDescriptorSetLayoutBindings.data();
	if (vkCreateDescriptorSetLayout(sGlobalConfig.mLogicDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		printf("failed to create descriptor set layout!\n");
		return nullptr;
	}
	return descriptorSetLayout;
}
VkDescriptorPool InitDescriptorPool() {
	VkDescriptorPool descriptorPool=nullptr;
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(UniformInputsBindings::mDescriptorPoolSize.size());
	poolInfo.pPoolSizes = UniformInputsBindings::mDescriptorPoolSize.data();
	poolInfo.maxSets = 1;
	if (vkCreateDescriptorPool(sGlobalConfig.mLogicDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		printf("failed to create descriptor pool!\n");
	}
	return descriptorPool;
}
VkDescriptorSet InitDescriptorSet(VkDescriptorPool inVkDescriptorPool) {
	VkDescriptorSet descriptorSet = nullptr;
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = inVkDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &UniformInputsBindings::mDescriptorSetLayout;
	if (vkAllocateDescriptorSets(sGlobalConfig.mLogicDevice, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		printf("failed to allocate descriptor sets!\n");
	}
	return descriptorSet;
}
void SetColorAttachmentCount(PipelineStateObject*inPSO, int count) {
	inPSO->mColorBlendAttachmentStates.resize(count);
	for (int i = 0; i < count; i++) {
		inPSO->mColorBlendAttachmentStates[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		inPSO->mColorBlendAttachmentStates[i].blendEnable = VK_FALSE;
		inPSO->mColorBlendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		inPSO->mColorBlendAttachmentStates[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		inPSO->mColorBlendAttachmentStates[i].colorBlendOp = VK_BLEND_OP_ADD;
		inPSO->mColorBlendAttachmentStates[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		inPSO->mColorBlendAttachmentStates[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		inPSO->mColorBlendAttachmentStates[i].alphaBlendOp = VK_BLEND_OP_ADD;
	}
}
void CreateGraphicPipeline(PipelineStateObject* inPSO, int inVertexDataSize, GPUProgram* inGPUProgram, bool inEnableDepthTest, bool inEnableDepthWrite, VkFrontFace inVkFrontFace,VkPrimitiveTopology inVkPrimitiveTopology,VkCullModeFlags inCullMode) {
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = inVertexDataSize;
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	attributeDescriptions.resize(inVertexDataSize/(sizeof(float)*4));
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[0].offset = 0;

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[1].offset = sizeof(float) * 4;

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[2].offset = sizeof(float) * 8;

	if (inVertexDataSize == sizeof(StaticMeshVertexDataEx)) {
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[3].offset = sizeof(float) * 12;
	}

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
	inPSO->mColorBlendState.attachmentCount = inPSO->mColorBlendAttachmentStates.size();
	inPSO->mColorBlendState.pAttachments = inPSO->mColorBlendAttachmentStates.data();

	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {};
	pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineInputAssemblyStateCreateInfo.topology = inVkPrimitiveTopology;
	pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_DEPTH_BIAS
	};
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 3;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo;
	pipelineViewportStateCreateInfo = { };
	pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineViewportStateCreateInfo.viewportCount = 1;
	pipelineViewportStateCreateInfo.pViewports = &inPSO->mViewport;
	pipelineViewportStateCreateInfo.scissorCount = 1;
	pipelineViewportStateCreateInfo.pScissors = &inPSO->mScissor;

	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {};
	pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
	pipelineRasterizationStateCreateInfo.cullMode = inCullMode;
	pipelineRasterizationStateCreateInfo.frontFace = inVkFrontFace;
	pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_TRUE;
	pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	pipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {};
	pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineMultisampleStateCreateInfo.minSampleShading = 1.0f;
	pipelineMultisampleStateCreateInfo.pSampleMask = nullptr;
	pipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	pipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
	pipelineMultisampleStateCreateInfo.rasterizationSamples = inPSO->mSampleCount;

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {};
	pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineDepthStencilStateCreateInfo.depthTestEnable = inEnableDepthTest;
	pipelineDepthStencilStateCreateInfo.depthWriteEnable = inEnableDepthWrite;
	pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	pipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0f;
	pipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0f;
	pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	pipelineDepthStencilStateCreateInfo.front = {};
	pipelineDepthStencilStateCreateInfo.back = {};

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = inGPUProgram->mShaderStageCount;
	pipelineInfo.pStages = inGPUProgram->mShaderStage;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
	pipelineInfo.pViewportState = &pipelineViewportStateCreateInfo;
	pipelineInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
	pipelineInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
	pipelineInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
	pipelineInfo.pColorBlendState = &inPSO->mColorBlendState;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = inPSO->mPipelineLayout;
	pipelineInfo.renderPass = inPSO->mRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	if (vkCreateGraphicsPipelines(sGlobalConfig.mLogicDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &inPSO->mPipeline) != VK_SUCCESS) {
		printf("create pipeline fail\n");
	}
}
Texture::Texture(VkFormat inFormat, VkImageAspectFlags inImageAspectFlag) {
	mImage = nullptr;
	mMemory = nullptr;
	mImageView = nullptr;
	mFormat = inFormat;
	mImageAspectFlag = inImageAspectFlag;
}
Texture::~Texture() {
	if (mMemory != nullptr) {
		vkFreeMemory(sGlobalConfig.mLogicDevice, mMemory, nullptr);
	}
	if (mImageView != nullptr) {
		vkDestroyImageView(sGlobalConfig.mLogicDevice, mImageView, nullptr);
	}
	if (mImage != nullptr) {
		vkDestroyImage(sGlobalConfig.mLogicDevice, mImage, nullptr);
	}
}
FrameBuffer::FrameBuffer() {
	mDepthAttachment = nullptr;
	mResolveBuffer = nullptr;
}
FrameBuffer::~FrameBuffer() {
	if (mDepthAttachment != nullptr) {
		delete mDepthAttachment;
		mDepthAttachment = nullptr;
	}
	for (Texture* colorAttachment:mColorAttachments){
		delete colorAttachment;
	}
	mColorAttachments.clear();
}
GPUProgram::GPUProgram() {
	mShaderStageCount = 0;
	memset(mShaderStage, 0, sizeof(VkPipelineShaderStageCreateInfo) * 2);
}
GPUProgram::~GPUProgram() {
	if (mShaderStage[0].module != 0) {
		vkDestroyShaderModule(sGlobalConfig.mLogicDevice, mShaderStage[0].module, nullptr);
	}
	if (mShaderStage[1].module != 0) {
		vkDestroyShaderModule(sGlobalConfig.mLogicDevice, mShaderStage[1].module, nullptr);
	}
}
void GPUProgram::AttachShader(VkShaderStageFlagBits inVkShaderStageFlagBits, const char* inFilePath) {
	int fileLen = 0;
	unsigned char* fileContent = LoadFileContent(inFilePath, fileLen);
	VkShaderModule shader = InitShaderWithCode(fileContent, fileLen);
	mShaderStage[mShaderStageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	mShaderStage[mShaderStageCount].stage = inVkShaderStageFlagBits;
	mShaderStage[mShaderStageCount].module = shader;
	mShaderStage[mShaderStageCount].pName = "main";
	mShaderStageCount++;
}
void BufferObject::Write(void* inData, int inDataSize) {
	void* data=nullptr;
	if (inDataSize == 0) {
		inDataSize = mSize;
	}
	MapMemory(mMemory, 0, inDataSize, 0, &data);
	memcpy(data, inData, inDataSize);
	UnmapMemory(mMemory);
}
PipelineStateObject::PipelineStateObject() :mPipeline(0) {
	mViewport = {};
	mScissor = {};

	mColorBlendState = {};
	mColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	mColorBlendState.logicOpEnable = VK_FALSE;
	mColorBlendState.logicOp = VK_LOGIC_OP_COPY;
	mColorBlendState.attachmentCount = 0;
	mColorBlendState.pAttachments = nullptr;
	mColorBlendState.blendConstants[0] = 0.0f;
	mColorBlendState.blendConstants[1] = 0.0f;
	mColorBlendState.blendConstants[2] = 0.0f;
	mColorBlendState.blendConstants[3] = 0.0f;

	mRenderPass = 0;
	mSampleCount = VK_SAMPLE_COUNT_1_BIT;

	mDepthConstantFactor = 0.0f;
	mDepthClamp = 0.0f;
	mDepthSlopeFactor = 0.0f;
}
PipelineStateObject::~PipelineStateObject() {
	CleanUp();
}
void PipelineStateObject::CleanUp() {
	if (mPipeline != 0) {
		vkDestroyPipeline(sGlobalConfig.mLogicDevice, mPipeline, nullptr);
	}
}
GlobalConfig& GetGlobalConfig() {
	return sGlobalConfig;
}
static bool IsLayerSupported(const char* inLayerName) {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	if (layerCount == 0) {
		return false;
	}
	VkLayerProperties* layers = new VkLayerProperties[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, layers);
	bool layerFound = false;
	for (uint32_t i = 0; i < layerCount; ++i) {
		printf("IsLayerSupported : [%s] : [%d][%s]\n",inLayerName,i,layers[i].layerName);
		if (strcmp(inLayerName, layers[i].layerName) == 0) {
			layerFound = true;
			break;
		}
	}
	delete[] layers;
	return layerFound;
}
static void InitPreferedLayers() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	if (layerCount == 0) {
		return;
	}
	VkLayerProperties* layers = new VkLayerProperties[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, layers);
	int enabledLayerCount = 0;
	for (uint32_t i = 0; i < layerCount; ++i) {
		if (nullptr!=strstr(layers[i].layerName, "validation")) {
			enabledLayerCount++;
		}
	}
	sGlobalConfig.mEnabledLayerCount= enabledLayerCount;
	if (enabledLayerCount > 0) {
		sGlobalConfig.mEnabledLayers = new char* [enabledLayerCount];
		int enabledLayerIndex = 0;
		for (uint32_t i = 0; i < layerCount; i++) {
			if (nullptr != strstr(layers[i].layerName, "validation")) {
				sGlobalConfig.mEnabledLayers[enabledLayerIndex] = new char[64];
				memset(sGlobalConfig.mEnabledLayers[enabledLayerIndex], 0, 64);
				strcpy_s(sGlobalConfig.mEnabledLayers[enabledLayerIndex], 64, layers[i].layerName);
				printf("InitPreferedLayers enable %s\n", sGlobalConfig.mEnabledLayers[enabledLayerIndex]);
				enabledLayerIndex++;
			}
		}
	}
	delete[] layers;
}
// 创建 VkInstance（启用 validation 与调试扩展）
static bool InitVulkanInstance() {
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "BattleFireVulkan";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "BattleFire";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = sizeof(sEnabledExtensions) / sizeof(sEnabledExtensions[0]);
	createInfo.ppEnabledExtensionNames = sEnabledExtensions;
	InitPreferedLayers();
	createInfo.enabledLayerCount = sGlobalConfig.mEnabledLayerCount;
	createInfo.ppEnabledLayerNames = sGlobalConfig.mEnabledLayers;
	if (vkCreateInstance(&createInfo, nullptr, &sGlobalConfig.mVulkanInstance) != VK_SUCCESS) {
		printf("create vulkan instance fail\n");
		return false;
	}
	return true;
}
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{
	printf("validation layer: %s\n", msg);
	return VK_FALSE;
}
static bool InitDebugger() {
	VkDebugReportCallbackCreateInfoEXT createDebugInfo = {};
	createDebugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createDebugInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createDebugInfo.pfnCallback = debugCallback;
	if (vkCreateDebugReportCallback(sGlobalConfig.mVulkanInstance, &createDebugInfo, nullptr, &sVulkanDebugger) != VK_SUCCESS) {
		return false;
	}
	return true;
}
static bool InitSurface() {
	VkWin32SurfaceCreateInfoKHR createSurfaceInfo;
	createSurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createSurfaceInfo.hwnd = (HWND)sGlobalConfig.mHWND;
	createSurfaceInfo.hinstance = GetModuleHandle(nullptr);
	createSurfaceInfo.pNext = nullptr;
	createSurfaceInfo.flags = 0;
	if (vkCreateWin32SurfaceKHR(sGlobalConfig.mVulkanInstance, &createSurfaceInfo, nullptr, &sGlobalConfig.mVulkanSurface) != VK_SUCCESS) {
		return false;
	}
	return true;
}
static VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDevice inVkPhysicalDevice) {
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(inVkPhysicalDevice, &physicalDeviceProperties);
	VkSampleCountFlags sampleCountFlags = physicalDeviceProperties.limits.framebufferColorSampleCounts > physicalDeviceProperties.limits.framebufferDepthSampleCounts ?
		physicalDeviceProperties.limits.framebufferDepthSampleCounts : physicalDeviceProperties.limits.framebufferColorSampleCounts;
	if (sampleCountFlags & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (sampleCountFlags & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (sampleCountFlags & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (sampleCountFlags & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (sampleCountFlags & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (sampleCountFlags & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}
bool InitPhysicDevice() {
	uint32_t deviceCount = 0; 
	vkEnumeratePhysicalDevices(sGlobalConfig.mVulkanInstance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		printf("cannot find any gpu on your computer!\n");
		return false;
	}
	VkPhysicalDevice* devices = new VkPhysicalDevice[deviceCount];
	vkEnumeratePhysicalDevices(sGlobalConfig.mVulkanInstance, &deviceCount, devices);

	for (uint32_t i = 0; i < deviceCount; ++i) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
		vkGetPhysicalDeviceFeatures(devices[i], &deviceFeatures);
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, nullptr);
		VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)alloca(queueFamilyCount * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies);
		sGlobalConfig.mGraphicQueueFamilyIndex = -1;
		sGlobalConfig.mPresentQueueFamilyIndex = -1;
		for (uint32_t j = 0; j < queueFamilyCount; ++j) {
			if (queueFamilies[j].queueCount > 0 && queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				sGlobalConfig.mGraphicQueueFamilyIndex = j;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, sGlobalConfig.mVulkanSurface, &presentSupport);
			if (queueFamilies[j].queueCount > 0 && presentSupport) {
				sGlobalConfig.mPresentQueueFamilyIndex = j;
			}
			if (sGlobalConfig.mGraphicQueueFamilyIndex != -1 && sGlobalConfig.mPresentQueueFamilyIndex != -1) {
				sGlobalConfig.mPhysicalDevice = devices[i];
				sGlobalConfig.mCurrentPhysicalDeviceMaxSampleCount = GetMaxUsableSampleCount(devices[i]);
				if (sGlobalConfig.mPreferedSampleCount > sGlobalConfig.mCurrentPhysicalDeviceMaxSampleCount) {
					sGlobalConfig.mPreferedSampleCount = sGlobalConfig.mCurrentPhysicalDeviceMaxSampleCount;
				}
				return true;
			}
		}
	}
	return false;
}
void InitVulkanLogicDevice() {
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { sGlobalConfig.mGraphicQueueFamilyIndex, sGlobalConfig.mPresentQueueFamilyIndex };
	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = VK_TRUE;

	VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT nonSeamlessCubeMapFeatures = {};
	nonSeamlessCubeMapFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NON_SEAMLESS_CUBE_MAP_FEATURES_EXT;
	nonSeamlessCubeMapFeatures.nonSeamlessCubeMap = VK_TRUE;

	VkDeviceCreateInfo createDeviceInfo = {};
	createDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createDeviceInfo.pQueueCreateInfos = queueCreateInfos.data();
	createDeviceInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();

	createDeviceInfo.pEnabledFeatures = &deviceFeatures;
	createDeviceInfo.enabledExtensionCount = 0;

	createDeviceInfo.enabledLayerCount = sGlobalConfig.mEnabledLayerCount;
	createDeviceInfo.ppEnabledLayerNames = sGlobalConfig.mEnabledLayers;

	createDeviceInfo.enabledExtensionCount = sizeof(sEnabledLogicDeviceExtensions) / sizeof(sEnabledLogicDeviceExtensions[0]);;
	createDeviceInfo.ppEnabledExtensionNames = sEnabledLogicDeviceExtensions;
	createDeviceInfo.pNext = &nonSeamlessCubeMapFeatures;

	if (vkCreateDevice(sGlobalConfig.mPhysicalDevice, &createDeviceInfo, nullptr, &sGlobalConfig.mLogicDevice) != VK_SUCCESS) {
		printf("create vulkan logic device fail\n");
	}
	vkGetDeviceQueue(sGlobalConfig.mLogicDevice, sGlobalConfig.mGraphicQueueFamilyIndex, 0, &sGlobalConfig.mGraphicQueue);
	vkGetDeviceQueue(sGlobalConfig.mLogicDevice, sGlobalConfig.mPresentQueueFamilyIndex, 0, &sGlobalConfig.mPresentQueue);
}
void InitSwapChainDetailInfo() {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(sGlobalConfig.mPhysicalDevice, sGlobalConfig.mVulkanSurface, &sGlobalConfig.mSurfaceCapabilities);
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(sGlobalConfig.mPhysicalDevice, sGlobalConfig.mVulkanSurface, &formatCount, nullptr);

	if (formatCount != 0) {
		sGlobalConfig.mSurfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(sGlobalConfig.mPhysicalDevice, sGlobalConfig.mVulkanSurface, &formatCount, sGlobalConfig.mSurfaceFormats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(sGlobalConfig.mPhysicalDevice, sGlobalConfig.mVulkanSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		sGlobalConfig.mPresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(sGlobalConfig.mPhysicalDevice, sGlobalConfig.mVulkanSurface, &presentModeCount, sGlobalConfig.mPresentModes.data());
	}
}
void InitSwapChain() {
	InitSwapChainDetailInfo();
	VkSurfaceFormatKHR surfaceFormat;
	for (const auto& availableFormat : sGlobalConfig.mSurfaceFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			surfaceFormat.colorSpace=availableFormat.colorSpace;
			surfaceFormat.format = availableFormat.format;
			break;
		}
	}
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& availablePresentMode : sGlobalConfig.mPresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = availablePresentMode;
			break;
		}
	}
	VkExtent2D extent = { uint32_t(sGlobalConfig.mViewportWidth), uint32_t(sGlobalConfig.mViewportHeight) };

	uint32_t imageCount = sGlobalConfig.mSurfaceCapabilities.minImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = sGlobalConfig.mVulkanSurface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	printf("image count %u\n", imageCount);
	uint32_t queueFamilyIndices[] = { (uint32_t)sGlobalConfig.mGraphicQueueFamilyIndex, (uint32_t)sGlobalConfig.mPresentQueueFamilyIndex };

	if (sGlobalConfig.mGraphicQueueFamilyIndex != sGlobalConfig.mPresentQueueFamilyIndex) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = sGlobalConfig.mSurfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(sGlobalConfig.mLogicDevice, &createInfo, nullptr, &sGlobalConfig.mSwapChain) != VK_SUCCESS) {
		printf("failed to create swap chain!\n");
	}

	vkGetSwapchainImagesKHR(sGlobalConfig.mLogicDevice, sGlobalConfig.mSwapChain, &imageCount, nullptr);
	sGlobalConfig.mSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(sGlobalConfig.mLogicDevice, sGlobalConfig.mSwapChain, &imageCount, sGlobalConfig.mSwapChainImages.data());

	sGlobalConfig.mSwapChainImageFormat = surfaceFormat.format;
	sGlobalConfig.mSwapChainExtent = extent;
	sGlobalConfig.mSwapChainImageViews.resize(imageCount);
	sGlobalConfig.mSystemFrameBufferCount = imageCount;
	for (uint32_t i = 0; i < imageCount; i++) {
		sGlobalConfig.mSwapChainImageViews[i] = GenImageView2D(sGlobalConfig.mSwapChainImages[i], sGlobalConfig.mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}
static void InitSystemRenderPass() {
	VkSampleCountFlagBits sample_count = sGlobalConfig.mPreferedSampleCount;
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = sGlobalConfig.mSwapChainImageFormat;
	colorAttachment.samples = sample_count;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (sample_count == VK_SAMPLE_COUNT_1_BIT) {
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}
	else {
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = VK_FORMAT_D24_UNORM_S8_UINT;
	depthAttachment.samples = sample_count;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription attachments[3];
	int attachment_count = 2;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	if (sample_count == VK_SAMPLE_COUNT_1_BIT) {
		memcpy(&attachments[0], &colorAttachment, sizeof(VkAttachmentDescription));
		memcpy(&attachments[1], &depthAttachment, sizeof(VkAttachmentDescription));
	}
	else {
		VkAttachmentDescription colorAttachmentResolve = {};
		colorAttachmentResolve.format = sGlobalConfig.mSwapChainImageFormat;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentResolveRef = {};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;
		attachment_count = 3;
		memcpy(&attachments[0], &colorAttachment, sizeof(VkAttachmentDescription));
		memcpy(&attachments[1], &depthAttachment, sizeof(VkAttachmentDescription));
		memcpy(&attachments[2], &colorAttachmentResolve, sizeof(VkAttachmentDescription));
	}

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachment_count;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(sGlobalConfig.mLogicDevice, &renderPassInfo, nullptr, &sGlobalConfig.mSystemRenderPass) != VK_SUCCESS) {
		printf("create global render pass fail\n");
	}
}
static void SystemFrameBufferFinish(int inIndex,FrameBuffer& framebuffer) {
	VkImageView attachments[3];
	int attachment_count = 2;
	if (sGlobalConfig.mPreferedSampleCount == VK_SAMPLE_COUNT_1_BIT) {
		attachments[0] = sGlobalConfig.mSwapChainImageViews[inIndex];
		attachments[1] = framebuffer.mDepthAttachment->mImageView;
	}
	else {
		attachment_count = 3;
		attachments[0] = framebuffer.mColorAttachments[0]->mImageView;
		attachments[1] = framebuffer.mDepthAttachment->mImageView;
		attachments[2] = sGlobalConfig.mSwapChainImageViews[inIndex];
	}
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = sGlobalConfig.mSystemRenderPass;
	framebufferInfo.pAttachments = attachments;
	framebufferInfo.attachmentCount = attachment_count;
	framebufferInfo.width = sGlobalConfig.mViewportWidth;
	framebufferInfo.height = sGlobalConfig.mViewportHeight;
	framebufferInfo.layers = 1;
	if (vkCreateFramebuffer(sGlobalConfig.mLogicDevice, &framebufferInfo, nullptr, &framebuffer.mFrameBuffer) != VK_SUCCESS) {
		printf("create system frame buffer fail\n");
	}
}
void InitSystemFrameBuffer() {
	if (sGlobalConfig.mSystemRenderPass != nullptr) {
		vkDestroyRenderPass(sGlobalConfig.mLogicDevice, sGlobalConfig.mSystemRenderPass, nullptr);
		sGlobalConfig.mSystemRenderPass = nullptr;
	}
	InitSystemRenderPass();
	if (sGlobalConfig.mSystemFrameBuffers != nullptr) {
		delete[] sGlobalConfig.mSystemFrameBuffers;
	}
	sGlobalConfig.mSystemFrameBuffers = new FrameBuffer[sGlobalConfig.mSystemFrameBufferCount];

	for (int i = 0; i < sGlobalConfig.mSystemFrameBufferCount; i++) {
		Texture* colorBuffer = new Texture(sGlobalConfig.mSwapChainImageFormat);
		GenImage(colorBuffer, sGlobalConfig.mViewportWidth, sGlobalConfig.mViewportHeight, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, sGlobalConfig.mPreferedSampleCount);
		colorBuffer->mImageView=GenImageView2D(colorBuffer->mImage,colorBuffer->mFormat,colorBuffer->mImageAspectFlag);
		sGlobalConfig.mSystemFrameBuffers[i].mColorAttachments.push_back(colorBuffer);

		Texture* depthBuffer = new Texture(VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT);
		GenImage(depthBuffer, sGlobalConfig.mViewportWidth, sGlobalConfig.mViewportHeight, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, sGlobalConfig.mPreferedSampleCount);
		depthBuffer->mImageView=GenImageView2D(depthBuffer->mImage, depthBuffer->mFormat, depthBuffer->mImageAspectFlag);
		sGlobalConfig.mSystemFrameBuffers[i].mDepthAttachment = depthBuffer;

		SystemFrameBufferFinish(i,sGlobalConfig.mSystemFrameBuffers[i]);
	}
}
void InitCommandPool() {
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = sGlobalConfig.mGraphicQueueFamilyIndex;
	poolInfo.flags = 0;
	if (vkCreateCommandPool(sGlobalConfig.mLogicDevice, &poolInfo, nullptr, &sGlobalConfig.mCommandPool) != VK_SUCCESS) {
		printf("create command pool fail\n");
	}
}
void InitSemaphores() {
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(sGlobalConfig.mLogicDevice, &semaphoreInfo, nullptr, &sGlobalConfig.mReadyToRender) != VK_SUCCESS ||
		vkCreateSemaphore(sGlobalConfig.mLogicDevice, &semaphoreInfo, nullptr, &sGlobalConfig.mReadyToPresent) != VK_SUCCESS) {
		printf("create semaphore fail\n");
	}
}
bool InitVulkan(void* param, int width, int height) {
	sGlobalConfig.mHWND = param;
	sGlobalConfig.mViewportWidth = width;
	sGlobalConfig.mViewportHeight = height;
	if (!InitVulkanInstance()) {
		return false;
	}
	__vkCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(sGlobalConfig.mVulkanInstance, "vkCreateDebugReportCallbackEXT");
	__vkDestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(sGlobalConfig.mVulkanInstance, "vkDestroyDebugReportCallbackEXT");
	__vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(sGlobalConfig.mVulkanInstance, "vkCreateWin32SurfaceKHR");
	if (!InitDebugger()) {
		return false;
	}
	if (!InitSurface()) {
		return false;
	}
	if (!InitPhysicDevice()) {
		return false;
	}
	InitVulkanLogicDevice();
	InitSwapChain();
	InitSystemFrameBuffer();
	InitCommandPool();
	InitSemaphores();
	UniformInputsBindings::Init();
	PipelineStateObject::Init();
	return true;
}
void OnViewportChangedVulkan(int inWidth, int inHeight) {
	if (sGlobalConfig.mViewportWidth != inWidth || sGlobalConfig.mViewportHeight != inHeight) {
		sGlobalConfig.mViewportWidth = inWidth;
		sGlobalConfig.mViewportHeight = inHeight;
		vkDeviceWaitIdle(sGlobalConfig.mLogicDevice);
		for (int i = 0; i < sGlobalConfig.mSystemFrameBufferCount; ++i) {
			vkDestroyImageView(sGlobalConfig.mLogicDevice, sGlobalConfig.mSwapChainImageViews[i], nullptr);
			vkDestroyFramebuffer(sGlobalConfig.mLogicDevice, sGlobalConfig.mSystemFrameBuffers[i].mFrameBuffer, nullptr);
		}
		vkDestroySwapchainKHR(sGlobalConfig.mLogicDevice, sGlobalConfig.mSwapChain, nullptr);
		InitSwapChain();
		InitSystemFrameBuffer();
	}
}
static uint32_t sNextSystemFrameBufferToRender=-1;
// 获取当前 swapchain 图像索引，用于最终画到屏幕
VkFramebuffer AquireRenderTarget() {
	vkAcquireNextImageKHR(sGlobalConfig.mLogicDevice, sGlobalConfig.mSwapChain, std::numeric_limits<uint64_t>::max(), sGlobalConfig.mReadyToRender, VK_NULL_HANDLE, &sNextSystemFrameBufferToRender);
	return sGlobalConfig.mSystemFrameBuffers[sNextSystemFrameBufferToRender].mFrameBuffer;
}
VkResult GenCommandBuffer(VkCommandBuffer* command_buffer, int count, VkCommandBufferLevel level) {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = level;
	allocInfo.commandPool = sGlobalConfig.mCommandPool;
	allocInfo.commandBufferCount = count;
	allocInfo.pNext = nullptr;
	return vkAllocateCommandBuffers(sGlobalConfig.mLogicDevice, &allocInfo, command_buffer);
}
void DeleteCommandBuffer(VkCommandBuffer* command_buffer, int count) {
	vkFreeCommandBuffers(sGlobalConfig.mLogicDevice, sGlobalConfig.mCommandPool, count, command_buffer);
}
void VulkanSubmitDrawCommand(VkCommandBuffer* commandbuffer, int count) {
	VkSemaphore waitSemaphores[] = { sGlobalConfig.mReadyToRender };
	VkSemaphore signalSemaphores[] = { sGlobalConfig.mReadyToPresent };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.pCommandBuffers = commandbuffer;
	submitInfo.commandBufferCount = count;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(sGlobalConfig.mGraphicQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		printf("vkQueueSubmit failed!\n");
	}
}
void VulkanSwapBuffers() {
	VkSemaphore signalSemaphores[] = { sGlobalConfig.mReadyToPresent };

	VkPresentInfoKHR presentInfo = {};
	uint32_t current_render_target_index = sNextSystemFrameBufferToRender;
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pSwapchains = &sGlobalConfig.mSwapChain;
	presentInfo.swapchainCount = 1;
	presentInfo.pImageIndices = &current_render_target_index;

	vkQueuePresentKHR(sGlobalConfig.mPresentQueue, &presentInfo);
	vkQueueWaitIdle(sGlobalConfig.mPresentQueue);
}
void SwapBuffers() {
	VulkanSubmitDrawCommand(&sGlobalConfig.mSystemRenderPassCommandBuffer, 1);
	VulkanSwapBuffers();
	DeleteCommandBuffer(&sGlobalConfig.mSystemRenderPassCommandBuffer, 1);
	sGlobalConfig.mSystemRenderPassCommandBuffer = nullptr;
}
VkResult BeginOneTimeCommandBuffer(VkCommandBuffer* command_buffer) {
	VkResult ret = GenCommandBuffer(command_buffer, 1);
	if (ret != VK_SUCCESS) {
		return ret;
	}
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pNext = nullptr;
	beginInfo.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(*command_buffer, &beginInfo);
	return ret;
}
void WaitForCommmandFinish(VkCommandBuffer commandbuffer) {
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandbuffer;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;
	VkFence fence;
	vkCreateFence(sGlobalConfig.mLogicDevice, &fenceCreateInfo, nullptr, &fence);
	vkQueueSubmit(sGlobalConfig.mGraphicQueue, 1, &submitInfo, fence);

	vkWaitForFences(sGlobalConfig.mLogicDevice, 1, &fence, VK_TRUE, 100000000000);
	vkDestroyFence(sGlobalConfig.mLogicDevice, fence, nullptr);
}
VkResult EndOneTimeCommandBuffer(VkCommandBuffer command_buffer) {
	vkEndCommandBuffer(command_buffer);
	WaitForCommmandFinish(command_buffer);
	vkFreeCommandBuffers(sGlobalConfig.mLogicDevice, sGlobalConfig.mCommandPool, 1, &command_buffer);
	return VK_SUCCESS;
}
// 开始渲染到 Swapchain 的 framebuffer（Acquire 已在此完成）
VkCommandBuffer BeginRendering(VkCommandBuffer cmd) {
	VkCommandBuffer commandbuffer;
	if (cmd != nullptr) {
		commandbuffer = cmd;
	}
	else {
		BeginOneTimeCommandBuffer(&commandbuffer);
	}
	VkFramebuffer render_target = AquireRenderTarget();
	VkRenderPass render_pass = sGlobalConfig.mSystemRenderPass;
	VkClearValue clearValues[2] = {};
	clearValues[0].color = { 0.1f,0.4f,0.6f,1.0f };
	clearValues[1].depthStencil = { 1.0f, 0u };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.framebuffer = render_target;
	renderPassInfo.renderPass = render_pass;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = { uint32_t(sGlobalConfig.mViewportWidth),uint32_t(sGlobalConfig.mViewportHeight) };
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(commandbuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	sGlobalConfig.mSystemRenderPassCommandBuffer = commandbuffer;
	return commandbuffer;
}
void EndRendering() {
	vkCmdEndRenderPass(sGlobalConfig.mSystemRenderPassCommandBuffer);
	vkEndCommandBuffer(sGlobalConfig.mSystemRenderPassCommandBuffer);
}
void SetDynamicState(PipelineStateObject*inPipelineStateObject, VkCommandBuffer commandbuffer) {
	vkCmdSetViewport(commandbuffer, 0, 1, &inPipelineStateObject->mViewport);
	vkCmdSetScissor(commandbuffer, 0, 1, &inPipelineStateObject->mScissor);
	vkCmdSetDepthBias(commandbuffer, inPipelineStateObject->mDepthConstantFactor, inPipelineStateObject->mDepthClamp, inPipelineStateObject->mDepthSlopeFactor);
}
void MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData) {
	vkMapMemory(sGlobalConfig.mLogicDevice, memory, offset, size, flags, ppData);
}
void UnmapMemory(VkDeviceMemory memory) {
	vkUnmapMemory(sGlobalConfig.mLogicDevice, memory);
}
Texture* LoadTextureFromFile(const char* inFilePath) {
	Texture* texture = new Texture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	int image_width, image_height, channel;
	unsigned char* pixel = LoadImageFromFile(inFilePath, image_width, image_height, channel, 4);
	GenImage(texture, image_width, image_height, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT);
	SubmitImage2D(texture, image_width, image_height, pixel);
	texture->mImageView = GenImageView2D(texture->mImage, texture->mFormat, texture->mImageAspectFlag);
	delete[] pixel;
	return texture;
}
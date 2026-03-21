#include "Mesh.h"

StaticMeshComponent::StaticMeshComponent() {
	mVBO = nullptr;
}
void StaticMeshComponent::SetPosition(int index, float x, float y, float z, float w) {
	mVertexData[index].mPosition[0] = x;
	mVertexData[index].mPosition[1] = y;
	mVertexData[index].mPosition[2] = z;
	mVertexData[index].mPosition[3] = w;
}
void StaticMeshComponent::SetTexcoord(int index, float x, float y, float z, float w) {
	mVertexData[index].mTexcoord[0] = x;
	mVertexData[index].mTexcoord[1] = y;
	mVertexData[index].mTexcoord[2] = z;
	mVertexData[index].mTexcoord[3] = w;
}
void StaticMeshComponent::SetNormal(int index, float x, float y, float z, float w) {
	mVertexData[index].mNormal[0] = x;
	mVertexData[index].mNormal[1] = y;
	mVertexData[index].mNormal[2] = z;
	mVertexData[index].mNormal[3] = w;
}
void StaticMeshComponent::LoadFromFile(const char* inFilePath) {
	// File format:
	// [int vertexCount]
	// [vertex array]
	// repeated:
	//   [int submeshNameLen][char name[]][int indexCount][uint index[]]
	FILE* pFile = nullptr;
	errno_t err = fopen_s(&pFile, inFilePath, "rb");
	if (err == 0) {
		int temp = 0;
		fread(&temp, sizeof(int), 1, pFile);
		mVertexCount = temp;
		mVertexData = new StaticMeshVertexData[mVertexCount];
		mVertexDataSize = sizeof(StaticMeshVertexData);
		fread(mVertexData, sizeof(StaticMeshVertexData), mVertexCount, pFile);
		while (!feof(pFile)) {
			int nameLen = 0, indexCount = 0;
			fread(&nameLen, 1, sizeof(int), pFile);
			if (feof(pFile)) {
				break;
			}
			char name[256] = { 0 };
			fread(name, 1, sizeof(char) * nameLen, pFile);
			fread(&indexCount, 1, sizeof(int), pFile);
			VulkanSubMesh* submesh = new VulkanSubMesh;
			submesh->mIndexCount = indexCount;
			submesh->mIndexes = new unsigned int[indexCount];
			fread(submesh->mIndexes, 1, sizeof(unsigned int) * indexCount, pFile);
			mSubMeshes.insert(std::pair<std::string, VulkanSubMesh*>(name, submesh));
			printf("Load StaticMesh [%s] : vertex count[%d] submesh[%s] Index[%d]\n", inFilePath, mVertexCount, name, indexCount);
		}
		fclose(pFile);
	}
}
void StaticMeshComponent::LoadFromFile2(const char* inFilePath) {
	// Same as LoadFromFile, but vertex type includes tangent (for normal mapping).
	FILE* pFile = nullptr;
	errno_t err = fopen_s(&pFile, inFilePath, "rb");
	if (err == 0) {
		int vertice_count;
		fread(&vertice_count, 1, sizeof(int), pFile);
		mVertexCount = vertice_count;
		mVertexData = new StaticMeshVertexDataEx[vertice_count];
		mVertexDataSize = sizeof(StaticMeshVertexDataEx);
		fread(mVertexData, 1, sizeof(StaticMeshVertexDataEx) * vertice_count, pFile);
		while (!feof(pFile)) {
			int nameLen = 0, indexCount = 0;
			fread(&nameLen, 1, sizeof(int), pFile);
			if (feof(pFile)) {
				break;
			}
			char name[256] = { 0 };
			fread(name, 1, sizeof(char) * nameLen, pFile);
			fread(&indexCount, 1, sizeof(int), pFile);
			VulkanSubMesh* submesh = new VulkanSubMesh;
			submesh->mIndexCount = indexCount;
			submesh->mIndexes = new unsigned int[indexCount];
			fread(submesh->mIndexes, 1, sizeof(unsigned int) * indexCount, pFile);
			mSubMeshes.insert(std::pair<std::string, VulkanSubMesh*>(name, submesh));
			printf("Load StaticMesh [%s] : vertex count[%d] submesh[%s] Index[%d]\n", inFilePath, vertice_count, name, indexCount);
		}
		fclose(pFile);
	}
}
void StaticMeshComponent::Update(VkCommandBuffer inCommandBuffer) {
	if (mVertexCount == 0) {
		return;
	}
	if (mVBO == nullptr) {
		// Lazy upload static mesh vertex data to GPU-local buffer.
		mVBO = CreateBuffer(mVertexDataSize * mVertexCount, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		BufferSubData(mVBO->mBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, mVertexData, mVertexDataSize * mVertexCount);
	}
	if (!mSubMeshes.empty()) {
		std::unordered_map<std::string, VulkanSubMesh*>::iterator iter = mSubMeshes.begin();
		if (iter->second->mIBO == nullptr) {
			// Lazy upload index buffers for each submesh.
			for (; iter != mSubMeshes.end(); iter++) {
				VulkanSubMesh* submesh = iter->second;
				submesh->mIBO = CreateBuffer(sizeof(unsigned int) * submesh->mIndexCount, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				BufferSubData(submesh->mIBO->mBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, submesh->mIndexes, sizeof(unsigned int) * submesh->mIndexCount);
			}
		}
	}
}
void StaticMeshComponent::Draw(VkCommandBuffer inCommandBuffer) {
	VkBuffer vertexBuffers[] = { mVBO->mBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(inCommandBuffer, 0, 1, vertexBuffers, offsets);
	if (mSubMeshes.empty()) {
		// Non-indexed path.
		vkCmdDraw(inCommandBuffer,mVertexCount,1,0,0);
	}
	else {
		// Indexed path (one draw per submesh).
		std::unordered_map<std::string, VulkanSubMesh*>::iterator iter = mSubMeshes.begin();
		for (; iter != mSubMeshes.end(); iter++) {
			vkCmdBindIndexBuffer(inCommandBuffer, iter->second->mIBO->mBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(inCommandBuffer, iter->second->mIndexCount, 1, 0, 0, 0);
		}
	}
}
void FullScreenQuadMeshComponent::Init() {
	mVertexCount = 4;
	mVertexData = new StaticMeshVertexData[4];
	mVertexDataSize = sizeof(StaticMeshVertexData);
	SetPosition(0, -1.0f, -1.0f, 0.0f);
	SetPosition(1, 1.0f, -1.0f, 0.0f);
	SetPosition(2, -1.0f, 1.0f, 0.0f);
	SetPosition(3, 1.0f, 1.0f, 0.0f);
	SetTexcoord(0, 0.0f, 0.0f);
	SetTexcoord(1, 1.0f, 0.0f);
	SetTexcoord(2, 0.0f, 1.0f);
	SetTexcoord(3, 1.0f, 1.0f);
}
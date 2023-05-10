#pragma once

#include "Macros.h"

// Changing this value here also requires changing it in the vertex shader
#define MAX_NUM_JOINTS 128u

struct Primitive {
	uint32_t firstIndex;
	uint32_t indexCount;
	uint32_t vertexCount;
	Material& material;
	bool hasIndices;
	BoundingBox bb;

	Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material& material) : firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount), material(material) {
		hasIndices = indexCount > 0;
	}
	void setBoundingBox(glm::vec3 min, glm::vec3 max) {
		bb.min = min;
		bb.max = max;
		bb.valid = true;
	}
};

struct Mesh {
	hbvk::VulkanDevice* device;
	std::vector<Primitive*> primitives;
	BoundingBox bb;
	BoundingBox aabb;

	struct UniformBuffer {
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDescriptorBufferInfo descriptor;
		VkDescriptorSet descriptorSet;
		void* mapped;
	} uniformBuffer;

	struct UniformBlock {
		glm::mat4 matrix;
		glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
		float jointcount{ 0 };
	} uniformBlock;

	Mesh(hbvk::VulkanDevice* device, glm::mat4 matrix) {
		this->device = device;
		this->uniformBlock.matrix = matrix;
		VK_CHECK_RESULT(device->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uniformBlock),
			&uniformBuffer.buffer,
			&uniformBuffer.memory,
			&uniformBlock));
		VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, uniformBuffer.memory, 0, sizeof(uniformBlock), 0, &uniformBuffer.mapped));
		uniformBuffer.descriptor = { uniformBuffer.buffer, 0, sizeof(uniformBlock) };
	}

	~Mesh() {
		vkDestroyBuffer(device->logicalDevice, uniformBuffer.buffer, nullptr);
		vkFreeMemory(device->logicalDevice, uniformBuffer.memory, nullptr);
		for (Primitive* p : primitives)
			delete p;
	}

	void setBoundingBox(glm::vec3 min, glm::vec3 max) {
		bb.min = min;
		bb.max = max;
		bb.valid = true;
	}
};
#pragma once

#include <iostream>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <vector>

#include "vulkan/vulkan.h"
#include "VulkanDevice.hpp"

#include "VulkanTexture.hpp"
#include "BoundingBox.hpp"
#include "Transform.hpp"
#include "Material.hpp"
#include "Mesh.hpp"

namespace vkglTF
{
	struct Node;

	struct Skin {
		std::string name;
		Node *skeletonRoot = nullptr;
		std::vector<glm::mat4> inverseBindMatrices;
		std::vector<Node*> joints;
	};

	struct Node {
		Node *parent;
		uint32_t index;
		std::vector<Node*> children;
		std::shared_ptr<Transform> transform;
		std::string name;
		Mesh *mesh;
		Skin *skin;
		int32_t skinIndex = -1;
		BoundingBox bvh;
		BoundingBox aabb;
		glm::mat4 getMatrix();
		void update();
		~Node();
	};

	struct AnimationChannel {
		enum PathType { TRANSLATION, ROTATION, SCALE };
		PathType path;
		Node *node;
		uint32_t samplerIndex;
	};

	struct AnimationSampler {
		enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
		InterpolationType interpolation;
		std::vector<float> inputs;
		std::vector<glm::vec4> outputsVec4;
	};

	struct Animation {
		std::string name;
		std::vector<AnimationSampler> samplers;
		std::vector<AnimationChannel> channels;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
	};

	enum PBRWorkflows { PBR_WORKFLOW_METALLIC_ROUGHNESS = 0, PBR_WORKFLOW_SPECULAR_GLOSINESS = 1 };

	struct Model {

		hbvk::VulkanDevice *device;

		struct Vertex {
			glm::vec3 pos;
			glm::vec3 normal;
			glm::vec2 uv0;
			glm::vec2 uv1;
			glm::vec4 joint0;
			glm::vec4 weight0;
			glm::vec4 color;
		};

		struct PushConstBlockMaterial {
			glm::vec4 baseColorFactor;
			glm::vec4 emissiveFactor;
			glm::vec4 diffuseFactor;
			glm::vec4 specularFactor;
			float workflow;
			int colorTextureSet;
			int PhysicalDescriptorTextureSet;
			int normalTextureSet;
			int occlusionTextureSet;
			int emissiveTextureSet;
			float metallicFactor;
			float roughnessFactor;
			float alphaMask;
			float alphaMaskCutoff;
		} pushConstBlockMaterial;

		struct Vertices {
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory memory;
		} vertices;
		struct Indices {
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory memory;
		} indices;

		glm::mat4 aabb;

		std::vector<Node*> nodes;
		std::vector<Node*> linearNodes;

		std::vector<Skin*> skins;
		std::shared_ptr<Transform> transform;

		std::vector<hbvk::Texture2D> textures;
		std::vector<hbvk::TextureSampler> textureSamplers;
		std::vector<Material> materials;
		std::vector<Animation> animations;
		std::vector<std::string> extensions;

		VkPipeline boundPipeline = VK_NULL_HANDLE;

		struct Dimensions {
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
		} dimensions;

		struct LoaderInfo {
			uint32_t* indexBuffer;
			Vertex* vertexBuffer;
			size_t indexPos = 0;
			size_t vertexPos = 0;
		};

		void destroy(VkDevice device);
		void loadNode(vkglTF::Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, LoaderInfo& loaderInfo, float globalscale);
		void getNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount);
		void loadSkins(tinygltf::Model& gltfModel);
		void loadTextures(tinygltf::Model& gltfModel, hbvk::VulkanDevice* device, VkQueue transferQueue);
		VkSamplerAddressMode getVkWrapMode(int32_t wrapMode);
		VkFilter getVkFilterMode(int32_t filterMode);
		void loadTextureSamplers(tinygltf::Model& gltfModel);
		void loadMaterials(tinygltf::Model& gltfModel);
		void loadAnimations(tinygltf::Model& gltfModel);
		void loadFromFile(std::string filename, hbvk::VulkanDevice* device, VkQueue transferQueue, float scale = 1.0f);
		void renderNodePBR(const VkPipeline& pbr, const VkPipeline& pbrDoubleSided,
			const VkPipeline& pbrAlphaBlend, const VkCommandBuffer& commandBuffer,
			const VkDescriptorSet& scene, const VkPipelineLayout& pipelineLayout,
			vkglTF::Node* node, Material::AlphaMode alphaMode);
		void calculateBoundingBox(Node* node, Node* parent);
		void getSceneDimensions();
		void renderPBR(const VkPipeline& pbr, const VkPipeline& pbrDoubleSided,
			const VkPipeline& pbrAlphaBlend, const VkCommandBuffer& commandBuffer,
			const VkDescriptorSet& scene, const VkPipelineLayout& pipelineLayout);
		void drawNode(Node* node, VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);
		void updateAnimation(uint32_t index, float time);
		Node* findNode(Node* parent, uint32_t index);
		Node* nodeFromIndex(uint32_t index);
	};
}

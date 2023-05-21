#pragma once
#include "ApplicationBase.h"
#include "VulkanglTFModel.h"

#include "UI.hpp"
#include "Light.hpp"
#include "FileUtils.hpp"

class ApplicationExample : public ApplicationBase
{
public:
	struct Textures {
		hbvk::TextureCubeMap environmentCube;
		hbvk::Texture2D empty;
		hbvk::Texture2D lutBrdf;
		hbvk::TextureCubeMap irradianceCube;
		hbvk::TextureCubeMap prefilteredCube;
	} textures;

	struct Models {
		vkglTF::Model scene;
		vkglTF::Model skybox;
	} models;

	struct UniformBufferSet {
		Buffer scene;
		Buffer skybox;
		Buffer params;
	};

	struct UBOMatrices {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
		glm::vec3 camPos;
	} shaderValuesScene, shaderValuesSkybox;

	struct shaderValuesParams {
		glm::vec4 lightDir;
		float exposure = 4.5f;
		float gamma = 2.2f;
		float prefilteredCubeMipLevels;
		float scaleIBLAmbient = 1.0f;
		float debugViewInputs = 0;
		float debugViewEquation = 0;
		alignas(16)glm::vec3 lightColor;
	} shaderValuesParams;

	VkPipelineLayout pipelineLayout;

	struct Pipelines {
		VkPipeline skybox;
		VkPipeline pbr;
		VkPipeline pbrDoubleSided;
		VkPipeline pbrAlphaBlend;
	} pipelines;
	VkPipeline boundPipeline = VK_NULL_HANDLE;

	struct DescriptorSetLayouts {
		VkDescriptorSetLayout scene;
		VkDescriptorSetLayout material;
		VkDescriptorSetLayout node;
	} descriptorSetLayouts;

	struct DescriptorSets {
		VkDescriptorSet scene;
		VkDescriptorSet skybox;
	};
	std::vector<DescriptorSets> descriptorSets;

	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<UniformBufferSet> uniformBuffers;

	std::vector<VkFence> waitFences;
	std::vector<VkSemaphore> renderCompleteSemaphores;
	std::vector<VkSemaphore> presentCompleteSemaphores;

	const uint32_t renderAhead = 2;
	uint32_t frameIndex = 0;

	int32_t animationIndex = 0;
	float animationTimer = 0.0f;
	bool animate = true;

	bool displayBackground = true;
	
	DirectionalLight *lightSource;

	UI *ui;

	bool rotateModel = false;

	enum PBRWorkflows{ PBR_WORKFLOW_METALLIC_ROUGHNESS = 0, PBR_WORKFLOW_SPECULAR_GLOSINESS = 1 };

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

	std::map<std::string, std::string> environments;
	std::string selectedEnvironment = "papermill";

	int32_t debugViewInputs = 0;
	int32_t debugViewEquation = 0;

	ApplicationExample() : ApplicationBase()
	{
		title = "Hummingbird Engine";
		iconPath = GetTexturePath() + "icon.png";
		width = 1920;
		height = 1080;
	}

	~ApplicationExample()
	{
		vkDestroyPipeline(device, pipelines.skybox, nullptr);
		vkDestroyPipeline(device, pipelines.pbr, nullptr);
		vkDestroyPipeline(device, pipelines.pbrAlphaBlend, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.scene, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.material, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.node, nullptr);

		models.scene.destroy(device);
		models.skybox.destroy(device);

		for (auto buffer : uniformBuffers) {
			buffer.params.destroy();
			buffer.scene.destroy();
			buffer.skybox.destroy();
		}
		for (auto fence : waitFences) {
			vkDestroyFence(device, fence, nullptr);
		}
		for (auto semaphore : renderCompleteSemaphores) {
			vkDestroySemaphore(device, semaphore, nullptr);
		}
		for (auto semaphore : presentCompleteSemaphores) {
			vkDestroySemaphore(device, semaphore, nullptr);
		}

		textures.environmentCube.destroy();
		textures.irradianceCube.destroy();
		textures.prefilteredCube.destroy();
		textures.lutBrdf.destroy();
		textures.empty.destroy();

		delete ui;
	}

	void renderNode(vkglTF::Node *node, uint32_t cbIndex, Material::AlphaMode alphaMode) {
		if (node->mesh) {
			// Render mesh primitives
			for (Primitive * primitive : node->mesh->primitives) {
				if (primitive->material.alphaMode == alphaMode) {

					VkPipeline pipeline = VK_NULL_HANDLE;
					switch (alphaMode) {
					case Material::ALPHAMODE_OPAQUE:
					case Material::ALPHAMODE_MASK:
						pipeline = primitive->material.doubleSided ? pipelines.pbrDoubleSided : pipelines.pbr;
						break;
					case Material::ALPHAMODE_BLEND:
						pipeline = pipelines.pbrAlphaBlend;
						break;
					}

					if (pipeline != boundPipeline) {
						vkCmdBindPipeline(commandBuffers[cbIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
						boundPipeline = pipeline;
					}

					const std::vector<VkDescriptorSet> descriptorsets = {
						descriptorSets[cbIndex].scene,
						primitive->material.descriptorSet,
						node->mesh->uniformBuffer.descriptorSet,
					};
					vkCmdBindDescriptorSets(commandBuffers[cbIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorsets.size()), descriptorsets.data(), 0, NULL);

					// Pass material parameters as push constants
					PushConstBlockMaterial pushConstBlockMaterial{};					
					pushConstBlockMaterial.emissiveFactor = primitive->material.emissiveFactor;
					// To save push constant space, availabilty and texture coordiante set are combined
					// -1 = texture not used for this material, >= 0 texture used and index of texture coordinate set
					pushConstBlockMaterial.colorTextureSet = primitive->material.baseColorTexture != nullptr ? primitive->material.texCoordSets.baseColor : -1;
					pushConstBlockMaterial.normalTextureSet = primitive->material.normalTexture != nullptr ? primitive->material.texCoordSets.normal : -1;
					pushConstBlockMaterial.occlusionTextureSet = primitive->material.occlusionTexture != nullptr ? primitive->material.texCoordSets.occlusion : -1;
					pushConstBlockMaterial.emissiveTextureSet = primitive->material.emissiveTexture != nullptr ? primitive->material.texCoordSets.emissive : -1;
					pushConstBlockMaterial.alphaMask = static_cast<float>(primitive->material.alphaMode == Material::ALPHAMODE_MASK);
					pushConstBlockMaterial.alphaMaskCutoff = primitive->material.alphaCutoff;

					// TODO: glTF specs states that metallic roughness should be preferred, even if specular glosiness is present

					if (primitive->material.pbrWorkflows.metallicRoughness) {
						// Metallic roughness workflow
						pushConstBlockMaterial.workflow = static_cast<float>(PBR_WORKFLOW_METALLIC_ROUGHNESS);
						pushConstBlockMaterial.baseColorFactor = primitive->material.baseColorFactor;
						pushConstBlockMaterial.metallicFactor = primitive->material.metallicFactor;
						pushConstBlockMaterial.roughnessFactor = primitive->material.roughnessFactor;
						pushConstBlockMaterial.PhysicalDescriptorTextureSet = primitive->material.metallicRoughnessTexture != nullptr ? primitive->material.texCoordSets.metallicRoughness : -1;
						pushConstBlockMaterial.colorTextureSet = primitive->material.baseColorTexture != nullptr ? primitive->material.texCoordSets.baseColor : -1;
					}

					if (primitive->material.pbrWorkflows.specularGlossiness) {
						// Specular glossiness workflow
						pushConstBlockMaterial.workflow = static_cast<float>(PBR_WORKFLOW_SPECULAR_GLOSINESS);
						pushConstBlockMaterial.PhysicalDescriptorTextureSet = primitive->material.extension.specularGlossinessTexture != nullptr ? primitive->material.texCoordSets.specularGlossiness : -1;
						pushConstBlockMaterial.colorTextureSet = primitive->material.extension.diffuseTexture != nullptr ? primitive->material.texCoordSets.baseColor : -1;
						pushConstBlockMaterial.diffuseFactor = primitive->material.extension.diffuseFactor;
						pushConstBlockMaterial.specularFactor = glm::vec4(primitive->material.extension.specularFactor, 1.0f);
					}

					vkCmdPushConstants(commandBuffers[cbIndex], pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstBlockMaterial), &pushConstBlockMaterial);

					if (primitive->hasIndices) {
						vkCmdDrawIndexed(commandBuffers[cbIndex], primitive->indexCount, 1, primitive->firstIndex, 0, 0);
					} else {
						vkCmdDraw(commandBuffers[cbIndex], primitive->vertexCount, 1, 0, 0);
					}
				}
			}

		};
		for (auto child : node->children) {
			renderNode(child, cbIndex, alphaMode);
		}
	}

	void recordCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufferBeginInfo{};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkClearValue clearValues[3];
		if (settings.multiSampling) {
			clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[2].depthStencil = { 1.0f, 0 };
		}
		else {
			clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[1].depthStencil = { 1.0f, 0 };
		}

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = settings.multiSampling ? 3 : 2;
		renderPassBeginInfo.pClearValues = clearValues;

		for (uint32_t i = 0; i < commandBuffers.size(); ++i) {
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			VkCommandBuffer currentCB = commandBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(currentCB, &cmdBufferBeginInfo));
			vkCmdBeginRenderPass(currentCB, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport{};
			viewport.width = (float)width;
			viewport.height = (float)height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(currentCB, 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.extent = { width, height };
			vkCmdSetScissor(currentCB, 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };

			if (displayBackground) {
				vkCmdBindDescriptorSets(currentCB, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i].skybox, 0, nullptr);
				vkCmdBindPipeline(currentCB, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skybox);
				models.skybox.draw(currentCB);
			}

			vkglTF::Model &model = models.scene;

			vkCmdBindVertexBuffers(currentCB, 0, 1, &model.vertices.buffer, offsets);
			if (model.indices.buffer != VK_NULL_HANDLE) {
				vkCmdBindIndexBuffer(currentCB, model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			}

			boundPipeline = VK_NULL_HANDLE;

			// Opaque primitives first
			for (auto node : model.nodes) {
				renderNode(node, i, Material::ALPHAMODE_OPAQUE);
			}
			// Alpha masked primitives
			for (auto node : model.nodes) {
				renderNode(node, i, Material::ALPHAMODE_MASK);
			}
			// Transparent primitives
			// TODO: Correct depth sorting
			for (auto node : model.nodes) {
				renderNode(node, i, Material::ALPHAMODE_BLEND);
			}

			// User interface
			ui->draw(currentCB);

			vkCmdEndRenderPass(currentCB);
			VK_CHECK_RESULT(vkEndCommandBuffer(currentCB));
		}
	}

	void loadScene(std::string filename)
	{
		LOG_INFO("Loading scene from {}", filename);
		models.scene.destroy(device);
		animationIndex = 0;
		animationTimer = 0.0f;
		auto tStart = std::chrono::high_resolution_clock::now();
		models.scene.loadFromFile(filename, vulkanDevice, queue);
		auto tFileLoad = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - tStart).count();
		LOG_INFO("Loading took {} ms", tFileLoad);
		camera->setPosition({ 0.0f, 0.0f, 1.0f });
		camera->setRotation({ 0.0f, 0.0f, 0.0f });
	}

	void loadEnvironment(std::string filename)
	{
		LOG_INFO("Loading environment from {}", filename);
		if (textures.environmentCube.image) {
			textures.environmentCube.destroy();
			textures.irradianceCube.destroy();
			textures.prefilteredCube.destroy();
		}
		textures.environmentCube.loadFromFile(filename, VK_FORMAT_R16G16B16A16_SFLOAT, vulkanDevice, queue);
		
		//generate cubemap
		uint32_t numMips = 0;
		generateCubemaps(device, vulkanDevice, queue, pipelineCache, models.skybox, numMips, 
			textures.environmentCube, textures.irradianceCube,textures.prefilteredCube);
		shaderValuesParams.prefilteredCubeMipLevels = static_cast<float>(numMips);
	}

	void loadAssets()
	{
		const std::string assetpath = GetAssetPath();
		struct stat info;
		if (stat(assetpath.c_str(), &info) != 0) {
			LOG_ERROR("Could not locate asset path in \"{}\".\nMake sure binary is run from correct relative directory!", assetpath);
			exit(-1);
		}
		ReadDirectory(GetEnvironmentPath(), "*.ktx", environments, false);

		textures.empty.loadFromFile(GetTexturePath() + "empty.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);

		std::string sceneFile = GetModelPath() + "DamagedHelmet/glTF/DamagedHelmet.gltf";
		std::string envMapFile = GetEnvironmentPath() + "papermill.ktx";

		loadScene(sceneFile.c_str());
		models.skybox.loadFromFile(GetModelPath() + "Box/glTF-Embedded/Box.gltf", vulkanDevice, queue);

		loadEnvironment(envMapFile.c_str());
	}

	void setupNodeDescriptorSet(vkglTF::Node *node) {
		if (node->mesh) {
			VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
			descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocInfo.descriptorPool = descriptorPool;
			descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayouts.node;
			descriptorSetAllocInfo.descriptorSetCount = 1;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &node->mesh->uniformBuffer.descriptorSet));

			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.dstSet = node->mesh->uniformBuffer.descriptorSet;
			writeDescriptorSet.dstBinding = 0;
			writeDescriptorSet.pBufferInfo = &node->mesh->uniformBuffer.descriptor;

			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
		}
		for (auto& child : node->children) {
			setupNodeDescriptorSet(child);
		}
	}

	void setupDescriptors()
	{
		/*
			Descriptor Pool
		*/
		uint32_t imageSamplerCount = 0;
		uint32_t materialCount = 0;
		uint32_t meshCount = 0;

		// Environment samplers (radiance, irradiance, brdf lut)
		imageSamplerCount += 3;

		std::vector<vkglTF::Model*> modellist = { &models.skybox, &models.scene };
		for (auto &model : modellist) {
			for (auto &material : model->materials) {
				imageSamplerCount += 5;
				materialCount++;
			}
			for (auto node : model->linearNodes) {
				if (node->mesh) {
					meshCount++;
				}
			}
		}

		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (4 + meshCount) * swapChain.imageCount },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageSamplerCount * swapChain.imageCount }
		};
		VkDescriptorPoolCreateInfo descriptorPoolCI{};
		descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCI.poolSizeCount = 2;
		descriptorPoolCI.pPoolSizes = poolSizes.data();
		descriptorPoolCI.maxSets = (2 + materialCount + meshCount) * swapChain.imageCount;
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));

		/*
			Descriptor sets
		*/

		// Scene (matrices and environment maps)
		{
			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
				{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
				{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
				{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
				{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
				{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
			};
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
			descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
			descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.scene));

			for (auto i = 0; i < descriptorSets.size(); i++) {

				VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
				descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				descriptorSetAllocInfo.descriptorPool = descriptorPool;
				descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayouts.scene;
				descriptorSetAllocInfo.descriptorSetCount = 1;
				VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSets[i].scene));

				std::array<VkWriteDescriptorSet, 5> writeDescriptorSets{};

				writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSets[0].descriptorCount = 1;
				writeDescriptorSets[0].dstSet = descriptorSets[i].scene;
				writeDescriptorSets[0].dstBinding = 0;
				writeDescriptorSets[0].pBufferInfo = &uniformBuffers[i].scene.descriptor;

				writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSets[1].descriptorCount = 1;
				writeDescriptorSets[1].dstSet = descriptorSets[i].scene;
				writeDescriptorSets[1].dstBinding = 1;
				writeDescriptorSets[1].pBufferInfo = &uniformBuffers[i].params.descriptor;

				writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets[2].descriptorCount = 1;
				writeDescriptorSets[2].dstSet = descriptorSets[i].scene;
				writeDescriptorSets[2].dstBinding = 2;
				writeDescriptorSets[2].pImageInfo = &textures.irradianceCube.descriptor;

				writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets[3].descriptorCount = 1;
				writeDescriptorSets[3].dstSet = descriptorSets[i].scene;
				writeDescriptorSets[3].dstBinding = 3;
				writeDescriptorSets[3].pImageInfo = &textures.prefilteredCube.descriptor;

				writeDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets[4].descriptorCount = 1;
				writeDescriptorSets[4].dstSet = descriptorSets[i].scene;
				writeDescriptorSets[4].dstBinding = 4;
				writeDescriptorSets[4].pImageInfo = &textures.lutBrdf.descriptor;

				vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
			}
		}

		// Material (samplers)
		{
			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
				{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
				{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
				{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
				{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
				{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
			};
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
			descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
			descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.material));

			// Per-Material descriptor sets
			for (auto &material : models.scene.materials) {
				VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
				descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				descriptorSetAllocInfo.descriptorPool = descriptorPool;
				descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayouts.material;
				descriptorSetAllocInfo.descriptorSetCount = 1;
				VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &material.descriptorSet));

				std::vector<VkDescriptorImageInfo> imageDescriptors = {
					textures.empty.descriptor,
					textures.empty.descriptor,
					material.normalTexture ? material.normalTexture->descriptor : textures.empty.descriptor,
					material.occlusionTexture ? material.occlusionTexture->descriptor : textures.empty.descriptor,
					material.emissiveTexture ? material.emissiveTexture->descriptor : textures.empty.descriptor
				};

				// TODO: glTF specs states that metallic roughness should be preferred, even if specular glosiness is present

				if (material.pbrWorkflows.metallicRoughness) {
					if (material.baseColorTexture) {
						imageDescriptors[0] = material.baseColorTexture->descriptor;
					}
					if (material.metallicRoughnessTexture) {
						imageDescriptors[1] = material.metallicRoughnessTexture->descriptor;
					}
				}

				if (material.pbrWorkflows.specularGlossiness) {
					if (material.extension.diffuseTexture) {
						imageDescriptors[0] = material.extension.diffuseTexture->descriptor;
					}
					if (material.extension.specularGlossinessTexture) {
						imageDescriptors[1] = material.extension.specularGlossinessTexture->descriptor;
					}
				}

				std::array<VkWriteDescriptorSet, 5> writeDescriptorSets{};
				for (size_t i = 0; i < imageDescriptors.size(); i++) {
					writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					writeDescriptorSets[i].descriptorCount = 1;
					writeDescriptorSets[i].dstSet = material.descriptorSet;
					writeDescriptorSets[i].dstBinding = static_cast<uint32_t>(i);
					writeDescriptorSets[i].pImageInfo = &imageDescriptors[i];
				}

				vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
			}

			// Model node (matrices)
			{
				std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
					{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr },
				};
				VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
				descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
				descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
				VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.node));

				// Per-Node descriptor set
				for (auto &node : models.scene.nodes) {
					setupNodeDescriptorSet(node);
				}
			}

		}

		// Skybox (fixed set)
		for (auto i = 0; i < uniformBuffers.size(); i++) {
			VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
			descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocInfo.descriptorPool = descriptorPool;
			descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayouts.scene;
			descriptorSetAllocInfo.descriptorSetCount = 1;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSets[i].skybox));

			std::array<VkWriteDescriptorSet, 3> writeDescriptorSets{};

			writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[0].descriptorCount = 1;
			writeDescriptorSets[0].dstSet = descriptorSets[i].skybox;
			writeDescriptorSets[0].dstBinding = 0;
			writeDescriptorSets[0].pBufferInfo = &uniformBuffers[i].skybox.descriptor;

			writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[1].descriptorCount = 1;
			writeDescriptorSets[1].dstSet = descriptorSets[i].skybox;
			writeDescriptorSets[1].dstBinding = 1;
			writeDescriptorSets[1].pBufferInfo = &uniformBuffers[i].params.descriptor;

			writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSets[2].descriptorCount = 1;
			writeDescriptorSets[2].dstSet = descriptorSets[i].skybox;
			writeDescriptorSets[2].dstBinding = 2;
			writeDescriptorSets[2].pImageInfo = &textures.prefilteredCube.descriptor;

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateCI.lineWidth = 1.0f;

		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
		colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCI.attachmentCount = 1;
		colorBlendStateCI.pAttachments = &blendAttachmentState;

		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.depthTestEnable = VK_FALSE;
		depthStencilStateCI.depthWriteEnable = VK_FALSE;
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilStateCI.front = depthStencilStateCI.back;
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;

		VkPipelineViewportStateCreateInfo viewportStateCI{};
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.viewportCount = 1;
		viewportStateCI.scissorCount = 1;

		VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

		if (settings.multiSampling) {
			multisampleStateCI.rasterizationSamples = settings.sampleCount;
		}

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicStateCI{};
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		// Pipeline layout
		const std::vector<VkDescriptorSetLayout> setLayouts = {
			descriptorSetLayouts.scene, descriptorSetLayouts.material, descriptorSetLayouts.node
		};
		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		pipelineLayoutCI.pSetLayouts = setLayouts.data();
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.size = sizeof(PushConstBlockMaterial);
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pipelineLayoutCI.pushConstantRangeCount = 1;
		pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

		// Vertex bindings an attributes
		VkVertexInputBindingDescription vertexInputBinding = { 0, sizeof(vkglTF::Model::Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3 },
			{ 2, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6 },
			{ 3, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 8 },
			{ 4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 10 },
			{ 5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 14 },
			{ 6, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 18 }
		};
		VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
		vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCI.vertexBindingDescriptionCount = 1;
		vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
		vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes.data();

		// Pipelines
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = pipelineLayout;
		pipelineCI.renderPass = renderPass;
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pVertexInputState = &vertexInputStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

		if (settings.multiSampling) {
			multisampleStateCI.rasterizationSamples = settings.sampleCount;
		}

		// Skybox pipeline (background cube)
		shaderStages = {
			loadShader(device, "skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(device, "skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.skybox));
		for (auto shaderStage : shaderStages) {
			vkDestroyShaderModule(device, shaderStage.module, nullptr);
		}

		// PBR pipeline
		shaderStages = {
			loadShader(device, "pbr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(device, "pbr_khr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};
		depthStencilStateCI.depthWriteEnable = VK_TRUE;
		depthStencilStateCI.depthTestEnable = VK_TRUE;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.pbr));
		rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.pbrDoubleSided));

		rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.pbrAlphaBlend));

		for (auto shaderStage : shaderStages) {
			vkDestroyShaderModule(device, shaderStage.module, nullptr);
		}
	}

	/*
		Generate a BRDF integration map storing roughness/NdotV as a look-up-table
	*/
	void generateBRDFLUT()
	{
		auto tStart = std::chrono::high_resolution_clock::now();

		const VkFormat format = VK_FORMAT_R16G16_SFLOAT;
		const int32_t dim = 512;

		// Image
		VkImageCreateInfo imageCI{};
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = format;
		imageCI.extent.width = dim;
		imageCI.extent.height = dim;
		imageCI.extent.depth = 1;
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &textures.lutBrdf.image));
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device, textures.lutBrdf.image, &memReqs);
		VkMemoryAllocateInfo memAllocInfo{};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &textures.lutBrdf.deviceMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device, textures.lutBrdf.image, textures.lutBrdf.deviceMemory, 0));

		// View
		VkImageViewCreateInfo viewCI{};
		viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCI.format = format;
		viewCI.subresourceRange = {};
		viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCI.subresourceRange.levelCount = 1;
		viewCI.subresourceRange.layerCount = 1;
		viewCI.image = textures.lutBrdf.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &viewCI, nullptr, &textures.lutBrdf.view));

		// Sampler
		VkSamplerCreateInfo samplerCI{};
		samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCI.magFilter = VK_FILTER_LINEAR;
		samplerCI.minFilter = VK_FILTER_LINEAR;
		samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCI.minLod = 0.0f;
		samplerCI.maxLod = 1.0f;
		samplerCI.maxAnisotropy = 1.0f;
		samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerCI, nullptr, &textures.lutBrdf.sampler));

		// FB, Att, RP, Pipe, etc.
		VkAttachmentDescription attDesc{};
		// Color attachment
		attDesc.format = format;
		attDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpassDescription{};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;

		// Use subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 2> dependencies;
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Create the actual renderpass
		VkRenderPassCreateInfo renderPassCI{};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = 1;
		renderPassCI.pAttachments = &attDesc;
		renderPassCI.subpassCount = 1;
		renderPassCI.pSubpasses = &subpassDescription;
		renderPassCI.dependencyCount = 2;
		renderPassCI.pDependencies = dependencies.data();

		VkRenderPass renderpass;
		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderpass));

		VkFramebufferCreateInfo framebufferCI{};
		framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCI.renderPass = renderpass;
		framebufferCI.attachmentCount = 1;
		framebufferCI.pAttachments = &textures.lutBrdf.view;
		framebufferCI.width = dim;
		framebufferCI.height = dim;
		framebufferCI.layers = 1;

		VkFramebuffer framebuffer;
		VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferCI, nullptr, &framebuffer));

		// Desriptors
		VkDescriptorSetLayout descriptorsetlayout;
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
		descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorsetlayout));

		// Pipeline layout
		VkPipelineLayout pipelinelayout;
		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.setLayoutCount = 1;
		pipelineLayoutCI.pSetLayouts = &descriptorsetlayout;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelinelayout));

		// Pipeline
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
		rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateCI.lineWidth = 1.0f;

		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
		colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCI.attachmentCount = 1;
		colorBlendStateCI.pAttachments = &blendAttachmentState;

		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.depthTestEnable = VK_FALSE;
		depthStencilStateCI.depthWriteEnable = VK_FALSE;
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilStateCI.front = depthStencilStateCI.back;
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;

		VkPipelineViewportStateCreateInfo viewportStateCI{};
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.viewportCount = 1;
		viewportStateCI.scissorCount = 1;

		VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCI{};
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		
		VkPipelineVertexInputStateCreateInfo emptyInputStateCI{};
		emptyInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = pipelinelayout;
		pipelineCI.renderPass = renderpass;
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pVertexInputState = &emptyInputStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.stageCount = 2;
		pipelineCI.pStages = shaderStages.data();

		// Look-up-table (from BRDF) pipeline		
		shaderStages = {
			loadShader(device, "genbrdflut.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(device, "genbrdflut.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};
		VkPipeline pipeline;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
		for (auto shaderStage : shaderStages) {
			vkDestroyShaderModule(device, shaderStage.module, nullptr);
		}

		// Render
		VkClearValue clearValues[1];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderpass;
		renderPassBeginInfo.renderArea.extent.width = dim;
		renderPassBeginInfo.renderArea.extent.height = dim;
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = framebuffer;

		VkCommandBuffer cmdBuf = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.width = (float)dim;
		viewport.height = (float)dim;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.extent.width = dim;
		scissor.extent.height = dim;

		vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
		vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdDraw(cmdBuf, 3, 1, 0, 0);
		vkCmdEndRenderPass(cmdBuf);
		vulkanDevice->flushCommandBuffer(cmdBuf, queue);

		vkQueueWaitIdle(queue);

		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelinelayout, nullptr);
		vkDestroyRenderPass(device, renderpass, nullptr);
		vkDestroyFramebuffer(device, framebuffer, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorsetlayout, nullptr);

		textures.lutBrdf.descriptor.imageView = textures.lutBrdf.view;
		textures.lutBrdf.descriptor.sampler = textures.lutBrdf.sampler;
		textures.lutBrdf.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		textures.lutBrdf.device = vulkanDevice;

		auto tEnd = std::chrono::high_resolution_clock::now();
		auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
		LOG_INFO("Generating BRDF LUT took {} ms", tDiff);
	}

	/* 
		Prepare and initialize uniform buffers containing shader parameters
	*/
	void prepareUniformBuffers()
	{
		for (auto &uniformBuffer : uniformBuffers) {
			uniformBuffer.scene.create(vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(shaderValuesScene));
			uniformBuffer.skybox.create(vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(shaderValuesSkybox));
			uniformBuffer.params.create(vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(shaderValuesParams));
		}
		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		// Scene
		shaderValuesScene.projection = camera->matrices.perspective;
		shaderValuesScene.view = camera->matrices.view;
		
		// Center and scale model
		float scale = (1.0f / std::max(models.scene.aabb[0][0], std::max(models.scene.aabb[1][1], models.scene.aabb[2][2]))) * 0.5f;
		glm::vec3 translate = -glm::vec3(models.scene.aabb[3][0], models.scene.aabb[3][1], models.scene.aabb[3][2]);
		translate += -0.5f * glm::vec3(models.scene.aabb[0][0], models.scene.aabb[1][1], models.scene.aabb[2][2]);

		models.scene.transform->SetPositon(translate);
		models.scene.transform->SetScale(glm::vec3(scale));
		shaderValuesScene.model = models.scene.transform->GetLocalToWorldMatrix();

		shaderValuesScene.camPos = glm::vec3(
			-camera->transform->position.z * sin(glm::radians(camera->transform->eulerAngle.y)) * cos(glm::radians(camera->transform->eulerAngle.x)),
			-camera->transform->position.z * sin(glm::radians(camera->transform->eulerAngle.x)),
			 camera->transform->position.z * cos(glm::radians(camera->transform->eulerAngle.y)) * cos(glm::radians(camera->transform->eulerAngle.x))
		);

		// Skybox
		shaderValuesSkybox.projection = camera->matrices.perspective;
		shaderValuesSkybox.view = camera->matrices.view;
		shaderValuesSkybox.model = glm::mat4(glm::mat3(camera->matrices.view));
	}

	void updateParams()
	{
		shaderValuesParams.lightDir = glm::vec4(
			sin(glm::radians(lightSource->transform->eulerAngle.x)) * cos(glm::radians(lightSource->transform->eulerAngle.y)),
			sin(glm::radians(lightSource->transform->eulerAngle.y)),
			cos(glm::radians(lightSource->transform->eulerAngle.x)) * cos(glm::radians(lightSource->transform->eulerAngle.y)),
			0.0f);
		shaderValuesParams.lightColor = lightSource->color * lightSource->intensity;
	}

	void windowResized()
	{
		recordCommandBuffers();
		vkDeviceWaitIdle(device);
		updateUniformBuffers();
		updateOverlay();
	}

	void prepare()
	{
		ApplicationBase::prepare();

		camera = std::make_shared<Camera>();
		camera->type = Camera::CameraType::lookat;

		camera->setPerspective(45.0f, (float)width / (float)height, 0.1f, 256.0f);
		camera->rotationSpeed = 0.25f;
		camera->movementSpeed = 0.1f;
		camera->setPosition({ 0.0f, 0.0f, 1.0f });
		camera->setRotation({ 0.0f, 0.0f, 0.0f });

		lightSource = new DirectionalLight();

		waitFences.resize(renderAhead);
		presentCompleteSemaphores.resize(renderAhead);
		renderCompleteSemaphores.resize(renderAhead);
		commandBuffers.resize(swapChain.imageCount);
		uniformBuffers.resize(swapChain.imageCount);
		descriptorSets.resize(swapChain.imageCount);
		// Command buffer execution fences
		for (auto &waitFence : waitFences) {
			VkFenceCreateInfo fenceCI{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &waitFence));
		}
		// Queue ordering semaphores
		for (auto &semaphore : presentCompleteSemaphores) {
			VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
		}
		for (auto &semaphore : renderCompleteSemaphores) {
			VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
		}
		// Command buffers
		{
			VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
			cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufAllocateInfo.commandPool = cmdPool;
			cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdBufAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
			VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, commandBuffers.data()));
		}

		loadAssets();
		generateBRDFLUT();

		//generate cubemap
		uint32_t numMips = 0;
		generateCubemaps(device, vulkanDevice, queue, pipelineCache, models.skybox, numMips,
			textures.environmentCube, textures.irradianceCube, textures.prefilteredCube);
		shaderValuesParams.prefilteredCubeMipLevels = static_cast<float>(numMips);

		prepareUniformBuffers();
		setupDescriptors();
		preparePipelines();

		ui = new UI(vulkanDevice, renderPass, queue, pipelineCache, settings.sampleCount);
		updateOverlay();

		recordCommandBuffers();

		prepared = true;
	}

	/*
		Update ImGui user interface
	*/
	void updateOverlay()
	{
		ImGuiIO& io = ImGui::GetIO();

		ImVec2 lastDisplaySize = io.DisplaySize;
		io.DisplaySize = ImVec2((float)width, (float)height);
		io.DeltaTime = frameTimer;

		io.MousePos = ImVec2(mousePos.x, mousePos.y);
		io.MouseDown[0] = mouseButtons.left;
		io.MouseDown[1] = mouseButtons.right;

		ui->pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
		ui->pushConstBlock.translate = glm::vec2(-1.0f);

		bool updateShaderParams = false;
		bool updateCBs = false;
		float scale = 1.0f;

		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(10, 10));
		ImGui::SetNextWindowSize(ImVec2(200 * scale, (models.scene.animations.size() > 0 ? 470 : 390) * scale), ImGuiSetCond_Always);
		ImGui::Begin("Hummingbird Engine", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		ImGui::PushItemWidth(100.0f * scale);

		ui->text("%.1d fps (%.2f ms)", lastFPS, (1000.0f / lastFPS));

		if (ui->header("Scene")) {
			if (ui->button("Open gltf file")) {

				char const* lFilterPatterns[2] = { "*.gltf", "*.glb" };
				std::string defaultPath = GetModelPath() +"DamagedHelmet/glTF/DamagedHelmet.gltf";
				std::string fileName;
				if (OpenFileDialog(fileName, "Select a glTF file to load", defaultPath.c_str(), lFilterPatterns, 2, "gltf files")) {
					vkDeviceWaitIdle(device);
					loadScene(std::string(fileName));
					setupDescriptors();
					updateCBs = true;
				}
				else {
					ShowErrorMessageBox("Failed to open glTF file");
				}
			}
			if (ui->button("Take screenshot")) {
				std::string screenShotSavePath = GetScreenshotPath() + "screenshot.jpg";
				saveScreenshot(screenShotSavePath);
			}
			if (screenshotSaved) {
				ui->text("Screenshot saved");
			}
			if (ui->combo("Environment", selectedEnvironment, environments)) {
				vkDeviceWaitIdle(device);
				loadEnvironment(environments[selectedEnvironment]);
				setupDescriptors();
				updateCBs = true;
			}
		}

		if (ui->header("Environment")) {
			if (ui->checkbox("Background", &displayBackground)) {
				updateShaderParams = true;
			}
			if (ui->slider("Exposure", &shaderValuesParams.exposure, 0.1f, 10.0f)) {
				updateShaderParams = true;
			}
			if (ui->slider("Gamma", &shaderValuesParams.gamma, 0.1f, 4.0f)) {
				updateShaderParams = true;
			}
			if (ui->slider("IBL", &shaderValuesParams.scaleIBLAmbient, 0.0f, 1.0f)) {
				updateShaderParams = true;
			}
		}

		if (ui->header("Debug view")) {
			const std::vector<std::string> debugNamesInputs = {
				"none", "Base color", "Normal", "Occlusion", "Emissive", "Metallic", "Roughness"
			};
			if (ui->combo("Inputs", &debugViewInputs, debugNamesInputs)) {
				shaderValuesParams.debugViewInputs = static_cast<float>(debugViewInputs);
				updateShaderParams = true;
			}
			const std::vector<std::string> debugNamesEquation = {
				"none", "Diff (l,n)", "F (l,h)", "G (l,v,h)", "D (h)", "Specular"
			};
			if (ui->combo("PBR equation", &debugViewEquation, debugNamesEquation)) {
				shaderValuesParams.debugViewEquation = static_cast<float>(debugViewEquation);
				updateShaderParams = true;
			}
		}

		if (models.scene.animations.size() > 0) {
			if (ui->header("Animations")) {
				ui->checkbox("Animate", &animate);
				std::vector<std::string> animationNames;
				for (auto animation : models.scene.animations) {
					animationNames.push_back(animation.name);
				}
				ui->combo("Animation", &animationIndex, animationNames);
			}
		}

		ImGui::PopItemWidth();
		ImGui::End();
		ImGui::Render();

		ImDrawData* imDrawData = ImGui::GetDrawData();

		// Check if ui buffers need to be recreated
		if (imDrawData) {
			VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
			VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

			bool updateBuffers = (ui->vertexBuffer.buffer == VK_NULL_HANDLE) || (ui->vertexBuffer.count != imDrawData->TotalVtxCount) || (ui->indexBuffer.buffer == VK_NULL_HANDLE) || (ui->indexBuffer.count != imDrawData->TotalIdxCount);

			if (updateBuffers) {
				vkDeviceWaitIdle(device);
				if (ui->vertexBuffer.buffer) {
					ui->vertexBuffer.destroy();
				}
				ui->vertexBuffer.create(vulkanDevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBufferSize);
				ui->vertexBuffer.count = imDrawData->TotalVtxCount;
				if (ui->indexBuffer.buffer) {
					ui->indexBuffer.destroy();
				}
				ui->indexBuffer.create(vulkanDevice, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, indexBufferSize);
				ui->indexBuffer.count = imDrawData->TotalIdxCount;
			}

			// Upload data
			ImDrawVert* vtxDst = (ImDrawVert*)ui->vertexBuffer.mapped;
			ImDrawIdx* idxDst = (ImDrawIdx*)ui->indexBuffer.mapped;
			for (int n = 0; n < imDrawData->CmdListsCount; n++) {
				const ImDrawList* cmd_list = imDrawData->CmdLists[n];
				memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
				memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
				vtxDst += cmd_list->VtxBuffer.Size;
				idxDst += cmd_list->IdxBuffer.Size;
			}

			ui->vertexBuffer.flush();
			ui->indexBuffer.flush();

			updateCBs = updateCBs || updateBuffers;
		}

		if (lastDisplaySize.x != io.DisplaySize.x || lastDisplaySize.y != io.DisplaySize.y) {
			updateCBs = true;
		}

		if (updateCBs) {
			vkDeviceWaitIdle(device);
			recordCommandBuffers();
			vkDeviceWaitIdle(device);
		}

		if (updateShaderParams) {
			updateParams();
		}
	}

	virtual void render()
	{
		if (!prepared) {
			return;
		}

		updateOverlay();

		VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFences[frameIndex], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[frameIndex]));

		VkResult acquire = swapChain.acquireNextImage(presentCompleteSemaphores[frameIndex], &currentBuffer);
		if ((acquire == VK_ERROR_OUT_OF_DATE_KHR) || (acquire == VK_SUBOPTIMAL_KHR)) {
			windowResize();
		}
		else {
			VK_CHECK_RESULT(acquire);
		}

		// Update UBOs
		updateUniformBuffers();
		UniformBufferSet currentUB = uniformBuffers[currentBuffer];
		memcpy(currentUB.scene.mapped, &shaderValuesScene, sizeof(shaderValuesScene));
		memcpy(currentUB.params.mapped, &shaderValuesParams, sizeof(shaderValuesParams));
		memcpy(currentUB.skybox.mapped, &shaderValuesSkybox, sizeof(shaderValuesSkybox));

		const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = &waitDstStageMask;
		submitInfo.pWaitSemaphores = &presentCompleteSemaphores[frameIndex];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderCompleteSemaphores[frameIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentBuffer];
		submitInfo.commandBufferCount = 1;
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[frameIndex]));

		VkResult present = swapChain.queuePresent(queue, currentBuffer, renderCompleteSemaphores[frameIndex]);
		if (!((present == VK_SUCCESS) || (present == VK_SUBOPTIMAL_KHR))) {
			if (present == VK_ERROR_OUT_OF_DATE_KHR) {
				windowResize();
				return;
			}
			else {
				VK_CHECK_RESULT(present);
			}
		}

		frameIndex += 1;
		frameIndex %= renderAhead;

		if (!paused) {
			if (rotateModel) {
				models.scene.transform->Rotate(glm::vec3(0, frameTimer * 30.0f, 0));
			}
			if ((animate) && (models.scene.animations.size() > 0)) {
				animationTimer += frameTimer;
				if (animationTimer > models.scene.animations[animationIndex].end) {
					animationTimer -= models.scene.animations[animationIndex].end;
				}
				models.scene.updateAnimation(animationIndex, animationTimer);
			}
			updateParams();
			if (rotateModel) {
				updateUniformBuffers();
			}
		}
		if (camera->updated) {
			updateUniformBuffers();
		}
	}

	virtual void fileDropped(std::string filename)
	{
		vkDeviceWaitIdle(device);
		loadScene(filename);
		setupDescriptors();
		recordCommandBuffers();
	}

};

ApplicationExample *applicationExample;

int main() {
	try {
		applicationExample = new ApplicationExample();
		applicationExample->initWindow();
		applicationExample->initVulkan();
		applicationExample->prepare();
		applicationExample->mainLoop();
		delete(applicationExample);
	}
	catch (const std::exception& e) {
		LOG_ERROR(e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
#pragma once

#include "Macros.h"
#include "RenderPipelineCore.hpp"
#include "ScriptableRendererFeature.hpp"
#include "ScriptableRenderPass.hpp"

class ScriptableRenderer
{
public:
	std::string name; 

	std::vector<ScriptableRendererFeature> rendererFeatures;
	std::vector<std::shared_ptr<ScriptableRenderPass>> activeRenderPasses;

	ScriptableRenderer() {
		name = "ScriptableRenderer";
	}

	virtual ~ScriptableRenderer() {
	}

	void Dispose()
	{
		Dispose(true);
	}

	void EnqueuePass(std::shared_ptr<ScriptableRenderPass> pass)
	{
		activeRenderPasses.emplace_back(pass);
	}

	void SetCameraMatrices(/*CommandBuffer cmd, */std::shared_ptr<CameraData> cameraData, bool setInverseMatrices) {}
	void SetPerCameraShaderVariables(/*CommandBuffer cmd, */std::shared_ptr<CameraData> cameraData) {}
	//void SetShaderTimeValues(/*CommandBuffer cmd, */CameraData cameraData) {}
	virtual void AddRenderPasses(std::shared_ptr<RenderingData> renderingData) = 0;

	virtual void Execute(std::shared_ptr<RenderingData> renderingData) {
		SetupLights(renderingData);

		for (auto pass : activeRenderPasses) {
			ExecuteRenderPass(pass, renderingData);
		}
	}

	void SetRenderPassAttachments(std::shared_ptr<ScriptableRenderPass> renderPass, std::shared_ptr<CameraData> cameraData) {}

	void ExecuteRenderPass(std::shared_ptr<ScriptableRenderPass> renderPass, std::shared_ptr<RenderingData> renderingData) {
		renderPass->Configure();

		SetRenderPassAttachments(renderPass, renderingData->cameraData);

		renderPass->Execute(renderingData);
	}

	virtual void Setup(std::shared_ptr<RenderingData> renderingData) = 0;
	virtual void SetupLights(std::shared_ptr<RenderingData> renderingData) = 0;
	virtual void Dispose(bool disposing) = 0;
};
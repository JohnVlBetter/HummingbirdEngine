#pragma once

#include "Macros.h"
#include "RenderPipelineCore.hpp"
#include "ScriptableRendererFeature.hpp"
#include "ScriptableRenderPass.hpp"
#include "RenderBlocks.hpp"

class ScriptableRenderer
{
private:
	int MainRenderingOpaque = 0;
	int MainRenderingTransparent = 1;
	int AfterRendering = 2;

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

	void SetCameraMatrices(/*CommandBuffer cmd, */std::shared_ptr<CameraData> cameraData) {
		//Matrix4x4 viewMatrix = cameraData.GetViewMatrix();
		//Matrix4x4 projectionMatrix = cameraData.GetProjectionMatrix();

		// TODO: Investigate why SetViewAndProjectionMatrices is causing y-flip / winding order issue
		// for now using cmd.SetViewProjecionMatrices
		//SetViewAndProjectionMatrices(cmd, viewMatrix, cameraData.GetDeviceProjectionMatrix(), setInverseMatrices);
		//cmd.SetViewProjectionMatrices(viewMatrix, projectionMatrix);
	}
	
	void SetPerCameraShaderVariables(/*CommandBuffer cmd, */std::shared_ptr<CameraData> cameraData) {
		//SetGlobalVector(ShaderPropertyId.projectionParams, projectionParams);
		SetCameraMatrices(cameraData);
	}

	void SetShaderTimeValues(/*CommandBuffer cmd, */std::shared_ptr<CameraData> cameraData) {}
	virtual void AddRenderPasses(std::shared_ptr<RenderingData> renderingData) = 0;

	void StableSortPass(std::vector<std::shared_ptr<ScriptableRenderPass>>& list)
	{
		int j;
		for (int i = 1; i < list.size(); ++i)
		{
			std::shared_ptr<ScriptableRenderPass> curr = list[i];
			j = i - 1;
			for (; j >= 0 && curr->renderPassEvent < list[j]->renderPassEvent; --j)
				list[j + 1] = list[j];

			list[j + 1] = curr;
		}
	}

	virtual void Execute(std::shared_ptr<RenderingData> renderingData) {

		StableSortPass(activeRenderPasses);

		SetupLights(renderingData);
		SetShaderTimeValues(renderingData->cameraData);
		SetPerCameraShaderVariables(renderingData->cameraData);

		auto renderBlocks = new RenderBlocks(activeRenderPasses);

		ExecuteBlock(renderBlocks, MainRenderingOpaque, renderingData);
		ExecuteBlock(renderBlocks, MainRenderingTransparent, renderingData);
		ExecuteBlock(renderBlocks, AfterRendering, renderingData);
	}

	void ExecuteBlock(RenderBlocks* renderBlocks, int blockIdx, std::shared_ptr<RenderingData> renderingData) {
		int start, length;
		renderBlocks->GetRange(start, length, blockIdx);
		
		for (int idx = start; idx < length + start; ++idx) {
			ExecuteRenderPass(activeRenderPasses[idx], renderingData);
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
	virtual void Clear() = 0;
};
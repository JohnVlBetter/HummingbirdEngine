#pragma once

#include "Macros.h"
#include "RenderPipelineCore.hpp"
#include "ScriptableRendererFeature.hpp"
#include "ScriptableRenderPass.hpp"

class ScriptableRenderer
{
protected:
public:
	std::string name; 
	
	std::shared_ptr<ScriptableRenderer> current;
	std::vector<ScriptableRendererFeature> rendererFeatures;
	std::vector<ScriptableRenderPass> activeRenderPasses;

	ScriptableRenderer() {
		name = "ScriptableRenderer";
	}

	virtual ~ScriptableRenderer() {
	}

	void Dispose()
	{
		Dispose(true);
	}

	void EnqueuePass(ScriptableRenderPass pass)
	{
		activeRenderPasses.emplace_back(pass);
	}

	void SetCameraMatrices(/*CommandBuffer cmd, */CameraData cameraData, bool setInverseMatrices) {}
	void SetPerCameraShaderVariables(/*CommandBuffer cmd, */CameraData cameraData) {}
	//void SetShaderTimeValues(/*CommandBuffer cmd, */CameraData cameraData) {}
	virtual void AddRenderPasses(RenderingData renderingData) = 0;
	virtual void Execute(RenderingData renderingData) = 0;
	virtual void Setup(RenderingData renderingData) = 0;
	virtual void SetupLights(RenderingData renderingData) = 0;
	virtual void ExecuteRenderPass(ScriptableRenderPass renderPass, RenderingData renderingData) = 0;
	virtual void Dispose(bool disposing) = 0;
};
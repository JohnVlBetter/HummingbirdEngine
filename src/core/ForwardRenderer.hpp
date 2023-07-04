#pragma once

#include "ScriptableRenderer.hpp"

class ForwardRenderer : public ScriptableRenderer
{
public:

	ForwardRenderer() {
		LOG_INFO("Create ForwardRenderer");
		name = "ForwardRenderer";
	}

	~ForwardRenderer() {
		LOG_INFO("Delete ForwardRenderer");
	}

	void AddRenderPasses(std::shared_ptr<RenderingData> renderingData) {}
	void Execute(std::shared_ptr<RenderingData> renderingData) {}
	void Setup(std::shared_ptr<RenderingData> renderingData) {}
	void SetupLights(std::shared_ptr<RenderingData> renderingData) {}
	void ExecuteRenderPass(std::shared_ptr<ScriptableRenderPass> renderPass, std::shared_ptr<RenderingData> renderingData) {}
	void Dispose(bool disposing) {}
};
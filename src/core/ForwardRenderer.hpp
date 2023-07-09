#pragma once

#include "ScriptableRenderer.hpp"
#include "DrawObjectsPass.hpp"
#include "DrawSkyboxPass.hpp"

class ForwardRenderer : public ScriptableRenderer
{
public:
	std::shared_ptr<DrawSkyboxPass> drawSkyboxPass;
	std::shared_ptr<DrawObjectsPass> renderOpaqueForwardPass;
	std::shared_ptr<DrawObjectsPass> renderTransparentForwardPass;

	ForwardRenderer() {
		LOG_INFO("Create ForwardRenderer");
		name = "ForwardRenderer";
		drawSkyboxPass = std::make_shared<DrawSkyboxPass>();
		renderOpaqueForwardPass = std::make_shared<DrawObjectsPass>();
		renderTransparentForwardPass = std::make_shared<DrawObjectsPass>();
	}

	~ForwardRenderer() {
		LOG_INFO("Delete ForwardRenderer");
	}

	void Setup(std::shared_ptr<RenderingData> renderingData) {
		AddRenderPasses(renderingData);

		EnqueuePass(drawSkyboxPass);
		EnqueuePass(renderOpaqueForwardPass);
		EnqueuePass(renderTransparentForwardPass);
	}
	void SetupLights(std::shared_ptr<RenderingData> renderingData) {}

	void AddRenderPasses(std::shared_ptr<RenderingData> renderingData) {
		for (int i = 0; i < rendererFeatures.size(); ++i)
		{
			if (!rendererFeatures[i].isActive)
			{
				continue;
			}

			rendererFeatures[i].AddRenderPasses(this, renderingData);
		}
	}

	void Dispose(bool disposing) {}
	void Clear() {}
};
#pragma once

#include "ScriptableRenderer.hpp"
#include "DrawObjectsPass.hpp"
#include "DrawSkyboxPass.hpp"
#include "ScreenSpaceAmbientOcclusion.hpp"

class ForwardRenderer : public ScriptableRenderer
{
public:
	DrawSkyboxPass* drawSkyboxPass;
	DrawObjectsPass* renderOpaqueForwardPass;
	DrawObjectsPass* renderTransparentForwardPass;

	ForwardRenderer() {
		LOG_INFO("Create ForwardRenderer");
		name = "ForwardRenderer";
		drawSkyboxPass = new DrawSkyboxPass();
		renderOpaqueForwardPass = new DrawObjectsPass();
		renderTransparentForwardPass = new DrawObjectsPass();

		rendererFeatures.emplace_back(new ScreenSpaceAmbientOcclusion());
	}

	~ForwardRenderer() {
		LOG_INFO("Delete ForwardRenderer");
	}

	void Setup() {
		for (const auto& rf : rendererFeatures) {
			rf->Create();
		}

		AddRenderPasses();

		EnqueuePass(drawSkyboxPass);
		EnqueuePass(renderOpaqueForwardPass);
		EnqueuePass(renderTransparentForwardPass);
	}
	void SetupLights(RenderingData* renderingData) {}

	void AddRenderPasses() {
		for (int i = 0; i < rendererFeatures.size(); ++i)
		{
			if (!rendererFeatures[i]->isActive)
			{
				continue;
			}

			rendererFeatures[i]->AddRenderPasses(this);
		}
	}

	void Dispose(bool disposing) {
		delete renderTransparentForwardPass;
		delete renderOpaqueForwardPass;
		delete drawSkyboxPass;
	}
	void Clear() {}
};
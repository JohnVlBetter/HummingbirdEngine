#pragma once

#include "ScriptableRendererFeature.hpp"
#include "ScreenSpaceAmbientOcclusionPass.hpp"


class ScreenSpaceAmbientOcclusion : public ScriptableRendererFeature
{
public:
	std::shared_ptr<ScreenSpaceAmbientOcclusionPass> pass;

	ScreenSpaceAmbientOcclusion() {
		LOG_INFO("Create ScreenSpaceAmbientOcclusion");
		name = "ScreenSpaceAmbientOcclusion";
		isActive = true;
	}

	virtual ~ScreenSpaceAmbientOcclusion() {
		LOG_INFO("Delete ScreenSpaceAmbientOcclusion");
		Dispose();
	}
	
	void SetActive(bool active)
	{
		isActive = active;
	}

	void Dispose()
	{
		Dispose(true);
	}

	virtual void AddRenderPasses(ScriptableRenderer* renderer, std::shared_ptr<RenderingData> renderingData) {
		pass->Setup();
		renderer->EnqueuePass(pass);
	}
	virtual void Dispose(bool disposing) {
	}
	virtual void Create() {
		pass = std::make_shared<ScreenSpaceAmbientOcclusionPass>();
	}
};
#pragma once

#include "ScriptableRendererFeature.hpp"
#include "ScreenSpaceAmbientOcclusionPass.hpp"


class ScreenSpaceAmbientOcclusion : public ScriptableRendererFeature
{
public:
	ScreenSpaceAmbientOcclusionPass* pass;

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

	virtual void AddRenderPasses(ScriptableRenderer* renderer) {
		pass->Setup();
		renderer->EnqueuePass(pass);
	}
	virtual void Dispose(bool disposing) {
		delete pass;
	}
	virtual void Create() {
		pass = new ScreenSpaceAmbientOcclusionPass();
	}
};
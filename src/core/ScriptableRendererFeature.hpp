#pragma once

#include "Macros.h"
#include "RenderPipelineCore.hpp"
#include "ScriptableRenderer.hpp"

class ScriptableRenderer;

class ScriptableRendererFeature
{
public:
	std::string name;
	bool isActive;

	ScriptableRendererFeature() {
		LOG_INFO("Create ScriptableRendererFeature");
		name = "ScriptableRendererFeature";
		isActive = true;
	}

	virtual ~ScriptableRendererFeature() {
		LOG_INFO("Delete ScriptableRendererFeature");
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

	virtual void AddRenderPasses(ScriptableRenderer* renderer) = 0;
	virtual void Dispose(bool disposing) = 0;
	virtual void Create() = 0;
};
#pragma once

#include "Macros.h"
#include "RenderPipelineCore.hpp"
#include "ScriptableRenderer.hpp"

class ScriptableRendererFeature
{
protected:
	bool isActive;
public:
	std::string name;

	ScriptableRendererFeature() {
		name = "ScriptableRendererFeature";
		isActive = true;
	}

	virtual ~ScriptableRendererFeature() {
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

	virtual void AddRenderPasses(ScriptableRenderer renderer, RenderingData& renderingData) = 0;
	virtual void Dispose(bool disposing) = 0;
	virtual void Create() = 0;
};
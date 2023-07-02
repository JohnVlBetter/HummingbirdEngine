#pragma once

#include "Macros.h"
#include "Camera.hpp"
#include "ScriptableRenderer.hpp"

class ScriptableRenderPipeline
{
protected:
public:
	std::string name;

	ScriptableRenderPipeline() {
		name = "ScriptableRenderPipeline";
	}

	virtual ~ScriptableRenderPipeline() {
	}

	virtual void Render(Camera cameras[]) = 0;
};
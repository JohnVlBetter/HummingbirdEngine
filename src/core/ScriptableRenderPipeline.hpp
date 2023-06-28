#pragma once

#include "Macros.h"
#include "Camera.hpp"

class ScriptableRenderPipeline
{
protected:
public:
	std::string name;

	ScriptableRenderPipeline() {
		name = "ScriptableRenderPipeline";
	}

	~ScriptableRenderPipeline() {
	}

	virtual void Render(Camera cameras[]) = 0;
};
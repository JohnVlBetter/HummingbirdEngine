#pragma once

#include "ScriptableRenderPipeline.hpp"

class RasteriztionRenderPipeline : public ScriptableRenderPipeline
{
private:
public:
	RasteriztionRenderPipeline() {
		name = "RasteriztionRenderPipeline";
	}

	~RasteriztionRenderPipeline() {
	}

	void Render(std::vector<Camera*> cameras) {
		//SortCameras(cameras);

		for (int i = 0; i < cameras.size(); ++i) {
			auto camera = cameras[i];
			CameraData cameraData;
			InitializeCameraData(camera, cameraData);
			LOG_INFO("Camear Data:{}", cameraData.camera->type);
			RenderSingleCamera(cameraData);
		}
	}

	static void InitializeCameraData(Camera* camera, CameraData& cameraData) {
		//test
		cameraData.camera = camera;
	}
	static void RenderSingleCamera(CameraData camera) {}
};
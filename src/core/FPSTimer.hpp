#pragma once

#include "Macros.h"

class FPSTimer {
public:
	float frameTimer = 1.0f;
	uint32_t lastFPS = 0;

	void tickBegin() {
		tStart = std::chrono::high_resolution_clock::now();
	}

	void tickEnd() {
		frameCounter++;
		auto tEnd = std::chrono::high_resolution_clock::now();
		auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
		frameTimer = (float)tDiff / 1000.0f;
		fpsTimer += (float)tDiff;
		if (fpsTimer > 1000.0f) {
			lastFPS = static_cast<uint32_t>((float)frameCounter * (1000.0f / fpsTimer));
			fpsTimer = 0.0f;
			frameCounter = 0;
		}
	}

private:
	std::chrono::time_point<std::chrono::steady_clock> tStart;
	float fpsTimer = 0.0f;
	uint32_t frameCounter = 0;
};
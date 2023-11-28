#ifndef SRC_SETTINGS_H_
#define SRC_SETTINGS_H_

#include <string>

#include "constants.h"
#include "typedefs.h"

struct DisplaySettings {
	/* Is window in full screen mode */
	bool fullscreen = false;
    /* Window width (pixels) */
	int width = 1280;
	/* Window height (pixels) */
	int height = 720;
	/* Anti-aliasing samples */
	int samples = 0;
	/* GLFW swap interval value, 0 - unlimited fps, 1 - vsync*/
	int swapInterval = 1;
	/* Window title */
	const char* title = "VoxelEngine-Cpp v" ENGINE_VERSION;
};

struct ChunksSettings {
    /* Max milliseconds that engine uses for chunks loading only */
	uint loadSpeed = 10;
	/* Radius of chunks loading zone (chunk is unit) */
	uint loadDistance = 22;
	/* Buffer zone where chunks are not unloading (chunk is unit)*/
	uint padding = 2;
};

struct CameraSettings {
    /* Camera dynamic field of view effects */
    bool fovEvents = true;
    /* Camera movement shake */
    bool shaking = true;
};

struct GraphicsSettings {
	/* Fog opacity is calculated as `pow(depth*k, fogCurve)` where k depends on chunksLoadDistance.
	   Use values in range [1.0 - 2.0] where 1.0 is linear, 2.0 is quadratic */
	float fogCurve = 1.6f;
	/* Enable blocks backlight to prevent complete darkness */
	bool backlight = true;
};

struct DebugSettings {
	/* Turns off chunks saving/loading */
	bool generatorTestMode = false;
	bool showChunkBorders = false;
};

struct EngineSettings {
    DisplaySettings display;
	ChunksSettings chunks;
	CameraSettings camera;
	GraphicsSettings graphics;
	DebugSettings debug;
};

#endif // SRC_SETTINGS_H_
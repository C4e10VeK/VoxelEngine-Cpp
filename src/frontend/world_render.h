#ifndef WORLD_RENDERER_CPP
#define WORLD_RENDERER_CPP

#include <vector>
#include <algorithm>
#include <GL/glew.h>
#include <string>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../graphics/GfxContext.h"

class Level;
class Camera;
class LineBatch;
class ChunksRenderer;
class IShader;
class ITexture;
class Frustum;
class Engine;
class Chunks;
class ContentGfxCache;
class Skybox;

class WorldRenderer {
	Engine* engine;
	Level* level;
	Frustum* frustumCulling;
	LineBatch* lineBatch;
	ChunksRenderer* renderer;
	Skybox* skybox;
	bool drawChunk(size_t index, Camera* camera, IShader* shader, bool occlusion);
	void drawChunks(Chunks* chunks, Camera* camera, IShader* shader, bool occlusion);
public:
	WorldRenderer(Engine* engine, Level* level, const ContentGfxCache* cache);
	~WorldRenderer();

	void draw(const GfxContext& context, Camera* camera, bool occlusion);
	void drawDebug(const GfxContext& context, Camera* camera);
};


#endif // WORLD_RENDERER_CPP

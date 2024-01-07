#include "engine.h"

#include <memory>
#include <iostream>
#include <assert.h>
#include <vector>
#include <glm/glm.hpp>
#include <filesystem>
#define GLEW_STATIC

#include "audio/Audio.h"
#include "assets/Assets.h"
#include "assets/AssetsLoader.h"
#include "window/Window.h"
#include "window/Events.h"
#include "window/Camera.h"
#include "window/input.h"

#include "graphics-common/graphicsDefenitions.h"

#ifdef USE_VULKAN
#include "graphics-vk/VulkanContext.h"
#else
#include "graphics/Shader.h"
#endif

#include "graphics/ImageData.h"
#include "frontend/gui/GUI.h"
#include "frontend/screens.h"
#include "frontend/menu.h"
#include "util/platform.h"

#include "coders/json.h"
#include "coders/png.h"
#include "coders/GLSLExtension.h"
#include "files/files.h"
#include "files/engine_paths.h"
#include "graphics-common/IShader.h"

#include "content/Content.h"
#include "content/ContentPack.h"
#include "content/ContentLoader.h"
#include "frontend/locale/langs.h"
#include "logic/scripting/scripting.h"

#include "definitions.h"

namespace fs = std::filesystem;

Engine::Engine(EngineSettings& settings, EnginePaths* paths)
	   : settings(settings), paths(paths) {
	if (Window::initialize(settings.display)){
		throw initialize_error("could not initialize window");
	}

#ifdef USE_VULKAN
	vulkan::VulkanContext::initialize();
#endif

    auto resdir = paths->getResources();
    scripting::initialize(paths);

	std::cout << "-- loading assets" << std::endl;
    std::vector<fs::path> roots {resdir};
    resPaths.reset(new ResPaths(resdir, roots));
    assets.reset(new Assets());
	AssetsLoader loader(assets.get(), resPaths.get());
	AssetsLoader::createDefaults(loader);
	AssetsLoader::addDefaults(loader, true);

#ifndef USE_VULKAN
    Shader::preprocessor->setPaths(resPaths.get());
#endif
	while (loader.hasNext()) {
		if (!loader.loadNext()) {
			assets.reset();
#ifdef USE_VULKAN
			vulkan::VulkanContext::finalize();
#endif
			Window::terminate();
			throw initialize_error("could not to initialize assets");
		}
	}

	Audio::initialize();
	gui = std::make_unique<gui::GUI>();
    if (settings.ui.language == "auto") {
        settings.ui.language = langs::locale_by_envlocale(platform::detect_locale(), paths->getResources());
    }
    setLanguage(settings.ui.language);
	std::cout << "-- initializing finished" << std::endl;
}

void Engine::updateTimers() {
	frame++;
	double currentTime = Window::time();
	delta = currentTime - lastTime;
	lastTime = currentTime;
}

void Engine::updateHotkeys() {
	if (Events::jpressed(keycode::F2)) {
		std::unique_ptr<ImageData> image(Window::takeScreenshot());
		image->flipY();
		fs::path filename = paths->getScreenshotFile("png");
		png::write_image(filename.string(), image.get());
		std::cout << "saved screenshot as " << filename << std::endl;
	}
	if (Events::jpressed(keycode::F11)) {
		Window::toggleFullscreen();
	}
}

void Engine::mainloop() {
    setScreen(std::make_shared<MenuScreen>(this));
	
	std::cout << "-- preparing systems" << std::endl;

	Batch2D batch(5000);
	lastTime = Window::time();

	while (!Window::isShouldClose()){
		assert(screen != nullptr);
		updateTimers();
		updateHotkeys();

		gui->act(delta);
		screen->update(delta);

		if (!Window::isIconified()) {
			screen->draw(delta);
			gui->draw(&batch, assets.get());

#ifdef USE_VULKAN
			vulkan::VulkanContext::get().draw();
#else
            Window::swapInterval(settings.display.swapInterval);
#endif
        }
		Window::swapBuffers();
		Events::pollEvents();
	}
#ifdef USE_VULKAN
	vulkan::VulkanContext::waitIdle();
#endif
}

Engine::~Engine() {
    scripting::close();
	screen = nullptr;

	Audio::finalize();

	std::cout << "-- shutting down" << std::endl;
    assets.reset();
#ifdef USE_VULKAN
	vulkan::VulkanContext::finalize();
#endif
	Window::terminate();
	std::cout << "-- engine finished" << std::endl;
}

void Engine::loadContent() {
    auto resdir = paths->getResources();
    ContentBuilder contentBuilder;
    setup_definitions(&contentBuilder);
    
    std::vector<fs::path> resRoots;
    for (auto& pack : contentPacks) {
        ContentLoader loader(&pack);
        loader.load(&contentBuilder);
        resRoots.push_back(pack.folder);
    }
    content.reset(contentBuilder.build());
    resPaths.reset(new ResPaths(resdir, resRoots));

#ifndef USE_VULKAN
    Shader::preprocessor->setPaths(resPaths.get());
#endif

    std::unique_ptr<Assets> new_assets(new Assets());
	std::cout << "-- loading assets" << std::endl;
	AssetsLoader loader(new_assets.get(), resPaths.get());
    AssetsLoader::createDefaults(loader);
    AssetsLoader::addDefaults(loader, false);
	while (loader.hasNext()) {
		if (!loader.loadNext()) {
			new_assets.reset();
			throw std::runtime_error("could not to load assets");
		}
	}
    assets->extend(*new_assets.get());
}

void Engine::loadAllPacks() {
	auto resdir = paths->getResources();
	contentPacks.clear();
	ContentPack::scan(resdir/fs::path("content"), contentPacks);
}

void Engine::setScreen(std::shared_ptr<Screen> screen) {
	this->screen = screen;
}

void Engine::setLanguage(std::string locale) {
	settings.ui.language = locale;
	langs::setup(paths->getResources(), locale, contentPacks);
	menus::create_menus(this, gui->getMenu());
}

gui::GUI* Engine::getGUI() {
	return gui.get();
}

EngineSettings& Engine::getSettings() {
	return settings;
}

Assets* Engine::getAssets() {
	return assets.get();
}

const Content* Engine::getContent() const {
	return content.get();
}

std::vector<ContentPack>& Engine::getContentPacks() {
    return contentPacks;
}

EnginePaths* Engine::getPaths() {
	return paths;
}

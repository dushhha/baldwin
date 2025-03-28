#include "engine.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

#include "renderer/vulkan/vulkan_renderer.hpp"

namespace baldwin
{

Engine* loadedEngine = nullptr;
Engine& get() { return *loadedEngine; }

Engine::Engine(int width, int height, RenderAPI api)
  : _width(width)
  , _height(height)
  , _api(api)
{
    assert(loadedEngine == nullptr && "Another engine is already running");

#ifndef NDEBUG
    std::cout << "=== Engine init === \n";
#endif

    if (!initWindow())
        throw(::std::runtime_error("Could not init GLFW window"));

    switch (_api)
    {
        // NOTE: Always defaults to vulkan for now
        default:
            _renderer = std::make_unique<vk::VulkanRenderer>(
              _window, _width, _height, false);
    }
}

bool Engine::initWindow()
{
    if (glfwInit() != 1)
        throw(std::runtime_error("Could not call glfwInit"));

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    _window = glfwCreateWindow(
      _width, _height, "Baldwin Engine", nullptr, nullptr);

    glfwMakeContextCurrent(_window);
    glfwSetWindowUserPointer(_window, this);
    glfwSetWindowSizeCallback(_window, resizeCallback);

    return _window != nullptr;
}

void Engine::resizeCallback(GLFWwindow* w, int width, int height)
{
    if (width > 0 && height > 0)
    {
        Engine* engine = static_cast<Engine*>(glfwGetWindowUserPointer(w));

        if (engine)
            engine->getRenderer()->resizeSwapchain(width, height);
    }
}

void Engine::addToScene(const std::vector<std::shared_ptr<Mesh>>& meshes)
{
    for (auto& mesh : meshes)
    {
        _scene.push_back(mesh);
        _renderer->uploadMesh(mesh);
    }
    std::cout << "Scene size :" << _scene.size() << std::endl;
}

void Engine::run()
{
#ifndef NDEBUG
    std::cout << "=== Engine run === \n";
#endif

    while (!glfwWindowShouldClose(_window))
    {
        glfwPollEvents();
        _renderer->render(_frame, _scene);
        _frame++;
    }
}

Engine::~Engine()
{
#ifndef NDEBUG
    std::cout << "=== Engine cleanup === \n";
#endif

    loadedEngine = nullptr;
    _renderer.reset();
    glfwDestroyWindow(_window);
    glfwTerminate();
}

} // namespace baldwin

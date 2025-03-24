#include "engine.hpp"

#include <cassert>
#include <iostream>

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

    assert(loadedEngine == nullptr);
    // TODO: replace assert by user error handling
    assert(initWindow() == true);
    switch (_api)
    {
            /* Always defaults to vulkan for now */
        default:
            _renderer = std::move(std::make_unique<vk::VulkanRenderer>(
              _window, _width, _height, false));
    }

    std::cout << "- Engine init\n";
}

bool Engine::initWindow()
{
    assert(glfwInit() == 1);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    /*glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);*/

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
        {
            // engine->getRenderer()->resizeSwapchain(width, height);
        }
    }
}

void Engine::run()
{
    std::cout << "- Engine run\n";

    while (!glfwWindowShouldClose(_window))
    {
        glfwPollEvents();
        _renderer->run(_frame);
        _frame++;
    }
}

Engine::~Engine()
{
    glfwDestroyWindow(_window);
    glfwTerminate();

    std::cout << "- Engine cleanup\n";
}

} // namespace baldwin

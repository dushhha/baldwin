#pragma once

#include <memory>
#include <GLFW/glfw3.h>

#include "scene/mesh.hpp"
#include "renderer/renderer.hpp"

namespace baldwin
{

enum RenderAPI
{
    Vulkan = 0,
    DirectX12 = 1
};

class Engine
{
  public:
    static Engine& get();
    Engine(int width, int height, RenderAPI api);
    ~Engine();
    void run();

    Renderer* getRenderer() { return _renderer.get(); }
    void addToScene(const std::vector<std::shared_ptr<Mesh>>& meshes);

  private:
    bool initWindow();
    static void resizeCallback(GLFWwindow* w, int width, int height);
    bool initImgui();

    std::vector<std::shared_ptr<Mesh>> _scene;
    int _frame = 0;
    int _width, _height;
    GLFWwindow* _window = nullptr;
    const RenderAPI _api;
    std::unique_ptr<Renderer> _renderer;
};

} // namespace baldwin

#pragma once

#include <deque>
#include <memory>
#include <functional>
#include <GLFW/glfw3.h>

#include "render_types.hpp"

namespace baldwin
{

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void pushFunction(std::function<void()>&& function)
    {
        deletors.push_back(function);
    }

    void flush()
    {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
            (*it)();
        deletors.clear();
    }
};
class Renderer
{
  public:
    virtual ~Renderer() {};
    virtual void render(int frameNum,
                        const std::vector<std::shared_ptr<Mesh>>& scene) = 0;
    virtual void uploadMesh(const std::shared_ptr<Mesh> mesh) = 0;
    virtual void resizeSwapchain(int width, int height) = 0;
};

} // namespace baldwin

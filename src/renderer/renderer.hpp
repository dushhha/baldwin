#pragma once

#include <deque>
#include <functional>
#include <GLFW/glfw3.h>

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
        {
            (*it)(); // call the function
        }
        deletors.clear();
    }
};
class Renderer
{
  public:
    virtual void render(int frameNum) = 0;
    // virtual void resizeSwapchain(int width, int height) = 0;
};

} // namespace baldwin

#pragma once

#include <vulkan/vulkan.hpp>

#include <memory>

namespace polar
{

void vkCallImplementation(VkResult result, const char* command, const char* file, const char* function, int line);
#define VK_CALL(result) vkCallImplementation(result, #result, __FILE__, __func__, __LINE__)

template <typename F> class Defer
{
  public:
    Defer(F func) : m_func(func)
    {
    }
    ~Defer()
    {
        m_func();
    }

    Defer(const Defer&) = delete;
    Defer& operator=(const Defer&) = delete;
    Defer(Defer&&)                 = delete;
    Defer& operator=(Defer&&) = delete;

  private:
    F m_func;
};

template <typename T, auto F> using CustomUniquePtr = std::unique_ptr<T, std::integral_constant<decltype(F), F>>;

} // namespace polar
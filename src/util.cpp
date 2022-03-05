#include "util.hpp"

#include <spdlog/fmt/fmt.h>
#include <stdexcept>

namespace polar
{

void vkCallImplementation(VkResult result, const char* command, const char* file, const char* function, int line)
{
    if (result == VK_SUCCESS)
    {
        return;
    }

    throw std::runtime_error(
        fmt::format("Vulkan operation: {} failed with code: {} in file: {} in function: {} on line: {}", command, result, file, function, line));
}

} // namespace polar
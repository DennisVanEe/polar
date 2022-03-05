#pragma once

#include <cstdint>
#include <string_view>

#include <vulkan/vulkan.hpp>

namespace polar
{

class Context
{
  public:
    struct Param
    {
        bool enableValidation         = false;
        bool enableCallback           = false;
        bool enableRobustBufferAccess = false;
    };

    Context(const Param& param);

    Context(const Context&)            = delete;
    Context(Context&&)                 = delete;
    Context& operator=(const Context&) = delete;
    Context& operator=(Context&&)      = delete;

    const vk::Instance&       instance()       const { return *m_instance;      }
    const vk::Device&         device()         const { return *m_device;        }
    const vk::PhysicalDevice& physicalDevice() const { return m_physicalDevice; }

    const vk::Queue& queue()         const { return m_queue;         }
    const vk::Queue& transferQueue() const { return m_transferQueue; }
    const vk::Queue& computeQueue()  const { return m_computeQueue;  }

    std::uint32_t queueFamilyIndex()         const { return m_queueFamilyIndex;         }
    std::uint32_t transferQueueFamilyIndex() const { return m_transferQueueFamilyIndex; }
    std::uint32_t computeQueueFamilyIndex()  const { return m_computeQueueFamilyIndex;  }

  private:
    vk::DynamicLoader m_dynamicLoader;

    vk::UniqueInstance               m_instance;
    vk::UniqueDebugUtilsMessengerEXT m_debugUtilsMessenger;
    vk::UniqueDevice                 m_device;
    vk::PhysicalDevice               m_physicalDevice;

    vk::Queue m_queue;
    vk::Queue m_transferQueue;
    vk::Queue m_computeQueue;

    std::uint32_t m_queueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    std::uint32_t m_transferQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    std::uint32_t m_computeQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
};

constexpr std::uint64_t DEFAULT_FENCE_TIMEOUT = 6e+10;
void submitAndWait(const Context& context, vk::ArrayProxyNoTemporaries<const vk::CommandBuffer> commandBuffers, std::uint64_t timeout,
                   std::string_view description);

} // namespace polar
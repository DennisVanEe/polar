#include "context.hpp"

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <vector>

#include <configure.hpp>
#include <util.hpp>
#include <ranges>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace polar
{

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugUtilsMessengerCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT      msgSeverity,
    const VkDebugUtilsMessageTypeFlagsEXT             msgType,
    const VkDebugUtilsMessengerCallbackDataEXT* const callbackData,
    void* const                                       userData)
{
    const auto msgTypeStr = [&]() {
        switch (msgType)
        {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            return "[General]";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            return "[Performance]";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            return "[Validation]";
        default:
            return "[Unknown]";
        }
    }();

    switch (msgSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        spdlog::warn("{}: {}", msgTypeStr, callbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        spdlog::error("{}: {}", msgTypeStr, callbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    default:
        spdlog::info("{}: {}", msgTypeStr, callbackData->pMessage);
        break;
    }

    return VK_FALSE;
}

static std::vector<const char*>
getRequiredInstanceLayers(const Context::Param& param)
{
    if (param.enableValidation)
    {
        return {"VK_LAYER_KHRONOS_validation"};
    }

    return {};
}

static std::vector<const char*>
getRequiredInstanceExtensions(const Context::Param& param)
{
    if (param.enableCallback)
    {
        return {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    }

    return {};
}

static std::vector<const char*>
getRequiredDeviceExtensions(const Context::Param& param)
{
    return {
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
    };
}

Context::Context(const Param& param)
{
    const auto vkGetInstanceProcAddr = m_dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    const vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
        .messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
        .pfnUserCallback = debugUtilsMessengerCallback,
    };

    const auto requiredInstanceLayers     = getRequiredInstanceLayers(param);
    const auto requiredInstanceExtensions = getRequiredInstanceExtensions(param);
    const auto requiredDeviceExtensions   = getRequiredDeviceExtensions(param);

    //
    // Instance
    //

    const vk::ApplicationInfo applicationInfo{
        .pApplicationName   = PROJECT_NAME,
        .applicationVersion = VK_MAKE_API_VERSION(0, PROJECT_VER_MAJOR, PROJECT_VER_MINOR, PROJECT_VER_PATCH),
        .pEngineName        = ENGINE_NAME,
        .engineVersion      = VK_MAKE_API_VERSION(0, PROJECT_VER_MAJOR, PROJECT_VER_MINOR, PROJECT_VER_PATCH),
        .apiVersion         = VK_API_VERSION_1_2,
    };

    const vk::InstanceCreateInfo instanceCreateInfo{
        .pNext                   = param.enableCallback ? &debugUtilsMessengerCreateInfo : nullptr,
        .pApplicationInfo        = &applicationInfo,
        .enabledLayerCount       = static_cast<uint32_t>(requiredInstanceLayers.size()),
        .ppEnabledLayerNames     = requiredInstanceLayers.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(requiredInstanceExtensions.size()),
        .ppEnabledExtensionNames = requiredInstanceExtensions.data(),                                 
    };

    m_instance = vk::createInstanceUnique(instanceCreateInfo);

    spdlog::info("Created instance.");

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance);

    //
    // Debug Utils
    //

    if (param.enableCallback)
    {
        m_debugUtilsMessenger = m_instance->createDebugUtilsMessengerEXTUnique(debugUtilsMessengerCreateInfo);
    }

    //
    // Physical Device
    //

    const auto physicalDevices = m_instance->enumeratePhysicalDevices();

    const auto physicalDeviceItr = std::ranges::find_if(physicalDevices, [&](const vk::PhysicalDevice& physicalDevice) {
        const auto deviceExtensionProperties = physicalDevice.enumerateDeviceExtensionProperties();

        // Check that, for all required extension, it's present in deviceExtensionProperties.
        return std::ranges::all_of(requiredDeviceExtensions, [&](const char* const requiredDeviceExtension) {
            return std::ranges::any_of(deviceExtensionProperties, [&](const vk::ExtensionProperties& deviceExtensionProperties) {
                    return std::strcmp(deviceExtensionProperties.extensionName.data(), requiredDeviceExtension) == 0;
                });
        });
    });

    if (physicalDeviceItr == physicalDevices.end())
    {
        throw std::runtime_error("Could not find a physical device with all required extensions.");
    }

    m_physicalDevice = *physicalDeviceItr;

    const auto features = m_physicalDevice.getFeatures2<
        vk::PhysicalDeviceFeatures2, 
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>();

    const auto properties = m_physicalDevice.getProperties2<
        vk::PhysicalDeviceProperties2,
        vk::PhysicalDeviceVulkan11Properties,
        vk::PhysicalDeviceVulkan12Properties,
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    spdlog::info("Found compatible physical device: {}", properties.get<vk::PhysicalDeviceProperties2>().properties.deviceName.data());

    //
    // Device
    //

    const auto queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();

    // Create an array of queue create infos for all queue family properties:
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(queueFamilyProperties.size());

    std::uint32_t maxQueueCount = 0, queueFamilyIndex = 0;
    for (const auto& queueFamilyProperty : queueFamilyProperties)
    {
        queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo{
            .queueFamilyIndex = queueFamilyIndex,
            .queueCount       = queueFamilyProperty.queueCount,
        });
        maxQueueCount = std::max(maxQueueCount, queueFamilyProperty.queueCount);
    }

    // Set the priority of all queues to 1:
    std::vector<float> queuePriorities;
    queuePriorities.reserve(maxQueueCount);

    for (std::uint32_t i = 0; i < maxQueueCount; ++i)
    {
        queuePriorities.emplace_back(1.f);
    }

    for (auto& queueCreateInfo : queueCreateInfos)
    {
        queueCreateInfo.pQueuePriorities = queuePriorities.data();
    }

    const vk::DeviceCreateInfo deviceCreateInfo{
        .pNext                   = &features.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos       = queueCreateInfos.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = requiredDeviceExtensions.data(),
    };

    m_device = m_physicalDevice.createDeviceUnique(deviceCreateInfo);

    spdlog::info("Created virtual device.");

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);

    //
    // Queues
    //

    struct QueueScore
    {
        int            score          = 0;
        uint32_t       familyIndex    = 0;
        uint32_t       queueCount     = 0;
        uint32_t       currQueueCount = 0;
        vk::QueueFlags flags;
    };

    std::vector<QueueScore> queueScores;
    queueScores.reserve(queueFamilyProperties.size());

    uint32_t currFamilyIndex = 0;
    for (const auto& queueFamilyProperty : queueFamilyProperties)
    {
        const auto flags = queueFamilyProperty.queueFlags;
        queueScores.emplace_back(QueueScore{
            .score       = static_cast<bool>(vk::QueueFlagBits::eCompute & flags) +
                           static_cast<bool>(vk::QueueFlagBits::eGraphics & flags) +
                           static_cast<bool>(vk::QueueFlagBits::eTransfer & flags),
            .familyIndex = currFamilyIndex,
            .queueCount  = queueFamilyProperty.queueCount,
            .flags       = flags,
        });
        ++currFamilyIndex;
    }

    // Sort in such a way to give priority to queues with a low score:
    std::ranges::sort(queueScores, [&](const QueueScore& a, const QueueScore& b) { return a.score < b.score; });

    // Find a transfer, compute, and general queue:
    const auto findQueue = [&](const vk::QueueFlags& flags) {
        auto itr = std::ranges::find_if(queueScores, [&](const QueueScore& score) { return score.flags & flags; });

    };

    const auto queueItr = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
        [&](const vk::QueueFamilyProperties& queueFamilyProperty) {
            const auto flags = queueFamilyProperty.queueFlags;
            return (vk::QueueFlagBits::eCompute & flags) && (vk::QueueFlagBits::eGraphics & flags) && (vk::QueueFlagBits::eTransfer & flags);
        });

    if (queueItr == queueFamilyProperties.end())
    {
        throw std::runtime_error("Could not find a queue that supports graphics, compute, and transfer.");
    }

    m_queueFamilyIndex = std::distance(queueFamilyProperties.begin(), queueItr);
    m_queue = m_device->getQueue(m_queueFamilyIndex, 0);

    spdlog::info("Found graphics, compute, and transfer queue.");

    spdlog::info("Finished creating Vulkan context.");
}

const vk::Instance& Context::instance() const
{
    return *m_instance;
}

const vk::Device& Context::device() const
{
    return *m_device;
}

const vk::PhysicalDevice& Context::physicalDevice() const
{
    return m_physicalDevice;
}

std::uint32_t Context::queueFamilyIndex() const
{
    return m_queueFamilyIndex;
}

const vk::Queue& Context::queue() const
{
    return m_queue;
}

void submitAndWait(const Context& context, const vk::ArrayProxyNoTemporaries<const vk::CommandBuffer> commandBuffers, const std::uint64_t timeout,
                   const std::string_view description)
{
    for (const auto& commandBuffer : commandBuffers)
    {
        commandBuffer.end();
    }

    const auto fence = context.device().createFence(vk::FenceCreateInfo());

    context.queue().submit(vk::SubmitInfo().setCommandBuffers(commandBuffers), fence);

    if (context.device().waitForFences(fence, VK_TRUE, timeout) == vk::Result::eTimeout)
    {
        throw std::runtime_error(fmt::format("Fence timed out waiting on command submission for {}", description));
    }
}

} // namespace polar
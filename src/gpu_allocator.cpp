#include "gpu_allocator.hpp"

#include <util.hpp>

namespace polar
{

GPUBufferUnique::GPUBufferUnique(const vk::Buffer& buffer, const VmaAllocation allocation) : m_buffer(buffer), m_allocation(allocation)
{
}

GPUBufferUnique::GPUBufferUnique(GPUBufferUnique&& other)
{
    if (this == &other)
    {
        return;
    }

    m_buffer       = other.m_buffer;
    m_allocation   = other.m_allocation;
    other.m_buffer = VK_NULL_HANDLE;
}

GPUBufferUnique ::~GPUBufferUnique()
{
    if (m_buffer)
    {
        vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
    }
}

GPUBufferUnique& prism::GPUBufferUnique::operator=(GPUBufferUnique&& other)
{
    if (this == &other)
    {
        return *this;
    }

    // Free current memory first (if present):
    if (m_buffer)
    {
        vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
    }

    m_buffer       = other.m_buffer;
    m_allocation   = other.m_allocation;
    other.m_buffer = VK_NULL_HANDLE;

    return *this;
}

const vk::Buffer* GPUBufferUnique::operator->() const
{
    return &m_buffer;
}

const vk::Buffer& GPUBufferUnique::operator*() const
{
    return m_buffer;
}

const vk::Buffer& GPUBufferUnique::get() const
{
    return m_buffer;
}

GPUBufferUnique::operator bool() const
{
    return m_buffer;
}

vk::DeviceAddress GPUBufferUnique::deviceAddress(const Context& context) const
{
    return context.device().getBufferAddress(vk::BufferDeviceAddressInfo().setBuffer(m_buffer));
}

void* GPUBufferUnique::map() const
{
    void* memory{};
    VK_CALL(vmaMapMemory(m_allocator, m_allocation, &memory));
    return memory;
}

void GPUBufferUnique::unmap() const
{
    vmaUnmapMemory(m_allocator, m_allocation);
}

GPUAllocator::GPUAllocator(const Context& context)
{
    // We want to use the functions loaded from the dynamic dispatcher. I'm not a big fan of this implementation, I need
    // to look for a way to automate this process...
    VmaVulkanFunctions vmaVulkanFunctions{};
    vmaVulkanFunctions.vkGetPhysicalDeviceProperties       = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties;
    vmaVulkanFunctions.vkGetPhysicalDeviceMemoryProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties;
    vmaVulkanFunctions.vkAllocateMemory                    = VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory;
    vmaVulkanFunctions.vkFreeMemory                        = VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory;
    vmaVulkanFunctions.vkMapMemory                         = VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory;
    vmaVulkanFunctions.vkUnmapMemory                       = VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory;
    vmaVulkanFunctions.vkFlushMappedMemoryRanges           = VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges;
    vmaVulkanFunctions.vkInvalidateMappedMemoryRanges      = VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges;
    vmaVulkanFunctions.vkBindBufferMemory                  = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory;
    vmaVulkanFunctions.vkBindImageMemory                   = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory;
    vmaVulkanFunctions.vkGetBufferMemoryRequirements       = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements;
    vmaVulkanFunctions.vkGetImageMemoryRequirements        = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements;
    vmaVulkanFunctions.vkCreateBuffer                      = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer;
    vmaVulkanFunctions.vkDestroyBuffer                     = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer;
    vmaVulkanFunctions.vkCreateImage                       = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage;
    vmaVulkanFunctions.vkDestroyImage                      = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage;
    vmaVulkanFunctions.vkCmdCopyBuffer                     = VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer;
#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
    vmaVulkanFunctions.vkGetBufferMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2;
    vmaVulkanFunctions.vkGetImageMemoryRequirements2KHR  = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2;
#endif
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
    vmaVulkanFunctions.vkBindBufferMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2;
    vmaVulkanFunctions.vkBindImageMemory2KHR  = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2;
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
    vmaVulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties2;
#endif

    VmaAllocatorCreateInfo createInfo{};
    createInfo.flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    createInfo.physicalDevice   = context.physicalDevice();
    createInfo.device           = context.device();
    createInfo.pVulkanFunctions = &vmaVulkanFunctions;
    createInfo.instance         = context.instance();
    createInfo.vulkanApiVersion = VK_API_VERSION_1_2;

    VmaAllocator allocator{};
    VK_CALL(vmaCreateAllocator(&createInfo, &allocator));

    m_allocator.reset(allocator);
}

GPUBufferUnique GPUAllocator::allocate(const vk::DeviceSize size, const vk::BufferUsageFlags bufferUsage, const VmaMemoryUsage memoryUsage) const
{
    VmaAllocationCreateInfo allocationCreateInfo{};
    allocationCreateInfo.usage = memoryUsage;

    const auto bufferCreateInfo = vk::BufferCreateInfo().setSize(size).setUsage(bufferUsage);

    VkBuffer      buffer{};
    VmaAllocation allocation{};
    VK_CALL(
        vmaCreateBuffer(m_allocator.get(), &static_cast<VkBufferCreateInfo>(bufferCreateInfo), &allocationCreateInfo, &buffer, &allocation, nullptr));

    return GPUBufferUnique(buffer, allocation);
}

GPUBufferUnique GPUAllocator::addCopyStagingToBuffer(const vk::CommandBuffer& commandBuffer, const GPUBufferUnique& dstBuffer, void* const data,
                                                     const std::size_t dataSize) const
{
    auto stagingBuffer = allocate(dataSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

    std::memcpy(stagingBuffer.map(), data, dataSize);
    stagingBuffer.unmap();

    commandBuffer.copyBuffer(*stagingBuffer, *dstBuffer, vk::BufferCopy().setSize(dataSize));

    return stagingBuffer;
}

} // namespace polar
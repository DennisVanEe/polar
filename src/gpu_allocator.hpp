#pragma once

#include <vk_mem_alloc.h>

#include <cstddef>
#include <cstring>

#include <context.hpp>
#include <util.hpp>

namespace polar
{

class GPUBufferUnique
{
  public:
    GPUBufferUnique() = default;
    GPUBufferUnique(const vk::Buffer& buffer, VmaAllocation allocation);
    GPUBufferUnique(GPUBufferUnique&& buffer);
    ~GPUBufferUnique();

    GPUBufferUnique& operator=(GPUBufferUnique&& other);

    const vk::Buffer* operator->() const;
    const vk::Buffer& operator*() const;
    const vk::Buffer& get() const;

    operator bool() const;

    GPUBufferUnique(const GPUBufferUnique&) = delete;
    GPUBufferUnique& operator=(const GPUBufferUnique&) = delete;

    vk::DeviceAddress deviceAddress(const Context& context) const;
    void*             map() const;
    void              unmap() const;

  private:
    vk::Buffer    m_buffer     = {};
    VmaAllocation m_allocation = nullptr;
    VmaAllocator  m_allocator  = nullptr;
};

class GPUAllocator
{
  public:
    GPUAllocator(const Context& context);

    GPUAllocator(const GPUAllocator&) = delete;
    GPUAllocator(GPUAllocator&&)      = delete;
    GPUAllocator& operator=(const GPUAllocator&) = delete;
    GPUAllocator& operator=(GPUAllocator&&) = delete;

    GPUBufferUnique allocate(vk::DeviceSize size, vk::BufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage) const;

    // Adds copy operation by allocating a host staging buffer, copying data to it, and then adding the command that copies data from this
    // staging buffer to the destination buffer. Note that the staging buffer gets returned.
    GPUBufferUnique addCopyStagingToBuffer(const vk::CommandBuffer& commandBuffer, const GPUBufferUnique& dstBuffer, void* data,
                                           std::size_t dataSize) const;

  private:
    using UniqueVmaAllocator       = CustomUniquePtr<std::remove_pointer_t<VmaAllocator>, vmaDestroyAllocator>;
    UniqueVmaAllocator m_allocator = {};
};

} // namespace polar
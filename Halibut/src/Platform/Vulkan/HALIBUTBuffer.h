/*
vulkan 里面的 buffer，本质上就是一段“线性字节空间”
一个 buffer 的典型流程就是：
创建 vk::Buffer
查询 getMemoryRequirements()
选合适的 memory type
分配 vk::DeviceMemory
bindMemory
如有需要再 mapMemory
*/

#pragma once
#include "Core/Core.h"
#include "HALIBUTDevice.h"

namespace HALIBUT
{
    struct HALIBUTBufferDesc 
    {
        vk::DeviceSize Size = 0;
        vk::BufferUsageFlags Usage = {};
        vk::MemoryPropertyFlags MemoryProperties = {};
        vk::SharingMode SharingMode = vk::SharingMode::eExclusive;
        bool PersistentMap = false;
    };

    class HALIBUT_API HALIBUTBuffer
    {
    public:
        HALIBUTBuffer(HALIBUTDevice& device, const HALIBUTBufferDesc& desc);
        virtual ~HALIBUTBuffer();

        HALIBUTBuffer(const HALIBUTBuffer&) = delete;
        HALIBUTBuffer& operator=(const HALIBUTBuffer&) = delete;
        HALIBUTBuffer(HALIBUTBuffer&&) noexcept = default;
        HALIBUTBuffer& operator=(HALIBUTBuffer&&) noexcept = delete;

        // 给 host visible buffer 用。
        void* Map(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE);
        void  Unmap();
        // 往映射内存里写数据。
        void  Write(const void* data, vk::DeviceSize size, vk::DeviceSize offset = 0);
        // 当内存不是 HostCoherent 时要用。
        void  Flush(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE);
        void  Invalidate(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE);

        inline vk::Buffer GetBuffer() const { return *m_Buffer; }
        inline vk::DeviceSize GetSize() const { return m_Size; }
        bool IsHostVisible() const;
        inline bool IsMapped() const { return m_MappedPtr != nullptr; }
    private:
        HALIBUTDevice& m_Device;
        vk::raii::Buffer m_Buffer = nullptr;
        vk::raii::DeviceMemory m_Memory = nullptr;

        vk::DeviceSize m_Size = 0;
        vk::BufferUsageFlags m_Usage = {};
        vk::MemoryPropertyFlags m_MemoryProperties = {};

        void* m_MappedPtr = nullptr;
    };
}
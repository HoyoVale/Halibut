#pragma once
#include "Core/Core.h"
#include "HALIBUTBuffer.h"

namespace HALIBUT
{
    class HALIBUTRenderer;

    struct HALIBUTMeshBufferDesc
    {
        const void* VertexData = nullptr;
        vk::DeviceSize VertexDataSize = 0;
        uint32_t VertexStride = 0;

        const void* IndexData = nullptr;
        vk::DeviceSize IndexDataSize = 0;
        uint32_t IndexCount = 0;
        vk::IndexType IndexType = vk::IndexType::eUint32;
    };

    class HALIBUT_API HALIBUTMeshBuffer
    {
    public:
        HALIBUTMeshBuffer(HALIBUTDevice& device, const HALIBUTMeshBufferDesc& desc);
        ~HALIBUTMeshBuffer() = default;

        HALIBUTMeshBuffer(const HALIBUTMeshBuffer&) = delete;
        HALIBUTMeshBuffer& operator=(const HALIBUTMeshBuffer&) = delete;
        HALIBUTMeshBuffer(HALIBUTMeshBuffer&&) noexcept = default;
        HALIBUTMeshBuffer& operator=(HALIBUTMeshBuffer&&) noexcept = delete;

        void Bind(HALIBUTRenderer& renderer) const;
        void Draw(HALIBUTRenderer& renderer, uint32_t instanceCount = 1, uint32_t firstInstance = 0) const;

        bool HasIndexBuffer() const { return m_IndexBuffer != nullptr; }
        uint32_t GetVertexCount() const { return m_VertexCount; }
        uint32_t GetIndexCount() const { return m_IndexCount; }
        vk::IndexType GetIndexType() const { return m_IndexType; }
        HALIBUTBuffer& GetVertexBuffer() const { return *m_VertexBuffer; }
        HALIBUTBuffer* GetIndexBuffer() const { return m_IndexBuffer.get(); }

    private:
        static UPtr<HALIBUTBuffer> CreateDeviceLocalBuffer(
            HALIBUTDevice& device,
            const void* sourceData,
            vk::DeviceSize size,
            vk::BufferUsageFlags usage
        );

    private:
        UPtr<HALIBUTBuffer> m_VertexBuffer;
        UPtr<HALIBUTBuffer> m_IndexBuffer;
        uint32_t m_VertexCount = 0;
        uint32_t m_IndexCount = 0;
        vk::IndexType m_IndexType = vk::IndexType::eUint32;
    };
}

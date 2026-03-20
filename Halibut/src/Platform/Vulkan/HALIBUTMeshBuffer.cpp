#include "halibutpch.h"
#include "HALIBUTMeshBuffer.h"
#include "HALIBUTRenderer.h"

namespace HALIBUT
{
    UPtr<HALIBUTBuffer> HALIBUTMeshBuffer::CreateDeviceLocalBuffer(
        HALIBUTDevice& device,
        const void* sourceData,
        vk::DeviceSize size,
        vk::BufferUsageFlags usage
    )
    {
        if (sourceData == nullptr || size == 0)
        {
            throw std::runtime_error("mesh buffer upload source data is empty");
        }

        HALIBUTBufferDesc stagingBufferDesc{};
        stagingBufferDesc.Size = size;
        stagingBufferDesc.Usage = vk::BufferUsageFlagBits::eTransferSrc;
        stagingBufferDesc.MemoryProperties =
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        stagingBufferDesc.PersistentMap = true;

        HALIBUTBuffer stagingBuffer(device, stagingBufferDesc);
        stagingBuffer.Write(sourceData, size);
        stagingBuffer.Flush();

        HALIBUTBufferDesc deviceLocalBufferDesc{};
        deviceLocalBufferDesc.Size = size;
        deviceLocalBufferDesc.Usage = usage | vk::BufferUsageFlagBits::eTransferDst;
        deviceLocalBufferDesc.MemoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;

        UPtr<HALIBUTBuffer> deviceLocalBuffer = MakeUPtr<HALIBUTBuffer>(device, deviceLocalBufferDesc);
        device.CopyBufferImmediate(stagingBuffer.GetBuffer(), deviceLocalBuffer->GetBuffer(), size);
        return deviceLocalBuffer;
    }

    HALIBUTMeshBuffer::HALIBUTMeshBuffer(HALIBUTDevice& device, const HALIBUTMeshBufferDesc& desc)
    {
        if (desc.VertexData == nullptr || desc.VertexDataSize == 0)
        {
            throw std::runtime_error("mesh buffer requires vertex data");
        }
        if (desc.VertexStride == 0)
        {
            throw std::runtime_error("mesh buffer vertex stride must be greater than zero");
        }
        if ((desc.VertexDataSize % desc.VertexStride) != 0)
        {
            throw std::runtime_error("mesh buffer vertex data size must be a multiple of vertex stride");
        }

        m_VertexBuffer = CreateDeviceLocalBuffer(
            device,
            desc.VertexData,
            desc.VertexDataSize,
            vk::BufferUsageFlagBits::eVertexBuffer
        );
        m_VertexCount = static_cast<uint32_t>(desc.VertexDataSize / desc.VertexStride);

        if (desc.IndexData != nullptr || desc.IndexDataSize != 0 || desc.IndexCount != 0)
        {
            if (desc.IndexData == nullptr || desc.IndexDataSize == 0 || desc.IndexCount == 0)
            {
                throw std::runtime_error("mesh buffer index data is incomplete");
            }

            m_IndexBuffer = CreateDeviceLocalBuffer(
                device,
                desc.IndexData,
                desc.IndexDataSize,
                vk::BufferUsageFlagBits::eIndexBuffer
            );
            m_IndexCount = desc.IndexCount;
            m_IndexType = desc.IndexType;
        }
    }

    void HALIBUTMeshBuffer::Bind(HALIBUTRenderer& renderer) const
    {
        renderer.BindVertexBuffer(*m_VertexBuffer);
        if (m_IndexBuffer != nullptr)
        {
            renderer.BindIndexBuffer(*m_IndexBuffer, m_IndexType);
        }
    }

    void HALIBUTMeshBuffer::Draw(HALIBUTRenderer& renderer, uint32_t instanceCount, uint32_t firstInstance) const
    {
        if (m_IndexBuffer != nullptr)
        {
            renderer.DrawIndexed(m_IndexCount, instanceCount, 0, 0, firstInstance);
            return;
        }

        renderer.Draw(m_VertexCount, instanceCount, 0, firstInstance);
    }
}

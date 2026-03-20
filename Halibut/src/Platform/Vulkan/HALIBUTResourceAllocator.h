#pragma once
#include "Core/Core.h"
#include "HALIBUTDevice.h"

namespace HALIBUT
{
    class HALIBUT_API HALIBUTResourceAllocator
    {
    public:
        HALIBUTResourceAllocator(HALIBUTDevice& device);
        ~HALIBUTResourceAllocator();
    private:
        void CreateBuffer();
        void CreateImage();
        void DestoryBuffer();
        void DestoryImage();
    private:
        HALIBUTDevice& m_Device;


    };
}
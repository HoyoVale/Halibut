#pragma once
#include "Core.h"
#include "Timestep.h"
#include "Event/Event.h"

namespace HALIBUT
{
    class HALIBUTDevice;
    class HALIBUTSwapchain;
    class HALIBUTRenderer;

	class HALIBUT_API Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnImGuiRender(){}
		virtual void OnEvent(Event& event) {}
        virtual void OnSwapchainRecreated(HALIBUTDevice& device, HALIBUTSwapchain& swapchain, HALIBUTRenderer& renderer) {}

		inline const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};
}


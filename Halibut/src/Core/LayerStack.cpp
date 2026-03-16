#include "halibutpch.h"
#include "LayerStack.h"

namespace HALIBUT
{
	LayerStack::LayerStack()
	{
	}

	LayerStack::~LayerStack()
	{
        Clear();
	}

	void LayerStack::PushLayer(UPtr<Layer> layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, std::move(layer));
		m_LayerInsertIndex++;
	}

	void LayerStack::PushOverlay(UPtr<Layer> overlay)
	{
		m_Layers.emplace_back(std::move(overlay));
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto it = std::find_if(m_Layers.begin(), m_Layers.end(), [layer](const UPtr<Layer>& entry) {
			return entry.get() == layer;
		});
		if (it != m_Layers.end())
		{
			if (*it != nullptr)
			{
				(*it)->OnDetach();
			}
			m_Layers.erase(it);
			if (m_LayerInsertIndex > 0)
			{
				m_LayerInsertIndex--;
			}
		}
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find_if(m_Layers.begin(), m_Layers.end(), [overlay](const UPtr<Layer>& entry) {
			return entry.get() == overlay;
		});
		if (it != m_Layers.end())
		{
			if (*it != nullptr)
			{
				(*it)->OnDetach();
			}
			m_Layers.erase(it);
		}
	}

    void LayerStack::Clear()
    {
        for (UPtr<Layer>& layer : m_Layers)
        {
            if (layer != nullptr)
            {
                layer->OnDetach();
            }
        }
        m_Layers.clear();
        m_LayerInsertIndex = 0;
    }
}

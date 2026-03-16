#pragma once

#include "Core.h"
#include "Layer.h"

#include <vector>

namespace HALIBUT
{
	class HALIBUT_API LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(UPtr<Layer> layer);
		void PushOverlay(UPtr<Layer> overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);
        void Clear();

		std::vector<UPtr<Layer>>::iterator begin() { return m_Layers.begin(); }
		std::vector<UPtr<Layer>>::iterator end() { return m_Layers.end(); }
	private:
		std::vector<UPtr<Layer>> m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};

}

#include "halibutpch.h"
#include "Layer.h"

namespace HALIBUT
{
	Layer::Layer(const std::string& debugName)
	{
		m_DebugName = debugName;
	}

	Layer::~Layer(){}
}
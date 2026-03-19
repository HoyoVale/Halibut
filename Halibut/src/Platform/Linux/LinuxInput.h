#pragma once
#include "Core/Input.h"
#include <memory>

namespace HALIBUT
{
	class HALIBUT_API LinuxInput : public Input
	{
	protected:
		bool IsKeyPressedImpl(int keycode) override;
		bool IsMouseButtonPressedImpl(int button) override;
		std::pair<float, float> GetMousePositionImpl() override;
		float GetMouseXImpl() override;
		float GetMouseYImpl() override;
	};
}
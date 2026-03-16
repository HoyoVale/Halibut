#pragma once
#include <iostream>
#include "Application.h"
#include <exception>
#include <cstdlib>
#include <memory>

namespace HALIBUT
{
Application* CreateApplication();
}

int main(int argc, char** argv)
{
    HALIBUT::Application* App = HALIBUT::CreateApplication();
    App->Run();

}

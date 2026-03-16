#include "EditorLayer.h"
#include "core/EntryPoint.h"

namespace HALIBUT
{
    class  HoyoEditor : public HALIBUT::Application
{
    public:
        HoyoEditor()
            : Application("Halibut Application"/*, false*/)
        {
            PushLayer(MakeUPtr<EditorLayer>());
        }
        ~HoyoEditor(){};
    private:
};

Application* CreateApplication()
{
    return new HoyoEditor();
}
}

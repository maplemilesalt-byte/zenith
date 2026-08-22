#include "App.h"

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Core.h>

using namespace winrt;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;

namespace zenith::xbox::uwp
{
    void App::Initialize(CoreApplicationView const&)
    {
        // Stage 1 intentionally keeps initialization minimal.
        // Zenith core/ELF loading will be connected in a later stage.
    }

    void App::SetWindow(CoreWindow const&)
    {
        // Framebuffer/window integration comes in the graphics stage.
    }

    void App::Load(hstring const&)
    {
        // Reserved for future guest loading.
    }

    void App::Run()
    {
        // Keep the UWP host alive while the CoreApplication owns the view.
        CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(
            CoreProcessEventsOption::ProcessUntilQuit);
    }

    void App::Uninitialize()
    {
    }

    IFrameworkView AppSource::CreateView()
    {
        return make<App>();
    }
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    init_apartment();
    CoreApplication::Run(make<AppSource>());
    return 0;
}

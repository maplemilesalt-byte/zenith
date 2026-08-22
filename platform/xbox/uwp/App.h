#pragma once

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Core.h>

namespace zenith::xbox::uwp
{
    struct App final : winrt::implements<App, winrt::Windows::ApplicationModel::Core::IFrameworkView>
    {
        void Initialize(winrt::Windows::ApplicationModel::Core::CoreApplicationView const& view);
        void SetWindow(winrt::Windows::UI::Core::CoreWindow const& window);
        void Load(winrt::hstring const& entryPoint);
        void Run();
        void Uninitialize();
    };

    struct AppSource final : winrt::implements<AppSource, winrt::Windows::ApplicationModel::Core::IFrameworkViewSource>
    {
        winrt::Windows::ApplicationModel::Core::IFrameworkView CreateView();
    };
}

#pragma once

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Xaml.h>

namespace zenith::xbox::uwp
{
    struct App final : winrt::Windows::UI::Xaml::ApplicationT<App>
    {
        App();

        void OnLaunched(winrt::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs const& args);
    };
}

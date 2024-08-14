#pragma once
#include <guiddef.h>
#include <Unknwn.h>
#include <winrt/base.h>

// forward declare namespaces we alias
namespace winrt {
	namespace Microsoft::UI::Xaml::Controls {}
	namespace TranslucentTB::Xaml::Models::Primitives {}
	namespace Windows {
		namespace ApplicationModel {}
		namespace Foundation {
			namespace Collections {}
			namespace Numerics {}
		}
		namespace Graphics::Effects {}
		namespace UI::Xaml {
			namespace Controls {}
			namespace Hosting {}
		}
		namespace UI::Composition {}
	}
}

// alias some long namespaces for convenience
namespace mux = winrt::Microsoft::UI::Xaml;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;
namespace txmp = winrt::TranslucentTB::Xaml::Models::Primitives;
namespace wam = winrt::Windows::ApplicationModel;
namespace wf = winrt::Windows::Foundation;
namespace wfc = wf::Collections;
namespace wfn = wf::Numerics;
namespace wge = winrt::Windows::Graphics::Effects;
namespace wux = winrt::Windows::UI::Xaml;
namespace wuxc = wux::Controls;
namespace wuxh = wux::Hosting;
namespace wuc = winrt::Windows::UI::Composition;

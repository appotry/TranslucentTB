#include "api.hpp"
#include <WinBase.h>
#include <detours/detours.h>
#include <libloaderapi.h>
#include <wil/resource.h>

#include "constants.hpp"
#include "tapsite.hpp"
#include "win32.hpp"
#include "taskbarappearanceservice.hpp"

HRESULT InjectExplorerTAP(DWORD pid, REFIID riid, LPVOID* ppv) try
{
	TaskbarAppearanceService::InstallProxyStub();

	winrt::com_ptr<IUnknown> service;
	HRESULT hr = GetActiveObject(CLSID_TaskbarAppearanceService, nullptr, service.put());

	if (hr == MK_E_UNAVAILABLE)
	{
		const auto event = TAPSite::GetReadyEvent();

		wil::unique_process_handle proc(OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, false, pid));
		if (!proc) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		if (!DetourFindRemotePayload(proc.get(), EXPLORER_PAYLOAD, nullptr))
		{
			static constexpr uint32_t content = 0xDEADBEEF;
			if (!DetourCopyPayloadToProcess(proc.get(), EXPLORER_PAYLOAD, &content, sizeof(content))) [[unlikely]]
			{
				return HRESULT_FROM_WIN32(GetLastError());
			}
		}

		const auto [location, hr2] = win32::GetDllLocation(wil::GetModuleInstanceHandle());
		if (FAILED(hr2)) [[unlikely]]
		{
			return hr2;
		}

		const auto size = sizeof(wchar_t) * (location.native().size() + 1); // null terminator
		void* ptr = VirtualAllocEx(proc.get(), nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (!ptr)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		SIZE_T written = 0;
		if (!WriteProcessMemory(proc.get(), ptr, location.c_str(), size, &written))
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		if (written != size)
		{
			return HRESULT_FROM_WIN32(ERROR_PARTIAL_COPY);
		}

		wil::unique_handle handle(CreateRemoteThread(proc.get(), nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(&LoadLibrary), ptr, 0, nullptr));
		if (!handle)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		static constexpr DWORD LOAD_TIMEOUT =
#ifdef _DEBUG
			// do not timeout on debug builds, to allow debugging the DLL while it's loading in explorer
			INFINITE;
#else
			5000;
#endif

		if (WaitForSingleObject(handle.get(), LOAD_TIMEOUT) != WAIT_OBJECT_0)
		{
			return HRESULT_FROM_WIN32(WAIT_TIMEOUT);
		}

		DWORD exitCode = 0;
		if (!GetExitCodeThread(handle.get(), &exitCode))
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		if (!exitCode)
		{
			return E_FAIL;
		}

		static constexpr DWORD READY_TIMEOUT =
#ifdef _DEBUG
			// do not timeout on debug builds, to allow debugging the DLL while it's loading in explorer
			INFINITE;
#else
			35000;
#endif

		if (!event.wait(READY_TIMEOUT)) [[unlikely]]
		{
			return HRESULT_FROM_WIN32(WAIT_TIMEOUT);
		}

		hr = GetActiveObject(CLSID_TaskbarAppearanceService, nullptr, service.put());
	}

	if (FAILED(hr)) [[unlikely]]
	{
		return hr;
	}

	DWORD version = 0;
	hr = service.as<IVersionedApi>()->GetVersion(&version);
	if (SUCCEEDED(hr))
	{
		if (version != TAP_API_VERSION)
		{
			return HRESULT_FROM_WIN32(ERROR_PRODUCT_VERSION);
		}
	}
	else
	{
		return hr;
	}

	return service.as(riid, ppv);
}
catch (...)
{
	return winrt::to_hresult();
}

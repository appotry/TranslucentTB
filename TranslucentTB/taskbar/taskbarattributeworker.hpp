#pragma once
#include "arch.h"
#include <array>
#include <chrono>
#include <member_thunk/page.hpp>
#include <optional>
#include <ShObjIdl.h>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <wil/com.h>
#include <wil/resource.h>
#include "winrt.hpp"
#include <winrt/TranslucentTB.Xaml.Models.Primitives.h>
#include <winrt/Windows.Internal.Shell.Experience.h> // this is evil >:3
#include <winrt/WindowsUdk.UI.Shell.h> // this is less evil

#include "config/config.hpp"
#include "config/taskbarappearance.hpp"
#include "../dynamicloader.hpp"
#include "../ExplorerHooks/api.hpp"
#include "../ExplorerTAP/api.hpp"
#include "launchervisibilitysink.hpp"
#include "../windows/messagewindow.hpp"
#include "undoc/user32.hpp"
#include "undoc/uxtheme.hpp"
#include "util/color.hpp"
#include "util/null_terminated_string_view.hpp"
#include "wilx.hpp"
#include "../ProgramLog/error/win32.hpp"

class TaskbarAttributeWorker final : public MessageWindow {
private:
	class AttributeRefresher;
	friend AttributeRefresher;

	struct TaskbarInfo {
		Window TaskbarWindow;
		Window PeekWindow;
		Window InnerXamlContent;
		Window WorkerWWindow;
	};

	struct MonitorInfo {
		TaskbarInfo Taskbar;
		std::unordered_set<Window> MaximisedWindows;
		std::unordered_set<Window> NormalWindows;
	};

	// future improvements:
	// - better aero peek support: detect current peeked to window and include in calculation
	// - notification for owner changes
	// - notification for extended style changes
	// - notification for property changes: this is what is making the discord window not being correctly accounted for after restoring from tray
	// 	   for some reason it shows itself (which we do capture) and then unsets ITaskList_Deleted (which we don't capture)
	// - handle cases where we dont get EVENT_SYSTEM_FOREGROUND when unminimizing a window
	// - add an option for peek to consider main monitor only or all monitors
	// 	   if yes, should always refresh peek whenever anything changes
	// 	   and need some custom logic to check all monitors
	// - make settings optional so that they stack on top of each other?

	// The magic function that does the thing
	const PFN_SET_WINDOW_COMPOSITION_ATTRIBUTE SetWindowCompositionAttribute;
	const PFN_SHOULD_SYSTEM_USE_DARK_MODE ShouldSystemUseDarkMode;

	// State
	bool m_PowerSaver;
	bool m_TaskViewActive;
	bool m_PeekActive;
	bool m_disableAttributeRefreshReply;
	bool m_ResettingState;
	bool m_ResetStateReentered;
	HMONITOR m_CurrentStartMonitor;
	HMONITOR m_CurrentSearchMonitor;
	Window m_ForegroundWindow;
	std::unordered_map<HMONITOR, MonitorInfo> m_Taskbars;
	std::unordered_set<Window> m_NormalTaskbars;
	const Config &m_Config;

	// Hooks
	member_thunk::page m_ThunkPage;
	wil::unique_hwineventhook m_PeekUnpeekHook;
	wil::unique_hwineventhook m_CloakUncloakHook;
	wil::unique_hwineventhook m_MinimizeRestoreHook;
	wil::unique_hwineventhook m_ResizeMoveHook;
	wil::unique_hwineventhook m_ShowHideHook;
	wil::unique_hwineventhook m_CreateDestroyHook;
	wil::unique_hwineventhook m_ForegroundChangeHook;
	wil::unique_hwineventhook m_TitleChangeHook;
	wil::unique_hwineventhook m_ParentChangeHook;
	wil::unique_hwineventhook m_OrderChangeHook;
	wil::unique_hpowernotify m_PowerSaverHook;

	// IAppVisibility
	wil::com_ptr<IAppVisibility> m_IAV;
	wilx::unique_app_visibility_token m_IAVECookie;

	// ICortanaExperienceManager
	winrt::Windows::Internal::Shell::Experience::ICortanaExperienceManager m_SearchManager;
	winrt::event_token m_SuggestionsShownToken, m_SuggestionsHiddenToken;

	// ShellViewCoordinator
	winrt::WindowsUdk::UI::Shell::ShellViewCoordinator m_SearchViewCoordinator;
	winrt::event_token m_SearchViewVisibilityChangedToken;

	// Messages
	std::optional<UINT> m_TaskbarCreatedMessage;
	std::optional<UINT> m_RefreshRequestedMessage;
	std::optional<UINT> m_TaskViewVisibilityChangeMessage;
	std::optional<UINT> m_IsTaskViewOpenedMessage;
	std::optional<UINT> m_StartVisibilityChangeMessage;
	std::optional<UINT> m_SearchVisibilityChangeMessage;
	std::optional<UINT> m_ForceRefreshTaskbar;

	// Explorer crash detection
	std::chrono::steady_clock::time_point m_LastExplorerRestart;
	DWORD m_LastExplorerPid;

	// Color previews
	std::array<std::optional<Util::Color>, 7> m_ColorPreviews;

	// Hook DLL
	wil::unique_hmodule m_HookDll;
	PFN_INJECT_EXPLORER_HOOK m_InjectExplorerHook;
	std::vector<wil::unique_hhook> m_Hooks;

	// TAP DLL
	wil::unique_hmodule m_TAPDll;
	PFN_INJECT_EXPLORER_TAP m_InjectExplorerTAP;

	// Other
	bool m_IsWindows11;

	// Type aliases
	using taskbar_iterator = decltype(m_Taskbars)::iterator;

	// Callbacks
	template<DWORD insert, DWORD remove>
	void CALLBACK WindowInsertRemove(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD);

	void CALLBACK OnAeroPeekEnterExit(DWORD event, HWND, LONG, LONG, DWORD, DWORD);
	void CALLBACK OnWindowStateChange(DWORD, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD);
	void CALLBACK OnWindowCreateDestroy(DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD);
	void CALLBACK OnForegroundWindowChange(DWORD, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD);
	void CALLBACK OnWindowOrderChange(DWORD, HWND hwnd, LONG idObject, LONG idChild, DWORD, DWORD);
	void OnStartVisibilityChange(bool state);
	void OnTaskViewVisibilityChange(bool state);
	void OnSearchVisibilityChange(bool state);
	void OnForceRefreshTaskbar(Window taskbar);
	LRESULT OnSystemSettingsChange(UINT uiAction, std::wstring_view changedParameter);
	LRESULT OnPowerBroadcast(const POWERBROADCAST_SETTING *settings);
	LRESULT OnRequestAttributeRefresh(LPARAM lParam);
	LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// Config
	TaskbarAppearance GetConfig(taskbar_iterator taskbar) const;

	// Attribute
	void ShowAeroPeekButton(const TaskbarInfo &taskbar, bool show);
	void ShowTaskbarLine(const TaskbarInfo &taskbar, bool show);
	void SetAttribute(taskbar_iterator taskbar, TaskbarAppearance config);
	void RefreshAttribute(taskbar_iterator taskbar);
	void RefreshAllAttributes();

	// Log
	static void LogWindowInsertion(const std::pair<std::unordered_set<Window>::iterator, bool> &result, std::wstring_view state, HMONITOR mon);
	static void LogWindowRemoval(std::wstring_view state, Window window, HMONITOR mon);
	static void LogWindowRemovalDestroyed(std::wstring_view state, Window window, HMONITOR mon);

	// State
	void InsertWindow(Window window, bool refresh);

	template<void(*logger)(std::wstring_view, Window, HMONITOR) = LogWindowRemoval>
	void RemoveWindow(Window window, taskbar_iterator it, AttributeRefresher &refresher);

	// Other
	static bool SetNewWindowExStyle(Window wnd, LONG_PTR oldStyle, LONG_PTR newStyle);
	static bool SetContainsValidWindows(std::unordered_set<Window> &set);
	static void DumpWindowSet(std::wstring_view prefix, const std::unordered_set<Window> &set, bool showInfo = true);
	static std::wstring DumpWindow(Window window);
	void CreateAppVisibility();
	void CreateSearchManager();
	void UnregisterSearchCallbacks() noexcept;
	WINEVENTPROC CreateThunk(void (CALLBACK TaskbarAttributeWorker:: *proc)(DWORD, HWND, LONG, LONG, DWORD, DWORD));
	static wil::unique_hwineventhook CreateHook(DWORD eventMin, DWORD eventMax, WINEVENTPROC proc);
	void ReturnToStock();
	bool IsStartMenuOpened() const;
	bool IsSearchOpened() const;
	void InsertTaskbar(HMONITOR mon, Window window);
	std::filesystem::path GetDllPath(const std::optional<std::filesystem::path> &storageFolder, std::wstring_view dll);
	wil::unique_hmodule LoadDll(const std::optional<std::filesystem::path> &storageFolder, std::wstring_view dll);

	inline TaskbarAppearance WithPreview(txmp::TaskbarState state, const TaskbarAppearance &appearance) const
	{
		const auto &preview = m_ColorPreviews.at(static_cast<std::size_t>(state));
		if (preview)
		{
			return { appearance.Accent, *preview, appearance.ShowPeek, appearance.ShowLine };
		}
		else
		{
			return appearance;
		}
	}

	inline static HMONITOR GetStartMenuMonitor() noexcept
	{
		// we assume that start is the current foreground window;
		// haven't seen a case where that wasn't true yet.
		// NOTE: this only stands *when* we get notified that
		// start has opened (and as long as it is). when we get
		// notified that it's closed another window may be the
		// foreground window already (eg the user dismissed start
		// by clicking on a window)
		Sleep(5); // give it a bit of delay because sometimes it doesn't capture it right
		return Window::ForegroundWindow().monitor();
	}

	inline static HMONITOR GetSearchMonitor() noexcept
	{
		// same assumption for search
		Sleep(5);
		return Window::ForegroundWindow().monitor();
	}

	inline static wil::unique_hwineventhook CreateHook(DWORD event, WINEVENTPROC proc)
	{
		return CreateHook(event, event, proc);
	}

	template<typename T>
	static T GetProc(const wil::unique_hmodule &module, Util::null_terminated_string_view proc)
	{
		if (const auto ptr = GetProcAddress(module.get(), proc.c_str()))
		{
			return reinterpret_cast<T>(ptr);
		}
		else
		{
			LastErrorHandle(spdlog::level::critical, L"Failed to get address of procedure");
		}
	}

public:
	TaskbarAttributeWorker(const Config &cfg, HINSTANCE hInstance, DynamicLoader &loader, const std::optional<std::filesystem::path> &storageFolder);

	inline void ConfigurationChanged()
	{
		RefreshAllAttributes();
	}

	void ApplyColorPreview(txmp::TaskbarState state, Util::Color color)
	{
		m_ColorPreviews.at(static_cast<std::size_t>(state)) = color;
		ConfigurationChanged();
	}

	inline void RemoveColorPreview(txmp::TaskbarState state)
	{
		m_ColorPreviews.at(static_cast<std::size_t>(state)).reset();
		ConfigurationChanged();
	}

	void DumpState();
	void ResetState(bool manual = false);

	~TaskbarAttributeWorker() noexcept(false);
};

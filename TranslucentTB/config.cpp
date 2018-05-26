#include "config.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>

#include "common.hpp"
#include "ttblog.hpp"
#include "util.hpp"
#include "win32.hpp"

// Defaults

// Regular
Config::TASKBAR_APPEARANCE Config::REGULAR_APPEARANCE = { swca::ACCENT::ACCENT_ENABLE_TRANSPARENTGRADIENT, 0x0 };

// Maximised
bool Config::MAXIMISED_ENABLED = true;
Config::TASKBAR_APPEARANCE Config::MAXIMISED_APPEARANCE = { swca::ACCENT::ACCENT_ENABLE_BLURBEHIND, 0xaa000000 };
bool Config::MAXIMISED_REGULAR_ON_PEEK = true;

// Start
bool Config::START_ENABLED = true;
Config::TASKBAR_APPEARANCE Config::START_APPEARANCE = { swca::ACCENT::ACCENT_NORMAL, 0x0 };

// Timeline/Task View
bool Config::TIMELINE_ENABLED = true;
Config::TASKBAR_APPEARANCE Config::TIMELINE_APPEARANCE = { swca::ACCENT::ACCENT_NORMAL, 0x0 };

// Various
enum Config::PEEK Config::PEEK = PEEK::Dynamic;

// Advanced
uint8_t Config::SLEEP_TIME = 10;
bool Config::VERBOSE =
#ifndef _DEBUG
	false;
#else
	true;
#endif

void Config::Parse(const std::wstring &file)
{
	std::wifstream configstream(file);
	for (std::wstring line; std::getline(configstream, line);)
	{
		Util::ToLowerInplace(line);
		if (line.empty())
		{
			continue;
		}

		// Skip comments
		size_t comment_index = line.find(L';');
		if (comment_index == 0)
		{
			continue;
		}
		else if (comment_index != std::wstring::npos)
		{
			line = line.substr(0, comment_index);
		}

		size_t split_index = line.find(L'=');
		if (split_index != std::wstring::npos)
		{
			std::wstring key = Util::Trim(line.substr(0, split_index));
			std::wstring val = Util::Trim(line.substr(split_index + 1, line.length() - split_index - 1));

			ParseSingleConfigOption(key, val);
		}
		else
		{
			Log::OutputMessage(L"Invalid line in configuration file: " + line);
		}
	}
}

void Config::Save(const std::wstring &file)
{
	using namespace std;
	wofstream configstream(file);

	configstream << L"accent=" << left << setw(6) << setfill(L' ') << GetAccentText(REGULAR_APPEARANCE.ACCENT) << L"; accent values are: clear (default), fluent (only on build " << MIN_FLUENT_BUILD << L" and up), opaque, normal, or blur." << endl;
	configstream << L"color=" << GetColorText(REGULAR_APPEARANCE.COLOR) << L" ; A color in hexadecimal notation." << endl;
	configstream << L"opacity=" << GetOpacityText(REGULAR_APPEARANCE.COLOR) << L"  ; A value in the range 0 to 255." << endl;

	configstream << endl;
	configstream << L"; Dynamic Modes" << endl;
	configstream << L"; they all have their own accent, color and opacity configs." << endl;
	configstream << endl;
	configstream << L"; Dynamic Windows. State to use when a window is maximised." << endl;
	configstream << L"dynamic-ws=" << GetBoolText(MAXIMISED_ENABLED) << endl;
	configstream << L"dynamic-ws-accent=" << GetAccentText(MAXIMISED_APPEARANCE.ACCENT) << endl;
	configstream << L"dynamic-ws-color=" << GetColorText(MAXIMISED_APPEARANCE.COLOR) << L" ; A color in hexadecimal notation." << endl;
	configstream << L"dynamic-ws-opacity=" << GetOpacityText(MAXIMISED_APPEARANCE.COLOR) << L"  ; A value in the range 0 to 255." << endl;
	configstream << L"dynamic-ws-regular-on-peek=" << GetBoolText(MAXIMISED_REGULAR_ON_PEEK) << L" ; when using aero peek, behave as if no window was maximised." << endl;
	configstream << endl;
	configstream << L"; Dynamic Start. State to use when the start menu is opened." << endl;
	configstream << L"dynamic-start=" << GetBoolText(START_ENABLED) << endl;
	configstream << L"dynamic-start-accent=" << GetAccentText(START_APPEARANCE.ACCENT) << endl;
	configstream << L"dynamic-start-color=" << GetColorText(START_APPEARANCE.COLOR) << L" ; A color in hexadecimal notation." << endl;
	configstream << L"dynamic-start-opacity=" << GetOpacityText(START_APPEARANCE.COLOR) << L"  ; A value in the range 0 to 255." << endl;
	configstream << endl;
	configstream << L"; Dynamic Timeline. State to use when the timeline (or task view on older builds) is opened." << endl;
	configstream << L"dynamic-timeline=" << GetBoolText(TIMELINE_ENABLED) << endl;
	configstream << L"dynamic-timeline-accent=" << GetAccentText(TIMELINE_APPEARANCE.ACCENT) << endl;
	configstream << L"dynamic-timeline-color=" << GetColorText(TIMELINE_APPEARANCE.COLOR) << L" ; A color in hexadecimal notation." << endl;
	configstream << L"dynamic-timeline-opacity=" << GetOpacityText(TIMELINE_APPEARANCE.COLOR) << L"  ; A value in the range 0 to 255." << endl;

	configstream << endl;
	configstream << L"; Controls how the Aero Peek button behaves (dynamic, show or hide)" << endl;
	configstream << L"peek=";
	switch (PEEK)
	{
	case PEEK::Disabled:
		configstream << L"hide";
		break;
	case PEEK::Dynamic:
		configstream << L"dynamic";
		break;
	case PEEK::Enabled:
		configstream << L"show";
		break;
	}
	configstream << endl;

	configstream << endl;
	configstream << L"; Advanced settings" << endl;
	configstream << L"; sleep time in milliseconds, a shorter time reduces flicker when opening start, but results in higher CPU usage." << endl;
	configstream << L"sleep-time=" << std::dec << SLEEP_TIME << endl;
	configstream << L"; more informative logging. Can make huge log files." << endl;
	configstream << L"verbose=" << GetBoolText(VERBOSE) << endl;
}

void Config::UnknownValue(const std::wstring & key, const std::wstring & value)
{
	Log::OutputMessage(L"Unknown value found in configuration file: " + value + L" (for key: " + key + L')');
}

bool Config::ParseAccent(const std::wstring &value, swca::ACCENT &accent)
{
	if (value == L"blur")
	{
		accent = swca::ACCENT::ACCENT_ENABLE_BLURBEHIND;
	}
	else if (value == L"opaque")
	{
		accent = swca::ACCENT::ACCENT_ENABLE_GRADIENT;
	}
	else if (value == L"transparent" || value == L"translucent" || value == L"clear")
	{
		accent = swca::ACCENT::ACCENT_ENABLE_TRANSPARENTGRADIENT;
	}
	else if (value == L"normal")
	{
		accent = swca::ACCENT::ACCENT_NORMAL;
	}
	else if (value == L"fluent" && win32::IsAtLeastBuild(MIN_FLUENT_BUILD))
	{
		accent = swca::ACCENT::ACCENT_ENABLE_FLUENT;
	}
	else
	{
		return false;
	}

	return true;
}

bool Config::ParseColor(std::wstring value, uint32_t &color)
{
	Util::TrimInplace(value);

	if (value[0] == L'#')
	{
		value.erase(0, 1);
	}
	else if (value[0] == L'0' && value[1] == L'x')
	{
		value.erase(0, 2);
	}

	// Get only the last 6 characters, keeps compatibility with old version.
	// It stored AARRGGBB in color, but now we store it as RRGGBB.
	// We read AA from opacity instead, which the old version also saved alpha to.
	if (value.length() > 6)
	{
		value.erase(7);
	}

	try
	{
		color = (color & 0xFF000000) + (std::stoi(value, nullptr, 16) & 0x00FFFFFF);
	}
	catch (std::invalid_argument)
	{
		return false;
	}

	return true;
}

bool Config::ParseOpacity(const std::wstring &value, uint32_t &color)
{
	try
	{
		color = ((std::stoi(value) & 0xFF) << 24) + (color & 0x00FFFFFF);
		return true;
	}
	catch (std::invalid_argument)
	{
		Log::OutputMessage(L"Could not parse opacity found in configuration file: " + value);
		return false;
	}
}

bool Config::ParseBool(const std::wstring &value, bool &setting)
{
	if (value == L"true" || value == L"enable")
	{
		setting = true;
	}
	else if (value == L"false" || value == L"disable")
	{
		setting = false;
	}
	else
	{
		return false;
	}

	return true;
}

void Config::ParseSingleConfigOption(const std::wstring &arg, const std::wstring &value)
{
	if (arg == L"accent")
	{
		if (!ParseAccent(value, REGULAR_APPEARANCE.ACCENT))
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"color" || arg == L"tint")
	{
		if (!ParseColor(value, REGULAR_APPEARANCE.COLOR))
		{
			Log::OutputMessage(L"Could not parse color found in configuration file: " + value);
		}
	}
	else if (arg == L"opacity")
	{
		if (!ParseOpacity(value, REGULAR_APPEARANCE.COLOR))
		{
			Log::OutputMessage(L"Could not parse opacity found in configuration file: " + value);
		}
	}
	else if (arg == L"dynamic-ws")
	{
		if (!ParseBool(value, MAXIMISED_ENABLED))
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-ws-accent")
	{
		if (!ParseAccent(value, MAXIMISED_APPEARANCE.ACCENT))
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-ws-color" || arg == L"dynamic-ws-tint")
	{
		if (!ParseColor(value, MAXIMISED_APPEARANCE.COLOR))
		{
			Log::OutputMessage(L"Could not parse dynamic windows color found in configuration file: " + value);
		}
	}
	else if (arg == L"dynamic-ws-opacity")
	{
		if (!ParseOpacity(value, MAXIMISED_APPEARANCE.COLOR))
		{
			Log::OutputMessage(L"Could not parse dynamic windows opacity found in configuration file: " + value);
		}
	}
	else if (arg == L"dynamic-ws-regular-on-peek")
	{
		if (!ParseBool(value, MAXIMISED_REGULAR_ON_PEEK))
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-start")
	{
		if (!ParseBool(value, START_ENABLED))
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-start-accent")
	{
		if (!ParseAccent(value, START_APPEARANCE.ACCENT))
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-start-color" || arg == L"dynamic-start-tint")
	{
		if (!ParseColor(value, START_APPEARANCE.COLOR))
		{
			Log::OutputMessage(L"Could not parse dynamic start color found in configuration file: " + value);
		}
	}
	else if (arg == L"dynamic-start-opacity")
	{
		if (!ParseOpacity(value, START_APPEARANCE.COLOR))
		{
			Log::OutputMessage(L"Could not parse dynamic start opacity found in configuration file: " + value);
		}
	}
	else if (arg == L"dynamic-timeline")
	{
		if (!ParseBool(value, TIMELINE_ENABLED))
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-timeline-accent")
	{
		if (!ParseAccent(value, TIMELINE_APPEARANCE.ACCENT))
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"dynamic-timeline-color" || arg == L"dynamic-timeline-tint")
	{
		if (!ParseColor(value, TIMELINE_APPEARANCE.COLOR))
		{
			Log::OutputMessage(L"Could not parse dynamic timeline color found in configuration file: " + value);
		}
	}
	else if (arg == L"dynamic-timeline-opacity")
	{
		if (!ParseOpacity(value, TIMELINE_APPEARANCE.COLOR))
		{
			Log::OutputMessage(L"Could not parse dynamic timeline opacity found in configuration file: " + value);
		}
	}
	else if (arg == L"peek")
	{
		if (value == L"hide")
		{
			PEEK = PEEK::Disabled;
		}
		else if (value == L"dynamic")
		{
			PEEK = PEEK::Dynamic;
		}
		else if (value == L"show")
		{
			PEEK = PEEK::Enabled;
		}
		else
		{
			UnknownValue(arg, value);
		}
	}
	else if (arg == L"sleep-time")
	{
		try
		{
			SLEEP_TIME = std::stoi(value) & 0xFF;
		}
		catch (std::invalid_argument)
		{
			Log::OutputMessage(L"Could not parse sleep time found in configuration file: " + value);
		}
	}
	else if (arg == L"verbose")
	{
		if (!ParseBool(value, Config::VERBOSE))
		{
			UnknownValue(arg, value);
		}
	}
	else
	{
		Log::OutputMessage(L"Unknown key found in configuration file: " + arg);
	}
}

std::wstring Config::GetAccentText(const swca::ACCENT &accent)
{
	switch (accent)
	{
	case swca::ACCENT::ACCENT_ENABLE_GRADIENT:
		return L"opaque";
	case swca::ACCENT::ACCENT_ENABLE_TRANSPARENTGRADIENT:
		return L"clear";
	case swca::ACCENT::ACCENT_ENABLE_BLURBEHIND:
		return L"blur";
	case swca::ACCENT::ACCENT_NORMAL:
		return L"normal";
	case swca::ACCENT::ACCENT_ENABLE_FLUENT:
		return L"fluent";
	default:
		throw std::invalid_argument("accent was not one of the known values");
	}
}

std::wstring Config::GetColorText(const uint32_t &color)
{
	std::wostringstream stream;
	stream << std::right << std::setw(6) << std::setfill(L'0') << std::hex << (color & 0x00FFFFFF);
	return stream.str();
}

std::wstring Config::GetOpacityText(const uint32_t &color)
{
	std::wostringstream stream;
	stream << std::left << std::setw(3) << std::setfill(L' ') << std::dec << ((color & 0xFF000000) >> 24);
	return stream.str();
}

std::wstring Config::GetBoolText(const bool &value)
{
	return value ? L"enable" : L"disable";
}
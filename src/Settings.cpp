#include "Settings.h"

Settings* Settings::GetSingleton()
{
	static Settings singleton;
	return std::addressof(singleton);
}

void Settings::Load()
{
	constexpr auto path = L"Data/SKSE/Plugins/SkyrimVRESL.ini";

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	settings.Load(ini);

	ini.SaveFile(path);
}


void Settings::SettingValues::Load(CSimpleIniA& a_ini)
{
	const char* section = "Settings";

	detail::get_value(a_ini, logLevel, section, "iLogLevel", ";Log level of messages to buffer for printing: trace = 0, debug = 1, info = 2, warn = 3, err = 4, critical = 5, off = 6.");
	detail::get_value(a_ini, logLevel, section, "iFlushLevel", ";Log level to force messages to print from buffer.");
}




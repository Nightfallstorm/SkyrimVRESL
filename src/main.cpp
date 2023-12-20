#include "DataHandler.h"
#include "Papyrus.h"
#include "Settings.h"
#include "SkyrimVRESLAPI.h"
#include "eslhooks.h"
#include "hooks.h"
#include "saveloadhooks.h"
#include "sksevrhooks.h"
#include "startuphooks.h"
#include "tesfilehooks.h"

int GetMaxStdio() {
	const HMODULE crtStdioModule = GetModuleHandleA("API-MS-WIN-CRT-STDIO-L1-1-0.DLL");

	if (!crtStdioModule) {
		logger::critical("crt stdio module not found, failed to check stdio patch");
		return 0;
	}

	const auto maxStdio = reinterpret_cast<decltype(&_getmaxstdio)>(GetProcAddress(crtStdioModule, "_getmaxstdio"))();

	return maxStdio;
}


void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
	case SKSE::MessagingInterface::kPostLoad:
		{   // Called after all plugins have finished running
			// SKSEPlugin_Load.
			// It is now safe to do multithreaded operations, or operations against other plugins.
			if (SKSE::GetMessagingInterface()->RegisterListener(nullptr, SkyrimVRESLPluginAPI::ModMessageHandler))
				logger::info("Successfully registered SKSE listener {} with buildnumber {}",
					SkyrimVRESLPluginAPI::SkyrimVRESLPluginName, g_interface001.GetBuildNumber());
			else
				logger::info("Unable to register SKSE listener");

			if (GetMaxStdio() < 2048)
				logger::warn("Required Engine Fixes VR MaxStdio patch not detected. SkyrimVR will hang if you have more than {} plugins installed in /Data--even if inactive!", GetMaxStdio());
			break;
		}
	case SKSE::MessagingInterface::kDataLoaded:
		{
			logger::debug("kDataLoaded: Printing files");
			auto handler = DataHandler::GetSingleton();
			for (auto file : handler->files) {
				logger::debug("file {} recordFlags: {:x} index {:x}",
					std::string(file->fileName),
					file->recordFlags.underlying(),
					file->compileIndex);
			}

			logger::debug("kDataLoaded: Printing loaded files");
			for (auto file : handler->compiledFileCollection.files) {
				logger::debug("Regular file {} recordFlags: {:x} index {:x}",
					std::string(file->fileName),
					file->recordFlags.underlying(),
					file->compileIndex);
			}

			for (auto file : handler->compiledFileCollection.smallFiles) {
				logger::debug("Small file {} recordFlags: {:x} index {:x}",
					std::string(file->fileName),
					file->recordFlags.underlying(),
					file->smallFileCompileIndex);
			}

			auto [formMap, lock] = RE::TESForm::GetAllForms();
			lock.get().LockForRead();
			for (auto& [formID, form] : *formMap) {
				if (formID >> 24 == 0xFE) {
					logger::debug("ESL form (map ID){:x} (real ID){:x} from file {} found", formID, form->formID, std::string(form->GetFile()->fileName));
				}
			}
			lock.get().UnlockForRead();
			TestGetCompiledFileCollectionExtern();
		}
	default:
		break;
	}
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver <
		SKSE::RUNTIME_VR_1_4_15) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		//stl::report_and_fail("Failed to find standard logging directory"sv); // Doesn't work in VR
	}

	*path /= Version::PROJECT;
	*path += ".log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	auto settings = Settings::GetSingleton();
	log->set_level(settings->settings.logLevel);
	log->flush_on(settings->settings.flushLevel);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S:%e] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	try {
		Settings::GetSingleton()->Load();
	} catch (...) {
		logger::error("Exception caught when loading settings! Default settings will be used");
	}
	InitializeLog();
	logger::info("loaded plugin");

	SKSE::Init(a_skse);
	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);
	tesfilehooks::InstallHooks();
	startuphooks::InstallHooks();
	saveloadhooks::InstallHooks();
	eslhooks::InstallHooks();
	DataHandler::InstallHooks();
	SaveLoadGame::InstallHooks();
	SKSEVRHooks::Install(a_skse->SKSEVersion());
	logger::info("finish hooks");

	auto papyrus = SKSE::GetPapyrusInterface();
	papyrus->Register(Papyrus::Bind);

	return true;
}

/// @brief Get the SSE compatible TESFileCollection for SkyrimVR using GetProcAddress.
/// This should be called after kDataLoaded to ensure it's been populated.
/// This is not intended to be a stable API for other SKSE plugins. Use SkyrimVRESLAPI instead.
/// @return Pointer to TESFileCollection CompiledFileCollection.
extern "C" DLLEXPORT const RE::TESFileCollection* APIENTRY GetCompiledFileCollectionExtern()
{
	const auto& dh = DataHandler::GetSingleton();
	return &(dh->compiledFileCollection);
}

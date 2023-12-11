#include "DataHandler.h"
#include "eslhooks.h"
#include "hooks.h"
#include "saveloadhooks.h"
#include "sksevrhooks.h"
#include "startuphooks.h"
#include "tesfilehooks.h"

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		{
			logger::info("kDataLoaded: Printing files");
			auto handler = DataHandler::GetSingleton();
			for (auto file : handler->files) {
				logger::info("file {} recordFlags: {:x}",
					std::string(file->fileName),
					file->recordFlags.underlying());
			}

			logger::info("kDataLoaded: Printing loaded files");
			for (auto file : handler->compiledFileCollection.files) {
				logger::info("Regular file {} recordFlags: {:x} index {:x}",
					std::string(file->fileName),
					file->recordFlags.underlying(),
					file->compileIndex);
			}

			for (auto file : handler->compiledFileCollection.smallFiles) {
				logger::info("Small file {} recordFlags: {:x} index {:x}",
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
			// TODO: Runtime hooks here?
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

#ifdef _DEBUG
	log->set_level(spdlog::level::debug);
	log->flush_on(spdlog::level::debug);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S:%e] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
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
	return true;
}

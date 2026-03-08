#include "DataHandler.h"
#include "SkyrimVRESLAPI.h"
#include <chrono>
#include <list>

using namespace RE;

// Plugin load timing storage.
// std::list is used for filename/author so c_str() pointers remain stable on insert.
namespace
{
	std::list<std::string> g_filenameStorage;
	std::list<std::string> g_authorStorage;
	std::vector<SkyrimVRESLPluginAPI::VRESLPluginLoadTiming> g_timingData;
}

void RecordPluginLoadTiming(RE::TESFile* file, uint64_t durationNs)
{
	if (!file)
		return;
	const char* fn = file->fileName;
	for (auto& e : g_timingData) {
		if (e.filename && std::strcmp(e.filename, fn) == 0) {
			e.totalNs += durationNs;
			e.count++;
			logger::debug("[VRESL timing] {} accumulated: totalNs={} count={}", fn, e.totalNs, e.count);
			return;
		}
	}
	const auto& fnStr = g_filenameStorage.emplace_back(fn ? fn : "");
	const auto& authStr = g_authorStorage.emplace_back(
		(file->createdBy.empty()) ? "" : file->createdBy.c_str());
	g_timingData.push_back({
		.filename = fnStr.c_str(),
		.author = authStr.c_str(),
		.version = static_cast<double>(file->unk42C),
		.totalNs = durationNs,
		.count = 1u,
		._reserved = 0u,
		.openNs = 0u  // filled by RecordPluginOpenTiming (OpenTES runs before ConstructObjectList)
	});
	logger::info("[VRESL timing] Recorded {} ns for '{}' (author='{}' ver={:.3f}) [{} total]",
		durationNs, fn, authStr, static_cast<double>(file->unk42C),
		g_timingData.size());
}

void RecordPluginOpenTiming(RE::TESFile* file, uint64_t openNs)
{
	if (!file)
		return;
	const char* fn = file->fileName;
	for (auto& e : g_timingData) {
		if (e.filename && std::strcmp(e.filename, fn) == 0) {
			e.openNs += openNs;
			logger::debug("[VRESL timing] Open {} ns accumulated for '{}'", openNs, fn);
			return;
		}
	}
	// Entry not yet created (OpenTES runs before ConstructObjectList, so this is the normal path).
	const auto& fnStr = g_filenameStorage.emplace_back(fn ? fn : "");
	const auto& authStr = g_authorStorage.emplace_back(
		(file->createdBy.empty()) ? "" : file->createdBy.c_str());
	g_timingData.push_back({ .filename = fnStr.c_str(),
		.author = authStr.c_str(),
		.version = static_cast<double>(file->unk42C),
		.totalNs = 0u,
		.count = 0u,
		._reserved = 0u,
		.openNs = openNs });
	logger::debug("[VRESL timing] Open {} ns for '{}' (new entry)", openNs, fn);
}

DataHandler* DataHandler::GetSingleton()
{
	return reinterpret_cast<DataHandler*>(RE::TESDataHandler::GetSingleton(false));
}

struct DataHandlerCTORHook
{
	static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x1616D0) };

	static DataHandler* thunk(RE::TESDataHandler* a_handlerstruct)
	{
		RE::TESDataHandler* newHandler = reinterpret_cast<RE::TESDataHandler*>(RE::calloc<DataHandler>(1));
		return func(newHandler);
	}

	static inline REL::Relocation<decltype(thunk)> func;

	static void Install()
	{
		pstl::write_thunk_call<DataHandlerCTORHook>(target.address() + 0x3DB);
		logger::info("DataHandlerCTORHook installed at {:x}", target.address() + 0x3DB);
		logger::info("DataHandlerCTORHook installed at offset {:x}", target.offset() + 0x3DB);
	}
};

#ifndef BACKWARDS_COMPATIBLE
const RE::TESFile* DataHandler::LookupModByName(std::string_view a_modName)
{
	RE::TESDataHandler* handler = RE::TESDataHandler::GetSingleton();
	return handler->LookupModByName(a_modName);
}
#endif

void DataHandler::InstallHooks()
{
#ifdef BACKWARDS_COMPATIBLE
	DataHandlerCTORHook::Install();
#endif
}

SkyrimVRESLPluginAPI::SkyrimVRESLInterface001 g_interface001;
SkyrimVRESLPluginAPI::SkyrimVRESLInterface002 g_interface002;

// Constructs and returns an API of the revision number requested
void* GetApi(unsigned int revisionNumber)
{
	switch (revisionNumber) {
	case 1:
		logger::info("Interface revision 1 requested");
		return &g_interface001;
	case 2:
		logger::info("Interface revision 2 requested");
		return &g_interface002;
	}
	return nullptr;
}

// Handles skse mod messages requesting to fetch API functions
void SkyrimVRESLPluginAPI::ModMessageHandler(SKSE::MessagingInterface::Message* message)
{
	if (message->type == SkyrimVRESLMessage::kMessage_GetInterface) {
		SkyrimVRESLMessage* modmappermessage = (SkyrimVRESLMessage*)message->data;
		modmappermessage->GetApiFunction = GetApi;
		logger::info("Provided SkyrimVRESL plugin interface to {}", message->sender);
	}
}

// Fetches the SkyrimVRESL version number
unsigned int SkyrimVRESLPluginAPI::SkyrimVRESLInterface001::GetBuildNumber()
{
	return (Version::MAJOR >> 8) + (Version::MINOR >> 4) + Version::PATCH;
}

const RE::TESFileCollection* SkyrimVRESLPluginAPI::SkyrimVRESLInterface001::GetCompiledFileCollection()
{
	const auto& dh = DataHandler::GetSingleton();
	return &(dh->compiledFileCollection);
}

// ISkyrimVRESLInterface002 implementations (delegates to 001 for inherited methods)
unsigned int SkyrimVRESLPluginAPI::SkyrimVRESLInterface002::GetBuildNumber()
{
	return g_interface001.GetBuildNumber();
}

const RE::TESFileCollection* SkyrimVRESLPluginAPI::SkyrimVRESLInterface002::GetCompiledFileCollection()
{
	return g_interface001.GetCompiledFileCollection();
}

const SkyrimVRESLPluginAPI::VRESLPluginLoadTiming*
	SkyrimVRESLPluginAPI::SkyrimVRESLInterface002::GetPluginLoadTimings(uint32_t* outCount) const
{
	const auto n = static_cast<uint32_t>(g_timingData.size());
	logger::info("[VRESL] GetPluginLoadTimings called: {} entries available", n);
	if (n > 0) {
		const auto& first = g_timingData.front();
		const auto& last = g_timingData.back();
		logger::info("[VRESL] First: '{}' {}ns, Last: '{}' {}ns",
			first.filename ? first.filename : "(null)", first.totalNs,
			last.filename ? last.filename : "(null)", last.totalNs);
	}
	if (outCount)
		*outCount = n;
	return g_timingData.empty() ? nullptr : g_timingData.data();
}

void TestGetCompiledFileCollectionExtern()
{
	static const RE::TESFileCollection* VRcompiledFileCollection = nullptr;
	const auto VRhandle = GetModuleHandleA("skyrimvresl");
	if (!VRcompiledFileCollection) {
		const auto GetCompiledFileCollection = reinterpret_cast<const RE::TESFileCollection* (*)()>(GetProcAddress(VRhandle, "GetCompiledFileCollectionExtern"));
		if (GetCompiledFileCollection != nullptr) {
			VRcompiledFileCollection = GetCompiledFileCollection();
		}
		logger::info("Found VRcompiledFileCollection 0x{:x}", reinterpret_cast<std::uintptr_t>(VRcompiledFileCollection));
	}
}

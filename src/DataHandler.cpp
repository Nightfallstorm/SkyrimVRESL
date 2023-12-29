#include "DataHandler.h"
#include "SkyrimVRESLAPI.h"

using namespace RE;

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

// Constructs and returns an API of the revision number requested
void* GetApi(unsigned int revisionNumber)
{
	switch (revisionNumber) {
	case 1:
		logger::info("Interface revision 1 requested");
		return &g_interface001;
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

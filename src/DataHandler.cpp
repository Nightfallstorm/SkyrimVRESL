#include "DataHandler.h"

using namespace RE;

DataHandler* DataHandler::GetSingleton()
{
	return reinterpret_cast<DataHandler*>(RE::TESDataHandler::GetSingleton());
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

void DataHandler::InstallHooks()
{
#ifdef BACKWARDS_COMPATIBLE
	DataHandlerCTORHook::Install();
#endif
}

#include "Papyrus.h"
#include "DataHandler.h"

namespace Papyrus
{
	std::uint32_t GetModByName(RE::StaticFunctionTag*, RE::BSFixedString name) {
		DataHandler* pDataHandler = DataHandler::GetSingleton();
		const RE::TESFile* modInfo = pDataHandler->LookupModByName(name.data());
		if (!modInfo || modInfo->compileIndex == 0xFF)
			return 0xFF;

		if (modInfo->IsLight())
			return modInfo->smallFileCompileIndex + LIGHT_MOD_OFFSET;

		return modInfo->compileIndex;
	}

	bool Bind(VM* a_vm)
	{
		if (!a_vm) {
			logger::critical("couldn't get VM State"sv);
			return false;
		}

		logger::info("Overriding SKSEVR functions..."sv);

		
		BIND(GetModByName, true); 

		logger::info("Registered GetModByName"sv);

		return true;
	}
}

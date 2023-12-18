#include "Papyrus.h"
#include "DataHandler.h"

namespace Papyrus
{
	RE::TESFile* GetFileBySKSEIndex(std::uint32_t a_index)
	{
		DataHandler* pDataHandler = DataHandler::GetSingleton();
		if (a_index > 0xFF) {
			std::uint32_t adjusted = a_index - LIGHT_MOD_OFFSET;
			if (adjusted >= pDataHandler->compiledFileCollection.smallFiles.size())
				return nullptr;
			return pDataHandler->compiledFileCollection.smallFiles[adjusted];

		} else {
			if (a_index >= pDataHandler->compiledFileCollection.files.size())
				return nullptr;
			return pDataHandler->compiledFileCollection.files[a_index];
		}
	}
	// Copies of SKSE SE/AE functions related to mods and ESLs, converted to CLIB
	std::uint32_t GetModByName(StaticFunctionTag*, RE::BSFixedString name)
	{
		DataHandler* pDataHandler = DataHandler::GetSingleton();
		const RE::TESFile* modInfo = pDataHandler->LookupModByName(name.data());
		if (!modInfo || modInfo->compileIndex == 0xFF)
			return 0xFF;

		if (modInfo->IsLight())
			return modInfo->smallFileCompileIndex + LIGHT_MOD_OFFSET;

		return modInfo->compileIndex;
	}

	BSFixedString GetModName(StaticFunctionTag*, std::uint32_t index)
	{
		RE::TESFile* modInfo = GetFileBySKSEIndex(index);
		return (modInfo) ? modInfo->fileName : "";
	}

	BSFixedString GetModAuthor(StaticFunctionTag*, std::uint32_t index)
	{
		RE::TESFile* modInfo = GetFileBySKSEIndex(index);
		return (modInfo) ? modInfo->createdBy : "";
	}

	BSFixedString GetModDescription(StaticFunctionTag*, std::uint32_t index)
	{
		RE::TESFile* modInfo = GetFileBySKSEIndex(index);
		return (modInfo) ? modInfo->summary : "";
	}

	std::uint32_t GetModDependencyCount(StaticFunctionTag*, std::uint32_t index)
	{
		RE::TESFile* modInfo = GetFileBySKSEIndex(index);
		return (modInfo) ? modInfo->masterCount : 0;
	}

	std::uint32_t GetNthModDependency(StaticFunctionTag*, std::uint32_t index, std::uint32_t dep_index)
	{
		RE::TESFile* modInfo = GetFileBySKSEIndex(index);
		return (modInfo && dep_index < modInfo->masterCount) ? modInfo->masterPtrs[dep_index]->compileIndex : 0;
	}

	std::uint32_t GetLightModCount(StaticFunctionTag*)
	{
		DataHandler* pDataHandler = DataHandler::GetSingleton();
		return pDataHandler->compiledFileCollection.smallFiles.size();
	}

	std::uint32_t GetLightModByName(StaticFunctionTag*, BSFixedString name)
	{
		DataHandler* pDataHandler = DataHandler::GetSingleton();
		const RE::TESFile* modInfo = pDataHandler->LookupModByName(name.data());
		if (!modInfo || modInfo->compileIndex == 0xFF || !modInfo->IsLight())
			return 0xFFFF;

		return modInfo->smallFileCompileIndex;
	}

	BSFixedString GetLightModName(StaticFunctionTag*, std::uint32_t index)
	{
		DataHandler* pDataHandler = DataHandler::GetSingleton();
		if (index >= pDataHandler->compiledFileCollection.smallFiles.size())
			return "";

		RE::TESFile* modInfo = pDataHandler->compiledFileCollection.smallFiles[index];
		return (modInfo) ? modInfo->fileName : NULL;
	}

	BSFixedString GetLightModAuthor(StaticFunctionTag*, std::uint32_t index)
	{
		DataHandler* pDataHandler = DataHandler::GetSingleton();
		if (index >= pDataHandler->compiledFileCollection.smallFiles.size())
			return "";

		RE::TESFile* modInfo = pDataHandler->compiledFileCollection.smallFiles[index];
		return (modInfo) ? modInfo->createdBy : NULL;
	}

	BSFixedString GetLightModDescription(StaticFunctionTag*, std::uint32_t index)
	{
		DataHandler* pDataHandler = DataHandler::GetSingleton();
		if (index >= pDataHandler->compiledFileCollection.smallFiles.size())
			return "";

		RE::TESFile* modInfo = pDataHandler->compiledFileCollection.smallFiles[index];
		return (modInfo) ? modInfo->summary : NULL;
	}

	std::uint32_t GetLightModDependencyCount(StaticFunctionTag*, std::uint32_t index)
	{
		DataHandler* pDataHandler = DataHandler::GetSingleton();
		if (index >= pDataHandler->compiledFileCollection.smallFiles.size())
			return 0;

		RE::TESFile* modInfo = pDataHandler->compiledFileCollection.smallFiles[index];
		return (modInfo) ? modInfo->masterCount : 0;
	}

	std::uint32_t GetNthLightModDependency(StaticFunctionTag*, std::uint32_t index, std::uint32_t dep_index)
	{
		DataHandler* pDataHandler = DataHandler::GetSingleton();
		if (index >= pDataHandler->compiledFileCollection.smallFiles.size())
			return 0;

		RE::TESFile* modInfo = pDataHandler->compiledFileCollection.smallFiles[index];
		return (modInfo && dep_index < modInfo->masterCount) ? modInfo->masterPtrs[dep_index]->compileIndex : 0;
	}

	bool IsPluginInstalled(StaticFunctionTag*, BSFixedString name)
	{
		DataHandler* pDataHandler = DataHandler::GetSingleton();

		const RE::TESFile* modInfo = pDataHandler->LookupModByName(name.data());
		if (modInfo)
			return modInfo->compileIndex != 0xFF;

		return false;
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

		BIND(GetModName, true);

		logger::info("Registered GetModName"sv);

		BIND(GetModAuthor, true);

		logger::info("Registered GetModAuthor"sv);

		BIND(GetModDescription, true);

		logger::info("Registered GetModDescription"sv);

		BIND(GetModDependencyCount, true);

		logger::info("Registered GetModDependencyCount"sv);

		BIND(GetNthModDependency, true);

		logger::info("Registered GetNthModDependency"sv);

		BIND(GetLightModCount, true);

		logger::info("Registered GetLightModCount"sv);

		BIND(GetLightModByName, true);

		logger::info("Registered GetLightModByName"sv);

		BIND(GetLightModName, true);

		logger::info("Registered GetLightModName"sv);

		BIND(GetLightModAuthor, true);

		logger::info("Registered GetLightModAuthor"sv);

		BIND(GetLightModDescription, true);

		logger::info("Registered GetLightModDescription"sv);

		BIND(GetLightModDependencyCount, true);

		logger::info("Registered GetLightModDependencyCount"sv);

		BIND(GetNthLightModDependency, true);

		logger::info("Registered GetNthLightModDependency"sv);

		BIND(IsPluginInstalled, true);

		logger::info("Registered IsPluginInstalled"sv);

		return true;
	}
}

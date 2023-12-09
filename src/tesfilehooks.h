#pragma once
#include "DataHandler.h"
#include <detours/detours.h>

namespace tesfilehooks
{
	static inline RE::BSTArray<RE::TESFile*> filesArray = RE::BSTArray<RE::TESFile*>();

	// SE loops through handler->files and then handler->activeFile last
	// We use an array to mimic this for VR
	static void PopulateFilesArray()
	{
		auto handler = DataHandler::GetSingleton();
		if (filesArray.empty() ||
			filesArray.size() != handler->compiledFileCollection.files.size() + handler->compiledFileCollection.smallFiles.size()) {
			// Ensure filesArray is synchronized with handler
			filesArray.clear();
			for (auto file : handler->files) {
				if (file->compileIndex != 0xFF && file != handler->activeFile) {
					filesArray.push_back(file);
				}
			}

			if (handler->activeFile) {
				filesArray.push_back(handler->activeFile);
			}
		}
	}

	static std::uint32_t GetLoadedModCountSE(DataHandler* a_handler)
	{
		logger::info("GetLoadedModCountSE called!");
		PopulateFilesArray();
		return filesArray.size();
	}

	static RE::TESFile* GetModAtIndex(DataHandler* a_handler, std::uint32_t a_index)
	{
		logger::info("GetModAtIndex called for {:x}!", a_index);
		return filesArray[a_index];
	}

	struct TESQuestHook
	{
		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x389CA0) };
			SKSE::AllocTrampoline(14);
			SKSE::GetTrampoline().write_call<5>(target.address() + 0x181, GetLoadedModCountSE);
			SKSE::AllocTrampoline(14);
			SKSE::GetTrampoline().write_call<5>(target.address() + 0x199, GetModAtIndex);

			logger::info("Installed TESQuest LoadedModCheck hook");
		}
	};

	struct UnkTESTopicHook
	{
		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x389ED0) };
			SKSE::AllocTrampoline(14);
			SKSE::GetTrampoline().write_call<5>(target.address() + 0x147, GetLoadedModCountSE);
			SKSE::AllocTrampoline(14);
			SKSE::GetTrampoline().write_call<5>(target.address() + 0x169, GetModAtIndex);

			logger::info("Installed UnkTESTopicHook hook");
		}
	};

	struct UnkTerrainHook
	{
		static std::uint32_t thunk()
		{
			// SE only uses the regular files size. This function is unknown, but we will do the same here to be safe
			return DataHandler::GetSingleton()->compiledFileCollection.files.size();
		}

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x4B80A0 + 0x159) };
			SKSE::AllocTrampoline(14);
			SKSE::GetTrampoline().write_call<5>(target.address(), UnkTerrainHook::thunk);
			REL::Relocation<std::uintptr_t> target2{ REL::Offset(0x4B80A0 + 0xA3) };
			SKSE::AllocTrampoline(14);
			SKSE::GetTrampoline().write_call<5>(target2.address(), UnkTerrainHook::thunk);
		}
	};

	struct DuplicateHook
	{
		// Copy over the smallCompileIndex, which is padding in VR normally
		typedef RE::TESFile*(WINAPI* pFunc)(RE::TESFile*, std::uint32_t);  // typedef to simplify signature

		static RE::TESFile* thunk(RE::TESFile* a_self, std::uint32_t a_cacheSize)
		{
			RE::TESFile* duplicateFile = originalFunction(a_self, a_cacheSize);
			duplicateFile->smallFileCompileIndex = a_self->smallFileCompileIndex;
			return duplicateFile;
		}

		static inline pFunc originalFunction;

		// Install our hook at the specified address
		static inline void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::ID(13923) };
			const auto targetAddress = REL::ID(13923).address();
			const auto funcAddress = &thunk;
			originalFunction = (pFunc)targetAddress;
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourAttach(&(PVOID&)originalFunction, (PBYTE)&thunk);
			if (DetourTransactionCommit() == NO_ERROR)
				logger::info(
					"Installed TESFile::Duplicate at {0:x} with replacement from address {0:x}",
					targetAddress, (void*)funcAddress);
			else
				logger::warn("Failed to install TESFile::Duplicate hook");
		}
	};

	struct LoadedModCountHook
	{
		static std::uint32_t thunk(DataHandler* a_handler)
		{
			// This function should never be invoked since we hooked all usages, bad behavior occurred!
			// Crash here for crashlog to show missing usage
			throw std::invalid_argument("Unexpected invocation of LoadedModCountHook!");
		}

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x17EFE0) };
			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(14);
			trampoline.write_branch<5>(target.address(), thunk);
		}
	};

	struct GetModAtIndexHook
	{
		// TODO: Temp workaround. We MUST properly fixup uses of this function
		static RE::TESFile* thunk(DataHandler* a_handler, std::uint32_t a_index)
		{
			return a_handler->compiledFileCollection.files[a_index];
		}

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x182D40) };
			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(14);
			trampoline.write_branch<5>(target.address(), thunk);
		}
	};

	struct IsGameModdedHook
	{
		// Replica of Engine fixes. Return game isn't modded to enable achievements
		// For us, this removes one use of `loadedMods` that would cause problems otherwise
		static bool thunk(DataHandler* a_handler, std::uint32_t a_index)
		{
			return false;
		}

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x17FB90) };
			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(14);
			trampoline.write_branch<5>(target.address(), thunk);
		}
	};

	static inline void InstallHooks()
	{
		DuplicateHook::Install();
		LoadedModCountHook::Install();  // TODO: Remove this once we fully hook uses of this
		GetModAtIndexHook::Install();   // TODO: Remove this once we fully hook uses of this
		IsGameModdedHook::Install();
		TESQuestHook::Install();
		UnkTESTopicHook::Install();
		UnkTerrainHook::Install();
	}
}

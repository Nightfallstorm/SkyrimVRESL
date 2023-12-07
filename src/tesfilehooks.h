#pragma once
#include <detours/detours.h>
#include "DataHandlerSE.h"

namespace tesfilehooks
{
	struct AddTESFileHook
	{
		static bool HasFile(RE::BSTArray<RE::TESFile*> a_array, RE::TESFile* a_file) {
			for (auto file : a_array) {
				if (file == a_file) {
					return true;
				}
			}
			return false;
		}
		// Reinterpret datahandler as our custom datahandler with ESL support
		// and add the file with the correct compile indexing
		// This is a replica of what Skyrim SE does when adding files
		static void AddFile(DataHandlerSE* a_handler, RE::TESFile* a_file) {
			logger::info("AddFile called");
			auto& fileCollection = a_handler->compiledFileCollection;
			if (!a_file->IsLight() && HasFile(fileCollection.files, a_file)) {
				return;
			} else if (a_file->IsLight() && HasFile(fileCollection.smallFiles, a_file)) {
				return;
			}

			if (a_file->IsLight()) {
				fileCollection.smallFiles.push_back(a_file);
				auto smallFileCompileIndex = fileCollection.smallFiles.size() - 1;
				a_file->flags &= 0xFFFFFFu;
				a_file->flags |= 0xFE << 24;
				a_file->compileIndex = 0xFE;
				a_file->smallFileCompileIndex = smallFileCompileIndex;
			} else {
				fileCollection.files.push_back(a_file);
				auto fileCompileIndex = fileCollection.files.size() - 1;
				a_file->flags &= 0xFFFFFFu;
				a_file->flags |= fileCompileIndex << 24;
				a_file->compileIndex = fileCompileIndex;
				a_file->smallFileCompileIndex = 0;
			}
		}

		static void Install() {
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x17DFF0) };

			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(14);
			trampoline.write_branch<5>(target.address(), AddFile);
			logger::info("Install AddFile hook at {:x}", target.address());
			logger::info("Install AddFile hook at offset {:x}", target.offset());
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

	static inline void InstallHooks()
	{
		DuplicateHook::Install();
		AddTESFileHook::Install();
	}
}

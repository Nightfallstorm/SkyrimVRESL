#pragma once
#include "DataHandlerSE.h"
#include <detours/detours.h>

namespace loadedmodhooks
{
	static inline RE::BSTArray<RE::TESFile*> filesArray = RE::BSTArray<RE::TESFile*>();

	// SE loops through handler->files and then handler->activeFile last
	// We use an array to mimic this for VR
	static void PopulateFilesArray()
	{
		auto handler = reinterpret_cast<DataHandlerSE*>(RE::TESDataHandler::GetSingleton());
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

	static std::uint32_t GetLoadedModCountSE(DataHandlerSE* a_handler)
	{
		logger::info("GetLoadedModCountSE called!");
		PopulateFilesArray();
		return filesArray.size();
	}

	static RE::TESFile* GetModAtIndex(DataHandlerSE* a_handler, std::uint32_t a_index)
	{
		logger::info("GetModAtIndex called for {:x}!", a_index);
		return filesArray[a_index];
	}

	struct TESQuestHook {
		static void Install() {
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

	struct Savemodshook {

		static RE::BGSSaveGameBuffer* BGSSaveGameBufferCTOR(RE::BGSSaveGameBuffer* a_buffer)
		{
			using func_t = decltype(&BGSSaveGameBufferCTOR);
			REL::Relocation<func_t> func{ REL::Offset(0x5A0E70) };
			return func(a_buffer);
		}

		static void BGSSaveGameBuffer_SaveFileName(RE::BGSSaveGameBuffer* a_buffer, char* a_fileName)
		{
			using func_t = decltype(&BGSSaveGameBuffer_SaveFileName);
			REL::Relocation<func_t> func{ REL::Offset(0x5A1100) };
			return func(a_buffer, a_fileName);
		}

		static void saveUnk(RE::BGSSaveGameBuffer* a_buffer, Win32FileType* a_unk) {
			using func_t = decltype(&saveUnk);
			REL::Relocation<func_t> func{ REL::Offset(0x5A0F00) };
			return func(a_buffer, a_unk);
		}

		static void BGSSaveGameBufferDTOR(RE::BGSSaveGameBuffer* a_buffer)
		{
			using func_t = decltype(&BGSSaveGameBufferDTOR);
			REL::Relocation<func_t> func{ REL::Offset(0x5A0E90) };
			return func(a_buffer);
		}

		static void SaveModNames(std::uint64_t a_unk, Win32FileType* a_unk2) {
			logger::info("SaveModNames called");
			RE::BGSSaveGameBuffer* buffer = RE::malloc<RE::BGSSaveGameBuffer>();
			BGSSaveGameBufferCTOR(buffer);
			auto handler = reinterpret_cast<DataHandlerSE*>(RE::TESDataHandler::GetSingleton());
			auto fileSize = handler->compiledFileCollection.files.size();
			auto smallFileSize = handler->compiledFileCollection.smallFiles.size();
			buffer->SaveDataEndian(&fileSize, 1u);
			for (auto file : handler->compiledFileCollection.files) {
				BGSSaveGameBuffer_SaveFileName(buffer, file->fileName);
			}
			buffer->SaveDataEndian(&smallFileSize, 2u);
			for (auto file : handler->compiledFileCollection.smallFiles) {
				BGSSaveGameBuffer_SaveFileName(buffer, file->fileName);
			}
			saveUnk(buffer, a_unk2);
			BGSSaveGameBufferDTOR(buffer);
			logger::info("SaveModNames finished");
		}

		static void Install() {
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x585580) };
			SKSE::AllocTrampoline(14);
			SKSE::GetTrampoline().write_branch<5>(target.address(), SaveModNames);
		}
	};

	struct LoadModsHook
	{
		static RE::BGSLoadGameBuffer* BGSLoadGameBufferCTOR(RE::BGSLoadGameBuffer* a_buffer)
		{
			using func_t = decltype(&BGSLoadGameBufferCTOR);
			REL::Relocation<func_t> func{ REL::Offset(0x59EF30) };
			return func(a_buffer);
		}

		static void BGSLoadGameBufferFile(RE::BGSLoadGameBuffer* a_buffer, Win32FileType* a_unk) 
		{
			using func_t = decltype(&BGSLoadGameBufferFile);
			REL::Relocation<func_t> func{ REL::Offset(0x5978F0) };
			return func(a_buffer, a_unk);
		}

		static void BGSLoadGameBufferReadString(RE::BGSLoadGameBuffer* a_buffer, char a_string[])
		{
			using func_t = decltype(&BGSLoadGameBufferReadString);
			REL::Relocation<func_t> func{ REL::Offset(0x597B20) };
			return func(a_buffer, a_string);
		}

		static void BGSSaveGameBufferDTOR(RE::BGSLoadGameBuffer* a_buffer)
		{
			using func_t = decltype(&BGSSaveGameBufferDTOR);
			REL::Relocation<func_t> func{ REL::Offset(0x5978A0) };
			return func(a_buffer);
		}

		// TODO: We need to hook BGSSaveLoadGame and let it know about ESLs as a concept, just like SE
		// TODO: We skip the verification step for the sake of simplicity. We should add that verification and do it SE-style
		static bool LoadMods(RE::BGSSaveLoadGame* a_saveloadManager, Win32FileType* a_unk2, std::uint64_t a_unk3) {
			// Simple version, read in the same amount of data from savemodshook and only save the regular files
			// THIS IS A WORKAROUND, NOT THE ACTUAL FIX
			RE::BGSLoadGameBuffer* buffer = RE::malloc<RE::BGSLoadGameBuffer>();
			BGSLoadGameBufferCTOR(buffer);
			BGSLoadGameBufferFile(buffer, a_unk2);
			std::memset(a_saveloadManager->pluginList, 0xFF, 0xFF);
			std::memset(a_saveloadManager->unk18, 0xFF, 0xFF);
			std::uint32_t regularFileCount = 0;
			std::uint32_t smallFileCount = 0;
			buffer->LoadDataEndian(&regularFileCount, 1u, 0); // TODO: Offset doesn't exist, we should remove it
			char fileName[328]{ 0 };
			for (std::uint32_t i = 0; i < regularFileCount; i++) {
				BGSLoadGameBufferReadString(buffer, fileName);
				auto file = RE::TESDataHandler::GetSingleton()->LookupModByName(fileName);
				if (file && file->compileIndex != 0xFF) {
					a_saveloadManager->pluginList[i] = i;  // TODO: this should be BSTArray like in SE
					a_saveloadManager->unk18[i] = i; // TODO: also BSTArray like in SE
				}
			}

			// TODO: SE does a version check on save before loading ESLs, we should do the same for mid-save compatibily
			buffer->LoadDataEndian(&regularFileCount, 2u, 0);  // TODO: Offset doesn't exist, we should remove it
			for (std::uint32_t i = 0; i < smallFileCount; i++) {
				BGSLoadGameBufferReadString(buffer, fileName);
				// TODO: Grab and store the ESLs like SE does
			}
			delete buffer;
			return true; // TODO: True means save file matches load order, fix when we add verification checks
		}

		static void Install() {
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x585640) };
			SKSE::AllocTrampoline(14);
			SKSE::GetTrampoline().write_branch<5>(target.address(), LoadMods);
		}
	};

	struct UnkTerrainHook {
		static std::uint32_t thunk() {
			// SE only uses the regular files size. This function is unknown, but we will do the same here to be safe
			return reinterpret_cast<DataHandlerSE*>(RE::TESDataHandler::GetSingleton())->compiledFileCollection.files.size();
		}

		static void Install() {
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x4B80A0 + 0x159) };
			SKSE::AllocTrampoline(14);
			SKSE::GetTrampoline().write_branch<5>(target.address(), thunk);
		}
	};

	static inline void InstallHooks()
	{
		TESQuestHook::Install();
		UnkTESTopicHook::Install();
		Savemodshook::Install();
		LoadModsHook::Install();
		UnkTerrainHook::Install();
	}
}

namespace tesfilehooks
{
	static bool HasFile(RE::BSTArray<RE::TESFile*> a_array, RE::TESFile* a_file)
	{
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
	static void AddFile(RE::TESFile* a_file)
	{
		logger::info("AddFile invoked {} {:x}", std::string(a_file->fileName), a_file->compileIndex);
		DataHandlerSE* handler = reinterpret_cast<DataHandlerSE*>(RE::TESDataHandler::GetSingleton());
		auto& fileCollection = handler->compiledFileCollection;
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
		logger::info("AddFile finished");
	}

	struct AddTESFileHook
	{
		static void AddFileThunk(DataHandlerSE* a_handler, RE::TESFile* a_file)
		{
			return AddFile(a_file);
		}

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x17DFF0) };

			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(14);
			trampoline.write_branch<5>(target.address(), AddFileThunk);
			logger::info("Install AddFile hook at {:x}", target.address());
			logger::info("Install AddFile hook at offset {:x}", target.offset());
		}
	};

	struct AddTESFileHook1
	{
		struct TrampolineCall : Xbyak::CodeGenerator
		{
			// Call AddFile method, then jump back to rest of code
			TrampolineCall(std::uintptr_t jmpAfterCall, std::uintptr_t func = stl::unrestricted_cast<std::uintptr_t>(AddFile))
			{
				Xbyak::Label funcLabel;
				sub(rsp, 0x20);
				call(ptr[rip + funcLabel]);
				add(rsp, 0x20);
				mov(rcx, jmpAfterCall);
				jmp(rcx);
				L(funcLabel);
				dq(func);
			}
		};

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x1831F0 + 0x807) };
			REL::Relocation<std::uintptr_t> jmp{ REL::Offset(0x1831F0 + 0x852) };
			REL::safe_fill(target.address(), REL::NOP, jmp.address() - target.address());

			auto trampolineJmp = TrampolineCall(jmp.address());
			REL::safe_write(target.address(), trampolineJmp.getCode(), trampolineJmp.getSize());

			logger::info("Install AddFile1 hook at address {:x}", target.address());
			logger::info("Install AddFile1 hook at offset {:x}", target.offset());
		}
	};

	struct AddTESFileHook2
	{
		struct TrampolineCall : Xbyak::CodeGenerator
		{
			// Call AddFile method, then jump back to rest of code
			TrampolineCall(std::uintptr_t jmpAfterCall, std::uintptr_t func = stl::unrestricted_cast<std::uintptr_t>(AddFile))
			{
				Xbyak::Label funcLabel;
				sub(rsp, 0x20);
				call(ptr[rip + funcLabel]);
				add(rsp, 0x20);
				mov(rcx, jmpAfterCall);
				jmp(rcx);
				L(funcLabel);
				dq(func);
			}
		};

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x1831F0 + 0x807) };
			REL::Relocation<std::uintptr_t> jmp{ REL::Offset(0x1831F0 + 0x852) };
			REL::safe_fill(target.address(), REL::NOP, jmp.address() - target.address());

			auto trampolineJmp = TrampolineCall(jmp.address());
			REL::safe_write(target.address(), trampolineJmp.getCode(), trampolineJmp.getSize());

			logger::info("Install AddFile1 hook at address {:x}", target.address());
			logger::info("Install AddFile1 hook at offset {:x}", target.offset());
		}
	};

	struct CompileFilesHook
	{
		static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x17EFF0) };

		static inline REL::Relocation<int> iTotalForms{ REL::Offset(0x1F889B4) };

		struct TrampolineCall : Xbyak::CodeGenerator
		{
			// Call AddFile method, then jump back to rest of code
			TrampolineCall(std::uintptr_t jmpAfterCall, std::uintptr_t func = stl::unrestricted_cast<std::uintptr_t>(AddFile))
			{
				Xbyak::Label funcLabel;
				mov(rcx, rbx);
				sub(rsp, 0x20);
				call(ptr[rip + funcLabel]);
				add(rsp, 0x20);
				mov(rcx, jmpAfterCall);
				jmp(rcx);
				L(funcLabel);
				dq(func);
			}
		};

		struct TrampolineCall2 : Xbyak::CodeGenerator
		{
			// Call AddFile method, then jump back to rest of code
			TrampolineCall2(std::uintptr_t jmpAfterCall, std::uintptr_t func = stl::unrestricted_cast<std::uintptr_t>(AddFile))
			{
				Xbyak::Label funcLabel;
				sub(rsp, 0x20);
				call(ptr[rip + funcLabel]);
				add(rsp, 0x20);
				mov(rcx, jmpAfterCall);
				jmp(rcx);
				L(funcLabel);
				dq(func);
			}
		};

		// Hook where TESDataHandler->loadedMods is used, and replace with the ESL/ESP split

		static void EraseLoadedModCountReset()
		{
			REL::safe_fill(target.address() + 0x6B, REL::NOP, 0x3);
			logger::info("Erased LoadedModCountReset");
		}

		static void InstallAddFile()
		{
			std::uintptr_t start = target.address() + 0x196;
			std::uintptr_t end = target.address() + 0x1B9;
			REL::safe_fill(start, REL::NOP, end - start);

			auto trampolineJmp = TrampolineCall(end);

			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(trampolineJmp.getSize());
			auto result = trampoline.allocate(trampolineJmp);
			auto& trampoline2 = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(14);
			trampoline2.write_branch<5>(start, (std::uintptr_t)result);

			//REL::safe_write(start, trampolineJmp.getCode(), trampolineJmp.getSize());
			//if (trampolineJmp.getSize() > end - start) {
			//	logger::critical("InstallAddFile trampoline too big by {} bytes!", trampolineJmp.getSize() - (end - start));
			//}

			logger::info("Install LoadFilesHook hook at address {:x}", start);
		}

		static void InstallAddFile2()
		{
			std::uintptr_t start = target.address() + 0x1F0;
			std::uintptr_t end = target.address() + 0x217;
			REL::safe_fill(start, REL::NOP, end - start);

			auto trampolineJmp = TrampolineCall2(end);
			REL::safe_write(start, trampolineJmp.getCode(), trampolineJmp.getSize());

			if (trampolineJmp.getSize() > end - start) {
				logger::critical("InstallAddFile2 trampoline too big by {} bytes!", trampolineJmp.getSize() - (end - start));
			}

			logger::info("Install LoadFilesHook2 hook at address {:x}", start);
		}

		static void OpenTESLoopThunk()
		{
			logger::info("OpenTESLoop invoked!");
			// Replica of what SE does, but for VR
			auto handler = reinterpret_cast<DataHandlerSE*>(RE::TESDataHandler::GetSingleton());
			int* totalForms = reinterpret_cast<int*>(iTotalForms.address());
			for (auto file : handler->compiledFileCollection.files) {
				logger::info("OpenTESLoop opening file! {} {:x}", std::string(file->fileName), file->compileIndex);
				file->OpenTES(RE::NiFile::OpenMode::kReadOnly, 0);
				*totalForms += file->unk430;
			}
			for (auto file : handler->compiledFileCollection.smallFiles) {
				logger::info("OpenTESLoop opening small file! {} {:x}", std::string(file->fileName), file->compileIndex);
				file->OpenTES(RE::NiFile::OpenMode::kReadOnly, 0);
				*totalForms += file->unk430;
			}
			logger::info("OpenTESLoop finished!");
		}

		static void InstallOpenTESLoop()
		{
			std::uintptr_t start = target.address() + 0x21D;
			std::uintptr_t end = target.address() + 0x268;
			REL::safe_fill(start, REL::NOP, end - start);

			auto trampolineJmp = TrampolineCall2(end, stl::unrestricted_cast<std::uintptr_t>(OpenTESLoopThunk));
			REL::safe_write(start, trampolineJmp.getCode(), trampolineJmp.getSize());

			if (trampolineJmp.getSize() > end - start) {
				logger::critical("InstallOpenTESLoop trampoline too big by {} bytes!", trampolineJmp.getSize() - (end - start));
			}

			logger::info("Install LoadFilesHook3 hook at address {:x}", start);
		}

		static void ConstructObjectList(DataHandlerSE* a_handler, RE::TESFile* a_file, bool a_isFirstPlugin)
		{
			using func_t = decltype(&ConstructObjectList);
			REL::Relocation<func_t> func{ REL::Offset(0x180870) };
			return func(a_handler, a_file, a_isFirstPlugin);
		}

		static void ConstructObjectListThunk()
		{
			logger::info("ConstructObjectListThunk invoked!");
			// Replica of what SE does, but for VR
			auto handler = reinterpret_cast<DataHandlerSE*>(RE::TESDataHandler::GetSingleton());
			bool firstPlugin = true;
			for (auto file : handler->files) {
				if (file->compileIndex != 0xFF && file != handler->activeFile) {
					logger::info("ConstructObjectListThunk on file {} {:x}", std::string(file->fileName), file->compileIndex);
					ConstructObjectList(handler, file, firstPlugin);
					firstPlugin = false;
				}
			}
			if (handler->activeFile) {
				logger::info("ConstructObjectListThunk on active file {} {:x}",
					std::string(handler->activeFile->fileName),
					handler->activeFile->compileIndex);
				ConstructObjectList(handler, handler->activeFile, firstPlugin);
				firstPlugin = false;
			}
			logger::info("ConstructObjectListThunk finished!");
		}

		static void InstallConstructObjectListLoop()
		{
			std::uintptr_t start = target.address() + 0x29C;
			std::uintptr_t end = target.address() + 0x2D2;
			REL::safe_fill(start, REL::NOP, end - start);

			auto trampolineJmp = TrampolineCall2(end, stl::unrestricted_cast<std::uintptr_t>(ConstructObjectListThunk));
			REL::safe_write(start, trampolineJmp.getCode(), trampolineJmp.getSize());

			if (trampolineJmp.getSize() > end - start) {
				logger::critical("InstallConstructObjectListLoop trampoline too big by {} bytes!", trampolineJmp.getSize() - (end - start));
			}

			logger::info("Install LoadFilesHook4 hook at address {:x}", start);
		}

		static void Install()
		{
			EraseLoadedModCountReset();
			InstallAddFile();
			InstallAddFile2();
			InstallOpenTESLoop();
			InstallConstructObjectListLoop();
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
		// TODO: Temp workaround. We MUST properly fixup uses of this function
		static std::uint32_t thunk(DataHandlerSE* a_handler)
		{
			RE::DebugMessageBox("LoadedModCountHook invoked!!! not intended!");
			return a_handler->compiledFileCollection.files.size();
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
		static RE::TESFile* thunk(DataHandlerSE* a_handler, std::uint32_t a_index)
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
		static bool thunk(DataHandlerSE* a_handler, std::uint32_t a_index)
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
		AddTESFileHook::Install();
		AddTESFileHook1::Install();
		CompileFilesHook::Install();
		LoadedModCountHook::Install(); // TODO: Remove this once we fully hook uses of this
		GetModAtIndexHook::Install();// TODO: Remove this once we fully hook uses of this
		IsGameModdedHook::Install();
		loadedmodhooks::InstallHooks();
	}
}

#pragma once
#include <detours/detours.h>
#include "DataHandlerSE.h"

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
		static void AddFileThunk(DataHandlerSE* a_handler, RE::TESFile* a_file) {
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

	struct CompileFilesHook {
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

		static void EraseLoadedModCountReset() {
			REL::safe_fill(target.address() + 0x6B, REL::NOP, 0x3);
			logger::info("Erased LoadedModCountReset");
		}

		static void InstallAddFile() {
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

		static void OpenTESLoopThunk() {
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

		static void InstallConstructObjectListLoop() {
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

		static void Install() {
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

	static inline void InstallHooks()
	{
		DuplicateHook::Install();
		AddTESFileHook::Install();
		AddTESFileHook1::Install();
		CompileFilesHook::Install();
	}
}

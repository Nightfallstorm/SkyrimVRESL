#pragma once
#include "DataHandler.h"
#include "SaveLoadGame.h"
#include "eslhooks.h"
#include <detours/detours.h>

namespace saveloadhooks
{
	static inline int SESaveVersion = 78;
	static inline int VRSaveVersion = 77;

	struct SaveVersionHook
	{
		static void InstallGetSaveVersionFuncHook()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x58F1C0) };
			REL::safe_fill(target.address(), REL::NOP, 0x7);
			byte newSaveVersion[5] = { 0xB8, 0x4E, 0x00, 0x00, 0x00 };  // mov eax, 78
			REL::safe_write(target.address(), newSaveVersion, 0x5);
		}

		static void InstallSaveVersionGrab()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x58F1D0) };
			REL::safe_fill(target.address() + 0xBA, REL::NOP, 0x7);
			byte newSaveVersion[5] = { 0xBB, 0x4E, 0x00, 0x00, 0x00 };  // mov rbx, 78
			REL::safe_write(target.address() + 0xBA, newSaveVersion, 0x5);
		}

		static void Install()
		{
			InstallSaveVersionGrab();
			InstallGetSaveVersionFuncHook();
		}
	};

	struct SaveModshook
	{
		static RE::BGSSaveGameBuffer* BGSSaveGameBufferCTOR(RE::BGSSaveGameBuffer* a_buffer)
		{
			using func_t = decltype(&BGSSaveGameBufferCTOR);
			REL::Relocation<func_t> func{ REL::Offset(0x5A0E70) };
			return func(a_buffer);
		}

		static void BGSSaveGameBuffer_SaveFileName(RE::BGSSaveGameBuffer* a_buffer, const char* a_fileName)
		{
			using func_t = decltype(&BGSSaveGameBuffer_SaveFileName);
			REL::Relocation<func_t> func{ REL::Offset(0x5A1100) };
			return func(a_buffer, a_fileName);
		}

		static void saveUnk(RE::BGSSaveGameBuffer* a_buffer, Win32FileType* a_unk)
		{
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

		// Replica of what SE does
		static void SaveModNames(SaveLoadGame* a_saveLoadGame, Win32FileType* a_unk2)
		{
			logger::info("SaveModNames called");
			RE::BGSSaveGameBuffer* buffer = RE::calloc<RE::BGSSaveGameBuffer>(1);
			BGSSaveGameBufferCTOR(buffer);
			auto handler = DataHandler::GetSingleton();
			auto fileSize = handler->compiledFileCollection.files.size();
			auto smallFileSize = handler->compiledFileCollection.smallFiles.size();

			buffer->SaveDataEndian(&fileSize, 1u);
			for (auto file : handler->compiledFileCollection.files) {
				logger::debug("Saving file {}", std::string(file->fileName));
				BGSSaveGameBuffer_SaveFileName(buffer, std::string(file->fileName).c_str());
			}
			buffer->SaveDataEndian(&smallFileSize, 2u);
			for (auto file : handler->compiledFileCollection.smallFiles) {
				logger::debug("Saving small file {}", std::string(file->fileName));
				BGSSaveGameBuffer_SaveFileName(buffer, std::string(file->fileName).c_str());
			}

			saveUnk(buffer, a_unk2);
			BGSSaveGameBufferDTOR(buffer);
			logger::info("SaveModNames finished: regular {} light {}", fileSize, smallFileSize);
		}

		static void Install()
		{
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
			REL::Relocation<func_t> func{ REL::Offset(0x59EFB0) };
			return func(a_buffer, a_unk);
		}

		static void BGSLoadGameBufferReadString(RE::BGSLoadGameBuffer* a_buffer, char a_string[])
		{
			using func_t = decltype(&BGSLoadGameBufferReadString);
			REL::Relocation<func_t> func{ REL::Offset(0x59F1E0) };
			return func(a_buffer, a_string);
		}

		static void BGSLoadGameBufferDTOR(RE::BGSLoadGameBuffer* a_buffer)
		{
			using func_t = decltype(&BGSLoadGameBufferDTOR);
			REL::Relocation<func_t> func{ REL::Offset(0x59EF60) };
			return func(a_buffer);
		}

		// Copy of logic from SE/AE 
		static bool LoadMods(SaveLoadGame* a_saveloadGame, Win32FileType* a_unk2, std::uint64_t a_unk3)
		{
			logger::info("LoadMods called");
			bool loadOrderValid = true;  // Load order is valid for the current save
			RE::BGSLoadGameBuffer* buffer = RE::calloc<RE::BGSLoadGameBuffer>(1);
			BGSLoadGameBufferCTOR(buffer);
			BGSLoadGameBufferFile(buffer, a_unk2);
			a_saveloadGame->regularPluginList = RE::BSTArray<RE::TESFile*>();
			a_saveloadGame->smallPluginList = RE::BSTArray<RE::TESFile*>();
			std::uint32_t regularFileCount = 0;
			std::uint32_t smallFileCount = 0;
			buffer->LoadDataEndian(&regularFileCount, 1u, 0);  // TODO: Offset doesn't exist, we should remove it
			char fileName[328]{ 0 };
			for (std::uint32_t i = 0; i < regularFileCount; i++) {
				BGSLoadGameBufferReadString(buffer, fileName);
				auto file = const_cast<RE::TESFile*>(RE::TESDataHandler::GetSingleton()->LookupModByName(fileName));
				if (file && file->compileIndex != 0xFF) {
					logger::debug("Loading file {}", std::string(file->fileName));
					a_saveloadGame->regularPluginList.push_back(file);
				} else {
					logger::debug("File {} is not loaded for save", std::string(file->fileName));
					a_saveloadGame->regularPluginList.push_back(nullptr); 
					loadOrderValid = false;
				}
			}

			if (buffer->GetVersion() >= SESaveVersion) {
				buffer->LoadDataEndian(&smallFileCount, 2u, 0);
				for (std::uint32_t i = 0; i < smallFileCount; i++) {
					BGSLoadGameBufferReadString(buffer, fileName);
					auto file = const_cast<RE::TESFile*>(RE::TESDataHandler::GetSingleton()->LookupModByName(fileName));
					if (file && file->compileIndex != 0xFF) {
						a_saveloadGame->smallPluginList.push_back(file);
						logger::debug("Loading small file {}", std::string(file->fileName));
					} else {
						logger::debug("Small File {} is not loaded for save", std::string(file->fileName));
						a_saveloadGame->smallPluginList.push_back(nullptr);
						loadOrderValid = false;
					}
				}
			}

			BGSLoadGameBufferDTOR(buffer);
			logger::info("LoadMods finished: regular {} light {}", regularFileCount, smallFileCount);
			return loadOrderValid;
		}

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x585640) };
			SKSE::AllocTrampoline(14);
			SKSE::GetTrampoline().write_branch<5>(target.address(), LoadMods);
		}
	};

	struct FormIDLoadHook
	{
		static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x581660) };

		struct TrampolineCall : Xbyak::CodeGenerator
		{
			TrampolineCall(std::uintptr_t jmpAfterCall, std::uintptr_t func)
			{
				Xbyak::Label funcLabel;
				mov(rcx, rdi);
				sub(rsp, 0x20);
				call(ptr[rip + funcLabel]);
				add(rsp, 0x20);
				mov(edx, eax);
				mov(rcx, jmpAfterCall);
				jmp(rcx);
				L(funcLabel);
				dq(func);
			}
		};

		static RE::FormID ParseLoadedFormID(std::uint64_t a_unk, RE::FormID a_loadedFormID)
		{
			// Dynamic 0xFF index check already occurred at this point
			logger::trace("Parsing loaded form ID {:x}", a_loadedFormID);
			auto saveLoadGame = SaveLoadGame::GetSingleton();
			auto regIndex = a_loadedFormID >> 24;
			RE::TESFile* file = nullptr;
			if (regIndex == 0xFE) {
				auto smallIndex = (a_loadedFormID >> 12) & 0xFFF;
				if (smallIndex >= saveLoadGame->smallPluginList.size()) {
					return 0;
				}
				file = saveLoadGame->smallPluginList[smallIndex];
			} else {
				if (regIndex >= saveLoadGame->regularPluginList.size()) {
					return 0;
				}
				file = saveLoadGame->regularPluginList[regIndex];
			}

			if (file) {
				auto newID = a_loadedFormID;
				eslhooks::adjustFormID::AdjustFormIDFileIndex(file, newID);
				logger::trace("Parsing success: file {} form ID {:x}", file->fileName, newID);
				return newID;
			}

			// file invalid, this means save references a plugin that doesn't exist in the load order
			// Return 0 to indicate the form is invalid
			return 0;
		}

		static void Install()
		{
			std::uintptr_t start = target.address() + 0x85;
			std::uintptr_t end = target.address() + 0xB1;
			REL::safe_fill(start, REL::NOP, end - start);
			auto trampolineJmp = TrampolineCall(end, stl::unrestricted_cast<std::uintptr_t>(ParseLoadedFormID));
			REL::safe_write(start, trampolineJmp.getCode(), trampolineJmp.getSize());

			if (trampolineJmp.getSize() > (end - start)) {
				logger::critical("ParseLoadedFormID trampoline hook {} bytes too big!", trampolineJmp.getSize() - (end - start));
			}
			logger::info("Installed ParseLoadedFormID hook at address {:x}", start);
			logger::info("Installed ParseLoadedFormID hook at offset {:x}", target.offset() + 0x85);
		}
	};

	static inline void InstallHooks()
	{
		SaveModshook::Install();
		LoadModsHook::Install();
		FormIDLoadHook::Install();
		SaveVersionHook::Install();
	}
}

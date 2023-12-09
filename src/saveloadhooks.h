#pragma once
#include <detours/detours.h>
#include "DataHandler.h"

namespace saveloadhooks
{
	struct SaveModshook
	{
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
		static void SaveModNames(std::uint64_t a_unk, Win32FileType* a_unk2)
		{
			logger::info("SaveModNames called");
			RE::BGSSaveGameBuffer* buffer = RE::malloc<RE::BGSSaveGameBuffer>();
			BGSSaveGameBufferCTOR(buffer);
			auto handler = DataHandler::GetSingleton();
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

		static void BGSSaveGameBufferDTOR(RE::BGSLoadGameBuffer* a_buffer)
		{
			using func_t = decltype(&BGSSaveGameBufferDTOR);
			REL::Relocation<func_t> func{ REL::Offset(0x59EF60) };
			return func(a_buffer);
		}


		// TODO: We skip the verification step for the sake of simplicity. We should add that verification and do it SE-style
		static bool LoadMods(RE::BGSSaveLoadGame* a_saveloadManager, Win32FileType* a_unk2, std::uint64_t a_unk3)
		{
			// Simple version, read in the same amount of data from savemodshook and only save the regular files
			// THIS IS A WORKAROUND, NOT THE ACTUAL FIX
			RE::BGSLoadGameBuffer* buffer = RE::malloc<RE::BGSLoadGameBuffer>();
			BGSLoadGameBufferCTOR(buffer);
			BGSLoadGameBufferFile(buffer, a_unk2);
			std::memset(a_saveloadManager->pluginList, 0xFF, 0xFF);
			std::memset(a_saveloadManager->unk18, 0xFF, 0xFF);
			std::uint32_t regularFileCount = 0;
			std::uint32_t smallFileCount = 0;
			buffer->LoadDataEndian(&regularFileCount, 1u, 0);  // TODO: Offset doesn't exist, we should remove it
			char fileName[328]{ 0 };
			for (std::uint32_t i = 0; i < regularFileCount; i++) {
				BGSLoadGameBufferReadString(buffer, fileName);
				auto file = RE::TESDataHandler::GetSingleton()->LookupModByName(fileName);
				if (file && file->compileIndex != 0xFF) {
					a_saveloadManager->pluginList[i] = i;  // TODO: this should be BSTArray like in SE
					a_saveloadManager->unk18[i] = i;       // also BSTArray like in SE, seems unused in SE
				}
			}

			// TODO: SE does a version check on save before loading ESLs, we should do the same for mid-save compatibily
			buffer->LoadDataEndian(&regularFileCount, 2u, 0);
			for (std::uint32_t i = 0; i < smallFileCount; i++) {
				BGSLoadGameBufferReadString(buffer, fileName);
				// TODO: Grab and store the ESLs like SE does
			}
			delete buffer;
			return true;  // TODO: We need to add verification check that save matches load order. For now, we return true saying they do match
		}

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::Offset(0x585640) };
			SKSE::AllocTrampoline(14);
			SKSE::GetTrampoline().write_branch<5>(target.address(), LoadMods);
		}
	};

	static inline void InstallHooks()
	{
		SaveModshook::Install();
		LoadModsHook::Install();
		// TODO: We need to hook BGSSaveLoadGame and let it know about ESLs as a concept, just like SE
	}
}

#pragma once

// SKSEVR patching code from po3-Tweaks under MIT
// https://github.com/powerof3/po3-Tweaks/blob/850f80f298c0250565ff24ff3aba68a45ca8b73a/src/Fixes/CrosshairRefEventVR.cpp

namespace SKSEVRHooks
{

	// code converted from skse64
	static std::unordered_map<std::uint32_t, std::uint32_t> s_savedModIndexMap;

	void SavePluginsList(SKSE::SerializationInterface* intfc)
	{
		DataHandler* dhand = DataHandler::GetSingleton();

		std::uint16_t regCount = 0;
		std::uint16_t lightCount = 0;
		std::uint16_t fileCount = 0;
		for (auto file : dhand->files) {
			if (file && file->compileIndex != 0xFF) {
				if (file->compileIndex != 0xFE) {
					regCount++;
				} else {
					lightCount++;
				}
			}
		}
		fileCount = regCount + lightCount;
		bool saveSuccessful = true;
		saveSuccessful &= intfc->OpenRecord('PLGN', 0);
		saveSuccessful &= intfc->WriteRecordData(&fileCount, sizeof(fileCount));

		logger::info("Saving plugin list ({} regular {} light {} total):", regCount, lightCount, fileCount);

		for (auto file : dhand->files) {
			if (file && file->compileIndex != 0xFF) {
				saveSuccessful &= intfc->WriteRecordData(&file->compileIndex, sizeof(file->compileIndex));
				if (file->compileIndex == 0xFE) {
					saveSuccessful &= intfc->WriteRecordData(&file->smallFileCompileIndex, sizeof(file->smallFileCompileIndex));
				}

				auto nameLen = strlen(file->fileName);
				saveSuccessful &= intfc->WriteRecordData(&nameLen, sizeof(nameLen));
				saveSuccessful &= intfc->WriteRecordData(file->fileName, nameLen);
				if (file->compileIndex != 0xFE) {
					logger::info("\t[{}]\t{}", file->compileIndex, file->fileName);
				} else {
					logger::info("\t[FE:{}]\t{}", file->smallFileCompileIndex, file->fileName);
				}
			}
		}
		if (!saveSuccessful) {
			logger::error("SKSE cosave failed");
		}
	}

	//FUN_180028550 + 0x144
	//Probably not necessary but kept just in case
	void LoadLightModList(SKSE::SerializationInterface* intfc, std::uint32_t version = 1)
	{
		enum LightModVersion
		{
			kVersion1 = 1,
			kVersion2 = 2
		};

		logger::info("Loading light mod list:");
		// in skse64 version is a param, but this should coincide with VR
		DataHandler* dhand = DataHandler::GetSingleton();

		char name[0x104] = { 0 };
		std::uint16_t nameLen = 0;

		std::uint16_t numSavedMods = 0;
		if (version == kVersion1) {
			intfc->ReadRecordData(&numSavedMods, sizeof(std::uint8_t));
		} else if (version == kVersion2) {
			intfc->ReadRecordData(&numSavedMods, sizeof(std::uint16_t));
		}

		for (std::uint32_t i = 0; i < numSavedMods; i++) {
			intfc->ReadRecordData(&nameLen, sizeof(nameLen));
			intfc->ReadRecordData(&name, nameLen);
			name[nameLen] = 0;

			std::uint32_t lightIndex = 0xFE000 | i;

			const TESFile* file = dhand->LookupModByName(name);
			if (file) {
				std::uint32_t newIndex = file->GetPartialIndex();
				s_savedModIndexMap[lightIndex] = newIndex;
				logger::info("\t({} -> {})\t{}", lightIndex, newIndex, name);
			} else {
				s_savedModIndexMap[lightIndex] = 0xFF;
			}
			logger::info("\t({} -> {})\t{}", lightIndex, s_savedModIndexMap[lightIndex], name);
		}
	}

	//FUN_180028550 + 0x1cb LoadModList // 2871b
	void LoadModList(SKSE::SerializationInterface* intfc)
	{
		logger::info("Loading mod list:");

		DataHandler* dhand = DataHandler::GetSingleton();

		char name[0x104] = { 0 };
		std::uint16_t nameLen = 0;

		std::uint8_t numSavedMods = 0;
		intfc->ReadRecordData(&numSavedMods, sizeof(numSavedMods));
		for (std::uint32_t i = 0; i < numSavedMods; i++) {
			intfc->ReadRecordData(&nameLen, sizeof(nameLen));
			intfc->ReadRecordData(&name, nameLen);
			name[nameLen] = 0;

			const TESFile* modInfo = dhand->LookupModByName(name);
			if (modInfo) {
				std::uint32_t newIndex = modInfo->GetPartialIndex();
				s_savedModIndexMap[i] = newIndex;
				logger::info("\t({} -> {})\t{}", i, newIndex, name);
			} else {
				s_savedModIndexMap[i] = 0xFF;
			}
		}
	}

	//This is new for skse64 and should be triggered on 'PLGN':
	//Attempt to inject at SKSEVR::FUN_180028550 + 0x30 before the old case statement.
	void LoadPluginList(SKSE::SerializationInterface* intfc)
	{
		DataHandler* dhand = DataHandler::GetSingleton();

		logger::info("Loading plugin list:");

		char name[0x104] = { 0 };
		std::uint16_t nameLen = 0;

		std::uint16_t modCount = 0;
		intfc->ReadRecordData(&modCount, sizeof(modCount));
		for (std::uint32_t i = 0; i < modCount; i++) {
			std::uint8_t modIndex = 0xFF;
			std::uint16_t lightModIndex = 0xFFFF;
			intfc->ReadRecordData(&modIndex, sizeof(modIndex));
			if (modIndex == 0xFE) {
				intfc->ReadRecordData(&lightModIndex, sizeof(lightModIndex));
			}

			intfc->ReadRecordData(&nameLen, sizeof(nameLen));
			intfc->ReadRecordData(&name, nameLen);
			name[nameLen] = 0;

			std::uint32_t newIndex = 0xFF;
			std::uint32_t oldIndex = modIndex == 0xFE ? (0xFE000 | lightModIndex) : modIndex;

			const TESFile* modInfo = dhand->LookupModByName(name);
			if (modInfo) {
				newIndex = modInfo->GetPartialIndex();
			}

			s_savedModIndexMap[oldIndex] = newIndex;

			logger::info("\t({} -> {})\t{}", oldIndex, newIndex, name);
		}
	}

	struct Core_LoadCallback_Switch : Xbyak::CodeGenerator
	{
		Core_LoadCallback_Switch(std::uintptr_t beginSwitch, std::uintptr_t endLoop)
		{
			mov(rdx, dword[rsp + 0x8]);
			cmp(rdx, 0x504C474E);  // 'PLGN'
			jnz("KeepChecking");
			mov(rcx, rbx);  // LoadPluginList(intfc);
			call(&LoadPluginList);
			mov(rcx, endLoop);  // break out
			jmp(rcx);
			L("KeepChecking");
			mov(rcx, beginSwitch);  // keep switching
			jmp(rcx);
		}
		// Install our hook at the specified address

		static inline void Install(std::uintptr_t a_base)
		{
			std::uintptr_t target{ a_base + 0x28573 };
			std::uintptr_t beginSwitch{ a_base + 0x28584 };
			std::uintptr_t endSwitch{ a_base + 0x28753 };

			auto newCompareCheck = Core_LoadCallback_Switch(beginSwitch, endSwitch);
			int fillRange = beginSwitch - target;
			REL::safe_fill(target, REL::NOP, fillRange);
			auto& trampoline = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(newCompareCheck.getSize());
			auto result = trampoline.allocate(newCompareCheck);
			auto& trampoline2 = SKSE::GetTrampoline();
			SKSE::AllocTrampoline(14);
			trampoline2.write_branch<5>(target, (std::uintptr_t)result);

			logger::info("Core_LoadCallback_Switch hooked at address SKSEVR::{:x}", target);
			logger::info("Core_LoadCallback_Switch hooked at offset SKSEVR::{:x}", target);
		}
	};
	bool NOPpatch = false;

	struct SKSEVRPatches
	{
		std::string name;
		std::uintptr_t offset;
		std::uint8_t readBytes[5];
		void* function;
	};

	std::vector<SKSEVRPatches> patches{
		{ "SaveModList", 0x283bd, { 0xe8, 0x8e, 0xfc, 0xff, 0xff }, &SavePluginsList },
		{ "LoadModList", 0x2871b, { 0xe8, 0xc0, 0xf7, 0xff, 0xff }, &LoadModList },
		{ "LoadLightModList", 0x28694, { 0xe8, 0x77, 0xfa, 0xff, 0xff }, &LoadLightModList },
	};
	void Install(std::uint32_t a_skse_version)
	{
		auto sksevr_base = reinterpret_cast<uintptr_t>(GetModuleHandleA("sksevr_1_4_15"));
		if (a_skse_version == 33554624) {  //2.0.12
			logger::info("Found patchable sksevr_1_4_15.dll version {} with base {:x}", a_skse_version, sksevr_base);
		} else {
			logger::info("Found unknown sksevr_1_4_15.dll version {} with base {:x}; not patching", a_skse_version, sksevr_base);
			return;
		}
		// TODO: Determine why this section errors with `displacement is out of range`
		auto& tramp = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(14 * patches.size());
		for (const auto& patch : patches) {
			logger::info("Trying to patch {} at {:x}"sv, patch.name, sksevr_base + patch.offset);
			const std::uint8_t* read_addr = (std::uint8_t*)(uintptr_t)(sksevr_base + patch.offset);
			if (std::memcmp((const void*)read_addr, patch.readBytes, sizeof(patch.readBytes))) {
				logger::info("{} Read code is not as expected; not patching"sv, patch.name);
				continue;
			}
			if (NOPpatch) {
				REL::safe_fill((uintptr_t)read_addr, REL::NOP, sizeof(patch.readBytes));  // NOP
			} else {
				tramp.write_call<5>((uintptr_t)read_addr, patch.function);
			}
			logger::info("SKSEVR {} patched"sv, patch.name);
		}
		// TODO: Determine why this CTDs
		Core_LoadCallback_Switch::Install(sksevr_base);
	}
}

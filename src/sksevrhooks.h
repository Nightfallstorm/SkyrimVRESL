#pragma once

// SKSEVR patching code from po3-Tweaks under MIT
// https://github.com/powerof3/po3-Tweaks/blob/850f80f298c0250565ff24ff3aba68a45ca8b73a/src/Fixes/CrosshairRefEventVR.cpp

namespace SKSEVRHooks
{
	// code converted from skse64
	static std::unordered_map<std::uint32_t, std::uint32_t> s_savedModIndexMap;

	struct TrampolineJmp : Xbyak::CodeGenerator
	{
		TrampolineJmp(std::uintptr_t func)
		{
			Xbyak::Label funcLabel;

			jmp(ptr[rip + funcLabel]);
			L(funcLabel);
			dq(func);
		}
	};

	// Allocates executable memory within ±2GB of `target` by scanning for free
	// virtual address space. Falls back to system-chosen allocation if not found.
	static void* AllocNear(std::uintptr_t target, std::size_t size)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		const std::uintptr_t gran = si.dwAllocationGranularity;

		// ±2 GB window, clamped to valid address space
		const std::uintptr_t lo = (target > 0x7FFF0000ULL) ? target - 0x7FFF0000ULL : 0;
		const std::uintptr_t hi = (target + 0x7FFF0000ULL > target) ? target + 0x7FFF0000ULL : UINTPTR_MAX;

		MEMORY_BASIC_INFORMATION mbi;

		// Search forward from target
		for (std::uintptr_t addr = (target + gran - 1) & ~(gran - 1); addr < hi;) {
			if (!VirtualQuery(reinterpret_cast<void*>(addr), &mbi, sizeof(mbi)))
				break;
			if (mbi.State == MEM_FREE) {
				if (void* p = VirtualAlloc(reinterpret_cast<void*>(addr), size,
						MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))
					return p;
			}
			addr = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
			addr = (addr + gran - 1) & ~(gran - 1);
		}

		// Search backward from target
		for (std::uintptr_t addr = target & ~(gran - 1); addr > lo; addr -= gran) {
			if (!VirtualQuery(reinterpret_cast<void*>(addr), &mbi, sizeof(mbi)))
				break;
			if (mbi.State == MEM_FREE) {
				if (void* p = VirtualAlloc(reinterpret_cast<void*>(addr), size,
						MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))
					return p;
			}
		}

		logger::warn("SKSEVRHooks::AllocNear: no free region found within 2GB of {:x}; falling back", target);
		return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	}

	// Writes the Xbyak code `gen` to memory allocated near `target`, then patches
	// `target` with a 5-byte relative JMP to that memory. The region is kept alive
	// for the lifetime of the process (no free — it holds live code).
	static void WriteNearBranch(std::uintptr_t target, Xbyak::CodeGenerator& gen)
	{
		gen.ready();
		const std::size_t sz = gen.getSize();
		void* stub = AllocNear(target, sz);
		if (!stub) {
			logger::critical("SKSEVRHooks::WriteNearBranch: allocation failed for target {:x}", target);
			return;
		}
		std::memcpy(stub, gen.getCode(), sz);
		FlushInstructionCache(GetCurrentProcess(), stub, sz);

		const std::int32_t rel = static_cast<std::int32_t>(
			reinterpret_cast<std::intptr_t>(stub) - static_cast<std::intptr_t>(target + 5));
		const std::uint8_t jmpByte = 0xE9;
		REL::safe_write(target, &jmpByte, 1);
		REL::safe_write(target + 1, &rel, 4);
	}

	void EraseMap()
	{
		s_savedModIndexMap.clear();
	}

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

				std::uint16_t nameLen = strlen(file->fileName);
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

	std::uint32_t ResolveModIndex(std::uint32_t modIndex)
	{
		auto it = s_savedModIndexMap.find(modIndex);
		if (it != s_savedModIndexMap.end()) {
			return it->second;
		}

		return 0xFF;
	}

	struct ResolveFormIdHook
	{
		// Converted from SKSESE
		static bool ResolveFormId(std::uint32_t formId, std::uint32_t* formIdOut)
		{
			std::uint32_t modID = formId >> 24;
			if (modID == 0xFF) {
				*formIdOut = formId;
				return true;
			}

			if (modID == 0xFE) {
				modID = formId >> 12;
			}

			std::uint32_t loadedModID = ResolveModIndex(modID);
			if (loadedModID < 0xFF) {
				*formIdOut = (formId & 0x00FFFFFF) | (((std::uint32_t)loadedModID) << 24);
				return true;
			} else if (loadedModID > 0xFF) {
				*formIdOut = (loadedModID << 12) | (formId & 0x00000FFF);
				return true;
			}
			return false;
		}

		static void Install(std::uintptr_t a_base)
		{
			std::uintptr_t target = a_base + 0x9BD60;
			auto jmp = TrampolineJmp((std::uintptr_t)ResolveFormId);
			REL::safe_write(target, jmp.getCode(), jmp.getSize());
		}
	};

	struct ResolveHandleHook
	{
		// Converted from SKSESE
		static bool ResolveHandle(std::uint64_t handle, std::uint64_t* handleOut)
		{
			std::uint32_t modID = (handle & 0xFF000000) >> 24;
			if (modID == 0xFF) {
				*handleOut = handle;
				return true;
			}

			if (modID == 0xFE) {
				modID = (handle >> 12) & 0xFFFFF;
			}

			std::uint64_t loadedModID = (std::uint64_t)ResolveModIndex(modID);
			if (loadedModID < 0xFF) {
				*handleOut = (handle & 0xFFFFFFFF00FFFFFF) | (((std::uint64_t)loadedModID) << 24);
				return true;
			} else if (loadedModID > 0xFF) {
				*handleOut = (handle & 0xFFFFFFFF00000FFF) | (loadedModID << 12);
				return true;
			}
			return false;
		}

		static void Install(std::uintptr_t a_base)
		{
			std::uintptr_t target = a_base + 0x9BE00;
			auto jmp = TrampolineJmp((std::uintptr_t)ResolveHandle);
			REL::safe_write(target, jmp.getCode(), jmp.getSize());
		}
	};

	struct Core_LoadCallback_Switch : Xbyak::CodeGenerator
	{
		Core_LoadCallback_Switch(std::uintptr_t beginSwitch, std::uintptr_t endLoop, std::uintptr_t func = stl::unrestricted_cast<std::uintptr_t>(LoadPluginList))
		{
			Xbyak::Label funcLabel;
			mov(edx, dword[rsp + 0x30]);
			cmp(edx, 'PLGN');  // 'PLGN'
			jnz("KeepChecking");
			mov(rcx, rbx);  // LoadPluginList(intfc);
			sub(rsp, 0x20);
			call(ptr[rip + funcLabel]);
			add(rsp, 0x20);
			mov(rcx, endLoop);  // break out
			jmp(rcx);
			L("KeepChecking");
			cmp(edx, 'LMOD');
			mov(rcx, beginSwitch);  // keep switching
			jmp(rcx);
			L(funcLabel);
			dq(func);
		}
		static inline void Install(std::uintptr_t a_base)
		{
			std::uintptr_t target{ a_base + 0x28580 };
			std::uintptr_t beginSwitch{ a_base + 0x2858A };
			std::uintptr_t endSwitch{ a_base + 0x28753 };

			auto newCompareCheck = Core_LoadCallback_Switch(beginSwitch, endSwitch);
			REL::safe_fill(target, REL::NOP, beginSwitch - target);
			WriteNearBranch(target, newCompareCheck);
			logger::info("Core_LoadCallback_Switch hooked at SKSEVR+{:x}", target - a_base);
		}
	};

	struct Core_RevertCallbackHook : Xbyak::CodeGenerator
	{
		Core_RevertCallbackHook(std::uintptr_t jmpBack, std::uintptr_t func = stl::unrestricted_cast<std::uintptr_t>(EraseMap))
		{
			Xbyak::Label funcLabel;
			mov(ptr[rsp + 0x8], rbx);
			push(rdi);
			sub(rsp, 0x20);
			sub(rsp, 0x20);
			call(ptr[rip + funcLabel]);
			add(rsp, 0x20);
			mov(rcx, jmpBack);  // break out
			jmp(rcx);
			L(funcLabel);
			dq(func);
		}

		static inline void Install(std::uintptr_t a_base)
		{
			std::uintptr_t target{ a_base + 0x28210 };
			std::uintptr_t jmpBack{ a_base + 0x2821A };

			auto newCompareCheck = Core_RevertCallbackHook(jmpBack);
			REL::safe_fill(target, REL::NOP, jmpBack - target);
			WriteNearBranch(target, newCompareCheck);
			logger::info("Core_RevertCallbackHook hooked at SKSEVR+{:x}", target - a_base);
		}
	};

	struct SKSEVRPatches
	{
		std::string name;
		std::uintptr_t offset;
		void* function;
	};

	std::vector<SKSEVRPatches> patches{
		{ "SaveModList", 0x28050, SavePluginsList },
		{ "LoadModList", 0x27EE0, LoadModList },
		{ "LoadLightModList", 0x28110, LoadLightModList },
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

		for (const auto& patch : patches) {
			logger::info("Trying to patch {} at {:x} with {:x}"sv, patch.name, sksevr_base + patch.offset, (std::uintptr_t)patch.function);
			std::uintptr_t target = (uintptr_t)(sksevr_base + patch.offset);
			auto jmp = TrampolineJmp((std::uintptr_t)patch.function);
			REL::safe_write(target, jmp.getCode(), jmp.getSize());

			logger::info("SKSEVR {} patched"sv, patch.name);
		}

		Core_LoadCallback_Switch::Install(sksevr_base);
		Core_RevertCallbackHook::Install(sksevr_base);
		ResolveFormIdHook::Install(sksevr_base);
		ResolveHandleHook::Install(sksevr_base);
	}
}

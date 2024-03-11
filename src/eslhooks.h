#pragma once
#include "DataHandler.h"
#include <detours/detours.h>

struct
{
	std::uint64_t signature = 0x123456789ABCDEF;

} cehelper;
namespace eslhooks
{
	struct ESLFlagHook
	{
		static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x18AEB0) };
		using FileFlag = RE::TESFile::RecordFlag;
		struct TrampolineCall : Xbyak::CodeGenerator
		{
			TrampolineCall(std::uintptr_t jmpAfterCall, std::uintptr_t func)
			{
				Xbyak::Label funcLabel;
				mov(rcx, rdi);
				sub(rsp, 0x20);
				call(ptr[rip + funcLabel]);
				add(rsp, 0x20);
				mov(rcx, jmpAfterCall);
				jmp(rcx);
				L(funcLabel);
				dq(func);
			}
		};

		// Give ESL/ESM flag like SE does
		static void SetFormFlag(RE::TESFile* a_self)
		{
			logger::debug("SetFormFlag called on file {}", std::string(a_self->fileName));
			auto flags = a_self->currentform.flags;
			a_self->recordFlags.reset(FileFlag::kMaster, FileFlag::kOptimizedFile, FileFlag::kDelocalized, FileFlag::kSmallFile);
			if (flags & 0x1) {
				a_self->recordFlags.set(FileFlag::kMaster);
			}
			if (flags & 0x10) {
				a_self->recordFlags.set(FileFlag::kOptimizedFile);
			}
			if (flags & 0x80) {
				a_self->recordFlags.set(FileFlag::kDelocalized);
			}
			if (flags & 0x200) {
				a_self->recordFlags.set(FileFlag::kSmallFile);
			}
			if (flags & overlayBit) {
				setOverlay(a_self);
			}

			auto fileName = std::string(a_self->fileName);
			if (fileName.ends_with("esm")) {
				a_self->recordFlags.set(FileFlag::kMaster);
			} else if (fileName.ends_with("esl")) {
				a_self->recordFlags.set(FileFlag::kMaster, FileFlag::kSmallFile);
			}
			logger::debug("SetFormFlag flags on {}, {:x}", std::string(a_self->fileName), a_self->recordFlags.underlying());
		}

		// Install our hook at the specified address
		static inline void Install()
		{
			std::uintptr_t start = target.address() + 0x4B2;
			std::uintptr_t end = target.address() + 0x500;
			REL::safe_fill(start, REL::NOP, end - start);
			auto trampolineJmp = TrampolineCall(end, stl::unrestricted_cast<std::uintptr_t>(SetFormFlag));
			REL::safe_write(start, trampolineJmp.getCode(), trampolineJmp.getSize());

			if (trampolineJmp.getSize() > (end - start)) {
				logger::critical("SetESLFlag trampoline hook {} bytes too big!", trampolineJmp.getSize() - (end - start));
			}
		}
	};

	namespace adjustFormID
	{
		static RE::TESFile* GetMasterAtIndex(RE::TESFile* a_file, std::uint32_t a_index)
		{
			if (a_index > a_file->masterCount) {
				return nullptr;
			}
			if (!a_index) {
				return a_file;
			}
			if (!a_file->masterPtrs) {
				return nullptr;
			}
			return a_file->masterPtrs[a_index - 1];
		}

		static void AdjustFormIDFileIndex(RE::TESFile* a_file, RE::FormID& a_formID)
		{
			logger::trace("Adjust form {:x} for file {} (isLight: {})", a_formID, std::string(a_file->fileName), a_file->IsLight());

			if (isOverlay(a_file)) {
				auto errorMsg = std::format("Attempted to set formID {:x} to overlay file {}. This is a broken plugin!! Remove the new record or contact the mod author", a_formID, a_file->fileName);
				stl::report_and_fail(errorMsg);
			}
			a_formID &= 0xFFFFFFu;  // Strip file index, now 0x00XXXXXX;
			std::uint32_t a_fileIndex = 0;
			if (a_file->IsLight()) {
				a_formID &= 0xFFFu;  // Strip ESL index, now 0x00000XXX;
				a_fileIndex |= 0xFE000000u;
				a_fileIndex |= a_file->smallFileCompileIndex << 12;
			} else {
				a_fileIndex |= a_file->compileIndex << 24;
			}
			a_formID |= a_fileIndex;

			logger::trace("Adjust form result: {:x}", a_formID);
		}

		static bool IsFormIDReserved(RE::FormID a_formID)
		{
			return a_formID <= 0x7FF;
		}

		struct PapyrusGetFormFromFileHook
		{
			static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x9ACCA0) };

			struct TrampolineCall : Xbyak::CodeGenerator
			{
				TrampolineCall(std::uintptr_t jmpAfterCall, std::uintptr_t func)
				{
					Xbyak::Label funcLabel;
					mov(rcx, rbx);
					mov(edx, edi);
					sub(rsp, 0x20);
					call(ptr[rip + funcLabel]);
					add(rsp, 0x20);
					mov(rcx, jmpAfterCall);
					jmp(rcx);
					L(funcLabel);
					dq(func);
				}
			};

			static RE::TESForm* GetFormFromFile(RE::TESFile* a_file, RE::FormID a_rawID)
			{
				logger::debug("GetFormFromFile called for {} ,{:x}", a_file->fileName, a_rawID);
				auto formID = a_rawID;
				AdjustFormIDFileIndex(a_file, formID);
				logger::debug("GetFormFromFile formID looking up {:x}", formID);
				return RE::TESForm::LookupByID(formID);
			}

			static void Install()
			{
				std::uintptr_t start = target.address() + 0x50;
				std::uintptr_t end = target.address() + 0x6B;
				REL::safe_fill(start, REL::NOP, end - start);

				auto trampolineJmp = TrampolineCall(end, stl::unrestricted_cast<std::uintptr_t>(GetFormFromFile));
				auto& trampoline = SKSE::GetTrampoline();
				SKSE::AllocTrampoline(trampolineJmp.getSize());
				auto result = trampoline.allocate(trampolineJmp);
				SKSE::AllocTrampoline(14);
				trampoline.write_branch<5>(start, (std::uintptr_t)result);
			}
		};

		struct UnkFileFormReadHook
		{
			static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x180B80) };

			struct TrampolineCall : Xbyak::CodeGenerator
			{
				TrampolineCall(std::uintptr_t jmpAfterCall, std::uintptr_t func)
				{
					Xbyak::Label funcLabel;
					mov(rcx, rax);
					mov(edx, ebx);
					sub(rsp, 0x20);
					call(ptr[rip + funcLabel]);
					add(rsp, 0x20);
					mov(rcx, rax);
					mov(rdx, jmpAfterCall);
					jmp(rdx);
					L(funcLabel);
					dq(func);
				}
			};

			static RE::FormID AdjustFormID(RE::TESFile* a_file, RE::FormID a_rawID)
			{
				auto formID = a_rawID;
				AdjustFormIDFileIndex(a_file, formID);
				return formID;
			}

			static void Install()
			{
				std::uintptr_t start = target.address() + 0x43C;
				std::uintptr_t end = target.address() + 0x452;
				REL::safe_fill(start, REL::NOP, end - start);

				auto trampolineJmp = TrampolineCall(end, stl::unrestricted_cast<std::uintptr_t>(AdjustFormID));
				auto& trampoline = SKSE::GetTrampoline();
				SKSE::AllocTrampoline(trampolineJmp.getSize());
				auto result = trampoline.allocate(trampolineJmp);
				SKSE::AllocTrampoline(14);
				trampoline.write_branch<5>(start, (std::uintptr_t)result);
			}
		};

		struct AddCompileIndexHook
		{
			// TODO: HookName may not be accurate, read code and find better name
			static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x1A5510) };

			static void AddCompileIndex(RE::FormID& a_formID, RE::TESFile* a_file)
			{
				if (IsFormIDReserved(a_formID) || !a_file) {
					return;
				}

				auto index = (a_formID >> 24) + 1;
				auto master = GetMasterAtIndex(a_file, index);
				if (!master) {
					master = a_file;
				}
				AdjustFormIDFileIndex(master, a_formID);
			}

			static void Install()
			{
				SKSE::GetTrampoline().write_branch<5>(target.address(), AddCompileIndex);
			}
		};

		struct UnkDataHandlerWorldspaceFormLookupHook
		{
			static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x17C000) };

			struct TrampolineCall : Xbyak::CodeGenerator
			{
				TrampolineCall(std::uintptr_t jmpAfterCall, std::uintptr_t func)
				{
					Xbyak::Label funcLabel;
					mov(rcx, rax);
					mov(edx, edi);
					sub(rsp, 0x20);
					call(ptr[rip + funcLabel]);
					add(rsp, 0x20);
					mov(rcx, rax);
					mov(rdx, jmpAfterCall);
					jmp(rdx);
					L(funcLabel);
					dq(func);
				}
			};

			static RE::FormID AdjustFormID(RE::TESFile* a_file, RE::FormID a_rawID)
			{
				auto formID = a_rawID;
				AdjustFormIDFileIndex(a_file, formID);
				return formID;
			}

			static void Install()
			{
				std::uintptr_t start = target.address() + 0x24F;
				std::uintptr_t end = target.address() + 0x265;
				REL::safe_fill(start, REL::NOP, end - start);

				auto trampolineJmp = TrampolineCall(end, stl::unrestricted_cast<std::uintptr_t>(AdjustFormID));
				auto& trampoline = SKSE::GetTrampoline();
				SKSE::AllocTrampoline(trampolineJmp.getSize());
				auto result = trampoline.allocate(trampolineJmp);
				SKSE::AllocTrampoline(14);
				trampoline.write_branch<5>(start, (std::uintptr_t)result);
			}
		};

		struct UnkCurrentFormIDHook
		{
			static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x18EAD0) };

			struct TrampolineCall : Xbyak::CodeGenerator
			{
				TrampolineCall(std::uintptr_t jmpAfterCall, std::uintptr_t func)
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

			static RE::TESFile* GetMaster(RE::TESFile* a_file)
			{
				std::uint32_t formID = a_file->currentform.formID;
				auto masterIndex = formID >> 24;
				if (masterIndex == -1 || (masterIndex + 1u) > a_file->masterCount || !a_file->masterPtrs) {
					return a_file;
				}

				return a_file->masterPtrs[masterIndex];
			}

			static void AdjustCurrentFormID(RE::TESFile* a_file)
			{
				auto master = GetMaster(a_file);

				if (!IsFormIDReserved((a_file->currentform.formID))) {
					AdjustFormIDFileIndex(master, a_file->currentform.formID);
				}
			}

			static void Install()
			{
				std::uintptr_t start = target.address() + 0x113;
				std::uintptr_t end = target.address() + 0x198;
				REL::safe_fill(start, REL::NOP, end - start);
				auto trampolineJmp = TrampolineCall(end, stl::unrestricted_cast<std::uintptr_t>(AdjustCurrentFormID));
				REL::safe_write(start, trampolineJmp.getCode(), trampolineJmp.getSize());

				if (trampolineJmp.getSize() > (end - start)) {
					logger::critical("UnkCurrentFormIDHook trampoline {:x} bytes too big", trampolineJmp.getSize() - (end - start));
				}
			}
		};

		// Credit to Nukem for backported ESL support: https://github.com/Nukem9/skyrimse-backported-esl-support/blob/7d382032e535e594ffc2b2aef2ce970b8763993e/source/Impl/LoaderHooks.cpp
		struct BackwardsESLHeaderSupportHook
		{
			constexpr static float HedrVersion16659 = 1.70f;   // <= 1.6.659
			constexpr static float HedrVersion161126 = 1.71f;  // >= 1.6.1126

			static bool thunk(RE::TESFile* a_this, void* a_data, std::uint32_t a_maxSize)
			{
				if (!a_this->ReadData(a_data, a_maxSize))
					return false;

				// Patch the HEDR record data
				auto& version = *reinterpret_cast<float*>(a_data);

				if (version > HedrVersion16659 && version <= HedrVersion161126) {
					version = HedrVersion16659;
					SKSE::log::info("Emulated old header version for {}.", a_this->fileName);
				}

				return true;
			}

			static inline REL::Relocation<decltype(thunk)> func;

			static void Install()
			{
				REL::Relocation<std::uintptr_t> target{ REL::Offset(0x18AEB0 + 0x44E) };
				pstl::write_thunk_call<BackwardsESLHeaderSupportHook>(target.address());
			}
		};

		static void InstallHooks()
		{
			PapyrusGetFormFromFileHook::Install();
			UnkFileFormReadHook::Install();
			AddCompileIndexHook::Install();
			UnkDataHandlerWorldspaceFormLookupHook::Install();
			UnkCurrentFormIDHook::Install();
			BackwardsESLHeaderSupportHook::Install();
		}
	}

	namespace isesl
	{

		struct GetFactTintHook
		{
			static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x36FB70) };

			static void thunk(char* const buffer, const size_t a_size, const char* a_format, char a_fileName[], RE::FormID a_formID)
			{
				auto file = RE::TESForm::LookupByID(a_formID)->GetFile(0);
				auto formID = file->IsLight() ? a_formID & 0xFFF : a_formID & 0xFFFFFF;
				return func(buffer, a_size, a_format, a_fileName, formID);
			}

			static inline REL::Relocation<decltype(thunk)> func;

			static void Install()
			{
				REL::safe_fill(target.address() + 0x77, REL::NOP, 0x5);  // Erase truncation of plugin index on formID
				pstl::write_thunk_call<GetFactTintHook>(target.address() + 0x8A);
				logger::info("GetFactTintHook hooked at {:x}", target.address() + 0x8A);
				logger::info("GetFactTintHook hooked at offset {:x}", target.offset() + 0x8A);
			}
		};

		struct GetFactTintHook2
		{
			static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x36FC20) };

			static void thunk(char* const buffer, const size_t a_size, const char* a_format, char a_fileName[], RE::FormID a_formID)
			{
				auto file = RE::TESForm::LookupByID(a_formID)->GetFile(0);
				auto formID = file->IsLight() ? a_formID & 0xFFF : a_formID & 0xFFFFFF;
				return func(buffer, a_size, a_format, a_fileName, formID);
			}

			static inline REL::Relocation<decltype(thunk)> func;

			static void Install()
			{
				REL::safe_fill(target.address() + 0x70, REL::NOP, 0x6);  // Erase truncation of plugin index on formID
				pstl::write_thunk_call<GetFactTintHook2>(target.address() + 0x8F);
				logger::info("GetFactTintHook2 hooked at {:x}", target.address() + 0x8F);
				logger::info("GetFactTintHook2 hooked at offset {:x}", target.offset() + 0x8F);
			}
		};

		struct GetMeshFilenameHook
		{
			static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x372B30) };

			static void thunk(char* const buffer, const size_t a_size, const char* a_format, char a_fileName[], RE::FormID a_formID)
			{
				auto file = RE::TESForm::LookupByID(a_formID)->GetFile(0);
				auto formID = file->IsLight() ? a_formID & 0xFFF : a_formID & 0xFFFFFF;
				return func(buffer, a_size, a_format, a_fileName, formID);
			}

			static inline REL::Relocation<decltype(thunk)> func;

			static void Install()
			{
				REL::safe_fill(target.address() + 0x72, REL::NOP, 0x5);  // Erase truncation of plugin index on formID
				pstl::write_thunk_call<GetMeshFilenameHook>(target.address() + 0x87);
				logger::info("GetMeshFilenameHook hooked at {:x}", target.address() + 0x87);
				logger::info("GetMeshFilenameHook hooked at offset {:x}", target.offset() + 0x87);
			}
		};

		struct GetMeshFilenameHook2
		{
			static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x372BD0) };

			static void thunk(char* const buffer, const size_t a_size, const char* a_format, char a_fileName[], RE::FormID a_formID)
			{
				auto file = RE::TESForm::LookupByID(a_formID)->GetFile(0);
				auto formID = file->IsLight() ? a_formID & 0xFFF : a_formID & 0xFFFFFF;
				return func(buffer, a_size, a_format, a_fileName, formID);
			}

			static inline REL::Relocation<decltype(thunk)> func;

			static void Install()
			{
				REL::safe_fill(target.address() + 0xBD, REL::NOP, 0x6);  // Erase truncation of plugin index on formID
				pstl::write_thunk_call<GetMeshFilenameHook2>(target.address() + 0xDE);
				logger::info("GetMeshFilenameHook2 hooked at {:x}", target.address() + 0xDE);
				logger::info("GetMeshFilenameHook2 hooked at offset {:x}", target.offset() + 0xDE);
			}
		};

		struct UnkTopicHook2
		{
			static inline REL::Relocation<std::uintptr_t> target{ REL::Offset(0x39D660) };

			static void thunk(char* const buffer, const size_t a_size, const char* a_format, char a_unk[], RE::FormID a_formID, int a_unk2)
			{
				auto file = RE::TESForm::LookupByID(a_formID)->GetFile(0);
				auto formID = file->IsLight() ? a_formID & 0xFFF : a_formID & 0xFFFFFF;
				return func(buffer, a_size, a_format, a_unk, formID, a_unk2);
			}

			static inline REL::Relocation<decltype(thunk)> func;

			static void Install()
			{
				REL::safe_fill(target.address() + 0x7E, REL::NOP, 0x5);  // Erase truncation of plugin index on formID
				pstl::write_thunk_call<UnkTopicHook2>(target.address() + 0xA4);
				logger::info("UnkTopicHook2 hooked at {:x}", target.address() + 0xA4);
				logger::info("UnkTopicHook2 hooked at offset {:x}", target.offset() + 0xA4);
			}
		};

		static void InstallHooks()
		{
			GetFactTintHook::Install();
			GetFactTintHook2::Install();
			GetMeshFilenameHook::Install();
			GetMeshFilenameHook2::Install();
			UnkTopicHook2::Install();
		}
	}

	static void InstallHooks()
	{
		ESLFlagHook::Install();
		adjustFormID::InstallHooks();
		isesl::InstallHooks();
	}
}

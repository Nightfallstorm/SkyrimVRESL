#pragma once

#include "SaveLoadGame.h"

struct BGSSaveLoadGameCTORHook
{
	// Erase memset calls to pluginsList and unk18 in RE::BGSSaveLoadGame
	// We need it all zeroed out since we are replacing it with our BSTArrays from SE
	static void thunk(void* a_ptr, int a_val, size_t a_size)
	{
		return func(a_ptr, 0, a_size);
	}

	static inline REL::Relocation<decltype(thunk)> func;

	static void EraseMemset()
	{
		REL::Relocation<std::uintptr_t> target{ REL::Offset(0x5819F0) };
		pstl::write_thunk_call<BGSSaveLoadGameCTORHook>(target.address() + 0x144);
		pstl::write_thunk_call<BGSSaveLoadGameCTORHook>(target.address() + 0x158);
	}

	static void Install()
	{
		EraseMemset();
	}
};

void SaveLoadGame::InstallHooks()
{
	BGSSaveLoadGameCTORHook::Install();
}

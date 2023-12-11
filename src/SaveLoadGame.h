#pragma once

#include "RE/B/BSPointerHandle.h"
#include "RE/B/BSTArray.h"
#include "RE/B/BSTList.h"
#include "RE/B/BSTSingleton.h"
#include "RE/F/FormTypes.h"
#include "RE/I/InventoryChanges.h"
#include "RE/N/NiTArray.h"
#include "RE/N/NiTList.h"
#include "RE/T/TESForm.h"
#include <detours/detours.h>

using namespace RE;

class SaveLoadGame
{
public:
	static SaveLoadGame* GetSingleton()
	{
		return reinterpret_cast<SaveLoadGame*>(RE::BGSSaveLoadGame::GetSingleton());
	}

	// members
	// ~~~~~~~~~~~~~~~~~ below member differs from VR~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	BSTArray<const TESFile*> regularPluginList;  // 000
	BSTArray<const TESFile*> smallPluginList;    // 018

	std::uint64_t fakeVRPadding[0x3A];
	// ~~~~~~~~~~~~~~~~~ end VR difference ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	BGSSaveLoadFormIDMap worldspaceFormIDMap;                           // 200
	BSTHashMap<FormID, ActorHandle> unk98;                              // 268
	BGSSaveLoadReferencesMap unkC8;                                     // 298
	BSTHashMap<FormID, FormID> unk158;                                  // 328
	BGSConstructFormsInAllFilesMap reconstructFormsMap;                 // 358
	BGSSaveLoadQueuedSubBufferMap queuedSubBuffersMap;                  // 3D8
	BGSSaveLoadFormIDMap formIDMap;                                     // 468
	BSTArray<void*> saveLoadHistory;                                    // 4D0
	BSTArray<void*> unk318;                                             // 4E8
	BGSSaveLoadChangesMap* saveLoadChanges;                             // 500
	std::uint64_t unk338;                                               // 508
	stl::enumeration<RE::BGSSaveLoadGame::Flags, std::uint32_t> flags;  // 510
	std::uint8_t currentMinorVersion;                                   // 514
};
static_assert(sizeof(SaveLoadGame) == 0x518);

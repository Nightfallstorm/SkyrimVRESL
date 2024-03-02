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
#include "SkyrimVRESLAPI.h"
#include <detours/detours.h>

using namespace RE;

struct TESOverlayFileCollection : RE::TESFileCollection {
	BSTArray<TESFile*> overlayFiles;  // 20
};

static inline std::uint32_t overlayBit = 1 << 20;

static inline void setOverlay(RE::TESFile* a_file)
{
	auto recordFlagsAddr = reinterpret_cast<std::uintptr_t>(a_file) + offsetof(RE::TESFile, recordFlags);
	auto recordFlags = reinterpret_cast<int*>(recordFlagsAddr);
	auto newFlags = a_file->recordFlags.underlying() | overlayBit;
	*recordFlags = newFlags;
}

static inline bool isOverlay(RE::TESFile* a_file) {
	int isOverlay = a_file->recordFlags.underlying() & overlayBit;
	return isOverlay != 0;
}

class DataHandler : public
#ifndef BACKWARDS_COMPATIBLE
					BSTSingletonSDM<DataHandler>
#else
					TESDataHandler
#endif
{
public:
	static DataHandler* GetSingleton();
	static void InstallHooks();

#ifndef BACKWARDS_COMPATIBLE
	const RE::TESFile* LookupModByName(std::string_view a_modName);
	// members
	std::uint8_t pad001;                                               // 001
	std::uint16_t pad002;                                              // 002
	std::uint32_t pad004;                                              // 004
	TESObjectList* objectList;                                         // 008
	BSTArray<TESForm*> formArrays[stl::to_underlying(FormType::Max)];  // 010
	TESRegionList* regionList;                                         // D00
	NiTPrimitiveArray<TESObjectCELL*> interiorCells;                   // D08
	NiTPrimitiveArray<BGSAddonNode*> addonNodes;                       // D20
	NiTList<TESForm*> badForms;                                        // D38
	FormID nextID;                                                     // D50
	std::uint32_t padD54;                                              // D54
	TESFile* activeFile;                                               // D58
	BSSimpleList<TESFile*> files;                                      // D60
																	   // ~~~~~~~~~~~~~~~~~ below member differs from VR~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	TESOverlayFileCollection compiledFileCollection;                   // D70
	std::uint64_t fakeVRpadding[0xFA];                                 // D78
																	   /*
* 		std::uint32_t         loadedModCount;     // D70
		std::uint32_t         pad14;              // D74
		TESFile*              loadedMods[0xFF];   // D78
*/
																	   // ~~~~~~~~~~~~~~~~~ end VR difference ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	bool masterSave;                                                   // 1570
	bool blockSave;                                                    // 1571
	bool saveLoadGame;                                                 // 1572
	bool autoSaving;                                                   // 1574
	bool exportingPlugin;                                              // 1575
	bool clearingData;                                                 // 1576 - init'd to 1
	bool hasDesiredFiles;                                              // 1577
	bool checkingModels;                                               // 1578
	bool loadingFiles;                                                 // 1579
	bool dontRemoveIDs;                                                // 157A
	std::uint8_t pad157B[5];                                           // 157B
	TESRegionDataManager* regionDataManager;                           // 1580
	InventoryChanges* merchantInventory;                               // 1588
};
static_assert(sizeof(DataHandler) == 0x1590);
static_assert(offsetof(DataHandler, masterSave) == 0x1570);
#else
	TESOverlayFileCollection compiledFileCollection;  // 1590
};
static_assert(sizeof(DataHandler) == 0x15D8);
static_assert(offsetof(DataHandler, compiledFileCollection) == 0x1590);
#endif

namespace SkyrimVRESLPluginAPI
{
	// Handles skse mod messages requesting to fetch API functions from SkyrimVRESL
	void ModMessageHandler(SKSE::MessagingInterface::Message* message);

	// This object provides access to SkyrimVRESL's mod support API version 1
	struct SkyrimVRESLInterface001 : ISkyrimVRESLInterface001
	{
		virtual unsigned int GetBuildNumber();

		/// @brief Get the SSE compatible TESFileCollection for SkyrimVR.
		/// @return Pointer to CompiledFileCollection.
		const RE::TESFileCollection* GetCompiledFileCollection();
	};

}  // namespace SkyrimVRESLPluginAPI
extern SkyrimVRESLPluginAPI::SkyrimVRESLInterface001 g_interface001;

// Function that tests GetCompiledFileCollectionExtern() and prints to log.
// This is also an example of how to use GetHandle and GetProc
void TestGetCompiledFileCollectionExtern();

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
	TESFileCollection compiledFileCollection;                          // D70
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
	TESFileCollection compiledFileCollection;  // 1590
};
static_assert(sizeof(DataHandler) == 0x15C0);
static_assert(offsetof(DataHandler, compiledFileCollection) == 0x1590);
#endif

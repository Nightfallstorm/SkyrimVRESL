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

using namespace RE;

class DataHandlerSE : public BSTSingletonSDM<DataHandlerSE>
{
public:
	static void InstallDataHandlerHooks();

	DataHandlerSE* ctor();
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
	TESFileCollection compiledFileCollection;                          // D70
	bool masterSave;                                                   // DA0
	bool blockSave;                                                    // DA1
	bool saveLoadGame;                                                 // DA2
	bool autoSaving;                                                   // DA3
	bool exportingPlugin;                                              // DA4
	bool clearingData;                                                 // DA5
	bool hasDesiredFiles;                                              // DA6
	bool checkingModels;                                               // DA7
	bool loadingFiles;                                                 // DA8
	bool dontRemoveIDs;                                                // DA9
	std::uint8_t unkDAA;                                               // DAA
	std::uint8_t padDAB;                                               // DAB
	std::uint32_t padDAC;                                              // DAC
	TESRegionDataManager* regionDataManager;                           // DB0
	InventoryChanges* merchantInventory;                               // DB8

private:
	static void InstallPatches();
};
static_assert(sizeof(DataHandlerSE) == 0xDC0);


#include "DataHandlerSE.h"

using namespace RE;

static void unkCall(void* a_unk)
{
	using func_t = decltype(&unkCall);
	REL::Relocation<func_t> func{ REL::Offset(0xC8A430) };
	return func(a_unk);
}
static void* vctor(void* a_self)
{
	using func_t = decltype(&vctor);
	REL::Relocation<func_t> func{ REL::Offset(0x2CE8E0) };
	return func(a_self);
}

static TESRegionList* tctor(TESRegionList* a_self, bool a_unk)
{
	using func_t = decltype(&tctor);
	REL::Relocation<func_t> func{ REL::Offset(0x213E50) };
	return func(a_self, a_unk);
}

static TESRegionDataManager* trctor(TESRegionDataManager* a_self)
{
	using func_t = decltype(&trctor);
	REL::Relocation<func_t> func{ REL::Offset(0x210920) };
	return func(a_self);
}

DataHandlerSE* DataHandlerSE::ctor(){
	//for (auto& form : formArrays) {
		// skip
	//}
	interiorCells = NiTPrimitiveArray<TESObjectCELL*>(0x10000, 100); // May not work
	addonNodes = NiTPrimitiveArray<BGSAddonNode*>(0x10000);
	badForms = NiTList<TESForm*>();
	files = BSSimpleList<TESFile*>();
	compiledFileCollection.files = BSTArray<TESFile*>();
	compiledFileCollection.smallFiles = BSTArray<TESFile*>();
	nextID = 0x800;
	activeFile = nullptr;
	masterSave = false;
	exportingPlugin = false;
	hasDesiredFiles = true;
	dontRemoveIDs = false;
	TESRegionList* list = reinterpret_cast<TESRegionList*>(MemoryManager::GetSingleton()->Allocate(0x20, 0, 0));
	regionList = tctor(list, 1);
	merchantInventory = nullptr;
	TESRegionDataManager* manager = reinterpret_cast<TESRegionDataManager*>(MemoryManager::GetSingleton()->Allocate(0x10, 0, 0));
	regionDataManager = trctor(manager);
	void* unk = MemoryManager::GetSingleton()->Allocate(0x38, 0, 0);
	REL::Relocation<void*> unkArray{ REL::Offset(0x2FC49C8) };
	void** unkArrayPtr = reinterpret_cast<void**>(unkArray.address());
	*unkArrayPtr = vctor(unk);
	checkingModels = 0;
	REL::Relocation<void*> unk1{ REL::Offset(0x1E6EE80) };
	unkCall(unk1.get());
	return this;
};



struct DataHandlerOffsets
{
	static inline REL::Relocation<DataHandlerSE*> gDataHandler{ REL::Offset(0x1F82AD8) };
	static inline REL::Relocation<std::uintptr_t> ctor{ REL::Offset(0x1774C0) };
	static inline REL::Relocation<std::uintptr_t> tesregion__readfromfilestream{ REL::Offset(0x20E600 + 0x33A) };
};

void DataHandlerSE::InstallDataHandlerHooks(){
	auto& trampoline = SKSE::GetTrampoline();
	SKSE::AllocTrampoline(14);
	
	// ctor
	trampoline.write_branch<5>(DataHandlerOffsets::ctor.address(), &ctor);
	logger::info("Hooked {} at offset {:x}", "ctor", DataHandlerOffsets::ctor.offset());
	

	InstallPatches();
};

// Patches various VR functions to work with the SE version of DataHandler
void DataHandlerSE::InstallPatches() {
	byte tesregionPatch[] = { 0x48, 0x8B, 0x88, 0xB0, 0x0D, 0x00, 0x00 };  // mov rcx, [rax+DB0h]
	REL::safe_write(DataHandlerOffsets::tesregion__readfromfilestream.address(), tesregionPatch, 7);
	logger::info("Patched tesregion at {:x}", DataHandlerOffsets::tesregion__readfromfilestream.offset());
}



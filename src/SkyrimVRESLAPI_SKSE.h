#pragma once
#include "skse64/PluginAPI.h"
#include "skse64/GameReferences.h"
#include <skse64/GameData.h>
// Interface code based on https://github.com/adamhynek/higgs

namespace SkyrimVRESLPluginAPI
{
	constexpr const auto SkyrimVRESLPluginName = "SkyrimVRESL";
	// A message used to fetch SkyrimVRESL's interface
	struct SkyrimVRESLMessage
	{
		enum : uint32_t
		{
			kMessage_GetInterface = 0xeacb2bef
		};  // Randomly generated
		void* (*GetApiFunction)(unsigned int revisionNumber) = nullptr;
	};

	struct TESFileCollection
	{
	public:
		// members
		tArray<ModInfo*> files;       // 00
		tArray<ModInfo*> smallFiles;  // 18
	};
	STATIC_ASSERT(sizeof(TESFileCollection) == 0x30);

	// Returns an ISkyrimVRESLInterface001 object compatible with the API shown below
	// This should only be called after SKSE sends kMessage_PostLoad to your plugin
	struct ISkyrimVRESLInterface001;
	ISkyrimVRESLInterface001* GetSkyrimVRESLInterface001(const PluginHandle& pluginHandle, SKSEMessagingInterface* messagingInterface);

	// This object provides access to SkyrimVRESL's mod support API
	struct ISkyrimVRESLInterface001
	{
		// Gets the SkyrimVRESL build number
		virtual unsigned int GetBuildNumber() = 0;

		/// @brief Get the SSE compatible TESFileCollection for SkyrimVR.
		/// This should be called after kDataLoaded to ensure it's been populated.
		/// @return Pointer to TESFileCollection CompiledFileCollection.
		const virtual TESFileCollection* GetCompiledFileCollection() = 0;
	};

}  // namespace SkyrimVRESLPluginAPI
extern SkyrimVRESLPluginAPI::ISkyrimVRESLInterface001* g_SkyrimVRESLInterface;

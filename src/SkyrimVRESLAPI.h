#pragma once
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
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

	// Returns an ISkyrimVRESLInterface001 object compatible with the API shown below
	// This should only be called after SKSE sends kMessage_PostLoad to your plugin
	struct ISkyrimVRESLInterface001;
	ISkyrimVRESLInterface001* GetSkyrimVRESLInterface001();

	// This object provides access to SkyrimVRESL's mod support API
	struct ISkyrimVRESLInterface001
	{
		// Gets the SkyrimVRESL build number
		virtual unsigned int GetBuildNumber() = 0;

		/// @brief Get the SSE compatible TESFileCollection for SkyrimVR.
		/// This should be called after kDataLoaded to ensure it's been populated.
		/// @return Pointer to TESFileCollection CompiledFileCollection.
		const virtual RE::TESFileCollection* GetCompiledFileCollection() = 0;
	};

}  // namespace SkyrimVRESLPluginAPI
extern SkyrimVRESLPluginAPI::ISkyrimVRESLInterface001* g_SkyrimVRESLInterface;

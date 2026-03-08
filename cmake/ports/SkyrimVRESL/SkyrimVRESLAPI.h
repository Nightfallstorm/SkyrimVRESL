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

	// Per-plugin load timing entry recorded by VRESL's hook thunks.
	// Pointers are stable for the lifetime of the SkyrimVRESL DLL.
	struct VRESLPluginLoadTiming
	{
		const char* filename;  // plugin filename, e.g. "Skyrim.esm"
		const char* author;    // createdBy field; empty string if absent, never null
		double version;        // file->version
		uint64_t totalNs;      // sum of ConstructObjectList durations for this plugin (ns)
		uint32_t count;        // number of ConstructObjectList calls (>1 for merged ESLs)
		uint32_t _reserved;    // padding — always 0
		uint64_t openNs;       // OpenTES duration (file open+read phase) (ns)
							   // VRESL's NOP sled at CompileFiles replaces the original
							   // call site; this field restores equivalent timing data.
	};

	// Returns an ISkyrimVRESLInterface002 object compatible with the API shown below.
	// This should only be called after SKSE sends kMessage_PostLoad to your plugin.
	struct ISkyrimVRESLInterface002;
	ISkyrimVRESLInterface002* GetSkyrimVRESLInterface002();

	// ISkyrimVRESLInterface002 extends 001 with plugin load timing data.
	struct ISkyrimVRESLInterface002 : ISkyrimVRESLInterface001
	{
		/// @brief Get plugin load timings recorded during ConstructObjectList calls.
		/// Call after kDataLoaded; data is stable for the lifetime of the DLL.
		/// @param outCount Set to the number of entries in the returned array.
		/// @return Pointer to array of VRESLPluginLoadTiming, or nullptr if no data.
		virtual const VRESLPluginLoadTiming* GetPluginLoadTimings(uint32_t* outCount) const = 0;
	};

}  // namespace SkyrimVRESLPluginAPI
extern SkyrimVRESLPluginAPI::ISkyrimVRESLInterface001* g_SkyrimVRESLInterface;

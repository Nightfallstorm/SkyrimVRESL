#include "SkyrimVRESLAPI.h"
// Interface code based on https://github.com/adamhynek/higgs

// Stores the API after it has already been fetched
SkyrimVRESLPluginAPI::ISkyrimVRESLInterface001* g_SkyrimVRESLInterface = nullptr;
static SkyrimVRESLPluginAPI::ISkyrimVRESLInterface002* g_SkyrimVRESLInterface002 = nullptr;

// Fetches the interface to use from SkyrimVRESL
SkyrimVRESLPluginAPI::ISkyrimVRESLInterface001* SkyrimVRESLPluginAPI::GetSkyrimVRESLInterface001()
{
	// If the interface has already been fetched, rturn the same object
	if (g_SkyrimVRESLInterface) {
		return g_SkyrimVRESLInterface;
	}

	// Dispatch a message to get the plugin interface from SkyrimVRESL
	SkyrimVRESLMessage message;
	const auto skseMessaging = SKSE::GetMessagingInterface();
	skseMessaging->Dispatch(SkyrimVRESLMessage::kMessage_GetInterface, (void*)&message,
		sizeof(SkyrimVRESLMessage*), SkyrimVRESLPluginName);
	if (!message.GetApiFunction) {
		return nullptr;
	}

	// Fetch the API for this version of the SkyrimVRESL interface
	g_SkyrimVRESLInterface = static_cast<ISkyrimVRESLInterface001*>(message.GetApiFunction(1));
	return g_SkyrimVRESLInterface;
}

SkyrimVRESLPluginAPI::ISkyrimVRESLInterface002* SkyrimVRESLPluginAPI::GetSkyrimVRESLInterface002()
{
	if (g_SkyrimVRESLInterface002) {
		return g_SkyrimVRESLInterface002;
	}

	SkyrimVRESLMessage message;
	const auto skseMessaging = SKSE::GetMessagingInterface();
	skseMessaging->Dispatch(SkyrimVRESLMessage::kMessage_GetInterface, (void*)&message,
		sizeof(SkyrimVRESLMessage*), SkyrimVRESLPluginName);
	if (!message.GetApiFunction) {
		return nullptr;
	}

	g_SkyrimVRESLInterface002 = static_cast<ISkyrimVRESLInterface002*>(message.GetApiFunction(2));
	return g_SkyrimVRESLInterface002;
}

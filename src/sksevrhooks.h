#pragma once

// SKSEVR patching code from po3-Tweaks under MIT
// https://github.com/powerof3/po3-Tweaks/blob/850f80f298c0250565ff24ff3aba68a45ca8b73a/src/Fixes/CrosshairRefEventVR.cpp

namespace SKSEVRHooks
{
	void Install(std::uint32_t a_skse_version)
	{
		auto sksevr_base = reinterpret_cast<uintptr_t>(GetModuleHandleA("sksevr_1_4_15"));
		bool code_match = true;
		const uint8_t* read_addr = (uint8_t*)(uintptr_t)(sksevr_base + 0x283bd);
		static const uint8_t read_expected[] = { 0xe8, 0x8e, 0xfc, 0xff, 0xff };  // SaveModList call
		if (std::memcmp((const void*)read_addr, read_expected, sizeof(read_expected))) {
			logger::info("Read code is not as expected"sv);
			code_match = false;
		}
		if (a_skse_version == 33554624 && code_match) {  //2.0.12
			logger::info("Found patchable sksevr_1_4_15.dll version {} with base {:x}", a_skse_version, sksevr_base);
			// TODO: Instead of NOPing SKSEVR SaveModList cosave call, implement SKSE64 Save
			REL::safe_fill((uintptr_t)read_addr, REL::NOP, sizeof(read_expected));
			logger::info("SKSEVR patched"sv);
		} else
			logger::info("Found unknown sksevr_1_4_15.dll version {} with base {:x}; not patching", a_skse_version, sksevr_base);
	}
}

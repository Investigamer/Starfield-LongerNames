
#include "PCH.h"

/**
* STARFIELD MOD
* Increase Max Characters - Ship, Settlement, and Item Names
*
* DESCRIPTION:
* This plugin works by overwriting the opcode which
* writes to the address found at this pointer:
* - ["Starfield.exe" + 0x058F1378]
* - [Chain + 0x10]
* - [Chain + 0x78]
* - [Chain + 0x1D0]
* - [Chain + 0x130]
* - [Chain + 0xB8]
* - [Chain + 0x1A0]
* - [Chain + 0xC8] = Final Address
*
* We inject the opcode at:
* - "Starfield.exe" + 0xFFDBA3
**/

namespace ShipCharCount
{
    struct Prolog : Xbyak::CodeGenerator
    {
        Prolog()
        {
            // save rcx to the stack
            push(rcx);
            // save rax to the stack
            push(rax);
        }
    };

    struct Epilog : Xbyak::CodeGenerator
    {
        Epilog()
        {
            // write the return value from the hook function to r8d
            mov(r8d, eax);

            // restore rax from stack
            pop(rax);
            // restore rcx from stack
            pop(rcx);
        }
    };

    int Hook_GetMaxCharCount()
    {
        return Settings::GetSingleton()->GetShipNameMaxChars();
    }

    void Install()
    {
        auto patchAddress = reinterpret_cast<uintptr_t>(search_pattern<"48 8B 88 ?? ?? ?? ?? 44 89 81 ?? ?? ?? ?? C3">());
        if (patchAddress) {
            patchAddress += 7;
            INFO("Found the patch address: {:x}", patchAddress);

            Prolog prolog{};
            prolog.ready();
            Epilog epilog{};
            epilog.ready();

            // Create hook
            const auto caveHookHandle = AddCaveHook(
                patchAddress,
                { 0, 7 },
                FUNC_INFO(Hook_GetMaxCharCount),
                &prolog,
                &epilog,
                HookFlag::kRestoreAfterEpilog);
            caveHookHandle->Enable();
            INFO("Max character count patched successfully!")
        }
        else {
            ERROR("Couldn't find the patch address");
        }
    }
}

DLLEXPORT constinit auto SFSEPlugin_Version = []() noexcept {
	SFSE::PluginVersionData data{};

	data.PluginVersion(Plugin::Version);
	data.PluginName(Plugin::NAME);
	data.AuthorName(Plugin::AUTHOR);
	data.UsesSigScanning(true);
	//data.UsesAddressLibrary(true);
	data.HasNoStructUse(true);
	//data.IsLayoutDependent(true);
	data.CompatibleVersions({ 
        SFSE::RUNTIME_SF_1_7_23,
		SFSE::RUNTIME_SF_1_7_29,
		SFSE::RUNTIME_LATEST
	});

	return data;
}();

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type) {
		case SFSE::MessagingInterface::kPostLoad:
			{
                // Load the settings file
                Settings::GetSingleton()->Load();
                // Install plugin
                ShipCharCount::Install();
				break;
			}
		default:
			break;
		}
	}
}

/**
* For preload plugins:
void SFSEPlugin_Preload(SFSE::LoadInterface* a_sfse);
**/

DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
#ifndef NDEBUG
    MessageBoxW(NULL, L"Loaded. You can attach the debugger now, then press OK", L"Starfield-LongerNames SFSE Plugin", NULL);
	/*while (!IsDebuggerPresent()) {
		Sleep(100);
	}*/
#endif

	SFSE::Init(a_sfse, false);

	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));

	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	// Insert plugin to messaging interface
	SFSE::AllocTrampoline(1 << 8);
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	return true;
}

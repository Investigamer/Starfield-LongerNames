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

std::string RemoveFileNameFromPath(const std::string& path)
{
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        return path.substr(0, lastSlash);
    }
    return path;
}

// Load INI config values
BYTE getConfigVal()
{
    // Get module path
    char pBuf[MAX_PATH];
    DWORD modulePath = GetModuleFileNameA((HMODULE)nullptr, (LPSTR)pBuf, (DWORD)MAX_PATH);
    if (modulePath == 0) {
        INFO("Couldn't locate INI file! Reverting to default value.");
        return 25;
    }

    // Get INI file path
    std::string strModPath(pBuf, pBuf + modulePath);
    strModPath = RemoveFileNameFromPath(strModPath);
    std::string iniPath = strModPath + R"(\Data\SFSE\Plugins\Starfield-LongerNames.ini)";


    // INI config file
    CSimpleIniA iniFile;
    iniFile.SetUnicode();
    SI_Error rc = iniFile.LoadFile(iniPath.c_str());
    if (rc < 0) {
        INFO("Error reading INI file! Reverting to default value.");
        return (BYTE)25;
    }
    int maxCharCount = (int)iniFile.GetDoubleValue("Main", "ShipNameMaxChars", 25);
    INFO("ShipNameMaxChars(" + std::to_string(maxCharCount) + ") loaded from INI file!");
    if (maxCharCount < 0 || maxCharCount > 255) {
        return (BYTE)25;
    }
    return (BYTE)maxCharCount;
}

namespace ShipCharCount
{
    void Install()
    {
        // Address, process ID, and handle
        using namespace dku::Alias;
        uintptr_t BASE_ADDRESS = DKUtil::Hook::Module::get().base();
        std::array<std::uint8_t, 11> RawPatch{
            0xC7, 0x81, 0xC8, 0x00, 0x00, 0x00, (BYTE) getConfigVal(), 0x00, 0x00, 0x00, 0xC3 };

        // Create hook
        auto charCountHook = dku::Hook::AddASMPatch(
                BASE_ADDRESS + 0xFFDBA3, // Address to patch
                std::make_pair(0x0, 0x7), // Offset range
                { RawPatch.data(), RawPatch.size() }); // Raw patch
        charCountHook->Enable();
        INFO("Max character count patched successfully!")
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
    data.CompatibleVersions({ SFSE::RUNTIME_LATEST });

    return data;
}();

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type) {
		case SFSE::MessagingInterface::kPostLoad:
			{
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
/**#ifndef NDEBUG
	// Currently causes infinite wait loop even when not debugging?
    while (!IsDebuggerPresent()) {
		Sleep(100);
	}
#endif**/

	SFSE::Init(a_sfse);

	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));

	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	// Insert plugin to messaging interface
    SFSE::AllocTrampoline(1 << 8);
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	return true;
}

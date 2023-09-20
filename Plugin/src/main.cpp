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

template <std::size_t N, typename T = std::uint8_t>
std::unique_ptr<DKUtil::Hook::ASMPatchHandle> createHook(
	uintptr_t        targetOffset,
	int              targetLength,
	std::array<T, N> opcode)
{
	/**
     * Creates an ASM patch object.
     * @param targetOffset: Offset to add to the base address to find the target location to patch.
     * @param targetLength: Length of the opcode we want to overwrite with this patch, e.g. 0x8 or 8.
     * @param opcode: New opcode to write with this patch.
     * @returns: ASMPatchHandle that can be enabled.
     */
	uintptr_t BASE_ADDRESS = DKUtil::Hook::Module::get().base();
	return DKUtil::Hook::AddASMPatch(
		BASE_ADDRESS + targetOffset,
		std::make_pair(0x0, targetLength),
		{ opcode.data(), opcode.size() });
}

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
        // Charcount patch
        using namespace DKUtil::Alias;
        uintptr_t BASE_ADDRESS = DKUtil::Hook::Module::get().base();
        std::array<std::uint8_t, 11> opcode{
            0xC7, 0x81, 0xC8, 0x00, 0x00, 0x00, 
            (BYTE)getConfigVal(), 0x00, 0x00, 0x00, 
            0xC3 };
        auto charCountHook = createHook(0xFFDBA3, 0x7, opcode);
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
	data.CompatibleVersions({ 
        SFSE::RUNTIME_SF_1_7_23,
		SFSE::RUNTIME_SF_1_7_29,
		SFSE::RUNTIME_LATEST });

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
#ifndef NDEBUG
	while (!IsDebuggerPresent()) {
		Sleep(100);
	}
#endif

	SFSE::Init(a_sfse, false);

	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));

	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	// Insert plugin to messaging interface
	SFSE::AllocTrampoline(1 << 8);
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	return true;
}

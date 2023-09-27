#pragma once

#include "DKUtil/Config.hpp"

using namespace DKUtil::Alias;

class Settings : public dku::model::Singleton<Settings>
{
private:
    IniConfig mainConfig = COMPILE_PROXY(Plugin::SETTINGS_NAME);

    Integer shipNameMaxChars{ "ShipNameMaxChars", "Main"};
public:
    uint8_t GetShipNameMaxChars()
    {
        return static_cast<uint8_t>(*shipNameMaxChars);
    }

    void Load() noexcept
    {
        static std::once_flag bound;
        std::call_once(bound, [&]() { mainConfig.Bind<10, 255>(shipNameMaxChars, 25); });

        mainConfig.Load();
    }
};

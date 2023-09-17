#pragma once

// Base
#include <bitset>
#include <exception>
#include <source_location>
#include <string>


// SFSE
#include "SFSE/SFSE.h"

// Ini files
#include "SimpleIni.h"

// winnt
#include <ShlObj_core.h>

#undef min
#undef max

using namespace std;

#define DLLEXPORT extern "C" [[maybe_unused]] __declspec(dllexport)

// Plugin
#include "Plugin.h"

// DKUtil
#include "DKUtil/Hook.hpp"
#include "games.h"
#include "sim_core.h"
#include "sim_sdram.h"
#include "sim_ddr.h"
#include "mra_loader.h"

#include "F2.h"
#include "F2___024root.h"

#include "file_search.h"
#include <string.h>
#include <cstdio>

static const char *game_names[N_GAMES] = {
    "cadash", "cadash_test"
};

game_t game_find(const char *name)
{
    for (int i = 0; i < N_GAMES; i++)
    {
        if (!strcasecmp(name, game_names[i]))
        {
            return (game_t)i;
        }
    }

    return GAME_INVALID;
}

const char *game_name(game_t game)
{
    if (game == GAME_INVALID)
        return "INVALID";
    return game_names[game];
}

static bool load_audio(const char *name)
{
    std::vector<uint8_t> data;
    if (!g_fs.LoadFile(name, data))
    {
        printf("Could not open audio rom %s\n", name);
        return false;
    }

    memcpy(gSimCore.mTop->rootp->sim_top__DOT__f2_inst__DOT__sound_rom__DOT__ram.m_storage, data.data(), data.size());

    return true;
}

static void load_cadash()
{
    g_fs.addSearchPath("../roms/finalb.zip");

    load_audio("b82_10.ic5");

    gSimCore.mSDRAM->load_data("b82-09.ic23", CPU_ROM_SDR_BASE + 1, 2);
    gSimCore.mSDRAM->load_data("b82-17.ic11", CPU_ROM_SDR_BASE + 0, 2);

    gSimCore.mSDRAM->load_data("b82-07.ic34", SCN0_ROM_SDR_BASE + 1, 2);
    gSimCore.mSDRAM->load_data("b82-06.ic33", SCN0_ROM_SDR_BASE + 0, 2);

    gSimCore.mSDRAM->load_data("b82-02.ic1", ADPCMA_ROM_SDR_BASE, 1);
    gSimCore.mSDRAM->load_data("b82-01.ic2", ADPCMB_ROM_SDR_BASE, 1);

    gSimCore.mDDRMemory->load_data("b82-03.ic9", OBJ_DATA_DDR_BASE + 1, 4);
    gSimCore.mDDRMemory->load_data("b82-04.ic8", OBJ_DATA_DDR_BASE + 0, 4);
    gSimCore.mDDRMemory->load_data("b82-05.ic7", OBJ_DATA_DDR_BASE + 2, 4);
    gSimCore.mDDRMemory->load_data("b82-05.ic7", OBJ_DATA_DDR_BASE + 3, 4);

    gSimCore.SetGame(GAME_CADASH);
}

static void load_cadash_test()
{
    g_fs.addSearchPath("../testroms/build/finalb_test/finalb/");
    load_cadash();
}

bool game_init(game_t game)
{
    g_fs.clearSearchPaths();
    g_fs.addSearchPath(".");

    switch (game)
    {
    case GAME_CADASH:
        load_cadash();
        break;
        break;
    case GAME_CADASH_TEST:
        load_cadash_test();
        break;
    default:
        return false;
    }

    return true;
}

bool game_init_mra(const char *mra_path)
{
    g_fs.clearSearchPaths();

    // Add common ROM search paths
    std::vector<std::string> searchPaths = {".", "../roms/"};

    // Add ROM search paths
    for (const auto &path : searchPaths)
    {
        g_fs.addSearchPath(path);
    }

    // Add the directory containing the MRA file as a search path
    std::string mraPathStr(mra_path);
    size_t lastSlash = mraPathStr.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        g_fs.addSearchPath(mraPathStr.substr(0, lastSlash));
    }

    // Load the MRA file
    MRALoader loader;
    std::vector<uint8_t> romData;
    uint32_t address = 0;

    if (!loader.load(mra_path, romData, address))
    {
        printf("Failed to load MRA file '%s': %s\n", mra_path, loader.getLastError().c_str());
        return false;
    }

    printf("Loaded MRA: %s\n", mra_path);
    printf("ROM data size: %zu bytes\n", romData.size());

    if (address == 0)
    {
        if (!gSimCore.SendIOCTLData(0, romData))
        {
            printf("Failed to send ROM data via ioctl\n");
            return false;
        }
    }
    else
    {
        if (!gSimCore.SendIOCTLDataDDR(0, address, romData))
        {
            printf("Failed to send ROM data via DDR\n");
            return false;
        }
    }

    printf("Successfully loaded MRA: %s\n", mra_path);
    return true;
}

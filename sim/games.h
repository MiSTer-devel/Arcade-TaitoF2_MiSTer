#ifndef GAMES_H
#define GAMES_H 1

#include <stdint.h>

enum game_t : uint8_t
{
    GAME_CADASH = 0,

    GAME_CADASH_TEST,

    N_GAMES,

    GAME_INVALID = 0xff
};

static const uint32_t CPU_ROM_SDR_BASE = 0x00000000;
static const uint32_t SCN0_ROM_SDR_BASE = 0x00900000;
static const uint32_t SCN1_ROM_SDR_BASE = 0x01200000;
static const uint32_t ADPCMA_ROM_SDR_BASE = 0x00b00000;
static const uint32_t ADPCMB_ROM_SDR_BASE = 0x00d00000;
static const uint32_t PIVOT_ROM_SDR_BASE = 0x01000000;
static const uint32_t OBJ_DATA_DDR_BASE = 0x38100000;

game_t game_find(const char *name);
const char *game_name(game_t game);

bool game_init(game_t game);
bool game_init_mra(const char *mra_path);

#endif // GAMES_H

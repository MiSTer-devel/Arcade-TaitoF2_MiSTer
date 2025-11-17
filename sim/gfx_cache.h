#if !defined(GFX_CACHE_H)
#define GFX_CACHE_H 1

#include <SDL.h>

#include "sim_core.h"
#include "sim_hierarchy.h"

#include "F2.h"
#include "F2___024root.h"

struct GfxCacheEntry
{
    SDL_Texture *mTexture;
    uint64_t mLastUsed;

    GfxCacheEntry() : mTexture(nullptr), mLastUsed(0)
    {
    }

    GfxCacheEntry(SDL_Texture *tex, uint64_t used) : mTexture(tex), mLastUsed(used)
    {
    }

    GfxCacheEntry(GfxCacheEntry &&o)
    {
        mTexture = o.mTexture;
        mLastUsed = o.mLastUsed;
        o.mTexture = nullptr;
    }

    ~GfxCacheEntry()
    {
        if (mTexture)
            SDL_DestroyTexture(mTexture);
    }

    GfxCacheEntry(const GfxCacheEntry &) = delete;
};

struct GfxPalette
{
    uint32_t mRGB[16];
    uint64_t mHash;
    uint64_t mCount;
};

#define FNV_64_PRIME ((uint64_t)0x100000001b3ULL)

enum class GfxCacheFormat
{
    TC0200OBJ,
    TC0480SCP,
    TC0100SCN,
    TC0100SCN_FG
};

class GfxCache
{
  public:
    std::map<uint64_t, GfxCacheEntry> mCache;
    GfxPalette mPalettes[512];

    MemoryInterface *mColormem;
    SDL_Renderer *mRenderer = nullptr;
    uint64_t mUsedIdx = 0;

    static uint64_t CalcHash(const void *buf, size_t len, uint64_t hval)
    {
        const unsigned char *bp = (const unsigned char *)buf;
        const unsigned char *be = bp + len;

        while (bp < be)
        {
            hval *= FNV_64_PRIME;
            hval ^= (uint64_t)*bp++;
        }

        return hval;
    }

    const GfxPalette *GetPalette(uint16_t index)
    {
        index %= 512;

        GfxPalette *entry = &mPalettes[index];
        entry->mCount++;

        if (entry->mHash != 0 && (entry->mCount & 0xff) != 0)
        {
            return entry;
        }

        uint16_t addr = index * 32;
        uint8_t rawpal[32];
        mColormem->Read(addr, 32, rawpal);

        uint64_t hash = CalcHash(rawpal, sizeof(rawpal), 0);

        if (hash == entry->mHash)
        {
            return entry;
        }

        entry->mHash = hash;

        bool dar260 = false; //G_F2_SIGNAL(cfg_260dar);
        bool bpp15 = false; // G_F2_SIGNAL(cfg_bpp15);
        bool bppmix = false; //G_F2_SIGNAL(cfg_bppmix);

        for (int i = 0; i < 16; i++)
        {
            uint8_t r, g, b;
            uint16_t p = (rawpal[i * 2 + 1]) | (rawpal[i * 2 + 0] << 8);

            if (dar260)
            {
                if (bpp15 && bppmix)
                {
                    r = ((p & 0xf000) >> 8) | ((p & 0x0008) >> 0);
                    g = ((p & 0x0f00) >> 4) | ((p & 0x0004) << 1);
                    b = ((p & 0x00f0) >> 0) | ((p & 0x0002) << 2);
                }
                else if (bpp15)
                {
                    r = ((p & 0x7c00) >> 7);
                    g = ((p & 0x03e0) >> 2);
                    b = ((p & 0x001f) << 3);
                }
                else
                {
                    r = ((p & 0xf000) >> 8);
                    g = ((p & 0x0f00) >> 4);
                    b = ((p & 0x00f0) << 0);
                }
            }
            else
            {
                b = ((p & 0xfc00) >> 7);
                g = ((p & 0x03e0) >> 2);
                r = ((p & 0x001f) << 3);
            }

            uint32_t c = (r << 24) | (g << 16) | (b << 8);
            if (i & 15)
                entry->mRGB[i] = c | 0xff;
            else
                entry->mRGB[i] = 0xff0000ff;
        }
        return entry;
    }

    void Init(SDL_Renderer *renderer, MemoryInterface &colormem)
    {
        mCache.clear();
        mUsedIdx = 0;
        mRenderer = renderer;
        mColormem = &colormem;
    }

    void PruneCache()
    {
        if (mCache.size() < 2048)
            return;

        size_t numToRemove = 128;

        std::vector<std::pair<uint64_t, uint64_t>> hashAges;
        for (const auto &it : mCache)
        {
            hashAges.push_back({it.second.mLastUsed, it.first});
        }
        std::sort(hashAges.begin(), hashAges.end());
        for (size_t i = 0; i < numToRemove; i++)
        {
            mCache.erase(hashAges[i].second);
        }
    }

    SDL_Texture *GetTexture(MemoryRegion region, GfxCacheFormat format, uint16_t code, uint8_t paletteIdx)
    {
        return GetTexture(gSimCore.Memory(region), format, code, paletteIdx);
    }

    SDL_Texture *GetTexture(MemoryInterface &gfxmem, GfxCacheFormat format, uint16_t code, uint8_t paletteIdx)
    {
        const GfxPalette *palette = GetPalette(paletteIdx);

        int size;
        int bytesize;
        bool dynamic;

        const bool bpp6 = G_F2_SIGNAL(tc0200obj, ctrl_6bpp);

        switch (format)
        {
        case GfxCacheFormat::TC0200OBJ:
        {
            size = 16;
            bytesize = bpp6 ? 16 * 16 : 16 * 8;
            dynamic = false;
            break;
        }

        case GfxCacheFormat::TC0100SCN:
        {
            size = 8;
            bytesize = 8 * 4;
            dynamic = false;
            break;
        }

        case GfxCacheFormat::TC0100SCN_FG:
        {
            size = 8;
            bytesize = 8 * 2;
            dynamic = true;
            break;
        }

        case GfxCacheFormat::TC0480SCP:
        {
            size = 16;
            bytesize = 16 * 8;
            dynamic = false;
            break;
        }
        }

        uint32_t addr = (code * bytesize);
        uint8_t srcData[16 * 16];

        uint64_t hash;

        if (dynamic)
        {
            gfxmem.Read(addr, bytesize, srcData);
            hash = CalcHash(srcData, bytesize, palette->mHash);
        }
        else
        {
            hash = CalcHash(&code, sizeof(code), palette->mHash);
        }

        auto it = mCache.find(hash);
        if (it != mCache.end())
        {
            it->second.mLastUsed = mUsedIdx;
            mUsedIdx++;
            return it->second.mTexture;
        }

        if (!dynamic)
        {
            gfxmem.Read(addr, bytesize, srcData);
        }

        uint32_t pixels[16 * 16];
        const uint32_t *pal32 = palette->mRGB;

        uint32_t *dest = pixels;
        const uint8_t *src = srcData;

        if (format == GfxCacheFormat::TC0200OBJ)
        {
            size = 16;
            if (bpp6)
            {
                for (int i = 0; i < 64; i++)
                {
                    dest[1] = pal32[((src[1] & 0xf0) >> 4)];
                    dest[0] = pal32[((src[1] & 0x0f) >> 0)];
                    dest[3] = pal32[((src[0] & 0xf0) >> 4)];
                    dest[2] = pal32[((src[0] & 0x0f) >> 0)];
                    dest += 4;
                    src += 4;
                }
            }
            else
            {
                for (int i = 0; i < 128; i++)
                {
                    dest[1] = pal32[((*src & 0xf0) >> 4)];
                    dest[0] = pal32[((*src & 0x0f) >> 0)];
                    dest += 2;
                    src++;
                }
            }
        }
        else if (format == GfxCacheFormat::TC0480SCP)
        {
            for (int i = 0; i < 16; i++)
            {
                dest[15] = pal32[((src[4] & 0xf0) >> 4)];
                dest[14] = pal32[((src[4] & 0x0f) >> 0)];
                dest[13] = pal32[((src[5] & 0xf0) >> 4)];
                dest[12] = pal32[((src[5] & 0x0f) >> 0)];
                dest[11] = pal32[((src[6] & 0xf0) >> 4)];
                dest[10] = pal32[((src[6] & 0x0f) >> 0)];
                dest[9] = pal32[((src[7] & 0xf0) >> 4)];
                dest[8] = pal32[((src[7] & 0x0f) >> 0)];
                dest[7] = pal32[((src[0] & 0xf0) >> 4)];
                dest[6] = pal32[((src[0] & 0x0f) >> 0)];
                dest[5] = pal32[((src[1] & 0xf0) >> 4)];
                dest[4] = pal32[((src[1] & 0x0f) >> 0)];
                dest[3] = pal32[((src[2] & 0xf0) >> 4)];
                dest[2] = pal32[((src[2] & 0x0f) >> 0)];
                dest[1] = pal32[((src[3] & 0xf0) >> 4)];
                dest[0] = pal32[((src[3] & 0x0f) >> 0)];
                dest += 16;
                src += 8;
            }
        }
        else if (format == GfxCacheFormat::TC0100SCN)
        {
            for (int i = 0; i < 8; i++)
            {
                dest[2] = pal32[((src[0] & 0xf0) >> 4)];
                dest[3] = pal32[((src[0] & 0x0f) >> 0)];
                dest[0] = pal32[((src[1] & 0xf0) >> 4)];
                dest[1] = pal32[((src[1] & 0x0f) >> 0)];
                dest[6] = pal32[((src[2] & 0xf0) >> 4)];
                dest[7] = pal32[((src[2] & 0x0f) >> 0)];
                dest[4] = pal32[((src[3] & 0xf0) >> 4)];
                dest[5] = pal32[((src[3] & 0x0f) >> 0)];
                dest += 8;
                src += 4;
            }
        }
        else if (format == GfxCacheFormat::TC0100SCN_FG)
        {
            for (int i = 0; i < 8; i++)
            {
                dest[7] = pal32[((src[1] >> 0) & 0x1) | ((src[0] << 1) & 0x2)];
                dest[6] = pal32[((src[1] >> 1) & 0x1) | ((src[0] << 0) & 0x2)];
                dest[5] = pal32[((src[1] >> 2) & 0x1) | ((src[0] >> 1) & 0x2)];
                dest[4] = pal32[((src[1] >> 3) & 0x1) | ((src[0] >> 2) & 0x2)];
                dest[3] = pal32[((src[1] >> 4) & 0x1) | ((src[0] >> 3) & 0x2)];
                dest[2] = pal32[((src[1] >> 5) & 0x1) | ((src[0] >> 4) & 0x2)];
                dest[1] = pal32[((src[1] >> 6) & 0x1) | ((src[0] >> 5) & 0x2)];
                dest[0] = pal32[((src[1] >> 7) & 0x1) | ((src[0] >> 6) & 0x2)];
                dest += 8;
                src += 2;
            }
        }

        SDL_Texture *tex = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, size, size);

        SDL_UpdateTexture(tex, nullptr, pixels, size * 4);

        auto r = mCache.emplace(hash, GfxCacheEntry(tex, mUsedIdx));
        return r.first->second.mTexture;
    }
};

#endif

#include <mod/amlmod.h>
#include <mod/logger.h>
#include <sautils.h>
#include <stdint.h>
#include <vector>

struct AdditionalSetting
{
    int nSettingId;
    eTypeOfSettings eType;
    const char* szName;
    OnSettingChangedFn fnOnValueChange;
    OnSettingDrawedFn fnOnValueDraw;
    bool bIsSlider;
    int nInitVal;
    int nSavedVal;
    int nMaxVal;
};

/* Saves */
std::vector<AdditionalSetting*> gMoreSettings;
int nNextSettingNum = MODS_SETTINGS_STARTING_FROM - 1;
int pNewSettings[8 * MAX_SETTINGS]; // A new char MobileSettings::settings[37*8*4]

/* Funcs */
typedef void* (*TextureDBGetTextureFn)(uintptr_t a1, uintptr_t a2);
typedef void* (*SettingsAddItemFn)(void* a1, uintptr_t a2);

/* GTASA Pointers */
extern uintptr_t pGameLib;
unsigned short* gxtErrorString;
//unsigned char* aScreens;
SettingsAddItemFn fnSettingsAddItem;
/* GTASA Pointers */

bool bIsThisModdedSlider = false;
int nSettingId = 0;
DECL_HOOK(void, SelectScreenRender, uintptr_t self, float a1, float a2, float a3, float a4, float a5, float a6)
{
    nSettingId = *(int*)(self + 8);
    if(nSettingId >= MODS_SETTINGS_STARTING_FROM && nSettingId < MAX_SETTINGS && pNewSettings[8 * nSettingId + 7] == 1)
    {
        bIsThisModdedSlider = true;
    }
    SelectScreenRender(self, a1, a2, a3, a4, a5, a6);
    bIsThisModdedSlider = false;
}

DECL_HOOK(unsigned short*, AsciiToGxtChar, const char* txt, unsigned short* ret)
{
    if(bIsThisModdedSlider)
    {
        auto vStart = gMoreSettings.begin();
        auto vEnd = gMoreSettings.end();
        while(vStart != vEnd)
        {
            if((*vStart)->nSettingId == nSettingId)
            {
                if((*vStart)->fnOnValueDraw != nullptr) return AsciiToGxtChar((*vStart)->fnOnValueDraw(pNewSettings[8 * nSettingId + 2]), ret);
                break;
            }
            ++vStart;
        }
    }
    return AsciiToGxtChar(txt, ret);
}

DECL_HOOK(unsigned short*, GxtTextGet, void* self, const char* txt)
{
    static unsigned short gxtTxt[0x7F];
    unsigned short* ret = GxtTextGet(self, txt);
    if(ret == gxtErrorString)
    {
        AsciiToGxtChar(txt, gxtTxt);
        return gxtTxt;
    }
    return ret;
}

bool bPassRTDBtn = false;
uintptr_t pLatestRTDPointer = 0;
eTypeOfSettings nLatestSettingsOpened;
DECL_HOOK(void, SelectScreenAddItem, void* self, uintptr_t item)
{
    if(bPassRTDBtn && !strcmp(*(char**)(item + 4), "MOB_RTD"))
    {
        pLatestRTDPointer = item;
        return;
    }
    SelectScreenAddItem(self, item);
}
void AddSettingsToScreen(void* screen)
{
    auto vStart = gMoreSettings.begin();
    auto vEnd = gMoreSettings.end();
    while(vStart != vEnd)
    {
        if((*vStart)->eType == nLatestSettingsOpened)
        {
            uintptr_t menuItem = (uintptr_t)(new char[0x1Cu]);
            *(uintptr_t*)menuItem = pGameLib + 0x662848;
            *(const char**)(menuItem + 4) = (*vStart)->szName;
            *(int*)(menuItem + 8) = (*vStart)->nSettingId;
            *(int*)(menuItem + 12) = 0;
            *(int*)(menuItem + 16) = 0;
            SelectScreenAddItem(screen, menuItem);
        }
        ++vStart;
    }
    // Bring back "Reset To Defaults" button
    if(pLatestRTDPointer != 0) SelectScreenAddItem(screen, pLatestRTDPointer);
}
void SettingsScreenClosed()
{
    auto vStart = gMoreSettings.begin();
    auto vEnd = gMoreSettings.end();
    while(vStart != vEnd)
    {
        if((*vStart)->eType == nLatestSettingsOpened)
        {
            int nNewVal = sautils->ValueOfSettingsItem((*vStart)->nSettingId);
            if(nNewVal != (*vStart)->nSavedVal)
            {
                if((*vStart)->fnOnValueChange != nullptr) (*vStart)->fnOnValueChange((*vStart)->nSavedVal, nNewVal);
                (*vStart)->nSavedVal = nNewVal;
            }
        }
        ++vStart;
    }
}

DECL_HOOK(void*, NewScreen_Controls, void* self)
{
    pLatestRTDPointer = 0;
    bPassRTDBtn = true;
    void* ret = NewScreen_Controls(self);
    bPassRTDBtn = false;
    nLatestSettingsOpened = Controller;
    AddSettingsToScreen(self);
    return ret;
}

DECL_HOOK(void*, NewScreen_Game, void* self)
{
    pLatestRTDPointer = 0;
    bPassRTDBtn = true;
    void* ret = NewScreen_Game(self);
    bPassRTDBtn = false;
    nLatestSettingsOpened = Game;
    AddSettingsToScreen(self);
    return ret;
}

DECL_HOOK(void*, NewScreen_Display, void* self)
{
    pLatestRTDPointer = 0;
    bPassRTDBtn = true;
    void* ret = NewScreen_Display(self);
    bPassRTDBtn = false;
    nLatestSettingsOpened = Display;
    AddSettingsToScreen(self);
    return ret;
}

DECL_HOOK(void*, NewScreen_Audio, void* self)
{
    pLatestRTDPointer = 0;
    bPassRTDBtn = true;
    void* ret = NewScreen_Audio(self);
    bPassRTDBtn = false;
    nLatestSettingsOpened = Audio;
    AddSettingsToScreen(self);
    return ret;
}

DECL_HOOK(void*, NewScreen_Language, void* self)
{
    pLatestRTDPointer = 0;
    bPassRTDBtn = true;
    void* ret = NewScreen_Language(self);
    bPassRTDBtn = false;
    nLatestSettingsOpened = Language;
    AddSettingsToScreen(self);
    return ret;
}

DECL_HOOK(void, SelectScreenOnDestroy, void* self)
{
    SettingsScreenClosed();
    SelectScreenOnDestroy(self);
}

void SAUtils::InitializeSAUtils()
{
    gxtErrorString = (unsigned short*)(pGameLib + 0xA01A90);
    //aScreens = (unsigned char*)(pGameLib + 0x6AB480);

    aml->Unprot(pGameLib + 0x679A40, sizeof(void*));
    *(uintptr_t*)(pGameLib + 0x679A40) = (uintptr_t)pNewSettings;
    memcpy(pNewSettings, (int*)(pGameLib + 0x6E03F4), 1184);

    HOOKPLT(SelectScreenRender, pGameLib + 0x662850);
    HOOKPLT(AsciiToGxtChar, pGameLib + 0x6724F8);
    HOOKPLT(GxtTextGet, pGameLib + 0x66E78C);
    HOOKPLT(NewScreen_Controls, pGameLib + 0x675CD8);
    HOOKPLT(NewScreen_Game, pGameLib + 0x674310);
    HOOKPLT(NewScreen_Display, pGameLib + 0x675150);
    HOOKPLT(NewScreen_Audio, pGameLib + 0x66FBA4);
    HOOKPLT(NewScreen_Language, pGameLib + 0x675D90);
    HOOKPLT(SelectScreenAddItem, pGameLib + 0x674518);
    HOOKPLT(SelectScreenOnDestroy, pGameLib + 0x673FD8);

    fnSettingsAddItem = (SettingsAddItemFn)(pGameLib + 0x19C840);
}
void SAUtils::InitializeVCUtils()
{
    gxtErrorString = (unsigned short*)(pGameLib + 0x716C2C);
    //aScreens = (unsigned char*)(pGameLib + 0x6AB480);

    aml->Unprot(pGameLib + 0x679A40, sizeof(void*));
    *(uintptr_t*)(pGameLib + 0x679A40) = (uintptr_t)pNewSettings;
    memcpy(pNewSettings, (int*)(pGameLib + 0x6E03F4), 1184);

    HOOKPLT(GxtTextGet, pGameLib + 0x66E78C);
    HOOKPLT(NewScreen_Controls, pGameLib + 0x675CD8);
    HOOKPLT(NewScreen_Game, pGameLib + 0x674310);
    HOOKPLT(NewScreen_Display, pGameLib + 0x675150);
    HOOKPLT(NewScreen_Audio, pGameLib + 0x66FBA4);
    HOOKPLT(NewScreen_Language, pGameLib + 0x675D90);
    HOOKPLT(SelectScreenAddItem, pGameLib + 0x674518);
    HOOKPLT(SelectScreenOnDestroy, pGameLib + 0x673FD8);

    fnSettingsAddItem = (SettingsAddItemFn)(pGameLib + 0x19C840);
}



/* Interface */
/* Interface */
/* Interface */

uintptr_t SAUtils::IsFLALoaded()
{
    return m_pHasFLA;
}

int SAUtils::AddSettingsItem(eTypeOfSettings typeOf, const char* name, int initVal, int minVal, int maxVal, OnSettingChangedFn fnOnValueChange, bool isSlider, void* switchesArray)
{
    if(nNextSettingNum >= MAX_SETTINGS) return -1;

    ++nNextSettingNum;
    AdditionalSetting* pNew = new AdditionalSetting;
    pNew->nSettingId = nNextSettingNum;
    pNew->eType = typeOf;
    pNew->szName = name;
    pNew->fnOnValueChange = fnOnValueChange;
    pNew->bIsSlider = isSlider;
    pNew->nInitVal = (int)initVal;
    pNew->nSavedVal = (int)initVal;
    pNew->nMaxVal = maxVal;
    gMoreSettings.push_back(pNew);

    pNewSettings[8 * nNextSettingNum + 1] = (int)switchesArray; // Items of that setting
    pNewSettings[8 * nNextSettingNum + 2] = initVal; // Current value
    pNewSettings[8 * nNextSettingNum + 4] = minVal; // Min slider value (min is -2millions) OR min count of items (keep it 0 maybe, if u dont need others)
    pNewSettings[8 * nNextSettingNum + 5] = maxVal; // Max slider value (max is 2millions) OR max count-1 of items
    pNewSettings[8 * nNextSettingNum + 7] = isSlider?1:0; // Declare it as a slider (flags???)

    return nNextSettingNum;
}

int SAUtils::ValueOfSettingsItem(int settingId)
{
    if(settingId < MODS_SETTINGS_STARTING_FROM || settingId > nNextSettingNum) return 0;
    return pNewSettings[8 * settingId + 2];
}

// 1.1

int SAUtils::AddClickableItem(eTypeOfSettings typeOf, const char* name, int initVal, int minVal, int maxVal, const char** switchesArray, OnSettingChangedFn fnOnValueChange)
{
    if(nNextSettingNum >= MAX_SETTINGS) return -1;

    ++nNextSettingNum;
    AdditionalSetting* pNew = new AdditionalSetting;
    pNew->nSettingId = nNextSettingNum;
    pNew->eType = typeOf;
    pNew->szName = name;
    pNew->fnOnValueChange = fnOnValueChange;
    pNew->fnOnValueDraw = nullptr;
    pNew->bIsSlider = false;
    pNew->nInitVal = (int)initVal;
    pNew->nSavedVal = (int)initVal;
    pNew->nMaxVal = maxVal;
    gMoreSettings.push_back(pNew);

    pNewSettings[8 * nNextSettingNum + 1] = (int)switchesArray;
    pNewSettings[8 * nNextSettingNum + 2] = initVal;
    pNewSettings[8 * nNextSettingNum + 4] = minVal;
    pNewSettings[8 * nNextSettingNum + 5] = maxVal;
    pNewSettings[8 * nNextSettingNum + 7] = 0;

    return nNextSettingNum;
}
int SAUtils::AddSliderItem(eTypeOfSettings typeOf, const char* name, int initVal, int minVal, int maxVal, OnSettingChangedFn fnOnValueChange, OnSettingDrawedFn fnOnValueDraw)
{
    if(nNextSettingNum >= MAX_SETTINGS) return -1;

    ++nNextSettingNum;
    AdditionalSetting* pNew = new AdditionalSetting;
    pNew->nSettingId = nNextSettingNum;
    pNew->eType = typeOf;
    pNew->szName = name;
    pNew->fnOnValueChange = fnOnValueChange;
    pNew->fnOnValueDraw = fnOnValueDraw;
    pNew->bIsSlider = true;
    pNew->nInitVal = (int)initVal;
    pNew->nSavedVal = (int)initVal;
    pNew->nMaxVal = maxVal;
    gMoreSettings.push_back(pNew);

    pNewSettings[8 * nNextSettingNum + 1] = (int)nullptr;
    pNewSettings[8 * nNextSettingNum + 2] = initVal;
    pNewSettings[8 * nNextSettingNum + 4] = minVal;
    pNewSettings[8 * nNextSettingNum + 5] = maxVal;
    pNewSettings[8 * nNextSettingNum + 7] = 1;

    return nNextSettingNum;
}

static SAUtils sautilsLocal;
ISAUtils* sautils = &sautilsLocal;
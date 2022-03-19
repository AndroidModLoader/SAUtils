#include <mod/amlmod.h>
#include <mod/logger.h>
#include <dlfcn.h>
#include <sautils.h>
#include <stdint.h>
#include <vector>
#include <cstring> // memcpy

#include "GTASA_STRUCTS.h"

MYMODDECL();
void Redirect(uintptr_t addr, uintptr_t to);

/* Saves */
std::vector<AdditionalSetting*> gMoreSettings;
std::vector<AdditionalTexDB*>   gMoreTexDBs;
std::vector<const char*>        gMoreIMGs;
int nNextSettingNum = MODS_SETTINGS_STARTING_FROM - 1;
int nCurrentSliderId = 0;
eTypeOfSettings nCurrentItemTab = Mods;
bool g_bIsGameStartedAlready = false;

/* Patches vars */
int pNewSettings[8 * MAX_SETTINGS]; // A new char MobileSettings::settings[37*8*4]
char pNewStreamingFiles[48 * (MAX_IMG_ARCHIVES + 2)]; // A new char CStreaming::ms_files[48 * 8]; // 0 and 1 are used for player and something

/* Funcs */
typedef void* (*SettingsAddItemFn)(void* a1, uintptr_t a2);
RwTexture* (*GetTextureFromDB)(const char*);
uintptr_t (*ProcessMenuPending)(uintptr_t globalMobileMenuPtr);
void      (*InitializeMenuPtr)(uintptr_t mobileMenuPtr, const char* topname, bool isCreatedNowMaybeIdk);
uintptr_t (*LoadTextureDB)(const char* dbFile, bool fullLoad, int txdbFormat);
void      (*RegisterTextureDB)(uintptr_t dbPtr);
int       (*CdStreamOpen)(const char* filename, bool);
int       AddImageToListPatched(const char* imgName, bool isPlayerImg = false);

/* GTASA Pointers */
extern uintptr_t pGameLib;
extern void* pGameHandle;
unsigned short* gxtErrorString;
SettingsAddItemFn AddSettingsItemFn;
uintptr_t OnRestoreDefaultsFn;
uintptr_t OnRestoreDefaultsAudioFn;
uintptr_t pgMobileMenu;
int* pCurrentMenuPointer;
int* dword_6E0090; // Probably "YesOrNo" window is visible
int* dword_6E0094;

/* SAUtils */
void AddRestoreDefaultsItem(void* screen, bool isAudio = false)
{
    ButtonSettingItem* mob_rtd = new ButtonSettingItem;
    mob_rtd->vtable = pGameLib + 0x66281C;
    mob_rtd->itemText = "MOB_RTD";
    mob_rtd->actionFn = isAudio ? OnRestoreDefaultsAudioFn : OnRestoreDefaultsFn;
    mob_rtd->flag = 0;
    AddSettingsItemFn(screen, (uintptr_t)mob_rtd);
}
void AddSettingsToScreen(void* screen)
{
    int size = gMoreSettings.size();
    for(int i = 0; i < size; ++i)
    {
        AdditionalSetting* setting = gMoreSettings[i];
        if(setting->eType == nCurrentItemTab)
        {
            if(setting->byteItemType == Button)
            {
                ButtonSettingItem* mob_rtd = new ButtonSettingItem;
                mob_rtd->vtable = pGameLib + 0x66281C;
                mob_rtd->itemText = setting->szName;
                mob_rtd->actionFn = (uintptr_t)(setting->fnOnButtonPressed);
                mob_rtd->flag = 0;
                AddSettingsItemFn(screen, (uintptr_t)mob_rtd);
            }
            else
            {
                uintptr_t menuItem = (uintptr_t)(new char[0x1Cu]);
                *(uintptr_t*)menuItem = pGameLib + 0x662848;
                *(const char**)(menuItem + 4) = setting->szName;
                *(int*)(menuItem + 8) = setting->nSettingId;
                *(int*)(menuItem + 12) = 0;
                *(int*)(menuItem + 16) = 0;
                AddSettingsItemFn(screen, menuItem);
            }
        }
    }
}

DECL_HOOK(unsigned short*, AsciiToGxtChar, const char* txt, unsigned short* ret)
{
    if(nCurrentSliderId != 0)
    {
        int size = gMoreSettings.size();
        for(int i = 0; i < size; ++i)
        {
            AdditionalSetting* setting = gMoreSettings[i];
            if(setting->nSettingId == nCurrentSliderId)
            {
                if(setting->fnOnValueDraw != NULL) return AsciiToGxtChar(setting->fnOnValueDraw(pNewSettings[8 * nCurrentSliderId + 2]), ret);
                break;
            }
        }
    }
    return AsciiToGxtChar(txt, ret);
}

void SettingsScreenClosed()
{
    int size = gMoreSettings.size();
    for(int i = 0; i < size; ++i)
    {
        AdditionalSetting* setting = gMoreSettings[i];
        if(setting->byteItemType != Button && setting->eType == nCurrentItemTab)
        {
            int nNewVal = sautils->ValueOfSettingsItem(setting->nSettingId);
            if(nNewVal != setting->nSavedVal)
            {
                if(setting->fnOnValueChange != NULL) setting->fnOnValueChange(setting->nSavedVal, nNewVal);
                setting->nSavedVal = nNewVal;
            }
        }
    }
}

DECL_HOOK(void, SelectScreenOnDestroy, void* self)
{
    SettingsScreenClosed();
    SelectScreenOnDestroy(self);
}
DECL_HOOK(void, SelectScreenRender, uintptr_t self, float a1, float a2, float a3, float a4, float a5, float a6)
{
    nCurrentSliderId = *(int*)(self + 8);
    if(!(nCurrentSliderId >= MODS_SETTINGS_STARTING_FROM && nCurrentSliderId < MAX_SETTINGS && pNewSettings[8 * nCurrentSliderId + 7] == 1)) nCurrentSliderId = 0;
    SelectScreenRender(self, a1, a2, a3, a4, a5, a6);
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
int None(...) {return 0;}
char szSautilsVer[32];
uintptr_t OnModSettingsOpened()
{
    nCurrentItemTab = Mods;
    snprintf(szSautilsVer, sizeof(szSautilsVer), "SAUtils v%s", modinfo->VersionString());
    char* menuScreenPointer = new char[0x44];
    InitializeMenuPtr((uintptr_t)menuScreenPointer, "Mod Settings", true);
    *(uintptr_t*)menuScreenPointer = pGameLib + 0x6628D0; // Vtable


    ButtonSettingItem* sautilsVer = new ButtonSettingItem;
    sautilsVer->vtable = pGameLib + 0x66281C;
    sautilsVer->itemText = szSautilsVer;
    sautilsVer->actionFn = (uintptr_t)None;
    sautilsVer->flag = 0;
    AddSettingsItemFn((void*)menuScreenPointer, (uintptr_t)sautilsVer); // SAUtils version

    AddSettingsToScreen((void*)menuScreenPointer); // Custom items

    ButtonSettingItem* sautilsLine = new ButtonSettingItem;
    sautilsLine->vtable = pGameLib + 0x66281C;
    sautilsLine->itemText = "";
    sautilsLine->actionFn = (uintptr_t)None;
    sautilsLine->flag = 0;
    AddSettingsItemFn((void*)menuScreenPointer, (uintptr_t)sautilsLine); // Empty line


    *(bool*)(menuScreenPointer + 48) = true; // Ready to be shown! Or... the other thingy?
    if(*dword_6E0090)
    {
        (*(void(**)(char*, int))(*(int*)menuScreenPointer + 20))(menuScreenPointer, *(int*)(*dword_6E0094 + 4 * *dword_6E0090 - 4));
    }
    if(*pCurrentMenuPointer != 0) ProcessMenuPending(pgMobileMenu);
    *pCurrentMenuPointer = (int)menuScreenPointer;
    return pgMobileMenu;
}
DECL_HOOK(uintptr_t, SettingsScreen, uintptr_t self)
{
    SettingsScreen(self);

    // New "Mods" tab should be there!
    RwTexture* tex = GetTextureFromDB("menu_mainsettings");
    ++tex->refCount;
    int& tabsCount = *(int*)(self + 64);
    uintptr_t container; // Maybe a storage for those tabs
    if(*(int*)(self + 60) >= tabsCount + 1) // If we have a place for tabs
    {
        container = *(uintptr_t*)(self + 68);
    }
    else // If we dont have a place for tabs, reallocate more
    {
        int reallocCount = 4 * (tabsCount + 1) / 3u + 3;
        void* newContainer = malloc(12 * reallocCount);
        void* oldContainer = *(void **)(self + 68);
        container = (uintptr_t)newContainer;
        if (oldContainer)
        {
            memcpy(newContainer, *(const void **)(self + 68), 12 * tabsCount);
            free(oldContainer);
            tabsCount = *(int*)(self + 64);
        }
        *(int*)(self + 60) = reallocCount;
        *(int*)(self + 68) = (int)container;
    }
    container = container + 12 * tabsCount;
    *(RwTexture**)(container + 0) = tex;
    *(const char**)(container + 4) = "Mods settings";
    *(uintptr_t*)(container + 8) = (uintptr_t)OnModSettingsOpened;
    ++tabsCount;
    // New "Mods" tab should be there!

    return self;
}

DECL_HOOKv(InitialiseRenderWare)
{
    InitialiseRenderWare();

    auto vStart = gMoreTexDBs.begin();
    auto vEnd = gMoreTexDBs.end();
    AdditionalTexDB* tdb;
    uintptr_t dbPtr;
    while(vStart != vEnd)
    {
        tdb = *vStart;
        dbPtr = LoadTextureDB(tdb->szName, false, 5);
        if(dbPtr != 0 && tdb->bRegister) RegisterTextureDB(dbPtr);
        ++vStart;
    }
}

DECL_HOOKv(InitialiseGame_SecondPass)
{
    InitialiseGame_SecondPass();

    auto vStart = gMoreIMGs.begin();
    auto vEnd = gMoreIMGs.end();
    while(vStart != vEnd)
    {
        AddImageToListPatched(*vStart, false);
        ++vStart;
    }
    g_bIsGameStartedAlready = true;
}

uintptr_t NewScreen_Controls_backto, NewScreen_Game_backto, NewScreen_Display_backto, NewScreen_Audio_backto;
extern "C" void NewScreen_Controls_inject(void* self)
{
    nCurrentItemTab = Controller;
    AddSettingsToScreen(self);
    AddRestoreDefaultsItem(self);
}
extern "C" void NewScreen_Game_inject(void* self)
{
    nCurrentItemTab = Game;
    AddSettingsToScreen(self);
    AddRestoreDefaultsItem(self);
}
extern "C" void NewScreen_Display_inject(void* self)
{
    nCurrentItemTab = Display;
    AddSettingsToScreen(self);
    AddRestoreDefaultsItem(self);
}
extern "C" void NewScreen_Audio_inject(void* self)
{
    nCurrentItemTab = Audio;
    AddSettingsToScreen(self);
    AddRestoreDefaultsItem(self, true);
}

__attribute__((optnone)) __attribute__((naked)) void NewScreen_Controls_stub(void)
{
    asm("PUSH {R0}\nMOV R0, R8");
    asm("BL NewScreen_Controls_inject");
    asm volatile("MOV R12, %0\n" :: "r"(NewScreen_Controls_backto));
    asm("POP {R0}\nBX R12");
}
__attribute__((optnone)) __attribute__((naked)) void NewScreen_Game_stub(void)
{
    asm("PUSH {R0}\nMOV R0, R4");
    asm("BL NewScreen_Game_inject");
    asm volatile("MOV R12, %0\n" :: "r"(NewScreen_Game_backto));
    asm("POP {R0}\nBX R12");
}
__attribute__((optnone)) __attribute__((naked)) void NewScreen_Display_stub(void)
{
    asm("PUSH {R0}\nMOV R0, R4");
    asm("BL NewScreen_Display_inject");
    asm volatile("MOV R12, %0\n" :: "r"(NewScreen_Display_backto));
    asm("POP {R0}\nBX R12");
}
__attribute__((optnone)) __attribute__((naked)) void NewScreen_Audio_stub(void)
{
    asm("PUSH {R0}\nMOV R0, R4");
    asm("BL NewScreen_Audio_inject");
    asm volatile("MOV R12, %0\n" :: "r"(NewScreen_Audio_backto));
    asm("POP {R0}\nBX R12");
}

DECL_HOOK(void*, NewScreen_Language, void* self)
{
    nCurrentItemTab = Language;
    NewScreen_Language(self);
    AddSettingsToScreen(self);
    return self;
}

int AddImageToListPatched(const char* imgName, bool isPlayerImg)
{
    for(unsigned char i = 0; i < (MAX_IMG_ARCHIVES + 2); ++i)
    {
        if(pNewStreamingFiles[48 * i + 0] == '\0')
        {
            strcpy(&pNewStreamingFiles[48 * i + 0], imgName);
            pNewStreamingFiles[48 * i + 40] = isPlayerImg;
            *(int*)(&pNewStreamingFiles[48 * i + 44]) = CdStreamOpen(imgName, false);
            return i;
        }
    }
    logger->Error("Not enough space in CStreaming::ms_files for %s!", imgName);
    return 0;
}

void SAUtils::InitializeSAUtils()
{
    // Bump settings limit
    aml->Unprot(pGameLib + 0x679A40, sizeof(void*));
    *(uintptr_t*)(pGameLib + 0x679A40) = (uintptr_t)pNewSettings;
    memcpy(pNewSettings, (int*)(pGameLib + 0x6E03F4), 1184);

    // Hook functions
    HOOKPLT(AsciiToGxtChar,             pGameLib + 0x6724F8);
    HOOKPLT(SelectScreenOnDestroy,      pGameLib + 0x673FD8);
    HOOKPLT(SelectScreenRender,         pGameLib + 0x662850);
    HOOKPLT(GxtTextGet,                 pGameLib + 0x66E78C);
    HOOKPLT(SettingsScreen,             pGameLib + 0x674018);
    HOOKPLT(InitialiseRenderWare,       pGameLib + 0x66F2D0);
    HOOKPLT(InitialiseGame_SecondPass,  pGameLib + 0x672178);

    // Hooked settings functions
    Redirect(pGameLib + 0x29E6AA + 0x1, (uintptr_t)NewScreen_Controls_stub); NewScreen_Controls_backto = pGameLib + 0x29E6D2 + 0x1;
    Redirect(pGameLib + 0x2A49F6 + 0x1, (uintptr_t)NewScreen_Game_stub); NewScreen_Game_backto = pGameLib + 0x2A4A1E + 0x1;
    Redirect(pGameLib + 0x2A4BD4 + 0x1, (uintptr_t)NewScreen_Display_stub); NewScreen_Display_backto = pGameLib + 0x2A4BFC + 0x1;
    Redirect(pGameLib + 0x2A4D3C + 0x1, (uintptr_t)NewScreen_Audio_stub); NewScreen_Audio_backto = pGameLib + 0x2A4D64 + 0x1;
    HOOKPLT(NewScreen_Language,         pGameLib + 0x675D90);

    SET_TO(OnRestoreDefaultsFn,         aml->GetSym(pGameHandle, "_ZN12SelectScreen17OnRestoreDefaultsEPS_i"));
    SET_TO(OnRestoreDefaultsAudioFn,    aml->GetSym(pGameHandle, "_ZN11AudioScreen17OnRestoreDefaultsEP12SelectScreeni"));
    SET_TO(GetTextureFromDB,            aml->GetSym(pGameHandle, "_ZN22TextureDatabaseRuntime10GetTextureEPKc"));
    SET_TO(ProcessMenuPending,          aml->GetSym(pGameHandle, "_ZN10MobileMenu14ProcessPendingEv"));
    SET_TO(InitializeMenuPtr,           aml->GetSym(pGameHandle, "_ZN16CharSelectScreenC2EPKcb"));
    SET_TO(LoadTextureDB,               aml->GetSym(pGameHandle, "_ZN22TextureDatabaseRuntime4LoadEPKcb21TextureDatabaseFormat"));
    SET_TO(RegisterTextureDB,           aml->GetSym(pGameHandle, "_ZN22TextureDatabaseRuntime8RegisterEPS_"));
    SET_TO(CdStreamOpen,                aml->GetSym(pGameHandle, "_Z12CdStreamOpenPKcb"));
    SET_TO(AddSettingsItemFn,           aml->GetSym(pGameHandle, "_ZN12SelectScreen7AddItemEPNS_13MenuSelectionE"));

    SET_TO(pCurrentMenuPointer,         pGameLib + 0x6E0098);
    SET_TO(dword_6E0090,                pGameLib + 0x6E0090);
    SET_TO(dword_6E0094,                pGameLib + 0x6E0094);
    SET_TO(gxtErrorString,              aml->GetSym(pGameHandle, "GxtErrorString"));
    SET_TO(pgMobileMenu,                aml->GetSym(pGameHandle, "gMobileMenu"));

    // Patch IMG limit
    aml->Unprot(pGameLib + 0x676AC4, sizeof(void*));
    *(uintptr_t*)(pGameLib + 0x676AC4) = (uintptr_t)pNewStreamingFiles;
    aml->Unprot(pGameLib + 0x46BD78, sizeof(char));
    *(unsigned char*)(pGameLib + 0x46BD78) = (unsigned char)MAX_IMG_ARCHIVES+2;
    aml->Unprot(pGameLib + 0x46BFE4, sizeof(char));
    *(unsigned char*)(pGameLib + 0x46BFE4) = (unsigned char)MAX_IMG_ARCHIVES+2;
    Redirect(aml->GetSym(pGameHandle, "_ZN10CStreaming14AddImageToListEPKcb"), (uintptr_t)AddImageToListPatched);
}
void SAUtils::InitializeVCUtils()
{
    gxtErrorString = (unsigned short*)(pGameLib + 0x716C2C);

    //aml->Unprot(pGameLib + 0x679A40, sizeof(void*));
    //*(uintptr_t*)(pGameLib + 0x679A40) = (uintptr_t)pNewSettings;
    //memcpy(pNewSettings, (int*)(pGameLib + 0x6E03F4), 1184);

    HOOK(GxtTextGet, dlsym(pGameHandle, "_ZN5CText3GetEPKc"));
    HOOK(AsciiToGxtChar, dlsym(pGameHandle, "_Z14AsciiToUnicodePKcPt"));
    //HOOKPLT(NewScreen_Controls, pGameLib + 0x675CD8);
    //HOOKPLT(NewScreen_Game, pGameLib + 0x674310);
    //HOOKPLT(NewScreen_Display, pGameLib + 0x675150);
    //HOOKPLT(NewScreen_Audio, pGameLib + 0x66FBA4);
    //HOOKPLT(NewScreen_Language, pGameLib + 0x675D90);
    //HOOKPLT(SelectScreenAddItem, pGameLib + 0x674518);
    HOOK(SelectScreenOnDestroy, dlsym(pGameHandle, "_ZN12CMenuManager4BackEv"));

    //fnSettingsAddItem = (SettingsAddItemFn)(pGameLib + 0x19C840);
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
    pNew->byteItemType = isSlider ? Slider : WithItems;
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
    if(settingId < 0 || settingId > nNextSettingNum) return 0;
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
    pNew->fnOnValueDraw = NULL;
    pNew->fnOnButtonPressed = NULL;
    pNew->byteItemType = WithItems;
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
    pNew->fnOnButtonPressed = NULL;
    pNew->byteItemType = Slider;
    pNew->nInitVal = (int)initVal;
    pNew->nSavedVal = (int)initVal;
    pNew->nMaxVal = maxVal;
    gMoreSettings.push_back(pNew);

    pNewSettings[8 * nNextSettingNum + 1] = (int)NULL;
    pNewSettings[8 * nNextSettingNum + 2] = initVal;
    pNewSettings[8 * nNextSettingNum + 4] = minVal;
    pNewSettings[8 * nNextSettingNum + 5] = maxVal;
    pNewSettings[8 * nNextSettingNum + 7] = 1;

    return nNextSettingNum;
}

// 1.2
void SAUtils::AddButton(eTypeOfSettings typeOf, const char* name, OnButtonPressedFn fnOnButtonPressed)
{
    if(fnOnButtonPressed == NULL) return;

    AdditionalSetting* pNew = new AdditionalSetting;
    pNew->nSettingId = -1;
    pNew->eType = typeOf;
    pNew->szName = name;
    pNew->fnOnValueChange = NULL;
    pNew->fnOnValueDraw = NULL;
    pNew->fnOnButtonPressed = fnOnButtonPressed;
    pNew->byteItemType = Button;
    pNew->nInitVal = 0;
    pNew->nSavedVal = 0;
    pNew->nMaxVal = 0;
    gMoreSettings.push_back(pNew);
}

void SAUtils::AddTextureDB(const char* name, bool registerMe)
{
    if(!name || !name[0]) return;
    AdditionalTexDB* pNew = new AdditionalTexDB;
    pNew->szName = name;
    pNew->bRegister = registerMe;
    gMoreTexDBs.push_back(pNew);

    if(g_bIsGameStartedAlready)
    {
        uintptr_t dbPtr = LoadTextureDB(name, false, 5);
        if(dbPtr != 0 && registerMe) RegisterTextureDB(dbPtr);
    }
}

int* SAUtils::GetSettingValuePointer(int settingId)
{
    return &pNewSettings[8 * nNextSettingNum + 2];
}

void SAUtils::AddIMG(const char* imgName)
{
    if(!imgName || !imgName[0]) return;
    gMoreIMGs.push_back(imgName);

    if(g_bIsGameStartedAlready) AddImageToListPatched(imgName, false);
}

static SAUtils sautilsLocal;
ISAUtils* sautils = &sautilsLocal;
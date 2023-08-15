#include <mod/amlmod.h>
#include <mod/logger.h>
#include <dlfcn.h>
#include <sautils.h>
#include <stdint.h>
#include <vector>
#include <cstring> // memcpy, memcmp

#include "AArch64_ModHelper/Thumbv7_ASMHelper.h"
#include "GTASA_STRUCTS.h"
using namespace ThumbV7;

MYMODDECL();
extern uintptr_t pGameLib;
extern void* pGameHandle;

// ScriptingRelated

void InitializeSAScripting();

int None(...){return 0;}

/* Callbacks */
std::vector<SimpleFn>                  gCreateWidgetFns;
std::vector<OnPlayerProcessFn>         gPlayerUpdateFns;
std::vector<OnPlayerProcessFn>         gPlayerUpdatePostFns;
std::vector<SimpleFn>                  gInitRWFns;
std::vector<LookingForTextureFn>       gTextureLookupFns;
std::vector<SimpleDataFn>              gRenderOfTypeFns[RENDEROFTYPE_MAX];

/* Saves */
std::vector<AdditionalSetting*>        gMoreSettings;
std::vector<AdditionalTexDB*>          gMoreTexDBs;
std::vector<const char*>               gMoreIMGs;
std::vector<AdditionalSettingsButton*> gMoreSettingButtons[TABBUTTONLOC_MAX];
int             nNextSettingNum = MODS_SETTINGS_STARTING_FROM - 1;
int             nCurrentSliderId = 0;
eTypeOfSettings nCurrentItemTab = SetType_Mods;
bool            g_bIsGameStartedAlready = false;

/* Patched vars */
MobileSettings::Setting                pNewSettings[MAX_SETTINGS] {0}; // A new char MobileSettings::settings[37*8*4]
CWidget*                               pNewWidgets[MAX_WIDGETS] {NULL};
CStreamingFile                         pNewStreamingFiles[MAX_IMG_ARCHIVES + 2] {0}; // A new char CStreaming::ms_files[48 * 8]; // 0 and 1 are used for player and something

/* Vars */
CPool<CPed, CCopPed> **ms_pPedPool;
CPool<CVehicle, CHeli> **ms_pVehiclePool;
CPool<CObject, CCutsceneObject> **ms_pObjectPool;

/* Funcs */
typedef void* (*SettingsAddItemFn)(SelectScreen* a1, SelectScreen::MenuSelection* a2);
RwTexture*    (*GetTextureFromDB)(const char*);
uintptr_t     (*ProcessMenuPending)(MobileMenu* mobilemenu);
void          (*InitializeMenuPtr)(SelectScreen* mobileMenuPtr, const char* topname, bool isCreatedNowMaybeIdk);
uintptr_t     (*LoadTextureDB)(const char* dbFile, bool fullLoad, int txdbFormat);
uintptr_t     (*GetTexDB)(const char* dbName);
void          (*RegisterTexDB)(uintptr_t dbPtr);
void          (*UnregisterTexDB)(uintptr_t dbPtr);
int           (*CdStreamOpen)(const char* filename, bool);
int           (*AddImageToList)(const char* imgName, bool isPlayerImg);
void          (*WidgetButton_Constructor)(CWidgetButton*, char const*, WidgetPosition const&, unsigned int, unsigned int, HIDMapping);
void          (*SetSpriteTexture)(CSprite2d*, const char*);
RwTexture*    (*CopyRWTexture)(RwTexture*);
RwTexture*    (*RwTextureCreate)(RwRaster*);
void          (*RwTextureDestroy)(RwTexture*);
bool          (*Widget_IsTouched)(CWidget*);
bool          (*Widget_IsDoubleTapped)(CWidget*);
bool          (*Widget_IsReleased)(CWidget*);
bool          (*Widget_IsSwipedLeft)(CWidget*);
bool          (*Widget_IsSwipedRight)(CWidget*);
bool          (*Widget_IsSwipedUp)(CWidget*);
bool          (*Widget_IsSwipedDown)(CWidget*);
bool          (*Touch_IsWidgetTouched)(int widgetId, void* useless, int frames);
bool          (*Touch_IsWidgetDoubleTapped)(int widgetId, bool doTapEffect, int frames);
bool          (*Touch_IsWidgetReleased)(int widgetId, void* useless, int frames);
bool          (*Touch_IsWidgetSwipedLeft)(int widgetId, int frames);
bool          (*Touch_IsWidgetSwipedRight)(int widgetId, int frames);
bool          (*Touch_IsWidgetSwipedUp)(int widgetId, int frames);
bool          (*Touch_IsWidgetSwipedDown)(int widgetId, int frames);
RwImage*      (*RtPNGImageRead)(const char* filename);
RwImage*      (*RtBMPImageRead)(const char* filename);
void          (*RwImageFindRasterFormat)(RwImage*, int, int*, int*, int*, int*);
RwRaster*     (*RwRasterCreate)(int, int, int, int);
void          (*RwRasterSetFromImage)(RwRaster*, RwImage*);
void          (*RwImageDestroy)(RwImage*);RwStream* (*RwStreamOpen)(int, int, const char*);
bool          (*RwStreamFindChunk)(RwStream*, int, int, int);
RpClump*      (*RpClumpStreamRead)(RwStream*);
void          (*RwStreamClose)(RwStream*, int);
RpAtomic*     (*GetFirstAtomic)(RpClump*);
void          (*SetFilterModeOnAtomicsTextures)(RpAtomic*, int);
void          (*RpGeometryLock)(RpGeometry*, int);
void          (*RpGeometryUnlock)(RpGeometry*);
void          (*RpGeometryForAllMaterials)(RpGeometry*, RpMaterial* (*)(RpMaterial*, RwRGBA&), RwRGBA&);
void          (*RpMaterialSetTexture)(RpMaterial*, RwTexture*);
RpAtomic*     (*RpAtomicClone)(RpAtomic*);
void          (*RpClumpDestroy)(RpClump*);
RwFrame*      (*RwFrameCreate)();
void          (*RpAtomicSetFrame)(RpAtomic*, RwFrame*);
void          (*RenderAtomicWithAlpha)(RpAtomic*, int alphaVal);
RpGeometry*   (*RpGeometryCreate)(int, int, unsigned int); //
RpMaterial*   (*RpGeometryTriangleGetMaterial)(RpGeometry*, RpTriangle*); //
void          (*RpGeometryTriangleSetMaterial)(RpGeometry*, RpTriangle*, RpMaterial*); //
void          (*RpAtomicSetGeometry)(RpAtomic*, RpGeometry*, unsigned int);
RpAtomicCallBackRender AtomicDefaultRenderCallBack;
FSButtonCallback OnMainMenuExit;

/* GTASA Pointers */
SettingsAddItemFn AddSettingsItemFn;
MobileMenu *gMobileMenu;
CPlayerInfo *WorldPlayers;
unsigned short* gxtErrorString;
void (*OnRestoreDefaultsFn)(SelectScreen*, int);
void (*OnRestoreDefaultsAudioFn)(SelectScreen*, int);
unsigned int* m_snTimeInMilliseconds;
float* game_FPS;
CWidget** orgWidgetsPtr;
uintptr_t _ZTVN12SelectScreen15ActionSelectionE, _ZTVN12SelectScreen16SettingSelectionE, _ZTV13DisplayScreen;

/* SAUtils */
static void AddRestoreDefaultsItem(SelectScreen* screen, bool isAudio = false)
{
    SelectScreen::ActionSelection* mob_rtd = new SelectScreen::ActionSelection;
    mob_rtd->vtable = _ZTVN12SelectScreen15ActionSelectionE;
    mob_rtd->tag = "MOB_RTD";
    mob_rtd->OnSelect = isAudio ? OnRestoreDefaultsAudioFn : OnRestoreDefaultsFn;
    mob_rtd->data = 0;
    AddSettingsItemFn(screen, mob_rtd);
}
static void AddSettingsToScreen(SelectScreen* screen)
{
    int size = gMoreSettings.size();
    for(int i = 0; i < size; ++i)
    {
        AdditionalSetting* setting = gMoreSettings[i];
        if(setting->eType == nCurrentItemTab)
        {
            if(setting->byteItemType == ItemType_Button)
            {
                SelectScreen::ActionSelection* mob_rtd = new SelectScreen::ActionSelection;
                mob_rtd->vtable = _ZTVN12SelectScreen15ActionSelectionE;
                mob_rtd->tag = setting->szName;
                mob_rtd->OnSelect = setting->fnOnButtonPressed == NULL ? (void(*)(SelectScreen*,int))None : (void(*)(SelectScreen*,int))setting->fnOnButtonPressed;
                mob_rtd->data = 0;
                AddSettingsItemFn(screen, mob_rtd);
            }
            else
            {
                SelectScreen::SettingSelection* menuItem = new SelectScreen::SettingSelection;
                menuItem->vtable = _ZTVN12SelectScreen16SettingSelectionE;
                menuItem->tag = setting->szName;
                menuItem->forSetting = (MobileSetting)setting->nSettingId;
                menuItem->curMoveReq = 0.0f;
                menuItem->sliderX1 = 0.0f;
                menuItem->sliderX2 = 0.0f;
                AddSettingsItemFn(screen, menuItem);
            }
        }
    }
}

DECL_HOOKv(CreateAllWidgets)
{
    CreateAllWidgets();
    memcpy(orgWidgetsPtr, pNewWidgets, sizeof(void*) * 150); // Hack to support old dumb CLEO's that cannot work the better way...
    int size = gCreateWidgetFns.size();
    for(int i = 0; i < size; ++i)
    {
        gCreateWidgetFns[i]();
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
                if(setting->fnOnValueDraw != NULL)
                {
                    int val = pNewSettings[nCurrentSliderId].value;
                    nCurrentSliderId = 0;
                    return AsciiToGxtChar(setting->fnOnValueDraw(val, setting->pOwnData), ret);
                }
                nCurrentSliderId = 0;
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
        if(setting->byteItemType != ItemType_Button && setting->eType == nCurrentItemTab)
        {
            int nNewVal = sautils->ValueOfSettingsItem(setting->nSettingId);
            if(nNewVal != setting->nSavedVal)
            {
                if(setting->fnOnValueChange != NULL)
                {
                    setting->fnOnValueChange(setting->nSavedVal, nNewVal, setting->pOwnData);
                }
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
DECL_HOOK(void, SettingSelectionRender, SelectScreen::SettingSelection* self, float a1, float a2, float a3, float a4, float a5, float a6)
{
    int sliderId = (int)self->forSetting;
    if(sliderId >= MODS_SETTINGS_STARTING_FROM && pNewSettings[sliderId].type == MST_Range) nCurrentSliderId = sliderId;
    SettingSelectionRender(self, a1, a2, a3, a4, a5, a6);
    nCurrentSliderId = 0;
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

char szSautilsVer[32];
int nCurScrItemsOffset = 0;
FlowScreen* curScr = NULL;
eSettingsTabButtonLoc curLoc;
MobileMenu* OnModSettingsOpened()
{
    nCurrentItemTab = SetType_Mods;
    snprintf(szSautilsVer, sizeof(szSautilsVer), "SAUtils v%s", modinfo->VersionString());
    CharSelectScreen* menuScreenPointer = New<CharSelectScreen>();
    InitializeMenuPtr(menuScreenPointer, "Mods Settings", true);
    menuScreenPointer->vtable() = _ZTV13DisplayScreen; // Vtable

    SelectScreen::ActionSelection* sautilsVer = new SelectScreen::ActionSelection;
    sautilsVer->vtable = _ZTVN12SelectScreen15ActionSelectionE;
    sautilsVer->tag = szSautilsVer;
    sautilsVer->OnSelect = (void(*)(SelectScreen*,int))None;
    sautilsVer->data = 0;
    AddSettingsItemFn(menuScreenPointer, sautilsVer); // SAUtils version

    AddSettingsToScreen(menuScreenPointer); // Custom items

    menuScreenPointer->renderLastAtBottom = false;
    
    if(gMobileMenu->m_nScreensCount > 0)
    {
        menuScreenPointer->SetPreviousScreen(gMobileMenu->m_pScreens[gMobileMenu->m_nScreensCount - 1]);
    }
    if(gMobileMenu->m_pTopScreen != NULL) ProcessMenuPending(gMobileMenu);
    gMobileMenu->m_pTopScreen = menuScreenPointer;
    return gMobileMenu;
}

MobileMenu* OnTabButtonClicked()
{
    nCurrentItemTab = (eTypeOfSettings)(curScr->m_nChosenButton - nCurScrItemsOffset);
    
    gMoreSettingButtons[curLoc][nCurrentItemTab]->fnButtonPressed(gMoreSettingButtons[curLoc][nCurrentItemTab]->pMenuData);
    
    return NULL;
}

MobileMenu* OnCustomModSettingsOpened()
{
    nCurrentItemTab = (eTypeOfSettings)(curScr->m_nChosenButton);
    CharSelectScreen* menuScreenPointer = New<CharSelectScreen>();
    InitializeMenuPtr(menuScreenPointer, gMoreSettingButtons[STB_Settings][nCurrentItemTab - SETTINGS_COUNT]->szName, true);
    menuScreenPointer->vtable() = _ZTV13DisplayScreen; // Vtable

    AddSettingsToScreen(menuScreenPointer); // Custom items

    menuScreenPointer->renderLastAtBottom = false;
    if(gMobileMenu->m_nScreensCount)
    {
        menuScreenPointer->SetPreviousScreen(gMobileMenu->m_pScreens[gMobileMenu->m_nScreensCount - 1]);
    }
    if(gMobileMenu->m_pTopScreen != NULL) ProcessMenuPending(gMobileMenu);
    gMobileMenu->m_pTopScreen = menuScreenPointer;
    return gMobileMenu;
}
void AddSettingsButton(FlowScreen* self, const char* name, const char* textureName, FSButtonCallback callback)
{
    RwTexture* tex = (RwTexture*)sautils->LoadRwTextureFromPNG(textureName);
    if(!tex) tex = GetTextureFromDB(textureName);
    if(!tex) tex = GetTextureFromDB("menu_mainsettings");
    ++tex->refCount;

    FlowScreenButton* btn = self->items.AllocNew();
    btn->m_pTexture = tex;
    btn->m_szName = name;
    btn->m_pOnPressed = callback;
}
DECL_HOOK(SettingsScreen*, SettingsScreen_Construct, SettingsScreen* self)
{
    curScr = self;
    SettingsScreen_Construct(self);

    // New "Mods" tab should be there!
    AddSettingsButton(self, "Mods settings", "AML/images/amllogo.png", OnModSettingsOpened);
    // New "Mods" tab should be there!

    // Custom tabs!
    int size = gMoreSettingButtons[STB_Settings].size();
    AdditionalSettingsButton* pBtn;
    for(int i = 0; i < size; ++i)
    {
        pBtn = gMoreSettingButtons[STB_Settings][i];
        if(pBtn->nBtnLoc == STB_Settings || pBtn->bUsesMenu)
            AddSettingsButton(self, pBtn->szName, pBtn->szTextureName, pBtn->bUsesMenu ? OnCustomModSettingsOpened : OnTabButtonClicked);
    }
    // Custom tabs!

    return self;
}

DECL_HOOKv(InitialiseRenderWare)
{
    InitialiseRenderWare();
    
    int size = gInitRWFns.size();
    for(int i = 0; i < size; ++i)
    {
        gInitRWFns[i]();
    }

    auto vStart = gMoreTexDBs.begin();
    auto vEnd = gMoreTexDBs.end();
    AdditionalTexDB* tdb;
    while(vStart != vEnd)
    {
        tdb = *vStart;
        tdb->nDBPointer = LoadTextureDB(tdb->szName, false, tdb->cacheType);
        if(tdb->nDBPointer != 0 && tdb->bRegister) RegisterTexDB(tdb->nDBPointer);
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
        AddImageToList(*vStart, false);
        ++vStart;
    }
    g_bIsGameStartedAlready = true;
}

DECL_HOOKv(PlayerProcess, CPlayerInfo* self, uint32_t playerIndex)
{
    if(playerIndex == 0)
    {
        int size = gPlayerUpdateFns.size();
        for(int i = 0; i < size; ++i) gPlayerUpdateFns[i]((uintptr_t)self);

        PlayerProcess(self, playerIndex);

        size = gPlayerUpdatePostFns.size();
        for(int i = 0; i < size; ++i) gPlayerUpdatePostFns[i]((uintptr_t)self);
    }
    else
    {
        PlayerProcess(self, playerIndex);
    }
}

uintptr_t NewScreen_Controls_backto, NewScreen_Game_backto, NewScreen_Display_backto, NewScreen_Audio_backto;
uintptr_t DrawSlider_backto;
extern "C" void NewScreen_Controls_inject(SelectScreen* self)
{
    nCurrentItemTab = SetType_Controller;
    AddSettingsToScreen(self);
    AddRestoreDefaultsItem(self);
}
extern "C" void NewScreen_Game_inject(SelectScreen* self)
{
    nCurrentItemTab = SetType_Game;
    AddSettingsToScreen(self);
    AddRestoreDefaultsItem(self);
}
extern "C" void NewScreen_Display_inject(SelectScreen* self)
{
    nCurrentItemTab = SetType_Display;
    AddSettingsToScreen(self);
    AddRestoreDefaultsItem(self);
}
extern "C" void NewScreen_Audio_inject(SelectScreen* self)
{
    nCurrentItemTab = SetType_Audio;
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

DECL_HOOK(void*, NewScreen_Language, SelectScreen* self)
{
    nCurrentItemTab = SetType_Language;
    NewScreen_Language(self);
    AddSettingsToScreen(self);
    return self;
}

extern "C" void DrawSlider_inject(void* self)
{
    
}
__attribute__((optnone)) __attribute__((naked)) void DrawSlider_stub(void)
{
    asm("PUSH {R0}\nMOV R0, R4");
    asm("BL DrawSlider_inject");
    asm volatile("MOV R12, %0\n" :: "r"(DrawSlider_backto));
    asm("POP {R0}\nBX R12");
}

int AddImageToListPatched(const char* imgName, bool registerIt)
{
    for(unsigned char i = 0; i < (MAX_IMG_ARCHIVES + 2); ++i)
    {
        if(pNewStreamingFiles[i].m_name[0] == '\0')
        {
            strncpy((char*)pNewStreamingFiles[i].m_name, imgName, 40);
            pNewStreamingFiles[i].m_bRegister = registerIt;
            pNewStreamingFiles[i].m_lsn = CdStreamOpen(imgName, false);
            return i;
        }
    }
    logger->Error("Not enough space in CStreaming::ms_files for %s!", imgName);
    return 0;
}

DECL_HOOK(void*, GetTextureFromDB_HOOKED, const char* texName)
{
    int size = gTextureLookupFns.size();
    for(int i = 0; i < size; ++i)
    {
        void* tex = gTextureLookupFns[i](texName);
        if(tex) return tex;
    }
    
    RwTexture* tex = (RwTexture*)GetTextureFromDB_HOOKED(texName);
    return tex;
}

DECL_HOOKv(RenderEffects)
{
    RenderEffects();
    int size = gRenderOfTypeFns[ROfType_Effects].size();
    for(int i = 0; i < size; ++i)
    {
        gRenderOfTypeFns[ROfType_Effects][i](NULL);
    }
}
DECL_HOOKv(RenderMenu, void* self)
{
    RenderMenu(self);
    int size = gRenderOfTypeFns[ROfType_Menu].size();
    for(int i = 0; i < size; ++i)
    {
        gRenderOfTypeFns[ROfType_Menu][i](self);
    }
}
DECL_HOOKv(RenderPed, void* self)
{
    RenderPed(self);
    int size = gRenderOfTypeFns[ROfType_Ped].size();
    for(int i = 0; i < size; ++i)
    {
        gRenderOfTypeFns[ROfType_Ped][i](self);
    }
}
DECL_HOOKv(RenderVehicle, void* self)
{
    RenderVehicle(self);
    int size = gRenderOfTypeFns[ROfType_Vehicle].size();
    for(int i = 0; i < size; ++i)
    {
        gRenderOfTypeFns[ROfType_Vehicle][i](self);
    }
}
DECL_HOOKv(RenderObject, void* self)
{
    RenderObject(self);
    int size = gRenderOfTypeFns[ROfType_Object].size();
    for(int i = 0; i < size; ++i)
    {
        gRenderOfTypeFns[ROfType_Object][i](self);
    }
}
DECL_HOOKv(DrawRadarBlips, float circleSize)
{
    DrawRadarBlips(circleSize);

    if(gMobileMenu->m_bDrawMenuMap)
    {
        int size = gRenderOfTypeFns[ROfType_MapBlips].size();
        for(int i = 0; i < size; ++i)
        {
            float circSize = circleSize;
            gRenderOfTypeFns[ROfType_MapBlips][i]((void*)&circSize);
        }
    }
    else
    {
        int size = gRenderOfTypeFns[ROfType_RadarBlips].size();
        for(int i = 0; i < size; ++i)
        {
            float circSize = circleSize;
            gRenderOfTypeFns[ROfType_RadarBlips][i]((void*)&circSize);
        }
    }
}
DECL_HOOKv(MainMenuAddItems, FlowScreen* self)
{
    MainMenuAddItems(self);
    curLoc = self->m_bIsPauseScreen ? STB_Pause : STB_MainMenu;
    curScr = self;
    nCurScrItemsOffset = self->items.Count();

    // Custom tabs!
    int size = gMoreSettingButtons[curLoc].size();
    AdditionalSettingsButton* pBtn;
    for(int i = 0; i < size; ++i)
    {
        pBtn = gMoreSettingButtons[curLoc][i];
        if(!pBtn->bUsesMenu)
            AddSettingsButton(self, pBtn->szName, pBtn->szTextureName, OnTabButtonClicked);
    }
    AddSettingsButton(self, "FEP_QUI", "menu_mainquit", OnMainMenuExit); // Bring back "EXIT" button
    // Custom tabs!
}
DECL_HOOKv(StartGameAddItems, FlowScreen* self)
{
    StartGameAddItems(self);
    curScr = self;
    curLoc = STB_StartGame;
    nCurScrItemsOffset = self->items.Count();

    // Custom tabs!
    int size = gMoreSettingButtons[STB_StartGame].size();
    AdditionalSettingsButton* pBtn;
    for(int i = 0; i < size; ++i)
    {
        pBtn = gMoreSettingButtons[STB_StartGame][i];
        if(!pBtn->bUsesMenu)
            AddSettingsButton(self, pBtn->szName, pBtn->szTextureName, OnTabButtonClicked);
    }
    // Custom tabs!
}
bool GeometrySetPrelitConstantColor(RpGeometry* geometry, CRGBA clr)
{
    if((geometry->flags & rpGEOMETRYPRELIT) == 0) return false;
    RpGeometryLock(geometry, 4095);

    RwRGBA* prelitClrPtr = geometry->preLitLum;
    if(prelitClrPtr)
    {
        RwInt32 numOfPrelits = geometry->numVertices;
        if(numOfPrelits)
        {
            memset(prelitClrPtr, clr.val, numOfPrelits * sizeof(RwRGBA));
        }
    }
    
    RpGeometryUnlock(geometry);
    return true;
}



/* !!! OUR SAUTILS INTERFACE IS BELOW !!! */

void SAUtils::InitializeFunctions()
{
    SET_TO(OnRestoreDefaultsFn,         aml->GetSym(pGameHandle, "_ZN12SelectScreen17OnRestoreDefaultsEPS_i"));
    SET_TO(OnRestoreDefaultsAudioFn,    aml->GetSym(pGameHandle, "_ZN11AudioScreen17OnRestoreDefaultsEP12SelectScreeni"));
    SET_TO(GetTextureFromDB,            aml->GetSym(pGameHandle, "_ZN22TextureDatabaseRuntime10GetTextureEPKc"));
    SET_TO(ProcessMenuPending,          aml->GetSym(pGameHandle, "_ZN10MobileMenu14ProcessPendingEv"));
    SET_TO(InitializeMenuPtr,           aml->GetSym(pGameHandle, "_ZN16CharSelectScreenC2EPKcb"));
    SET_TO(LoadTextureDB,               aml->GetSym(pGameHandle, "_ZN22TextureDatabaseRuntime4LoadEPKcb21TextureDatabaseFormat"));
    SET_TO(GetTexDB,                    aml->GetSym(pGameHandle, "_ZN22TextureDatabaseRuntime11GetDatabaseEPKc"));
    SET_TO(RegisterTexDB,               aml->GetSym(pGameHandle, "_ZN22TextureDatabaseRuntime8RegisterEPS_"));
    SET_TO(UnregisterTexDB,             aml->GetSym(pGameHandle, "_ZN22TextureDatabaseRuntime10UnregisterEPS_"));
    SET_TO(CdStreamOpen,                aml->GetSym(pGameHandle, "_Z12CdStreamOpenPKcb"));
    SET_TO(AddSettingsItemFn,           aml->GetSym(pGameHandle, "_ZN12SelectScreen7AddItemEPNS_13MenuSelectionE"));
    SET_TO(AddImageToList,              aml->GetSym(pGameHandle, "_ZN10CStreaming14AddImageToListEPKcb"));
    SET_TO(WidgetButton_Constructor,    aml->GetSym(pGameHandle, "_ZN13CWidgetButtonC2EPKcRK14WidgetPositionjj10HIDMapping"));
    SET_TO(SetSpriteTexture,            aml->GetSym(pGameHandle, "_ZN9CSprite2d10SetTextureEPc"));
    SET_TO(CopyRWTexture,               aml->GetSym(pGameHandle, "_ZN15CClothesBuilder11CopyTextureEP9RwTexture"));
    SET_TO(RwTextureCreate,             aml->GetSym(pGameHandle, "_Z15RwTextureCreateP8RwRaster"));
    SET_TO(RwTextureDestroy,            aml->GetSym(pGameHandle, "_Z16RwTextureDestroyP9RwTexture"));
    SET_TO(RtPNGImageRead,              aml->GetSym(pGameHandle, "_Z14RtPNGImageReadPKc"));
    SET_TO(RtBMPImageRead,              aml->GetSym(pGameHandle, "_Z14RtBMPImageReadPKc"));
    SET_TO(RwImageFindRasterFormat,     aml->GetSym(pGameHandle, "_Z23RwImageFindRasterFormatP7RwImageiPiS1_S1_S1_"));
    SET_TO(RwRasterCreate,              aml->GetSym(pGameHandle, "_Z14RwRasterCreateiiii"));
    SET_TO(RwRasterSetFromImage,        aml->GetSym(pGameHandle, "_Z20RwRasterSetFromImageP8RwRasterP7RwImage"));
    SET_TO(RwImageDestroy,              aml->GetSym(pGameHandle, "_Z14RwImageDestroyP7RwImage"));
    
    SET_TO(Widget_IsTouched,            aml->GetSym(pGameHandle, "_ZN7CWidget9IsTouchedEP9CVector2D"));
    SET_TO(Widget_IsDoubleTapped,       aml->GetSym(pGameHandle, "_ZN7CWidget14IsDoubleTappedEv"));
    SET_TO(Widget_IsReleased,           aml->GetSym(pGameHandle, "_ZN7CWidget10IsReleasedEP9CVector2D"));
    SET_TO(Widget_IsSwipedLeft,         aml->GetSym(pGameHandle, "_ZN7CWidget12IsSwipedLeftEv"));
    SET_TO(Widget_IsSwipedRight,        aml->GetSym(pGameHandle, "_ZN7CWidget13IsSwipedRightEv"));
    SET_TO(Widget_IsSwipedUp,           aml->GetSym(pGameHandle, "_ZN7CWidget10IsSwipedUpEv"));
    SET_TO(Widget_IsSwipedDown,         aml->GetSym(pGameHandle, "_ZN7CWidget12IsSwipedDownEv"));
    SET_TO(Touch_IsWidgetTouched,       aml->GetSym(pGameHandle, "_ZN15CTouchInterface9IsTouchedENS_9WidgetIDsEP9CVector2Di"));
    SET_TO(Touch_IsWidgetDoubleTapped,  aml->GetSym(pGameHandle, "_ZN15CTouchInterface14IsDoubleTappedENS_9WidgetIDsEbi"));
    SET_TO(Touch_IsWidgetReleased,      aml->GetSym(pGameHandle, "_ZN15CTouchInterface10IsReleasedENS_9WidgetIDsEP9CVector2Di"));
    SET_TO(Touch_IsWidgetSwipedLeft,    aml->GetSym(pGameHandle, "_ZN15CTouchInterface12IsSwipedLeftENS_9WidgetIDsEi"));
    SET_TO(Touch_IsWidgetSwipedRight,   aml->GetSym(pGameHandle, "_ZN15CTouchInterface13IsSwipedRightENS_9WidgetIDsEi"));
    SET_TO(Touch_IsWidgetSwipedUp,      aml->GetSym(pGameHandle, "_ZN15CTouchInterface10IsSwipedUpENS_9WidgetIDsEi"));
    SET_TO(Touch_IsWidgetSwipedDown,    aml->GetSym(pGameHandle, "_ZN15CTouchInterface12IsSwipedDownENS_9WidgetIDsEi"));

    SET_TO(RwStreamOpen,                aml->GetSym(pGameHandle, "_Z12RwStreamOpen12RwStreamType18RwStreamAccessTypePKv"));
    SET_TO(RwStreamFindChunk,           aml->GetSym(pGameHandle, "_Z17RwStreamFindChunkP8RwStreamjPjS1_"));
    SET_TO(RpClumpStreamRead,           aml->GetSym(pGameHandle, "_Z17RpClumpStreamReadP8RwStream"));
    SET_TO(RwStreamClose,               aml->GetSym(pGameHandle, "_Z13RwStreamCloseP8RwStreamPv"));
    SET_TO(GetFirstAtomic,              aml->GetSym(pGameHandle, "_Z14GetFirstAtomicP7RpClump"));
    SET_TO(SetFilterModeOnAtomicsTextures, aml->GetSym(pGameHandle, "_Z30SetFilterModeOnAtomicsTexturesP8RpAtomic19RwTextureFilterMode"));
    SET_TO(RpGeometryLock,              aml->GetSym(pGameHandle, "_Z14RpGeometryLockP10RpGeometryi"));
    SET_TO(RpGeometryUnlock,            aml->GetSym(pGameHandle, "_Z16RpGeometryUnlockP10RpGeometry"));
    SET_TO(RpGeometryForAllMaterials,   aml->GetSym(pGameHandle, "_Z25RpGeometryForAllMaterialsP10RpGeometryPFP10RpMaterialS2_PvES3_"));
    SET_TO(RpMaterialSetTexture,        aml->GetSym(pGameHandle, "_Z20RpMaterialSetTextureP10RpMaterialP9RwTexture"));
    SET_TO(RpAtomicClone,               aml->GetSym(pGameHandle, "_Z13RpAtomicCloneP8RpAtomic"));
    SET_TO(RpClumpDestroy,              aml->GetSym(pGameHandle, "_Z14RpClumpDestroyP7RpClump"));
    SET_TO(RwFrameCreate,               aml->GetSym(pGameHandle, "_Z13RwFrameCreatev"));
    SET_TO(RpAtomicSetFrame,            aml->GetSym(pGameHandle, "_Z16RpAtomicSetFrameP8RpAtomicP7RwFrame"));
    SET_TO(RenderAtomicWithAlpha,       aml->GetSym(pGameHandle, "_ZN18CVisibilityPlugins21RenderAtomicWithAlphaEP8RpAtomici"));
    SET_TO(RpGeometryCreate,            aml->GetSym(pGameHandle, "_Z16RpGeometryCreateiij"));
    SET_TO(RpGeometryTriangleGetMaterial, aml->GetSym(pGameHandle, "_Z29RpGeometryTriangleGetMaterialPK10RpGeometryPK10RpTriangle"));
    SET_TO(RpGeometryTriangleSetMaterial, aml->GetSym(pGameHandle, "_Z29RpGeometryTriangleSetMaterialP10RpGeometryP10RpTriangleP10RpMaterial"));
    SET_TO(RpAtomicSetGeometry,         aml->GetSym(pGameHandle, "_Z19RpAtomicSetGeometryP8RpAtomicP10RpGeometryj"));
    SET_TO(AtomicDefaultRenderCallBack, aml->GetSym(pGameHandle, "_Z27AtomicDefaultRenderCallBackP8RpAtomic"));
    SET_TO(OnMainMenuExit,              aml->GetSym(pGameHandle, "_ZN14MainMenuScreen6OnExitEv"));

    SET_TO(gxtErrorString,              aml->GetSym(pGameHandle, "GxtErrorString"));
    SET_TO(gMobileMenu,                 aml->GetSym(pGameHandle, "gMobileMenu"));
    SET_TO(m_snTimeInMilliseconds,      aml->GetSym(pGameHandle, "_ZN6CTimer22m_snTimeInMillisecondsE"));
    SET_TO(game_FPS,                    aml->GetSym(pGameHandle, "_ZN6CTimer8game_FPSE"));
    SET_TO(orgWidgetsPtr,               aml->GetSym(pGameHandle, "_ZN15CTouchInterface10m_pWidgetsE"));
    
    SET_TO(_ZTVN12SelectScreen15ActionSelectionE, aml->GetSym(pGameHandle, "_ZTVN12SelectScreen15ActionSelectionE"));
    _ZTVN12SelectScreen15ActionSelectionE += 2*sizeof(void*);
    SET_TO(_ZTVN12SelectScreen16SettingSelectionE, aml->GetSym(pGameHandle, "_ZTVN12SelectScreen16SettingSelectionE"));
    _ZTVN12SelectScreen16SettingSelectionE += 2*sizeof(void*);
    SET_TO(_ZTV13DisplayScreen, aml->GetSym(pGameHandle, "_ZTV13DisplayScreen"));
    _ZTV13DisplayScreen += 2*sizeof(void*);

    // And also variables
    SET_TO(ms_pPedPool,                 aml->GetSym(pGameHandle, "_ZN6CPools11ms_pPedPoolE"));
    SET_TO(ms_pObjectPool,              aml->GetSym(pGameHandle, "_ZN6CPools14ms_pObjectPoolE"));
    SET_TO(ms_pVehiclePool,             aml->GetSym(pGameHandle, "_ZN6CPools15ms_pVehiclePoolE"));
}

void SAUtils::InitializeSAUtils()
{
    // Freak ya FLA and your sh*t
    if(m_pHasFLA == 0 || !memcmp((void*)(aml->GetSym(pGameHandle, "_ZN10CStreaming13InitImageListEv") & ~0x1), (void*)"\xF0\xB5\x03\xAF\x4D\xF8", 6))
    {
        aml->Unprot(pGameLib + 0x676AC4, sizeof(void*));
        *(uintptr_t*)(pGameLib + 0x676AC4) = (uintptr_t)pNewStreamingFiles;
        aml->Unprot(pGameLib + 0x46BD78, sizeof(char));
        *(unsigned char*)(pGameLib + 0x46BD78) = (unsigned char)MAX_IMG_ARCHIVES+2;
        aml->Unprot(pGameLib + 0x46BFE4, sizeof(char)); 
        *(unsigned char*)(pGameLib + 0x46BFE4) = (unsigned char)MAX_IMG_ARCHIVES+2;
        aml->Redirect(aml->GetSym(pGameHandle, "_ZN10CStreaming14AddImageToListEPKcb"), (uintptr_t)AddImageToListPatched);

        logger->Info("IMG limit has been bumped!");
    }

    // Bump settings limit
    aml->Unprot(pGameLib + 0x679A40, sizeof(void*));
    *(uintptr_t*)(pGameLib + 0x679A40) = (uintptr_t)pNewSettings;
    memcpy(pNewSettings, (int*)(pGameLib + 0x6E03F4), 37 * sizeof(MobileSettings::Setting));

    // Bump widgets limit
    aml->Unprot(pGameLib + 0x67947C, sizeof(void*)); *(uintptr_t*)(pGameLib + 0x67947C)     = (uintptr_t)pNewWidgets;
    aml->Unprot(pGameLib + 0x2AE58E, sizeof(char));  *(unsigned char*)(pGameLib + 0x2AE58E) = (unsigned char)MAX_WIDGETS; // Create all
    aml->Unprot(pGameLib + 0x2AFBC0, sizeof(char));  *(unsigned char*)(pGameLib + 0x2AFBC0) = (unsigned char)MAX_WIDGETS; // Delete all
    aml->Unprot(pGameLib + 0x2B0B32, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B0B32) = (unsigned char)MAX_WIDGETS; // Update
    aml->Unprot(pGameLib + 0x2B0B50, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B0B50) = (unsigned char)MAX_WIDGETS; // Update
    aml->Unprot(pGameLib + 0x2B0C8C, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B0C8C) = (unsigned char)MAX_WIDGETS-1; // Visualize all
    aml->Unprot(pGameLib + 0x2B05B2, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B05B2) = (unsigned char)MAX_WIDGETS-1; // Clear
    aml->Unprot(pGameLib + 0x2B0644, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B0644) = (unsigned char)MAX_WIDGETS-1; // Clear
    aml->Unprot(pGameLib + 0x2B0748, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B0748) = (unsigned char)MAX_WIDGETS-1; // Clear
    aml->Unprot(pGameLib + 0x2B0986, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B0986) = (unsigned char)MAX_WIDGETS-1; // Clear
    aml->Unprot(pGameLib + 0x2B07D2, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B07D2) = (unsigned char)MAX_WIDGETS; // Clear
    aml->Unprot(pGameLib + 0x2B087E, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B087E) = (unsigned char)MAX_WIDGETS; // Clear
    aml->Unprot(pGameLib + 0x2B0C34, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B0C34) = (unsigned char)MAX_WIDGETS; // Draw All
    aml->Unprot(pGameLib + 0x2B28E8, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B28E8) = (unsigned char)MAX_WIDGETS-1; // AnyWidgetsUsingAltBack
    HOOKPLT(CreateAllWidgets, pGameLib + 0x6734E4);

    // Hook functions
    HOOKPLT(AsciiToGxtChar,             pGameLib + 0x6724F8);
    HOOKPLT(SelectScreenOnDestroy,      pGameLib + 0x673FD8);
    HOOKPLT(SettingSelectionRender,     pGameLib + 0x662850);
    HOOKPLT(GxtTextGet,                 pGameLib + 0x66E78C);
    HOOKPLT(SettingsScreen_Construct,   pGameLib + 0x674018);
    HOOKPLT(InitialiseRenderWare,       pGameLib + 0x66F2D0);
    HOOKPLT(InitialiseGame_SecondPass,  pGameLib + 0x672178);
    HOOKPLT(PlayerProcess,              pGameLib + 0x673E84);
    HOOK(RenderEffects,                 aml->GetSym(pGameHandle, "_Z13RenderEffectsv"));
    HOOKPLT(RenderMenu,                 pGameLib + 0x674254);
    HOOK(RenderPed,                     aml->GetSym(pGameHandle, "_ZN4CPed6RenderEv"));
    HOOK(RenderVehicle,                 aml->GetSym(pGameHandle, "_ZN8CVehicle6RenderEv"));
    HOOK(RenderObject,                  aml->GetSym(pGameHandle, "_ZN7CObject6RenderEv"));
    HOOKPLT(DrawRadarBlips,             pGameLib + 0x66E910);
    HOOKPLT(GetTextureFromDB_HOOKED,    pGameLib + 0x674708);
    HOOKPLT(MainMenuAddItems,           pGameLib + 0x67379C);
    //HOOK(StartGameAddItems,             aml->GetSym(pGameHandle, "_ZN14MainMenuScreen11OnStartGameEv"));
    HOOKPLT(StartGameAddItems,          pGameLib + 0x662AD8); // vtable fn

    // Hooked settings functions
    aml->Redirect(pGameLib + 0x29E6AA + 0x1, (uintptr_t)NewScreen_Controls_stub); NewScreen_Controls_backto = pGameLib + 0x29E6D2 + 0x1;
    aml->Redirect(pGameLib + 0x2A49F6 + 0x1, (uintptr_t)NewScreen_Game_stub);     NewScreen_Game_backto = pGameLib + 0x2A4A1E + 0x1;
    aml->Redirect(pGameLib + 0x2A4BD4 + 0x1, (uintptr_t)NewScreen_Display_stub);  NewScreen_Display_backto = pGameLib + 0x2A4BFC + 0x1;
    aml->Redirect(pGameLib + 0x2A4D3C + 0x1, (uintptr_t)NewScreen_Audio_stub);    NewScreen_Audio_backto = pGameLib + 0x2A4D64 + 0x1;
    HOOKPLT(NewScreen_Language,         pGameLib + 0x675D90);

    // Slider drawing hook
    //Redirect(pGameLib + 0x299660 + 0x1, (uintptr_t)DrawSlider_stub); DrawSlider_backto = pGameLib + 0x29967C + 0x1;
    //aml->Write(pGameLib + 0x29967C,     (uintptr_t)"\x0F\xA9", 2);

    InitializeFunctions();
    SET_TO(WorldPlayers,                *(void**)(pGameLib + 0x6783C8));
    
    // Remove an "EXIT" button from MainMenu (to set it manually)
    aml->PlaceB(pGameLib + 0x29BEBE + 0x1, pGameLib + 0x29BF48 + 0x1);

    // Scripting
    InitializeSAScripting();
}
void SAUtils::InitializeVCUtils()
{
    gxtErrorString = (unsigned short*)(pGameLib + 0x716C2C);

    //aml->Unprot(pGameLib + 0x679A40, sizeof(void*));
    //*(uintptr_t*)(pGameLib + 0x679A40) = (uintptr_t)pNewSettings;
    //memcpy(pNewSettings, (int*)(pGameLib + 0x6E03F4), 1184);

    HOOK(GxtTextGet, aml->GetSym(pGameHandle, "_ZN5CText3GetEPKc"));
    HOOK(AsciiToGxtChar, aml->GetSym(pGameHandle, "_Z14AsciiToUnicodePKcPt"));
    //HOOKPLT(NewScreen_Controls, pGameLib + 0x675CD8);
    //HOOKPLT(NewScreen_Game, pGameLib + 0x674310);
    //HOOKPLT(NewScreen_Display, pGameLib + 0x675150);
    //HOOKPLT(NewScreen_Audio, pGameLib + 0x66FBA4);
    //HOOKPLT(NewScreen_Language, pGameLib + 0x675D90);
    //HOOKPLT(SelectScreenAddItem, pGameLib + 0x674518);
    HOOK(SelectScreenOnDestroy, aml->GetSym(pGameHandle, "_ZN12CMenuManager4BackEv"));

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
    strncpy(pNew->szName, name, 32);
    pNew->fnOnValueChange = fnOnValueChange;
    pNew->byteItemType = isSlider ? ItemType_Slider : ItemType_WithItems;
    pNew->nInitVal = (int)initVal;
    pNew->nSavedVal = (int)initVal;
    pNew->nMaxVal = maxVal;
    pNew->pOwnData = NULL;
    gMoreSettings.push_back(pNew);

    pNewSettings[nNextSettingNum].values = (const char**)switchesArray; // Items of that setting
    pNewSettings[nNextSettingNum].value = initVal; // Current value
    pNewSettings[nNextSettingNum].defaultValue = initVal;
    pNewSettings[nNextSettingNum].min = minVal; // Min slider value (min is -2millions) OR min count of items (keep it 0 maybe, if u dont need others)
    pNewSettings[nNextSettingNum].max = maxVal; // Max slider value (max is 2millions) OR max count-1 of items
    pNewSettings[nNextSettingNum].type = isSlider ? MST_Range : MST_Toggle; // Declare it as a slider (flags???)

    return nNextSettingNum;
}

int SAUtils::ValueOfSettingsItem(int settingId)
{
    if(settingId < 0 || settingId > nNextSettingNum) return 0;
    return pNewSettings[settingId].value;
}

// 1.1

int SAUtils::AddClickableItem(eTypeOfSettings typeOf, const char* name, int initVal, int minVal, int maxVal, const char** switchesArray, OnSettingChangedFn fnOnValueChange)
{
    return AddClickableItem(typeOf, name, initVal, minVal, maxVal, switchesArray, fnOnValueChange, NULL);
}
int SAUtils::AddClickableItem(eTypeOfSettings typeOf, const char* name, int initVal, int minVal, int maxVal, const char** switchesArray, OnSettingChangedFn fnOnValueChange, void* data)
{
    if(nNextSettingNum >= MAX_SETTINGS) return -1;

    ++nNextSettingNum;
    AdditionalSetting* pNew = new AdditionalSetting;
    pNew->nSettingId = nNextSettingNum;
    pNew->eType = typeOf;
    strncpy(pNew->szName, name, 32);
    pNew->fnOnValueChange = fnOnValueChange;
    pNew->fnOnValueDraw = NULL;
    pNew->fnOnButtonPressed = NULL;
    pNew->byteItemType = ItemType_WithItems;
    pNew->nInitVal = (int)initVal;
    pNew->nSavedVal = (int)initVal;
    pNew->nMaxVal = maxVal;
    pNew->pOwnData = data;
    gMoreSettings.push_back(pNew);

    pNewSettings[nNextSettingNum].values = switchesArray;
    pNewSettings[nNextSettingNum].value = initVal;
    pNewSettings[nNextSettingNum].defaultValue = initVal;
    pNewSettings[nNextSettingNum].min = minVal;
    pNewSettings[nNextSettingNum].max = maxVal;
    pNewSettings[nNextSettingNum].type = MST_Toggle;

    return nNextSettingNum;
}

int SAUtils::AddSliderItem(eTypeOfSettings typeOf, const char* name, int initVal, int minVal, int maxVal, OnSettingChangedFn fnOnValueChange, OnSettingDrawedFn fnOnValueDraw)
{
    return AddSliderItem(typeOf, name, initVal, minVal, maxVal, fnOnValueChange, fnOnValueDraw, NULL);
}
int SAUtils::AddSliderItem(eTypeOfSettings typeOf, const char* name, int initVal, int minVal, int maxVal, OnSettingChangedFn fnOnValueChange, OnSettingDrawedFn fnOnValueDraw, void* data)
{
    if(nNextSettingNum >= MAX_SETTINGS) return -1;

    ++nNextSettingNum;
    AdditionalSetting* pNew = new AdditionalSetting;
    pNew->nSettingId = nNextSettingNum;
    pNew->eType = typeOf;
    strncpy(pNew->szName, name, 32);
    pNew->fnOnValueChange = fnOnValueChange;
    pNew->fnOnValueDraw = fnOnValueDraw;
    pNew->fnOnButtonPressed = NULL;
    pNew->byteItemType = ItemType_Slider;
    pNew->nInitVal = (int)initVal;
    pNew->nSavedVal = (int)initVal;
    pNew->nMaxVal = maxVal;
    pNew->pOwnData = data;
    gMoreSettings.push_back(pNew);

    pNewSettings[nNextSettingNum].values = NULL;
    pNewSettings[nNextSettingNum].value = initVal;
    pNewSettings[nNextSettingNum].defaultValue = initVal;
    pNewSettings[nNextSettingNum].min = minVal;
    pNewSettings[nNextSettingNum].max = maxVal;
    pNewSettings[nNextSettingNum].type = MST_Range;

    return nNextSettingNum;
}

// 1.2
void SAUtils::AddButton(eTypeOfSettings typeOf, const char* name, OnButtonPressedFn fnOnButtonPressed)
{
    AdditionalSetting* pNew = new AdditionalSetting;
    pNew->nSettingId = -1;
    pNew->eType = typeOf;
    strncpy(pNew->szName, name, 32);
    pNew->fnOnValueChange = NULL;
    pNew->fnOnValueDraw = NULL;
    pNew->fnOnButtonPressed = fnOnButtonPressed;
    pNew->byteItemType = ItemType_Button;
    pNew->nInitVal = 0;
    pNew->nSavedVal = 0;
    pNew->nMaxVal = 0;
    gMoreSettings.push_back(pNew);
}

uintptr_t* SAUtils::AddTextureDB(const char* name, bool registerMe)
{
    if(!name || !name[0]) return NULL;
    AdditionalTexDB* pNew = new AdditionalTexDB;
    strncpy(pNew->szName, name, 32);
    pNew->bRegister = registerMe;
    pNew->cacheType = 6;
    pNew->nDBPointer = 0;
    gMoreTexDBs.push_back(pNew);

    if(g_bIsGameStartedAlready)
    {
        pNew->nDBPointer = LoadTextureDB(name, false, 6);
        if(pNew->nDBPointer != 0 && registerMe) RegisterTexDB(pNew->nDBPointer);
    }
    return &pNew->nDBPointer;
}

int* SAUtils::GetSettingValuePointer(int settingId)
{
    return &pNewSettings[nNextSettingNum].value;
}

void SAUtils::AddIMG(const char* imgName)
{
    if(!imgName || !imgName[0]) return;
    gMoreIMGs.push_back(imgName);

    if(g_bIsGameStartedAlready) AddImageToList(imgName, false);
}

// 1.3
static unsigned char nTabsIdCount = SETTINGS_COUNT - 1;
eTypeOfSettings SAUtils::AddSettingsTab(const char* name, const char* textureName)
{
    AdditionalSettingsButton* pNew = new AdditionalSettingsButton;
    strncpy(pNew->szName, name, 32);
    strncpy(pNew->szTextureName, textureName, 32);
    pNew->bUsesMenu = true;
    gMoreSettingButtons[STB_Settings].push_back(pNew);
    return (eTypeOfSettings)(++nTabsIdCount);
}

unsigned int SAUtils::GetCurrentMs()
{
    return *m_snTimeInMilliseconds;
}

float SAUtils::GetCurrentFPS()
{
    return *game_FPS;
}

uintptr_t SAUtils::GetTextureDB(const char* texDbName)
{
    return GetTexDB(texDbName);
}

void SAUtils::RegisterTextureDB(uintptr_t textureDbPtr)
{
    RegisterTexDB(textureDbPtr);
}

void SAUtils::UnregisterTextureDB(uintptr_t textureDbPtr)
{
    UnregisterTexDB(textureDbPtr);
}

uintptr_t SAUtils::GetTexture(const char* texName)
{
    return (uintptr_t)GetTextureFromDB(texName);
}

void SAUtils::AddOnWidgetsCreateListener(SimpleFn fn)
{
    if(fn == NULL) return;
    gCreateWidgetFns.push_back(fn);
}

void SAUtils::AddPlayerUpdateListener(OnPlayerProcessFn fn, bool post)
{
    if(fn == NULL) return;
    if(post) gPlayerUpdatePostFns.push_back(fn);
    else     gPlayerUpdateFns.push_back(fn);
}

int SAUtils::FindFreeWidgetId()
{
    for(int i = MAX_WIDGETS_GAME; i < MAX_WIDGETS; ++i)
    {
        if(pNewWidgets[i] == NULL) return i;
    }
    return -1;
}

CWidgetButton* SAUtils::CreateWidget(int widgetId, int x, int y, float scale, const char* textureName)
{
    if(widgetId >= MAX_WIDGETS ||
       widgetId < WIDGETID_MAX ||
       pNewWidgets[widgetId] != NULL) return NULL;

    CWidgetButton* widget = New<CWidgetButton>();
    WidgetButton_Constructor(widget, textureName, WidgetPosition(x, y, scale), 1, 0, HID_MAPPING_UNKNOWN);
    pNewWidgets[widgetId] = widget;

    return widget;
}

int SAUtils::GetWidgetIndex(CWidgetButton* widget)
{
    for(int i = MAX_WIDGETS_GAME; i < MAX_WIDGETS; ++i)
        if(pNewWidgets[i] == widget) return i;
    return -1;
}

void SAUtils::SetWidgetIcon(CWidgetButton* widget, uintptr_t texturePtr)
{
    if(widget->widgetSprite != NULL)
    {
        RwTextureDestroy(widget->widgetSprite->m_pTexture);
        widget->widgetSprite->m_pTexture = NULL;
    }

    if(texturePtr != 0)
    {
        RwTexture* org = (RwTexture*)texturePtr;
        RwTexture* tex = CopyRWTexture(org);
        tex->filterAddressing = org->filterAddressing;
        widget->widgetSprite->m_pTexture = tex;
    }
}

void SAUtils::SetWidgetIcon(CWidgetButton* widget, const char* textureName)
{
    SetSpriteTexture(widget->widgetSprite, textureName);
}

void SAUtils::SetWidgetIcon2(CWidgetButton* widget, uintptr_t texturePtr)
{
    if(widget->additionalSprite != NULL)
    {
        RwTextureDestroy(widget->additionalSprite->m_pTexture);
        widget->additionalSprite->m_pTexture = NULL;
    }

    if(texturePtr != 0)
    {
        RwTexture* org = (RwTexture*)texturePtr;
        RwTexture* tex = CopyRWTexture(org);
        tex->filterAddressing = org->filterAddressing;
        widget->additionalSprite->m_pTexture = tex;
    }
}

void SAUtils::SetWidgetIcon2(CWidgetButton* widget, const char* textureName)
{
    SetSpriteTexture(widget->additionalSprite, textureName);
}

void SAUtils::ToggleWidget(CWidgetButton* widget, bool enable)
{
    widget->enabled = enable;
}

bool SAUtils::IsWidgetEnabled(int widgetId)
{
    return pNewWidgets[widgetId]->enabled;
}

void SAUtils::ClearWidgetTapHistory(int widgetId)
{
    memset(pNewWidgets[widgetId]->tapTimes, 0, sizeof(float)*10);
}

void SAUtils::ClearWidgetTapHistory(CWidgetButton* widget)
{
    memset(widget->tapTimes, 0, sizeof(float)*10);
}

int SAUtils::GetWidgetState(CWidgetButton* widget, eWidgetState stateToGet)
{
    if(!widget->enabled) return false;
    switch(stateToGet)
    {
        default: return false;

        case WState_Touched:
            return Widget_IsTouched(widget);

        case WState_Released:
            return Widget_IsReleased(widget);

        case WState_DoubleTapped:
            return Widget_IsDoubleTapped(widget);

        case WState_SwipedLeft:
            return Widget_IsSwipedLeft(widget);

        case WState_SwipedRight:
            return Widget_IsSwipedRight(widget);

        case WState_SwipedUp:
            return Widget_IsSwipedUp(widget);

        case WState_SwipedDown:
            return Widget_IsSwipedDown(widget);
    }
}

int SAUtils::GetWidgetState(int widgetId, eWidgetState stateToGet, bool doDoubleTapEff, int frames)
{
    if(!pNewWidgets[widgetId]->enabled) return false;
    switch(stateToGet)
    {
        default: return false;

        case WState_Touched:
            return Touch_IsWidgetTouched(widgetId, NULL, frames);

        case WState_Released:
            return Touch_IsWidgetReleased(widgetId, NULL, frames);

        case WState_DoubleTapped:
            return Touch_IsWidgetDoubleTapped(widgetId, doDoubleTapEff, frames);

        case WState_SwipedLeft:
            return Touch_IsWidgetSwipedLeft(widgetId, frames);

        case WState_SwipedRight:
            return Touch_IsWidgetSwipedRight(widgetId, frames);

        case WState_SwipedUp:
            return Touch_IsWidgetSwipedUp(widgetId, frames);

        case WState_SwipedDown:
            return Touch_IsWidgetSwipedDown(widgetId, frames);
    }
}

void SAUtils::GetWidgetPos(int widgetId, float* x, float* y, float* sx, float* sy)
{
    WidgetPosition& p  = pNewWidgets[widgetId]->posScale;
    if(x != NULL)  *x  = p.pos.x;
    if(y != NULL)  *y  = p.pos.y;
    if(sx != NULL) *sx = p.scale.x;
    if(sy != NULL) *sy = p.scale.y;
}

void SAUtils::SetWidgetPos(int widgetId, float x, float y, float sx, float sy)
{
    if(widgetId >= MAX_WIDGETS || widgetId < WIDGETID_MAX) return;
    WidgetPosition& p = pNewWidgets[widgetId]->posScale;
    p.pos.x = x;    p.pos.y = y;
    p.scale.x = sx; p.scale.y = sy;
}

void* SAUtils::LoadRwTextureFromPNG(const char* fn)
{
    RwTexture* pTexture = NULL;
    if (RwImage* pImage = RtPNGImageRead(fn))
    {
        int width, height, depth, flags;
        RwImageFindRasterFormat(pImage, rwRASTERTYPETEXTURE, &width, &height, &depth, &flags);
        if (RwRaster* pRaster = RwRasterCreate(width, height, depth, flags))
        {
            RwRasterSetFromImage(pRaster, pImage);
            pTexture = RwTextureCreate(pRaster);
        }
        RwImageDestroy(pImage);
    }
    return pTexture;
}

void* SAUtils::LoadRwTextureFromBMP(const char* fn)
{
    RwTexture* pTexture = NULL;
    if(RwImage* pImage = RtBMPImageRead(fn))
    {
        int width, height, depth, flags;
        RwImageFindRasterFormat(pImage, rwRASTERTYPETEXTURE, &width, &height, &depth, &flags);
        if(RwRaster* pRaster = RwRasterCreate(width, height, depth, flags | 0x4))
        {
            RwRasterSetFromImage(pRaster, pImage);
            pTexture = RwTextureCreate(pRaster);
        }
        RwImageDestroy(pImage);
    }
    return pTexture;
}

void SAUtils::AddOnRWInitListener(SimpleFn fn)
{
    gInitRWFns.push_back(fn);
}

void SAUtils::AddTextureLookupListener(LookingForTextureFn fn)
{
    gTextureLookupFns.push_back(fn);
}

int ScriptSACommandInner(const SCRIPT_COMMAND *pScriptCommand, va_list ap);
int SAUtils::ScriptCommand(const SCRIPT_COMMAND *pScriptCommand, ...)
{
    va_list ap;
    va_start(ap, pScriptCommand);
    int ret = ScriptSACommandInner(pScriptCommand, ap);
    va_end(ap);
    return ret;
}

void SAUtils::AddOnRenderListener(eRenderOfType typeOf, SimpleDataFn fn)
{
    if(typeOf >= RENDEROFTYPE_MAX) return;
    gRenderOfTypeFns[typeOf].push_back(fn);
}

// 1.4.1
void SAUtils::AddSettingsTabButton(const char* name, SimpleDataFn fn, eSettingsTabButtonLoc loc, const char* textureName, void* data)
{
    AdditionalSettingsButton* pNew = new AdditionalSettingsButton;
    strncpy(pNew->szName, name, 32);
    strncpy(pNew->szTextureName, textureName, 32);
    pNew->bUsesMenu = false;
    pNew->pMenuData = data;
    pNew->fnButtonPressed = fn;
    pNew->nBtnLoc = loc;
    gMoreSettingButtons[loc].push_back(pNew);
    if(loc == STB_Settings) ++nTabsIdCount;
}

bool SAUtils::LoadDFF(const char* name, bool doPrelit, RpAtomic** atomic, RwFrame** frame)
{
    RpClump* clump = NULL;
    RwStream* stream = RwStreamOpen(2, 1, name);
    if(!stream || !RwStreamFindChunk(stream, 16, 0, 0)) return false;
    clump = RpClumpStreamRead(stream);
    RwStreamClose(stream, 0);
    if(!clump) return false;

    RpAtomic* FirstAtomic = GetFirstAtomic(clump);
    if(!FirstAtomic) return false;
    //SetFilterModeOnAtomicsTextures(FirstAtomic, 4);

    if(doPrelit) GeometrySetPrelitConstantColor(FirstAtomic->geometry, rgbaWhite);

    RpAtomic* ClonedAtomic = RpAtomicClone(FirstAtomic);
    RwFrame* mdlFrame = RwFrameCreate();

    RpClumpDestroy(clump);
    RpAtomicSetFrame(ClonedAtomic, mdlFrame);
    ClonedAtomic->renderCallBack = AtomicDefaultRenderCallBack;

    if(atomic) *atomic = ClonedAtomic;
    if(frame) *frame = mdlFrame;

    return true;
}

eLoadedGame SAUtils::GetLoadedGame()
{
    return m_eLoadedGame;
}

uintptr_t SAUtils::GetLoadedGameLibAddress()
{
    return pGameLib;
}

// 1.5.1
uintptr_t* SAUtils::AddTextureDBOfType(const char* name, eTexDBType type, bool registerMe)
{
    if(type >= TEXDBTYPES_MAX || !name || !name[0]) return NULL;
    AdditionalTexDB* pNew = new AdditionalTexDB;
    strncpy(pNew->szName, name, 32);
    pNew->bRegister = registerMe;
    switch(type)
    {
        case TEXDBTYPE_ETC: pNew->cacheType = 5; break;
        case TEXDBTYPE_PVR: pNew->cacheType = 4; break;
        default: pNew->cacheType = 1; break; // DXT
    }
    pNew->nDBPointer = 0;
    gMoreTexDBs.push_back(pNew);

    if(g_bIsGameStartedAlready)
    {
        pNew->nDBPointer = LoadTextureDB(name, false, pNew->cacheType);
        if(pNew->nDBPointer != 0 && registerMe) RegisterTexDB(pNew->nDBPointer);
    }
    return &pNew->nDBPointer;
}

int SAUtils::GetPoolSize(ePoolType poolType)
{
    switch(poolType)
    {
        case POOLTYPE_PEDS:
            if(!*ms_pPedPool) return 0;
            return (*ms_pPedPool)->m_nSize;
        case POOLTYPE_VEHICLES:
            if(!*ms_pVehiclePool) return 0;
            return (*ms_pVehiclePool)->m_nSize;
        case POOLTYPE_OBJECTS:
            if(!*ms_pObjectPool) return 0;
            return (*ms_pObjectPool)->m_nSize;

        default: return 0;
    }
}

int SAUtils::GetPoolMemSize(ePoolType poolType)
{
    switch(poolType)
    {
        case POOLTYPE_PEDS:
            if(!*ms_pPedPool) return 0;
            return (*ms_pPedPool)->m_nSize * (*ms_pPedPool)->GetObjectSize();
        case POOLTYPE_VEHICLES:
            if(!*ms_pVehiclePool) return 0;
            return (*ms_pVehiclePool)->m_nSize * (*ms_pVehiclePool)->GetObjectSize();
        case POOLTYPE_OBJECTS:
            if(!*ms_pObjectPool) return 0;
            return (*ms_pObjectPool)->m_nSize * (*ms_pObjectPool)->GetObjectSize();

        default: return 0;
    }
}

int SAUtils::GetPoolIndex(void* entityPtr)
{
    if(!entityPtr) return -1;
    CEntity* ent = (CEntity*)entityPtr;
    switch(ent->m_nType)
    {
        default: return -1;

        case ENTITY_TYPE_PED:     return ((*ms_pPedPool)->IsFromObjectArray((CPed*)ent)) ? (*ms_pPedPool)->GetIndex((CPed*)ent) : -1;
        case ENTITY_TYPE_OBJECT:  return ((*ms_pObjectPool)->IsFromObjectArray((CObject*)ent)) ? (*ms_pObjectPool)->GetIndex((CObject*)ent) : -1;
        case ENTITY_TYPE_VEHICLE: return ((*ms_pVehiclePool)->IsFromObjectArray((CVehicle*)ent)) ? (*ms_pVehiclePool)->GetIndex((CVehicle*)ent) : -1;
    }
}

static SAUtils sautilsLocal;
ISAUtils* sautils = &sautilsLocal;
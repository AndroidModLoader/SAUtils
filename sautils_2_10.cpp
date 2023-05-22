#include <mod/amlmod.h>
#include <mod/logger.h>
#include <dlfcn.h>
#include <sautils_2_10.h>
#include <stdint.h>
#include <vector>
#include <cstring> // memcpy, memcmp

#include "GTASA_STRUCTS.h"

MYMODDECL();
extern uintptr_t pGameLib;
extern void* pGameHandle;

// ScriptingRelated

void InitializeSAScripting();

extern DECL_HOOKv(CreateAllWidgets);
extern DECL_HOOK(unsigned short*, AsciiToGxtChar, const char* txt, unsigned short* ret);
extern DECL_HOOK(void, SelectScreenOnDestroy, void* self);
extern DECL_HOOK(void, SettingSelectionRender, SelectScreen::SettingSelection* self, float a1, float a2, float a3, float a4, float a5, float a6);
extern DECL_HOOK(unsigned short*, GxtTextGet, void* self, const char* txt);
extern DECL_HOOK(SettingsScreen*, SettingsScreen_Construct, SettingsScreen* self);
extern DECL_HOOKv(InitialiseRenderWare);
extern DECL_HOOKv(InitialiseGame_SecondPass);
extern DECL_HOOKv(PlayerProcess, CPlayerInfo* self, uint32_t playerIndex);
extern DECL_HOOK(void*, GetTextureFromDB_HOOKED, const char* texName);
extern DECL_HOOKv(RenderEffects);
extern DECL_HOOKv(RenderMenu, void* self);
extern DECL_HOOKv(RenderPed, void* self);
extern DECL_HOOKv(RenderVehicle, void* self);
extern DECL_HOOKv(RenderObject, void* self);
extern DECL_HOOKv(MainMenuAddItems, FlowScreen* self);
extern DECL_HOOKv(StartGameAddItems, FlowScreen* self);
extern int AddImageToListPatched(const char* imgName, bool registerIt);

extern CStreamingFile pNewStreamingFiles[MAX_IMG_ARCHIVES + 2];
extern MobileSettings::Setting pNewSettings[MAX_SETTINGS];
extern CWidget* pNewWidgets[MAX_WIDGETS];
extern CPlayerInfo *WorldPlayers;

void SAUtils_2_10::InitializeSAUtils()
{
    // Loaded IMG archives limit
    aml->Unprot(pGameLib + 0x676AB8, sizeof(void*));
    *(uintptr_t*)(pGameLib + 0x676AB8) = (uintptr_t)pNewStreamingFiles;
    aml->Unprot(pGameLib + 0x46BDE8, sizeof(char));
    *(unsigned char*)(pGameLib + 0x46BDE8) = (unsigned char)MAX_IMG_ARCHIVES+2;
    aml->Unprot(pGameLib + 0x46BDF8, sizeof(char)); 
    *(unsigned char*)(pGameLib + 0x46BDF8) = (unsigned char)MAX_IMG_ARCHIVES+2;
    aml->Redirect(aml->GetSym(pGameHandle, "_ZN10CStreaming14AddImageToListEPKcb"), (uintptr_t)AddImageToListPatched);
    logger->Info("IMG limit has been bumped!");

    // Bump settings limit
    aml->Unprot(pGameLib + 0x679A3C, sizeof(void*));
    *(uintptr_t*)(pGameLib + 0x679A3C) = (uintptr_t)pNewSettings;
    memcpy(pNewSettings, (int*)(pGameLib + 0x6E03FC), 1184);

    // Bump widgets limit
    aml->Unprot(pGameLib + 0x679474, sizeof(void*)); *(uintptr_t*)(pGameLib + 0x679474)     = (uintptr_t)pNewWidgets;
    aml->Unprot(pGameLib + 0x2AE5FE, sizeof(char));  *(unsigned char*)(pGameLib + 0x2AE5FE) = (unsigned char)MAX_WIDGETS; // Create all
    aml->Unprot(pGameLib + 0x2AFC30, sizeof(char));  *(unsigned char*)(pGameLib + 0x2AFC30) = (unsigned char)MAX_WIDGETS; // Delete all
    aml->Unprot(pGameLib + 0x2B0BA2, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B0BA2) = (unsigned char)MAX_WIDGETS; // Update
    aml->Unprot(pGameLib + 0x2B0BC0, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B0BC0) = (unsigned char)MAX_WIDGETS; // Update
    aml->Unprot(pGameLib + 0x2B0CFC, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B0CFC) = (unsigned char)MAX_WIDGETS-1; // Visualize all
    aml->Unprot(pGameLib + 0x2B0622, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B0622) = (unsigned char)MAX_WIDGETS-1; // Clear
    aml->Unprot(pGameLib + 0x2B06B4, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B06B4) = (unsigned char)MAX_WIDGETS-1; // Clear
    aml->Unprot(pGameLib + 0x2B07B8, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B07B8) = (unsigned char)MAX_WIDGETS-1; // Clear
    aml->Unprot(pGameLib + 0x2B07B8, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B07B8) = (unsigned char)MAX_WIDGETS-1; // Clear
    aml->Unprot(pGameLib + 0x2B0842, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B0842) = (unsigned char)MAX_WIDGETS; // Clear
    aml->Unprot(pGameLib + 0x2B08EE, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B08EE) = (unsigned char)MAX_WIDGETS; // Clear
    aml->Unprot(pGameLib + 0x2B0CA4, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B0CA4) = (unsigned char)MAX_WIDGETS; // Draw All
    aml->Unprot(pGameLib + 0x2B2958, sizeof(char));  *(unsigned char*)(pGameLib + 0x2B2958) = (unsigned char)MAX_WIDGETS-1; // AnyWidgetsUsingAltBack
    HOOKPLT(CreateAllWidgets, pGameLib + 0x673504);

    // Hook functions
    HOOKPLT(AsciiToGxtChar,             pGameLib + 0x672508);
    HOOKPLT(SelectScreenOnDestroy,      pGameLib + 0x673FFC);
    HOOKPLT(SettingSelectionRender,     pGameLib + 0x662840);
    HOOKPLT(GxtTextGet,                 pGameLib + 0x66E784);
    HOOKPLT(SettingsScreen_Construct,   pGameLib + 0x67403C);
    HOOKPLT(InitialiseRenderWare,       pGameLib + 0x66F2D8);
    HOOKPLT(InitialiseGame_SecondPass,  pGameLib + 0x672194);
    HOOKPLT(PlayerProcess,              pGameLib + 0x673EA8);
    HOOK(RenderEffects,                 aml->GetSym(pGameHandle, "_Z13RenderEffectsv"));
    HOOK(RenderMenu,                    aml->GetSym(pGameHandle, "_ZN10MobileMenu6RenderEv"));
    HOOK(RenderPed,                     aml->GetSym(pGameHandle, "_ZN4CPed6RenderEv"));
    HOOK(RenderVehicle,                 aml->GetSym(pGameHandle, "_ZN8CVehicle6RenderEv"));
    HOOK(RenderObject,                  aml->GetSym(pGameHandle, "_ZN7CObject6RenderEv"));
    HOOK(GetTextureFromDB_HOOKED,       aml->GetSym(pGameHandle, "_ZN22TextureDatabaseRuntime10GetTextureEPKc"));
    HOOK(MainMenuAddItems,              aml->GetSym(pGameHandle, "_ZN14MainMenuScreen11AddAllItemsEv"));
    HOOK(StartGameAddItems,             aml->GetSym(pGameHandle, "_ZN14MainMenuScreen11OnStartGameEv"));

    InitializeFunctions();
    SET_TO(WorldPlayers, *(void**)(pGameLib + 0x6783C0));
    
    // Remove an "EXIT" button from MainMenu (to set it manually)
    aml->PlaceB(pGameLib + 0x29BF2E + 0x1, pGameLib + 0x29BFBC + 0x1);

    // Scripting
    InitializeSAScripting();
}
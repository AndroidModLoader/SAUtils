#include <isautils.h>

#define MODS_SETTINGS_STARTING_FROM  37
#define MAX_SETTINGS                 200

enum eLoadedGame
{
    Unknown = 0,
    GTASA_2_00 = 1,
    GTAVC_1_09 = 2,
};

class SAUtils : public ISAUtils
{
public:
    SAUtils() : m_eLoadedGame(Unknown) {}

    void InitializeSAUtils();
    void InitializeVCUtils();

    uintptr_t IsFLALoaded();
    int AddSettingsItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, OnSettingChangedFn fnOnValueChange = nullptr, bool isSlider = false, void* switchesArray = nullptr);
    int ValueOfSettingsItem(int settingId);
    
    // 1.1
    int AddClickableItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, const char** switchesArray = nullptr, OnSettingChangedFn fnOnValueChange = nullptr);
    int AddSliderItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, OnSettingChangedFn fnOnValueChange = nullptr, OnSettingDrawedFn fnOnValueDraw = nullptr);

public:
    eLoadedGame m_eLoadedGame;
    uintptr_t m_pHasFLA;
};
extern ISAUtils* sautils;
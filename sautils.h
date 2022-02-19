#include <isautils.h>

#define MODS_SETTINGS_STARTING_FROM  37
#define MAX_SETTINGS                 200

enum eLoadedGame
{
    Unknown = 0,
    GTASA_2_00 = 1,
    GTAVC_1_09 = 2,
};

struct AdditionalSetting
{
    int                nSettingId;
    eTypeOfSettings    eType;
    const char*        szName;
    OnSettingChangedFn fnOnValueChange;
    OnSettingDrawedFn  fnOnValueDraw;
    OnButtonPressedFn  fnOnButtonPressed;
    eTypeOfItem        byteItemType;
    int                nInitVal;
    int                nSavedVal;
    int                nMaxVal;
};

struct ButtonSettingItem
{
    uintptr_t          vtable;
    const char*        itemText;
    uintptr_t          actionFn;
    int                flag;
};

struct SettingItem
{
    uintptr_t          vtable;
    const char**       itemsArray;
    int                initialValue;
    int                unk2;
    int                minValue;
    int                maxValue;
    int                unk3;
    int                isSlider;
};

class SAUtils : public ISAUtils
{
public:
    SAUtils() : m_eLoadedGame(Unknown) {}

    void InitializeSAUtils();
    void InitializeVCUtils();

    uintptr_t IsFLALoaded();
    int AddSettingsItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, OnSettingChangedFn fnOnValueChange = NULL, bool isSlider = false, void* switchesArray = NULL);
    int ValueOfSettingsItem(int settingId);
    
    // 1.1
    int AddClickableItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, const char** switchesArray = NULL, OnSettingChangedFn fnOnValueChange = NULL);
    int AddSliderItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, OnSettingChangedFn fnOnValueChange = NULL, OnSettingDrawedFn fnOnValueDraw = NULL);

    // 1.2
    void AddButton(eTypeOfSettings typeOf, const char* name, OnButtonPressedFn fnOnButtonPressed = NULL);

public:
    eLoadedGame m_eLoadedGame;
    uintptr_t m_pHasFLA;
};
extern ISAUtils* sautils;
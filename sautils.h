#include <isautils.h>

#define MODS_SETTINGS_STARTING_FROM  37
#define MAX_SETTINGS                 1200

enum eLoadedGame
{
    Unknown =     0,
    GTASA_2_00 =  1,
    GTAVC_1_09 =  2, // Not works currently and idk what to do
    GTA3_1_09 =   3, // Not works currently and idk what to do
    GTALCS_1_09 = 4, // Not works currently and idk what to do
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
    void*              pOwnData;
};

struct AdditionalSettingsButton
{
    const char*        szName;
    const char*        szTextureName;
    bool               bUsesMenu;
    SimpleDataFn       fnButtonPressed;
    void*              pMenuData;
};

struct AdditionalTexDB
{
    const char*        szName;
    bool               bRegister;
    uintptr_t          nDBPointer;
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

    void            InitializeSAUtils();
    void            InitializeVCUtils();

    uintptr_t       IsFLALoaded();
    int             AddSettingsItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, OnSettingChangedFn fnOnValueChange = NULL, bool isSlider = false, void* switchesArray = NULL);
    int             ValueOfSettingsItem(int settingId);

    // 1.1
    int             AddClickableItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, const char** switchesArray = NULL, OnSettingChangedFn fnOnValueChange = NULL);
    int             AddSliderItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, OnSettingChangedFn fnOnValueChange = NULL, OnSettingDrawedFn fnOnValueDraw = NULL);

    // 1.2  
    void            AddButton(eTypeOfSettings typeOf, const char* name, OnButtonPressedFn fnOnButtonPressed = NULL);
    uintptr_t*      AddTextureDB(const char* name, bool registerMe = false);
    int*            GetSettingValuePointer(int settingId);
    void            AddIMG(const char* imgName);

    // 1.3
    eTypeOfSettings AddSettingsTab(const char* name, const char* textureName = "menu_mainsettings");
    unsigned int    GetCurrentMs();
    float           GetCurrentFPS();
    uintptr_t       GetTextureDB(const char* texDbName);
    void            RegisterTextureDB(uintptr_t textureDbPtr);
    void            UnregisterTextureDB(uintptr_t textureDbPtr);
    uintptr_t       GetTexture(const char* texName);
    void            AddOnWidgetsCreateListener(SimpleFn fn);
    void            AddPlayerUpdateListener(OnPlayerProcessFn fn, bool post = true);
    int             FindFreeWidgetId();
    CWidgetButton*  CreateWidget(int widgetId, int x, int y, float scale, const char* textureName);
    int             GetWidgetIndex(CWidgetButton* widget);
    void            SetWidgetIcon(CWidgetButton* widget, uintptr_t texturePtr);
    void            SetWidgetIcon(CWidgetButton* widget, const char* textureName);
    void            SetWidgetIcon2(CWidgetButton* widget, uintptr_t texturePtr);
    void            SetWidgetIcon2(CWidgetButton* widget, const char* textureName);
    void            ToggleWidget(CWidgetButton* widget, bool enable);
    bool            IsWidgetEnabled(int widgetId);
    void            ClearWidgetTapHistory(int widgetId);
    void            ClearWidgetTapHistory(CWidgetButton* widget);
    int             GetWidgetState(CWidgetButton* widget, eWidgetState stateToGet);
    int             GetWidgetState(int widgetId, eWidgetState stateToGet, bool doDoubleTapEff = true, int frames = 1);
    void            GetWidgetPos(int widgetId, float* x = NULL, float* y = NULL, float* sx = NULL, float* sy = NULL);
    void            SetWidgetPos(int widgetId, float x, float y, float sx, float sy);

    // 1.4
    void*           LoadRwTextureFromPNG(const char* filename);
    void*           LoadRwTextureFromBMP(const char* filename);
    void            AddOnRWInitListener(SimpleFn fn);
    void            AddTextureLookupListener(LookingForTextureFn fn);
    int             AddClickableItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, const char** switchesArray = NULL, OnSettingChangedFn fnOnValueChange = NULL, void* data = NULL); // +data
    int             AddSliderItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, OnSettingChangedFn fnOnValueChange = NULL, OnSettingDrawedFn fnOnValueDraw = NULL, void* data = NULL); // +data
    int             ScriptCommand(const SCRIPT_COMMAND *pScriptCommand, ...);
    void            AddOnRenderListener(eRenderOfType typeOf, SimpleDataFn fn);

    // 1.4.1
    void            AddSettingsTabButton(const char* name, SimpleDataFn fn, const char* textureName = "menu_mainsettings", void* data = NULL);
    
public:
    eLoadedGame     m_eLoadedGame;
    uintptr_t       m_pHasFLA;
};
extern ISAUtils*    sautils;

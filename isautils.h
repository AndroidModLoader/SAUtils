#ifndef _SAUTILS_INTERFACE
#define _SAUTILS_INTERFACE

#include <stdint.h>

typedef void        (*OnSettingChangedFn)(int nOldValue, int nNewValue);
typedef const char* (*OnSettingDrawedFn) (int nNewValue);
typedef void        (*OnButtonPressedFn) (uintptr_t screen); // "screen" is just a pointer of SelectScreen if you need it...
typedef void        (*OnPlayerProcessFn) (uintptr_t info); // "info" is a pointer of CPlayerInfo
typedef void        (*SimpleFn)();

#define MAX_IMG_ARCHIVES                    32 // Def. is 6

#define MAX_WIDGETS_GAME                    190
#define MAX_WIDGETS                         0xFF
class CWidgetButton;

// Controllers
#define SETITEM_SA_TOUCH_LAYOUT             9
#define SETITEM_SA_TOUCH_STEERING           3
#define SETITEM_SA_ACCELEROMETER            8
#define SETITEM_SA_INVERT_LOOK              15
  // For touch screens GetInputType==0
    #define SETITEM_SA_ANALOG_BIKE_STEERING 18
    #define SETITEM_SA_ANALOG_SENSITIVITY   23
  // Joypad GetInputType==1
    #define SETITEM_SA_JOY_ACCEL_X          25
    #define SETITEM_SA_JOY_ACCEL_Y          26
  // Mouse+Keyboard GetInputType==2
    #define SETITEM_SA_MOUSE_ACCEL_X        31
    #define SETITEM_SA_MOUSE_ACCEL_Y        32
#define SETITEM_SA_AUTO_CLIMB               35

// Game
#define SETITEM_SA_TRAFFIC_MODE             4
#define SETITEM_SA_TARGETING_MODE           6
#define SETITEM_SA_VEH_CAMERA_HEIGHT        10
#define SETITEM_SA_VIBRO_FEEDBACK           29
#define SETITEM_SA_FRAMELIMITER             30

// Display
#define SETITEM_SA_BRIGHTNESS               16
#define SETITEM_SA_EFFECTS_QUALITY          0
#define SETITEM_SA_RESOLUTION_SCALING       1
#define SETITEM_SA_DRAW_DISTANCE            2
#define SETITEM_SA_SHADOWS_QUALITY          5
#define SETITEM_SA_CAR_REFLECTIONS          7
#define SETITEM_SA_SUBTITLES                24

// Audio
#define SETITEM_SA_SFX_VOLUME               11
#define SETITEM_SA_RADIO_VOLUME             12
#define SETITEM_SA_POSITIONAL_SOUND         33
#define SETITEM_SA_RADIO_AUTOTUNE           13
#define SETITEM_SA_CURRENT_RADIO            14


enum eTypeOfSettings : unsigned char
{
    SetType_Controller = 0,
    SetType_Game = 1,
    SetType_Display = 2,
    SetType_Audio = 3,
    SetType_Language = 4,
    SetType_Mods = 5,

    SETTINGS_COUNT
};

enum eTypeOfItem : unsigned char
{
    ItemType_WithItems = 0,
    ItemType_Slider = 1,
    ItemType_Button = 2,

    ITEMTYPES_COUNT
};

enum eWidgetState : unsigned char
{
    WState_Touched = 0,
    WState_DoubleTapped,
    WState_Released,
    WState_HeldDown,
    WState_SwipedLeft,
    WState_SwipedRight,
    WState_SwipedUp,
    WState_SwipedDown,

    WIDGETSTATE_MAX
};

class ISAUtils
{
public:
/* Functions below added in 1.0.0.0 */

    /** Get an address of Fastman Limit Adjuster if included
     *
     *  \return Address of a FLA library
     */
    virtual uintptr_t IsFLALoaded() = 0;
    
    // switchesArray is an array of items of clickable item (byteItemType = WithItems)
    #if __cplusplus >= 201300 // Do not create errors on C++11 and lower :P
      [[deprecated("Use AddClickableItem or AddSliderItem")]]
    #endif
    virtual int AddSettingsItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, OnSettingChangedFn fnOnValueChange = NULL, bool isSlider = false, void* switchesArray = NULL) = 0;

    /** Get a value of setting (returned by AddClickableItem or AddSliderItem)
     *
     *  \param settingId Numeric ID of setting (0-200 for example, see SETITEM_*)
     *  \return Value of that setting
     */
    virtual int ValueOfSettingsItem(int settingId) = 0;


/* Functions below added in 1.1.0.0 */

    /** Adds clickable text button in menu settings
     *
     *  \param typeOf In which setting option that item should be added
     *  \param name Obviously a displayed name
     *  \param initVal Initial value (def: 0)
     *  \param minVal Minimum value (def: 0)
     *  \param maxVal Maximum value (def: 0)
     *  \param switchesArray An array that includes names of options
     *  \param fnOnValueChange A function that will be called on value being saved (def: null)
     *  \return Setting ID
     */
    virtual int AddClickableItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, const char** switchesArray = NULL, OnSettingChangedFn fnOnValueChange = NULL) = 0;

    /** Adds a slider in menu settings
     *
     *  \param typeOf In which setting option that item should be added
     *  \param name Obviously a displayed name
     *  \param initVal Initial value (def: 0)
     *  \param minVal Minimum value (def: 0)
     *  \param maxVal Maximum value (def: 0)
     *  \param fnOnValueChange A function that will be called on value being saved (def: null)
     *  \param fnOnValueDraw A function that will control a text of a slider (def: null)
     *  \return Setting ID
     */
    virtual int AddSliderItem(eTypeOfSettings typeOf, const char* name, int initVal = 0, int minVal = 0, int maxVal = 0, OnSettingChangedFn fnOnValueChange = NULL, OnSettingDrawedFn fnOnValueDraw = NULL) = 0;


/* Functions below added in 1.2.0.0 */

    /** Adds a clickable button in menu settings
     *
     *  \param typeOf In which setting option that item should be added
     *  \param name Obviously a displayed name
     *  \param fnOnButtonPressed A function that will be called on button press (def: null)
     */
    virtual void AddButton(eTypeOfSettings typeOf, const char* name, OnButtonPressedFn fnOnButtonPressed = NULL) = 0;

    /** Loads texture db
     *
     *  \param name Obviously a name of texdb
     *  \param registerMe Should this texdb be registered for searching in or just be loaded until we register it manually?
     *  \return Pointer of value that contains loaded TexDB address (since SAUtils v1.3)
     */
    virtual uintptr_t* AddTextureDB(const char* name, bool registerMe = false) = 0;

    /** Returns a pointer to setting value
     *
     *  \param settingId Numeric ID of setting (0-200 for example, see SETITEM_*)
     *  \return Pointer of that variable that holds value
     */
    virtual int* GetSettingValuePointer(int settingId) = 0;

    /** Loads IMG archive
     *
     *  \param imgName IMG archive name
     */
    virtual void AddIMG(const char* imgName) = 0;


/* Functions below added in 1.3.0.0 */

    /** Creates a custom tabs in settings (like "Mods Settings")
     *
     *  \param name A name of settings tab
     *  \param textureName A name of the texture from mobile.txt texdb (def: "menu_mainsettings")
     *  \return An id to use with: AddButton or AddSliderItem or AddClickableItem
     */
    virtual eTypeOfSettings AddSettingsTab(const char* name, const char* textureName = "menu_mainsettings") = 0;

    /** Get current game time in milliseconds
     *
     *  \return Current milliseconds
     */
    virtual unsigned int GetCurrentMs() = 0;

    /** Get current game frames per second
     *
     *  \return Current FPS
     */
    virtual float GetCurrentFPS() = 0;

    /** Get pointer of already loaded texture database
     *
     *  \param texDbName Name of the textureDb
     *  \return Texture Database pointer
     */
    virtual uintptr_t GetTextureDB(const char* texDbName) = 0;

    /** Registers Texture Database for texture lookup (you SHOULD unregister it later!!!)
     *
     *  \param textureDbPtr Texture Database pointer, obviously.
     */
    virtual void RegisterTextureDB(uintptr_t textureDbPtr) = 0;
    
    /** Unregisters Texture Database for texture lookup
     *
     *  \param textureDbPtr Texture Database pointer, obviously.
     */
    virtual void UnregisterTextureDB(uintptr_t textureDbPtr) = 0;
    
    /** Get a RwTexture* (as a uintptr_t) from REGISTERED texture databases
     *
     *  \param texName Texture name
     *  \return Texture pointer (RwTexture*)
     */
    virtual uintptr_t GetTexture(const char* texName) = 0;

    /** Add a listener of "Create All Widgets"
     *
     *  \param fn A function that will be called when Widgets are added
     */
    virtual void AddOnWidgetsCreateListener(SimpleFn fn) = 0;

    /** Add a listener of "On player updated"
     *
     *  \param fn A function that will be called when local player is updated
     */
    virtual void AddPlayerUpdateListener(OnPlayerProcessFn fn, bool post = true) = 0;

    /** Find a first widget id that can be occupied (free now)
     *
     *  \return Free widget id (-1 if no empty space)
     */
    virtual int FindFreeWidgetId() = 0;

    /** Creates a widget
     *
     *  \param widgetId An id of the widget
     *  \param x X coord on screen
     *  \param y Y coord on screen
     *  \param scale Scale of a widget
     *  \param textureName Initial texture for a widget (may be NULL to be invisible)
     *  \return Pointer of the widget (returns NULL if id is wrong or already taken)
     */
    virtual CWidgetButton* CreateWidget(int widgetId, int x, int y, float scale, const char* textureName = NULL) = 0;

    /** Get widget index (uses a loop)
     *
     *  \param widget A pointer of the widget
     *  \return Index
     */
    virtual int GetWidgetIndex(CWidgetButton* widget) = 0;

    /** Sets widget icon
     *
     *  \param widget A pointer of the widget
     *  \param texturePtr A pointer of the texture
     */
    virtual void SetWidgetIcon(CWidgetButton* widget, uintptr_t texturePtr) = 0;

    /** Sets widget icon
     *
     *  \param widget A pointer of the widget
     *  \param textureName A name of the texture
     */
    virtual void SetWidgetIcon(CWidgetButton* widget, const char* textureName) = 0;

    /** Sets widget icon (second one)
     *
     *  \param widget A pointer of the widget
     *  \param texturePtr A pointer of the texture
     */
    virtual void SetWidgetIcon2(CWidgetButton* widget, uintptr_t texturePtr) = 0;

    /** Sets widget icon (second one)
     *
     *  \param widget A pointer of the widget
     *  \param textureName A name of the texture
     */
    virtual void SetWidgetIcon2(CWidgetButton* widget, const char* textureName) = 0;

    /** Enable or disable widget
     *
     *  \param widget A pointer of the widget
     *  \param enable Enable? (def: true)
     */
    virtual void ToggleWidget(CWidgetButton* widget, bool enable = true) = 0;

    /** Is our widget enabled
     *
     *  \param widgetId An id of the widget
     *  \return True if enabled
     */
    virtual bool IsWidgetEnabled(int widgetId) = 0;

    /** Is our widget enabled
     *
     *  \param widget A pointer of the widget
     *  \return True if enabled
     */
    inline bool IsWidgetEnabled(CWidgetButton* widget) { return *(bool*)((int)widget + 77); };

    /** Clear widget's tap history
     *
     *  \param widgetId An id of the widget
     */
    virtual void ClearWidgetTapHistory(int widgetId) = 0;

    /** Clear widget's tap history
     *
     *  \param widget A pointer of the widget
     */
    virtual void ClearWidgetTapHistory(CWidgetButton* widget) = 0;

    /** Gets widget state (is double tapped &etc)
     *
     *  \param widget A pointer of the widget
     *  \param stateToGet See eWidgetPressState
     *  \return State value
     */
    virtual int GetWidgetState(CWidgetButton* widget, eWidgetState stateToGet) = 0;

    /** Gets widget state (is double tapped &etc).
     * This variation allows us to set frames (idk why you need to) and enable DoubleTap Effect
     *
     *  \param widget A pointer of the widget
     *  \param stateToGet See eWidgetPressState
     *  \param doDoubleTapEff Do double tap effect if stateToGet==WState_DoubleTapped
     *  \param frames Frames count (do not ask me, still did not understand)
     *  \return State value
     */
    virtual int GetWidgetState(int widgetId, eWidgetState stateToGet, bool doDoubleTapEff = true, int frames = 1) = 0;

    /** Gives us a pos or scale of a widget with the given id
     *
     *  \param widgetId An id of the widget (any widget id)
     *  \param x A pointer where to save X coord
     *  \param y A pointer where to save Y coord
     *  \param sx A pointer where to save X scale
     *  \param sy A pointer where to save Y scale
     */
    virtual void GetWidgetPos(int widgetId, float* x = NULL, float* y = NULL, float* sx = NULL, float* sy = NULL) = 0;

    /** Gives us a pos or scale of a widget with the given id
     *
     *  \param widgetId An id of the widget (only custom widgets id)
     *  \param x X coord
     *  \param y Y coord
     *  \param sx Scale X
     *  \param sy Scale Y
     */
    virtual void SetWidgetPos(int widgetId, float x, float y, float sx, float sy);
};

#endif // _SAUTILS_INTERFACE
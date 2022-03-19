#include <stdint.h>

typedef void        (*OnSettingChangedFn)(int nOldValue, int nNewValue);
typedef const char* (*OnSettingDrawedFn) (int nNewValue);
typedef void        (*OnButtonPressedFn) (uintptr_t screen); // Screen is just a pointer of SelectScreen if you need it...

// Unknown
// 2

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
    Controller = 0,
    Game = 1,
    Display = 2,
    Audio = 3,
    Language = 4,
    Mods = 5,

    SETTINGS_COUNT,
};

enum eTypeOfItem : unsigned char
{
    WithItems = 0,
    Slider = 1,
    Button = 2,

    ITEMTYPES_COUNT,
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
     */
    virtual void AddTextureDB(const char* name, bool registerMe = false) = 0;

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
};
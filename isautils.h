#ifndef _SAUTILS_INTERFACE
#define _SAUTILS_INTERFACE

#include <mod/amlmod.h>
#include <stdint.h>
//#include "sa_scripting.h"

/* Helper-defines */
#define _VA_ARGS(...) , ##__VA_ARGS__
#define MAX_SCRIPT_VARS 16
#define MAX_SCRIPT_SIZE	255
// Use those for easier CLEO opcodes calling and defining!
#define DEFOPCODE(__opcode, __name, __args) const SCRIPT_COMMAND scm_##__name = { 0x##__opcode, #__args }
#define CALLSCM(__name, ...)             sautils->ScriptCommand(&scm_##__name _VA_ARGS(__VA_ARGS__))

/* Structures */
struct SCRIPT_COMMAND
{
    uint16_t  opCode;
    char      params[MAX_SCRIPT_VARS];
};
// Example:
DEFOPCODE(FFFF, CRASH_GAME, ); // CALLSCM(CRASH_GAME); calls an unknown opcode FFFF and it crashes the game

/* Just a class declarations */
class CWidgetButton;
class RpAtomic;
class RwFrame;
class CPhysical;
class CEntity;
class CPed;
class CObject;
class CVehicle;

/* Type definitions */
typedef void        (*OnSettingChangedFn)(int nOldValue, int nNewValue, void* pData); // Has pData since v1.4
typedef const char* (*OnSettingDrawedFn) (int nNewValue, void* pData);
typedef void        (*OnButtonPressedFn) (uintptr_t screen); // "screen" is just a pointer of SelectScreen if you need it...
typedef void        (*OnPlayerProcessFn) (uintptr_t info); // "info" is a pointer of CPlayerInfo
typedef void*       (*LookingForTextureFn)(const char* name);
typedef void        (*SimpleFn)();
typedef void        (*SimpleDataFn)(void* data);

/* !!! UNCHANGEABLE VALUES !!! */
#define MAX_IMG_ARCHIVES                    32 // Def. is 6
#define MAX_WIDGETS_GAME                    190
#define MAX_WIDGETS                         0xFF

/* Settings "shortcuts" */
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

/* Enumerations */
enum eLoadedGame
{
    Unknown = 0,
    GTASA_2_00,
    GTAVC_1_09,
    GTA3_1_09,
    GTALCS_1_09,
    GTASA_2_10,
};

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

enum eRenderOfType : unsigned char
{
    ROfType_Effects = 0,
    ROfType_Menu,
    ROfType_Hud,
    ROfType_Ped,
    ROfType_Vehicle,
    ROfType_Object,
    ROfType_RadarBlips,
    ROfType_MapBlips,

    RENDEROFTYPE_MAX
};

enum eSettingsTabButtonLoc : unsigned char
{
    STB_MainMenu = 0,
    STB_Pause,
    STB_StartGame,
    STB_Settings,
    
    TABBUTTONLOC_MAX,
};

enum ePoolType : unsigned char
{
    POOLTYPE_PEDS,
    POOLTYPE_VEHICLES,
    POOLTYPE_OBJECTS,

    POOLTYPES_MAX,
};

enum eTexDBType : unsigned char
{
    TEXDBTYPE_DXT,
    TEXDBTYPE_ETC,
    TEXDBTYPE_PVR,

    TEXDBTYPES_MAX,
};

/* Interface */
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
     *  \param data A user-defined data
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
    #ifdef AML32
        inline bool IsWidgetEnabled(CWidgetButton* widget) { return *(bool*)((uintptr_t)widget + 77); };
    #else
        inline bool IsWidgetEnabled(CWidgetButton* widget) { return *(bool*)((uintptr_t)widget + 92); };
    #endif

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


/* Functions below added in 1.4.0.0 */
    
    /** Reads the png image file and converts it to RwTexture*
     *
     *  \param filename Self-explained?
     *  \return An actual pointer of RwTexture*
     */
    virtual void* LoadRwTextureFromPNG(const char* filename) = 0;
    
    /** Reads the bmp image file and converts it to RwTexture*
     *
     *  \param filename Self-explained?
     *  \return An actual pointer of RwTexture*
     */
    virtual void* LoadRwTextureFromBMP(const char* filename) = 0;

    /** Add a listener of "RenderWare Engine initialized"
     *
     *  \param fn A function that will be called when Widgets are added
     */
    virtual void AddOnRWInitListener(SimpleFn fn) = 0;

    /** Add a listener of "RenderWare Engine initialized"
     *
     *  \param fn A function that will be called when Widgets are added
     */
    virtual void AddTextureLookupListener(LookingForTextureFn fn) = 0;

    /** Adds clickable text button in menu settings + sends own data
     *
     *  \param typeOf In which setting option that item should be added
     *  \param name Obviously a displayed name
     *  \param initVal Initial value (def: 0)
     *  \param minVal Minimum value (def: 0)
     *  \param maxVal Maximum value (def: 0)
     *  \param switchesArray An array that includes names of options
     *  \param fnOnValueChange A function that will be called on value being saved (def: null)
     *  \param data An own data (def: null)
     *  \return Setting ID
     */
    virtual int AddClickableItem(eTypeOfSettings typeOf, const char* name, int initVal, int minVal, int maxVal, const char** switchesArray, OnSettingChangedFn fnOnValueChange, void* data) = 0;

    /** Adds a slider in menu settings + data
     *
     *  \param typeOf In which setting option that item should be added
     *  \param name Obviously a displayed name
     *  \param initVal Initial value (def: 0)
     *  \param minVal Minimum value (def: 0)
     *  \param maxVal Maximum value (def: 0)
     *  \param fnOnValueChange A function that will be called on value being saved (def: null)
     *  \param fnOnValueDraw A function that will control a text of a slider (def: null)
     *  \param data An own data (def: null)
     *  \return Setting ID
     */
    virtual int AddSliderItem(eTypeOfSettings typeOf, const char* name, int initVal, int minVal, int maxVal, OnSettingChangedFn fnOnValueChange, OnSettingDrawedFn fnOnValueDraw, void* data) = 0;

    /** Calls a script opcode (mini-cleo).
     *  A serious crash may appear if you are using this function and one of the arguments is a pointer (&scmHandle).
     *  Make sure, that the variable you need pointer for (scmHandle1 in this case)
     *  is defined OUTSIDE of the function, otherwise the stack corruption and crash will be your best friends!
     *  For more info, check SAUtils's sautils.cpp, line #1462, you will see.
     *
     *  \param pScriptCommand A pointer to SCRIPT_COMMAND struct var
     *  \param ... All arguments
     *  \return A possibly returned value
     */
    virtual int ScriptCommand(const SCRIPT_COMMAND *pScriptCommand, ...) = 0;
    
    /** Add a listener to "Render" function of something
     *
     *  \param typeOf What are listening to? See eRenderOfType
     *  \param fn Function that will be called
     */
    virtual void AddOnRenderListener(eRenderOfType typeOf, SimpleDataFn fn) = 0;


/* Functions below added in 1.4.1.0 */

    /** Creates a custom tabs in settings (like "Mods Settings") but as a button
     *
     *  \param name A name of settings tab btn
     *  \param textureName A name of the texture from mobile.txt texdb (def: "menu_mainsettings")
     *  \param data A user-defined data
     *  \noreturn
     */
    virtual void AddSettingsTabButton(const char* name, SimpleDataFn fn, eSettingsTabButtonLoc loc = STB_Settings, const char* textureName = "menu_mainsettings", void* data = NULL) = 0;

    /** Loads a DLL file from the folder
     *
     *  \param name A name of settings tab btn
     *  \param doPrelit Do a prelit process if there's no prelit info in a model (def: false)
     *  \param atomic Save RpAtomic struct to this pointer (def: null)
     *  \param frame Save RwFrame struct to this pointer (def: null)
     *  \return If this process was successful
     */
    virtual bool LoadDFF(const char* name, bool doPrelit = false, RpAtomic** atomic = NULL, RwFrame** frame = NULL) = 0;

    /** Gets the loaded game Enumeration (how did i forget that?!)
     *
     *  \return The enum which tells you about the loaded game!
     */
    virtual eLoadedGame GetLoadedGame() = 0;

    /** Gets the loaded game lib addr (how did i forget that?!)
     *
     *  \return The game library address
     */
    virtual uintptr_t GetLoadedGameLibAddress() = 0;


/* Functions below added in 1.5.1.0 */

    /** Loads texture db
     *
     *  \param name Obviously a name of texdb
     *  \param type A type of the texture database: DXT, ETC, PVR
     *  \param registerMe Should this texdb be registered for searching in or just be loaded until we register it manually?
     *  \return Pointer of value that contains loaded TexDB address
     */
    virtual uintptr_t* AddTextureDBOfType(const char* name, eTexDBType type, bool registerMe = false);

    /** Returns a number of items allocated in a pool
     *
     *  \param poolType Enumeration of which pool size you needed
     *  \return A size of Object pool (returns 0 if there is any error)
     */
    virtual int GetPoolSize(ePoolType poolType) = 0;

    /** Returns a number of allocated memory count for a pool
     *
     *  \param poolType Enumeration of which pool size you needed
     *  \return An allocated memory size of Object pool (returns 0 if there is any error)
     */
    virtual int GetPoolMemSize(ePoolType poolType) = 0;

    /** Returns a pool index of a given entity (only pools from ePoolType are supported!)
     *
     *  \param entityPtr A pointer of an entity
     *  \return An index of a value (value less than 0 if error occured)
     */
    virtual int GetPoolIndex(void* entityPtr) = 0;

    /** Returns a pool member locate at the index
     *
     *  \param poolType Enumeration of which pool's member pointer you needed
     *  \param index An index
     *  \return A pointer of a value located at the index (value is NULL if any error occured or member doesnt exists)
     */
    virtual void* GetPoolMember(ePoolType poolType, int index) = 0;


/* Functions below added in 1.5.2.0 */

    /** Is the game's engine has been loaded?
     *
     *  \return Is the game loaded
     */
    virtual bool IsGameInitialised();

    /** Teleport physical
     *
     *  \param ent Any physical (entity, object, vehicle, ped)
     *  \param x X coordinate
     *  \param y Y coordinate
     *  \param z Z coordinate
     *  \param resetRotation Should i reset the rotation of this physical
     *  \noreturn
     */
    virtual void SetPosition(CPhysical* ent, float x, float y, float z, bool resetRotation = false);

    /** Set angles for physical (faster)
     *
     *  \param ent Any physical (entity, object, vehicle, ped)
     *  \param axis An axis (x = 0, y = 1, z = 2)
     *  \param angle An angle in degrees
     *  \noreturn
     */
    virtual void SetAngle(CPhysical* ent, unsigned char axis, float angle);

    /** Set angles for physical
     *
     *  \param ent Any physical (entity, object, vehicle, ped)
     *  \param x X angle (from -359.99... to 359.99..., otherwise dont rotate)
     *  \param y Y angle (from -359.99... to 359.99..., otherwise dont rotate)
     *  \param z Z angle (from -359.99... to 359.99..., otherwise dont rotate)
     *  \noreturn
     */
    virtual void SetAngle(CPhysical* ent, float x = -999, float y = -999, float z = -999);

    /** Request model id to be loaded
     *
     *  \param modelId Id of the model
     *  \noreturn
     */
    virtual void LoadModelId(int modelId);

    /** Request the game to load collision at the specific area
     *
     *  \param x X coordinate
     *  \param y Y coordinate
     *  \noreturn
     */
    virtual void LoadArea(float x, float y);

    /** Creates ped
     *
     *  \param pedType Type of ped (ePedType, from 0 to 31)
     *  \param modelId Id of the model
     *  \param x X coordinate
     *  \param y Y coordinate
     *  \param z Z coordinate
     *  \param ref An optional pointer to save the script's handle of this ped
     *  \return Ped pointer (always check if it's NULL)
     */
    virtual CPed* CreatePed(int pedType, int modelId, float x, float y, float z, int *ref = NULL);

    /** Creates ped inside a vehicle
     *
     *  \param pedType Type of ped (ePedType, from 0 to 31)
     *  \param modelId Id of the model
     *  \param vehicle A vehicle ped should be spawned in
     *  \param seat Seat id (-1 = driver)
     *  \param ref An optional pointer to save the script's handle of this ped
     *  \return Ped pointer (always check if it's NULL)
     */
    virtual CPed* CreatePed(int pedType, int modelId, CVehicle* vehicle, int seat = -1, int *ref = NULL);

    /** Creates vehicle
     *
     *  \param modelId Id of the model
     *  \param x X coordinate
     *  \param y Y coordinate
     *  \param z Z coordinate
     *  \param ref An optional pointer to save the script's handle of this car
     *  \return Vehicle pointer (always check if it's NULL)
     */
    virtual CVehicle* CreateVehicle(int modelId, float x, float y, float z, int *ref = NULL);

    /** Creates object
     *
     *  \param modelId Id of the model
     *  \param x X coordinate
     *  \param y Y coordinate
     *  \param z Z coordinate
     *  \param ref An optional pointer to save the script's handle of this object
     *  \return Object pointer (always check if it's NULL)
     */
    virtual CObject* CreateObject(int modelId, float x, float y, float z, int *ref = NULL);

    /** Mark entity to be able to delete
     *
     *  \param ent Supported entities (object, vehicle, ped)
     *  \noreturn
     */
    virtual void MarkEntityAsNotNeeded(CEntity* ent);

    /** Mark model id to be able to be freed by streaming
     *
     *  \param modelId Id of the model
     *  \noreturn
     */
    virtual void MarkModelAsNotNeeded(int modelId);

    /** Places a ped into a vehicle
     *
     *  \param ped Ped pointer
     *  \param vehicle Vehicle pointer
     *  \param seat Seat id (-1 = driver)
     *  \noreturn
     */
    virtual void PutPedInVehicle(CPed* ped, CVehicle* vehicle, int seat = -1);
};

#endif // _SAUTILS_INTERFACE
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <Xinput.h>
#include "Common\StepTimer.h"

#include <DirectXMath.h>
#include <interlockedapi.h>

using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Devices::Input;

using namespace DirectX;

namespace SimpleSample_DirectXTK_UWP
{
    //
    // ** Begin enumerations **
    //

#pragma region InputManagerEnums

    // Bit values of all the actions a player can
    // initiate, and which can be returned by the
    // input manager after processing.
    // NOTE TO DEVELOPERS: When you change this list, please
    // increment or decrement DEFAULT_MAX_PLAYER_ACTION_TYPES  accordingly!
    enum PLAYER_ACTION_TYPES
    {
        // These are examples of input events a game might have:
        INPUT_NONE = 0,
        INPUT_COORDINATES_ONLY = 1, // raw coord data
        INPUT_MOVE,
        INPUT_AIM,
        INPUT_FIRE,
        INPUT_FIRE_UP,
        INPUT_FIRE_DOWN,
        INPUT_FIRE_PRESSED,
        INPUT_FIRE_RELEASED,
        INPUT_JUMP,
        INPUT_JUMP_UP,
        INPUT_JUMP_DOWN,
        INPUT_JUMP_PRESSED,
        INPUT_JUMP_RELEASED,
        INPUT_ACCEL,
        INPUT_BRAKE,
        INPUT_SELECT,
        INPUT_START,
        INPUT_CANCEL,
        INPUT_EXIT,
        INPUT_DIRECTIONAL,
        // ...
        // Note to developer: Add the input events your game demands!
        // ...

        INPUT_MAX
    };


    // Types of Windows input events. Used internally for handling passed-through input events.
    enum INPUT_EVENT_TYPE
    {
        INPUT_EVENT_TYPE_DOWN,
        INPUT_EVENT_TYPE_UP,
        INPUT_EVENT_TYPE_MOVED,
        INPUT_EVENT_TYPE_EXITED,

        INPUT_EVENT_TYPE_NUM
    };


    // Bit values of all the input devices supported by the input manager.
    enum INPUT_DEVICE_TYPES
    {
        INPUT_DEVICE_NONE     = 0x00,
        INPUT_DEVICE_MOUSE    = 0x01,
        INPUT_DEVICE_KEYBOARD = 0x02,
        INPUT_DEVICE_TOUCH    = 0x04,
        INPUT_DEVICE_XINPUT   = 0x08,
        INPUT_DEVICE_ALL = 
           (INPUT_DEVICE_MOUSE    |
            INPUT_DEVICE_KEYBOARD |
            INPUT_DEVICE_TOUCH    |
            INPUT_DEVICE_XINPUT)
    };


    // Bit values for all the players supported by the input manager.
    // NOTE TO DEVELOPERS: When you change this list, please
    // increment or decrement XUSER_MAX_COUNT accordingly!
    enum PLAYER_ID
    {
        PLAYER_ID_ONE   = 0x01,
        PLAYER_ID_TWO   = 0x02,
        PLAYER_ID_THREE = 0x04,
        PLAYER_ID_FOUR  = 0x08,
        // ...
        // Note to developer: Add more players as your game demands, and as practically 
        // supported by the hardware.
        // ...
        PLAYER_ID_MAX
    };

    // Types of touch screen virtual controls. 
    // NOTE TO DEVELOPERS: You can add to, or update, these touch-screen
    // control types.
    enum TOUCH_CONTROL_REGION_TYPES
    {
        TOUCH_CONTROL_REGION_REPORT_COORDS_ONLY = 0,
        TOUCH_CONTROL_REGION_ANALOG_STICK = 1,
        TOUCH_CONTROL_REGION_BUTTON = 2,
        TOUCH_CONTROL_REGION_ANALOG_SLIDER = 3,

        TOUCH_CONTROL_REGION_MAX
    };

    //
    // ** End InputManager enums **
    //
#pragma endregion

    // Keyboard/touch/mouse default to player 1 (index 0).
#define DEFAULT_KEYBOARD_PLAYER_ID               0
#define DEFAULT_POINTER_PLAYER_ID                0


    // Touch region definition errors
#define INVALID_TOUCH_REGION_ID                 -1
#define INVALID_TOUCH_REGION_OVERLAPS           -2
#define INVALID_TOUCH_REGION_INVERTED           -3
#define INVALID_TOUCH_REGION_OFFSCREEN          -4


#pragma region InputManagerConsts

    //
    // Constants for handling XInput
    //
    // Defines timeout window for dropped XInput controller connections.
#define XINPUT_CONTROLLER_ENUM_TIMEOUT  2000


    // Defines the maximum numbers of Xinput controllers to process. XInput
    // supports a maximum of 4 controllers.
#define XINPUT_MAX_CONTROLLERS          4


    // The maximum XInput analog throw value. Full range is
    // -32768 to 32767.
#define XINPUT_ANALOG_STICK_THROW_MAX   32767.0f


    //
    // Constants for handling pointer devices (mouse/touch)
    //
    // Pixel move radius from touchdown for virtual stick deadzone.
#define POINTER_VIRTUAL_STICK_DEADZONE  10.0f
#define POINTER_VIRTUAL_STICK_THROW_MAX 45.0f


#pragma endregion

#pragma region InputManagerStructs

    // Contains the state of an XInput controller. This state may be examined 
    // to see what controller actions were taken.
    struct XInputControllerAction
    {
        unsigned int    ControllerId;
        XINPUT_STATE    State;
    };


    // Contains the state of an individual pointer action. Note that a 
    // PointerId is NOT the same as a player or controller ID, as it is used 
    // to track a a press or move event from start to completion.
    // -- CHANGE LEFT/RIGHT/MIDDLE to IsTouchPressed;
    struct PointerControllerAction
    {
        unsigned int    PointerId;
        float           CurrentX;
        float           CurrentY;
        bool            IsTouchEvent;
        bool            IsMouseEvent;
        bool            IsLeftButtonPressed;
        bool            IsRightButtonPressed;
        bool            IsMiddleButtonPressed;
    };

    // Defines a final player input action state returned to the game. 
    struct PlayerInputData
    {
    public:
        // The zero-based player ID value for the player that initiated the action.
        unsigned int ID;

        // The PLAYER_ACTION_TYPES value for the initiated action.
        PLAYER_ACTION_TYPES PlayerAction;

        // The normalized value (i.e. between 0.f and 1.f) returned from the 
        // input device. For digital inputs, either a value of 0.f or 1.0f is 
        // returned, depending on the action. 
        float NormalizedInputValue;

        // Indicates whether this possible input state is different from last frame.

        // The raw screen coordinate data (x, y) for pointer events. For non-pointer events, 
        // such as virtual key presses or XInput analog stick and digital pad actions, both
        // of these values are used to indicate the direction of the action, where 1.0 could
        // indicate up/forward, and -1.0 could indicate down/back. It is up to your game to
        // interpret the meaning.
        float X;
        float Y;

        // Raw coordinate position for touch/mouse input. For non-pointer events, these values
        // are set to 0.f.
        float PointerRawX;
        float PointerRawY;

        // Value used to determine if the acion originated from a touch event.
        bool IsTouchAction;

        // Virtual throw for touch stick input. For non-touch actions, these values are set to
        // 0.f by default. You can use them for additional calibration or smoothing information
        // for custom controls.
        float PointerThrowX;
        float PointerThrowY;

        // ctor
        PlayerInputData() :
                ID(0),
                PlayerAction(INPUT_NONE),
                NormalizedInputValue(0.f),
                X(0.f),
                Y(0.f),
                PointerRawX(0.f),
                PointerRawY(0.f),
                IsTouchAction(false),
                PointerThrowX(-1.f), // -1 means this is not a relative touch input event.
                PointerThrowY(-1.f)
        {
        }
    };

    // Defines a touch control region rectangle.
    struct TouchControlRegion
    {
    public:
        const XMFLOAT2                      UpperLeftCoords;
        const XMFLOAT2                      LowerRightCoords;
        const TOUCH_CONTROL_REGION_TYPES    RegionType;
        const PLAYER_ACTION_TYPES           DefinedAction;
        bool                                IsEnabled;
        bool                                ProcessedThisFrame;
        PLAYER_ID                           PlayerID;

        // ctor
        TouchControlRegion(
            XMFLOAT2 const& upperLeft,
            XMFLOAT2 const& lowerRight,
            TOUCH_CONTROL_REGION_TYPES const& regionType,
            PLAYER_ACTION_TYPES const& definedAction,
            PLAYER_ID const& playerId
            ) :
            UpperLeftCoords(upperLeft),
            LowerRightCoords(lowerRight),
            RegionType(regionType),
            DefinedAction(definedAction),
            IsEnabled(true),
            ProcessedThisFrame(false),
            PlayerID(playerId)
        {
        };

        TouchControlRegion(
            ) :
            UpperLeftCoords(XMFLOAT2(0,0)),
            LowerRightCoords(XMFLOAT2(0, 0)),
            RegionType(TOUCH_CONTROL_REGION_REPORT_COORDS_ONLY),
            DefinedAction(PLAYER_ACTION_TYPES::INPUT_COORDINATES_ONLY),
            IsEnabled(false),
            ProcessedThisFrame(false),
            PlayerID(PLAYER_ID::PLAYER_ID_ONE)
        {
        };

    };


#pragma endregion

#pragma region InputManagerClassDecl

    // InputManager: the implementation of an input manager type that processes
    // raw XInput controller, keyboard, and touch/mouse pointer events and data
    // into a single queue of state-friendly player actions (such as firing
    // state, movement state, etc).
    class InputManager final
    {
    public:
        // ctor
        InputManager();
        InputManager(unsigned int inputDeviceConfigMask);

        // dtor
        ~InputManager();

        void InputManager::Initialize(Windows::UI::Core::CoreWindow^ coreWindow);

        void InputManager::Update(DX::StepTimer const& timer);

        // ** IMPORTANT **
        // Call this method on the game input update loop to get a vector collection of a specified player's actions.
        //
        std::vector<PlayerInputData> GetPlayersActions();

        //
        // Call this method to set a "touch region," which is a rectangular space on a touch screen
        // surface defined for use as a specific game control. For example, an analog stick or a button press.
        //
        DWORD SetDefinedTouchRegion(
            _In_ const TouchControlRegion * newRegion,
            _Out_ unsigned int& regionId
            );

        void ClearTouchRegions(void);

        void EnableTouchRegion(
            _In_ unsigned int regionId
            );

        void DisableTouchRegion(
            _In_ unsigned int regionId
            );

        // Call this method to set the input devices that will be processed. 
        // Default is INPUT_DEVICE_ALL.
        __forceinline void SetFilter(INPUT_DEVICE_TYPES mask)   { m_inputTypeFilterMask = mask; };

        // Gets metadata describing which players are connected.
        __forceinline unsigned int GetPlayersConnected(void)    { return m_playersConnected; };

        //
        // Pass-through handlers. These process CoreWindow input event data 
        // received by the inner ref class.
        //
        void OnPointerEvent(
            _In_ Windows::UI::Core::CoreWindow^ sender,
            _In_ PointerEventArgs^ args,
            INPUT_EVENT_TYPE type
            );
        void OnKeyEvent(
            _In_ Windows::UI::Core::CoreWindow^ sender,
            _In_ KeyEventArgs^ args,
            INPUT_EVENT_TYPE type
            );


    private: // Private fields for storing input data.

        //
        // Internal class variables
        //

        std::mutex                        m_stateMutex;           // The mutex held during internal state update operations.

        INPUT_DEVICE_TYPES                m_inputTypeFilterMask;  // The input types for which player action should be processed and returned.
        double                            m_timerSeconds;         // Step time for the current update. Used for determining controller disconnect.

        // 2D array used for processing the final set of player actions. This 
        // is used to resolve player action data from all input sources. Input 
        // actions are differentiated between multiple players.
        bool    m_actionsThisFrame[XUSER_MAX_COUNT][PLAYER_ACTION_TYPES::INPUT_MAX];

        // 2D array used to determine if a specific action for a specific 
        // player was returned to the game in the last update. This is for 
        // action state management.
        bool    m_actionsLastFrame[XUSER_MAX_COUNT][PLAYER_ACTION_TYPES::INPUT_MAX];

        // 2D array used to map into player actions. Stores the data as it is 
        // processed, then used to create the vector that's returned to the game loop.
        PlayerInputData m_resolvedActionsThisFrame[XUSER_MAX_COUNT][PLAYER_ACTION_TYPES::INPUT_MAX];

        //
        // Per-frame input source data
        //
        // These input vectors and maps used to track and manage the input 
        // from the three major input sources : XInput(controller), 
        //  pointer(mouse and touch), and keyboard.
        std::vector<XInputControllerAction>*                          m_pXInputActions;            // stores XInput actions for one input frame
        std::unordered_map<unsigned int, unsigned int>*               m_pKeyboardActions;          // pair is virtual key, player id
        std::unordered_map<unsigned int, XMFLOAT2>*                   m_pTouchDownMap;             // pair is pointer id, touchdown coordinates
        std::unordered_map<unsigned int, PointerControllerAction>*    m_pMouseActionsLastFrame;    // Used to track mouse pointers across frames.
        std::unordered_map<unsigned int, PointerControllerAction>*    m_pMouseActions;             // pair is pointer id, last recorded action.
        std::unordered_map<unsigned int, PointerControllerAction>*    m_pTouchActionsLastFrame;    // Used to track touch pointers across frames.
        std::unordered_map<unsigned int, PointerControllerAction>*    m_pTouchActions;             // pair is pointer id, last recorded action.


        //
        // Touch virtual control input region definition and management
        //
        std::vector<TouchControlRegion>* m_pTouchControlRegions; // pair is region id, region (coords and type)

        
        //
        // XInput data
        //
        XINPUT_CAPABILITIES   m_xinputCapabilities[XINPUT_MAX_CONTROLLERS];                  // Array of the capabilities of the XInput controllers. 
        volatile short        m_playersConnected;                                            // The current count of players connected.
        volatile short        m_controllersConnected;                                        // Similar, but tracks internally whether each controller is connected.
        double                m_lastTimeCheckedXInputConnection[XINPUT_MAX_CONTROLLERS];     // Array of the times each controller was last checked for connectivity.


    private: // Private methods for processing input data.

        //
        // Universal input processing methods
        //
        void ClearInputActions(void);
        void TranslateInputToPlayerActions(
            _Inout_ std::vector<PlayerInputData>* playerActions
            );
        void AddPlayerActionToMap(
            _In_ PlayerInputData * const playerInput
            );
        void UpdateLastFrameActionMap(void);
        void AddTransitoryStatesToEventMap(void);
        void ProcessStatesToPlayerActions(
            _Inout_ std::vector<PlayerInputData>* playerActions
            );

        //
        // Pointer processing methods
        //
        void ProcessPointerData(
            _In_ PointerEventArgs^ pointerArgs,
            _Out_ PointerControllerAction * pointerAction
            );
        void TranslateTouchPointerActionsToPlayerActionsMap(void);
        void TranslateMousePointerActionsToPlayerActionsMap(void);
        float InputManager::ComputeThumbstickMagnitudeFactor(
            _In_ float const stickX,
            _In_ float const stickY,
            _In_ int   const deadZone,
            _In_ float const maxThrow
            );

        //
        // Keyboard methods
        //
        void TranslateKeyboardToPlayerActionsMap(void);

        //
        // XInput methods
        //
        void UpdateXInputState(void);

        // Converts raw XInput data into specific player actions.
        void TranslateXInputToPlayerActionMap(void);

        //
        // Touch region methods
        //
        int IsTouchdownInRegion(
            _In_ XMFLOAT2 touchDownPoint
            );

    private: // Private ref class to encapsulate CoreWindow events.

        //
        // This wrapper class encapsulates ref code, limiting the impact of 
        // managed code on native performance. This is necessary because a 
        // CoreWindow can only register a public sealed ref class to receive 
        // input events.
        //
        ref class InputManagerRefWrapper sealed
        {
        internal:
            InputManagerRefWrapper(
                _In_ InputManager* const manager,
                _In_ std::function<void(InputManager* const, CoreWindow^, PointerEventArgs^, INPUT_EVENT_TYPE)> pointerCallback,
                _In_ std::function<void(InputManager* const, CoreWindow^, KeyEventArgs^, INPUT_EVENT_TYPE)> keyCallback
                );
            void Initialize(_In_ CoreWindow^ window);

        protected:
            // 
            // Delegate functions for CoreWindow touch, mouse, and keyboard.
            //
            void OnPointerPressed(
                _In_ CoreWindow^ sender,
                _In_ PointerEventArgs^ args
                );
            void OnPointerMoved(
                _In_ CoreWindow^ sender,
                _In_ PointerEventArgs^ args
                );
            void OnPointerReleased(
                _In_ CoreWindow^ sender,
                _In_ PointerEventArgs^ args
                );
            void OnPointerExited(
                _In_ CoreWindow^ sender,
                _In_ PointerEventArgs^ args
                );
            void OnKeyDown(
                _In_ CoreWindow^ sender,
                _In_ KeyEventArgs^ args
                );
            void OnKeyUp(
                _In_ CoreWindow^ sender,
                _In_ KeyEventArgs^ args
                );

            //
            // NOTE: Enable this delegate if you want to obtain mouse delta data for smoothing.
            //
            /*
            void OnMouseMoved(
            _In_ Windows::Devices::Input::MouseDevice^ mouseDevice,
            _In_ Windows::Devices::Input::MouseEventArgs^ args
            );
            */

        private:

            //
            // Internal class variables
            //
            Platform::Agile<CoreWindow> m_coreWindow;
            InputManager* const         m_manager;      // Native class that owns this class instance (used for callbacks).


            //
            // Registered callback functions
            //
            std::function<void(InputManager* const, CoreWindow^, PointerEventArgs^, INPUT_EVENT_TYPE)> m_pointerEventCallback;
            std::function<void(InputManager* const, CoreWindow^, KeyEventArgs^, INPUT_EVENT_TYPE)>     m_keyEventCallback;
        };

        //
        // This InputManager internal class member stores an active instance of the ref wrapper.
        //
        InputManagerRefWrapper^ m_refWrapper;
    };
#pragma endregion
}
   
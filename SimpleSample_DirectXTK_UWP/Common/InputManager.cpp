//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "InputManager.h"

using namespace DirectX;

using namespace SimpleSample;


#pragma region InputManagerClass


//
// ** BEGIN INITIALIZATION METHODS **
//
#pragma region InputManagerInitMethods

// Default constructor for InputManager class. The default device mask is 
// INPUT_DEVICE_ALL, meaning that the input manager will accept input from 
// keyboard, mouse, XInput controllers, and the touch screen.
InputManager::InputManager() :
    m_inputTypeFilterMask(INPUT_DEVICE_TYPES::INPUT_DEVICE_ALL),
    m_controllersConnected(0),
    m_playersConnected(0)
{
    // init vectors
    m_pXInputActions        = new std::vector<XInputControllerAction>();
    m_pTouchControlRegions  = new std::vector<TouchControlRegion>();

    // init maps
    m_pKeyboardActions          = new std::unordered_map<unsigned int, unsigned int>();
    m_pMouseActions             = new std::unordered_map<unsigned int, PointerControllerAction>();
    m_pMouseActionsLastFrame    = new std::unordered_map<unsigned int, PointerControllerAction>();
    m_pTouchActions             = new std::unordered_map<unsigned int, PointerControllerAction>();
    m_pTouchActionsLastFrame    = new std::unordered_map<unsigned int, PointerControllerAction>();
    m_pTouchDownMap             = new std::unordered_map<unsigned int, XMFLOAT2>();

    // Initialize the cache map of actions taken in the previous frame.
    ZeroMemory(&m_actionsLastFrame, sizeof(bool) * XUSER_MAX_COUNT * PLAYER_ACTION_TYPES::INPUT_MAX);
    ZeroMemory(&m_resolvedActionsThisFrame, sizeof(PlayerInputData) * XUSER_MAX_COUNT * PLAYER_ACTION_TYPES::INPUT_MAX);

    // Initialize the class that can receive CoreWindow events.
    m_refWrapper = ref new InputManagerRefWrapper(
        this,
        std::mem_fn(&InputManager::OnPointerEvent),
        std::mem_fn(&InputManager::OnKeyEvent)
        );
};

// Constructor overload for InputManager that takes a device configuration
// mask of INPUT_DEVICE_TYPES.
InputManager::InputManager(unsigned int inputDeviceConfigMask)
{
    InputManager();

    m_inputTypeFilterMask = (INPUT_DEVICE_TYPES)inputDeviceConfigMask;
};

// Destructor for this type. Destroys all allocated heap data (vectors/maps).
InputManager::~InputManager()
{
    // delete vectors
    delete m_pXInputActions;
    delete m_pTouchControlRegions;

    // delete maps
    delete m_pKeyboardActions;
    delete m_pMouseActions;
    delete m_pMouseActionsLastFrame;
    delete m_pTouchActions;
    delete m_pTouchActionsLastFrame;
    delete m_pTouchDownMap;
};

// Call this method when initializing your game object to start processing 
// CoreInput events (mouse, touch, keyboard).
void InputManager::Initialize(
    _In_ CoreWindow^ coreWindow
    )
{
    m_refWrapper->Initialize(coreWindow);

    // Additionally, check to see which Xbox controllers are initially connected.
    XINPUT_STATE xInputState;
    ZeroMemory(&xInputState, sizeof(XINPUT_STATE));
    for (int id = 0; id < XINPUT_MAX_CONTROLLERS; id++)
    {
        DWORD stateResult = XInputGetState(id, &xInputState);

        // Check whether the controller is aconnected
        if (stateResult == ERROR_SUCCESS)
        {
            m_controllersConnected |= (1 << id);
            m_playersConnected     |= (1 << id);
        }
    }
}

//
// ** END INITIALIZATION METHODS **
//
#pragma endregion



//
// ** BEGIN INPUT PROCESSING METHODS **
//
#pragma region InputProcessingMethods

// Updates the internal countdown between checking for new XInput controllers.
void InputManager::Update(DX::StepTimer const& timer)
{
    m_timerSeconds = timer.GetElapsedSeconds();
}

// Public method that returns a vector of gameplay actions initiated by the 
// player.
std::vector<PlayerInputData> InputManager::GetPlayersActions()
{
    std::vector<PlayerInputData> playerActions;

    // First, clear the map of current actions.
    ZeroMemory(&m_actionsThisFrame, sizeof(bool) * XUSER_MAX_COUNT * PLAYER_ACTION_TYPES::INPUT_MAX);
    
    // Process the set of input data received since the last frame.
    TranslateInputToPlayerActions(&playerActions);

    // Return the input data.
    return playerActions;
}

// Private method that performs the translation of raw input maps to a vector 
// of player gameplay actions. Converts the raw input lists into a single 
// unified vector of player actions that are returned to the game proper.
void InputManager::TranslateInputToPlayerActions(std::vector<PlayerInputData>* playerActions)
{
    if (playerActions == nullptr) return;

    // First process the XInput action vector.
    if ((m_inputTypeFilterMask & INPUT_DEVICE_TYPES::INPUT_DEVICE_XINPUT) == INPUT_DEVICE_TYPES::INPUT_DEVICE_XINPUT)
    {
        // Pull and process data from XInput.
        UpdateXInputState();
        TranslateXInputToPlayerActionMap();
    }

    // Lock the input event queues until we're done processing them.
    std::lock_guard<std::mutex> lock(m_stateMutex);

    // Now, process the Keyboard queue and update the action map.
    if ((m_inputTypeFilterMask & INPUT_DEVICE_TYPES::INPUT_DEVICE_KEYBOARD) == INPUT_DEVICE_TYPES::INPUT_DEVICE_KEYBOARD)
    {
        // Process keyboard events received since the last frame.
        TranslateKeyboardToPlayerActionsMap();
    }

    // Last, process the Pointer queue and update the action map (Mouse and Touch).
    if ((m_inputTypeFilterMask & INPUT_DEVICE_TYPES::INPUT_DEVICE_TOUCH) == INPUT_DEVICE_TYPES::INPUT_DEVICE_TOUCH)
    {
        // Process pointer events received since the last frame.
        TranslateTouchPointerActionsToPlayerActionsMap();

        // Process mouse events received since the last frame.
        TranslateMousePointerActionsToPlayerActionsMap();
    }

    // Now, if the state for any of the digital controls changed since the last frame, set the appropriate transitory state.
    AddTransitoryStatesToEventMap();

    // Process the set of input states into the client code's vector.
    ProcessStatesToPlayerActions(playerActions);
    
    // Store important data for detecting state transitions between frames.
    UpdateLastFrameActionMap();

    // Clear the raw input action vectors and internal input data state to prepare for the next frame.
    ClearInputActions();
}

// Adds a gameplay action to the vector returned by the input manager. This 
// method checks for redundancy, and merges similar events obtained within 
// the frame into each player's input state.
void InputManager::AddPlayerActionToMap(
    _In_  PlayerInputData * const playerInput
    )
{
    unsigned int idVal = playerInput->ID;
    unsigned int actionVal = playerInput->PlayerAction;

    if (m_actionsThisFrame[idVal][actionVal]) // action already taken this frame
    {
        if ((playerInput->PlayerAction == INPUT_MOVE)  || (playerInput->PlayerAction == INPUT_AIM))
        {
            // Resolve move conflicts with XInput and Touch controller if default pointer player ID
            // is the same as an attached XInput controller player ID.
            float combinedInput = m_resolvedActionsThisFrame[idVal][actionVal].NormalizedInputValue + playerInput->NormalizedInputValue;

            if (combinedInput > 1.0f) m_resolvedActionsThisFrame[idVal][actionVal].NormalizedInputValue = 1.0f; // ceiling combined positive input
            else if (combinedInput < -1.0f) m_resolvedActionsThisFrame[idVal][actionVal].NormalizedInputValue = -1.0f; // floor combined negative input
            else m_resolvedActionsThisFrame[idVal][actionVal].NormalizedInputValue = combinedInput; // add combined input to resolved actions map

            float combinedInputX = m_resolvedActionsThisFrame[idVal][actionVal].X + playerInput->X;
            if (combinedInputX > 1.0f) m_resolvedActionsThisFrame[idVal][actionVal].X = 1.0f; // ceiling combined positive X direction input
            else if (combinedInputX < -1.0f) m_resolvedActionsThisFrame[idVal][actionVal].X = -1.0f; // floor combined negative X input
            else m_resolvedActionsThisFrame[idVal][actionVal].X = combinedInputX; // add combined X direction input to resolved actions map

            float combinedInputY = m_resolvedActionsThisFrame[idVal][actionVal].Y + playerInput->Y;
            if (combinedInputY > 1.0f) m_resolvedActionsThisFrame[idVal][actionVal].Y = 1.0f; // ceiling combined positive Y direction input
            else if (combinedInputY < -1.0f) m_resolvedActionsThisFrame[idVal][actionVal].Y = -1.0f; // floor combined negative Y input
            else m_resolvedActionsThisFrame[idVal][actionVal].Y = combinedInputY; // add combined Y direction input to resolved actions map

            if (playerInput->IsTouchAction)
            {
                m_resolvedActionsThisFrame[idVal][actionVal].PointerRawX   = playerInput->PointerRawX;
                m_resolvedActionsThisFrame[idVal][actionVal].PointerRawY   = playerInput->PointerRawY;
                m_resolvedActionsThisFrame[idVal][actionVal].PointerThrowX = playerInput->PointerThrowX;
                m_resolvedActionsThisFrame[idVal][actionVal].PointerThrowY = playerInput->PointerThrowY;
                m_resolvedActionsThisFrame[idVal][actionVal].IsTouchAction = true;
            }
        }
    }
    else
    {
        // since this input is new this frame there is no resolve action needed. Add directly to input map.
        m_resolvedActionsThisFrame[idVal][actionVal] = *playerInput;
        m_actionsThisFrame[idVal][actionVal] = true;
    }
}

// Perform a cleanup pass resolving all state transitions (PRESSED, RELEASED et al) within the action vector.
void InputManager::ProcessStatesToPlayerActions(
    _Inout_ std::vector<PlayerInputData>* playerActions
    )
{
    for (unsigned int idVal = 0; idVal < XUSER_MAX_COUNT; idVal++)
    {
        for (unsigned int actionVal = 0; actionVal < PLAYER_ACTION_TYPES::INPUT_MAX; actionVal++)
        {
            // First, check if this a buttons state processing effort 
            if (m_actionsThisFrame[idVal][actionVal])
            {
                playerActions->push_back(m_resolvedActionsThisFrame[idVal][actionVal]);
            }
        }
    }
}

// After the player gameplay inputs have been interpreted, update an internal map
// so the next frame knows what was performed by the player in this frame.
void InputManager::UpdateLastFrameActionMap()
{    
    // Store mouse pointer actions that occurred this frame.
    if ((m_pMouseActions != nullptr) && (m_pMouseActions->size() > 0) && (m_pMouseActionsLastFrame != nullptr))
    {
        m_pMouseActionsLastFrame->clear();

        auto iter = m_pMouseActions->begin();
        while (iter != m_pMouseActions->end())
        {
            m_pMouseActionsLastFrame->emplace(std::pair<unsigned int, PointerControllerAction>(iter->first, iter->second));
            ++iter;
        }
    }

    // Store touch pointer actions that occurred this frame.
    if ((m_pTouchActions != nullptr) && (m_pTouchActions->size() > 0) && (m_pTouchActionsLastFrame != nullptr))
    {
        m_pTouchActionsLastFrame->clear();

        auto iter = m_pTouchActions->begin();
        while (iter != m_pTouchActions->end())
        {
            m_pTouchActionsLastFrame->emplace(std::pair<unsigned int, PointerControllerAction>(iter->first, iter->second));
            ++iter;
        }
    }

    // Copy the map from the current frame to the last action frame before tick
    CopyMemory(m_actionsLastFrame, m_actionsThisFrame, sizeof(bool) * XUSER_MAX_COUNT * PLAYER_ACTION_TYPES::INPUT_MAX);
}

// For each possible digital action, checks for a change in state between the 
// previous frame and the current frame.
void InputManager::AddTransitoryStatesToEventMap(void)
{

    // Note that the input manager does not check analog actions for state 
    // transitions. It is up to the game to account for (and respond to 
    // changes) in analog state in a way that's appropriate for gameplay.
    for (unsigned int idVal = 0; idVal < XUSER_MAX_COUNT; idVal++)
    {
        for (unsigned int actionVal = 0; actionVal < PLAYER_ACTION_TYPES::INPUT_MAX; actionVal++)
        {
            if (m_actionsLastFrame[idVal][actionVal] != m_actionsThisFrame[idVal][actionVal])
            {
                PLAYER_ACTION_TYPES action = PLAYER_ACTION_TYPES::INPUT_NONE;

                switch ((PLAYER_ACTION_TYPES)actionVal)
                {
                case INPUT_FIRE_DOWN:
                    if (m_actionsThisFrame[idVal][actionVal])
                    {
                        action = INPUT_FIRE_PRESSED;
                    }
                    else
                    {
                        action = INPUT_FIRE_RELEASED;
                    }
                    break;

                case INPUT_JUMP_DOWN:
                    if (m_actionsThisFrame[idVal][actionVal])
                    {
                        action = INPUT_JUMP_PRESSED;
                    }
                    else
                    {
                        action = INPUT_JUMP_RELEASED;
                    }
                    break;

                default:
                    break;
                };

                if (action != PLAYER_ACTION_TYPES::INPUT_NONE)
                {
                    PlayerInputData playerInput;

                    playerInput.ID = idVal;
                    playerInput.PlayerAction = action;
                    playerInput.NormalizedInputValue = 1.0f;

                    m_resolvedActionsThisFrame[idVal][static_cast<int>(action)] = playerInput;
                    m_actionsThisFrame[idVal][static_cast<int>(action)] = true;
                }
            }
        }
    }
}

// Clears the XInput raw input list, pointer input list, and any state variables that only count for a single frame.
void InputManager::ClearInputActions()
{
    // Clear the XInput input action vector.
    if ((m_pXInputActions != nullptr) && (m_pXInputActions->size() > 0))
    {
        m_pXInputActions->clear();
    }

    // Clear the mouse pointer input action map. 
    if ((m_pMouseActions != nullptr) && (m_pMouseActions->size() > 0))
    {
        m_pMouseActions->clear();
    }

    // Clear the touch pointer input action map. 
    if ((m_pTouchActions != nullptr) && (m_pTouchActions->size() > 0))
    {
        m_pTouchActions->clear();
    }
};

//
// ** END INPUT PROCESSING METHODS **
//
#pragma endregion



// 
// ** START XINPUT PROCESSING METHODS **
//
#pragma region XInputProcessingMethods

// Checks the state of the XInput controllers and updates internal fields that 
// represent controller connectivity and capabilities. This is called whenever 
// the game loop requests the current input state.
void InputManager::UpdateXInputState()
{
    // Loop through all controllers.
    for (unsigned int controllerId = 0; controllerId < XINPUT_MAX_CONTROLLERS; controllerId++)
    {
        unsigned int tmpId = controllerId;
        unsigned short controllerIdBitFlag = 1 << tmpId;
        
        // Check whether the controller is known to be connected.
        if (m_controllersConnected & controllerIdBitFlag)
        {
            XINPUT_STATE xInputState;
            ZeroMemory(&xInputState, sizeof(XINPUT_STATE));

            // If so, attempt to get the current controller state.
            DWORD stateResult = XInputGetState(controllerId, &xInputState);

            // Check whether the controller is still connected.
            if (stateResult == ERROR_SUCCESS)
            {
                // If so, append the action to the vector.
                XInputControllerAction controllerInput;
                ZeroMemory(&controllerInput, sizeof(XInputControllerAction));

                // Set the player controller ID.
                controllerInput.ControllerId = controllerId;

                // Push the controller state onto the XInput input action vector.
                // Note: State contains ALL actions for that controller received during poll.
                controllerInput.State = xInputState;
                m_pXInputActions->push_back(controllerInput);
            }
            else
            {
                // Controller was disconnected. Ensure this player and this
                // controller are marked inactive.
                // m_controllersConnected ^= controllerIdBitFlag;
                // m_playersConnected     ^= controllerIdBitFlag;
                InterlockedXor16(&m_controllersConnected, controllerIdBitFlag);
                InterlockedXor16(&m_playersConnected, controllerIdBitFlag);

                // Note:This marks player 1 as completely inactive. Use this to
                // pause the game when the controller is disconnected unexpectedly.
                //
                // The next keyboard, mouse, or input event will mark player 1 
                // as active again.
            }
        }
        else
        {
            m_lastTimeCheckedXInputConnection[controllerId] -= m_timerSeconds;

            if (m_lastTimeCheckedXInputConnection[controllerId]  <= 0.f) 
            {
                int i = 1;
                // If it's time to check whether the controller is connected, 
                // check for XInput controller connection by trying to get 
                // the capabilities.

                // Use a separate thread. XInputGetCapabilities is a blocking operation 
                // that is on the order of milliseconds to complete.
                Windows::System::Threading::ThreadPool::RunAsync(
                    ref new Windows::System::Threading::WorkItemHandler(
                    [&, controllerIdBitFlag, controllerId](IAsyncAction^ workItem) {
                        DWORD getCapsResult = XInputGetCapabilities(controllerId, XINPUT_FLAG_GAMEPAD, &m_xinputCapabilities[controllerId]);

                        // If it is connected then store the result.
                        if (getCapsResult == ERROR_SUCCESS)
                        {
                            // Ensure this player and controller are marked active.
                            // m_controllersConnected |= controllerIdBitFlag;
                            // m_playersConnected     |= controllerIdBitFlag;
                            InterlockedOr16(&m_controllersConnected, controllerIdBitFlag);
                            InterlockedOr16(&m_playersConnected, controllerIdBitFlag);
                        }
                }),
                    Windows::System::Threading::WorkItemPriority::Normal
                    );

                // Start the timer for the next check.
                m_lastTimeCheckedXInputConnection[controllerId] = XINPUT_CONTROLLER_ENUM_TIMEOUT/1000.f;
            }
        }
    }
}

// Converts XInput controller input data to player gameplay actions.
void InputManager::TranslateXInputToPlayerActionMap()
{
    // Interpret queues and add actions to playerActions queue.
    for (unsigned int i = 0; i != m_pXInputActions->size(); i++)
    {
        // Set up a player input data structure.
        PlayerInputData playerInput;
        playerInput.ID = (PLAYER_ID) (*m_pXInputActions)[i].ControllerId;

        /*
        NOTE: The wButton bitmask is defined as follows (from XInput.h):

        XINPUT_GAMEPAD_DPAD_UP          0x0001
        XINPUT_GAMEPAD_DPAD_DOWN        0x0002
        XINPUT_GAMEPAD_DPAD_LEFT        0x0004 
        XINPUT_GAMEPAD_DPAD_RIGHT       0x0008
        XINPUT_GAMEPAD_START            0x0010
        XINPUT_GAMEPAD_BACK             0x0020
        XINPUT_GAMEPAD_LEFT_THUMB       0x0040 -- l-stick click in
        XINPUT_GAMEPAD_RIGHT_THUMB      0x0080 -- r-stick click in
        XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100
        XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200
        XINPUT_GAMEPAD_A                0x1000
        XINPUT_GAMEPAD_B                0x2000
        XINPUT_GAMEPAD_X                0x4000
        XINPUT_GAMEPAD_Y                0x8000
        */

        // All digital inputs are set to a value of 1.0f for usage convenience.
        
        //
        // NOTE TO DEVELOPER: The following section of code is where you add 
        // or update behaviors for different XInput input actions. Use this 
        // implementation as a template when adding your own behaviors.
        //

        // Handle button presses.

        if ((*m_pXInputActions)[i].State.Gamepad.wButtons & XINPUT_GAMEPAD_START)
        {
            playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_START;
            playerInput.NormalizedInputValue = 1.0f;
            AddPlayerActionToMap(&playerInput);
        }

        if ((*m_pXInputActions)[i].State.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
        {
            playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_EXIT;
            playerInput.NormalizedInputValue = 1.0f;
            AddPlayerActionToMap(&playerInput);
        }

        if ((*m_pXInputActions)[i].State.Gamepad.wButtons & XINPUT_GAMEPAD_A)
        {
            playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_FIRE_DOWN;
            playerInput.NormalizedInputValue = 1.0f;
            AddPlayerActionToMap(&playerInput);
        }

        if ((*m_pXInputActions)[i].State.Gamepad.wButtons & XINPUT_GAMEPAD_B)
        {
            playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_JUMP_DOWN;
            playerInput.NormalizedInputValue = 1.0f;
            AddPlayerActionToMap(&playerInput);
        }

        if ((*m_pXInputActions)[i].State.Gamepad.wButtons & XINPUT_GAMEPAD_X)
        {
            playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_SELECT;
            playerInput.NormalizedInputValue = 1.0f;
            AddPlayerActionToMap(&playerInput);
        }

        if ((*m_pXInputActions)[i].State.Gamepad.wButtons & XINPUT_GAMEPAD_Y)
        {
            playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_CANCEL;
            playerInput.NormalizedInputValue = 1.0f;
            AddPlayerActionToMap(&playerInput);
        }

        // Handle analog inputs.

        // Handle trigger inputs.
        if ((*m_pXInputActions)[i].State.Gamepad.bLeftTrigger > 0)
        {
            // NOTE:  Value is between 0.f and 256.f. However, analog triggers often have a max value slightly less than 256.f.
            const float padding = 256.f * 0.99f;
            float paddedValue = (float) (*m_pXInputActions)[i].State.Gamepad.bLeftTrigger / (padding);

            playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_BRAKE;
            playerInput.NormalizedInputValue = (paddedValue > 1.f) ? 1.f : paddedValue; // Check for values slightly past our padded range.
            AddPlayerActionToMap(&playerInput);
        }

        if ((*m_pXInputActions)[i].State.Gamepad.bRightTrigger > 0)
        {
            playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_FIRE_DOWN;

            // As an example, we treat the analog R-trigger as digital.
            // Instead of using the input value we set the digital value of 1.f.
            // If this were an analog action (like braking a car) we would have used a floating point value:
            // playerInput.NormalizedInputValue = ((float) (*m_pXInputActions)[i].State.Gamepad.bRightTrigger / 256.0f);
            playerInput.NormalizedInputValue = 1.f;
            AddPlayerActionToMap(&playerInput);
        }
       

        // NOTE TO DEVELOPER: Values for thumbsticks return between -32768 and 32767. The
        // InputManager normalizes them such that all analog controls, both XInput and
        // virtual touchscreen implementations, return a value between -1.f and 1.f.

        float stickLPressMagnitude = ComputeThumbstickMagnitudeFactor(
            (float) (*m_pXInputActions)[i].State.Gamepad.sThumbLX,
            (float) (*m_pXInputActions)[i].State.Gamepad.sThumbLY,
            (int) XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
            XINPUT_ANALOG_STICK_THROW_MAX
            );

        float stickRPressMagnitude = ComputeThumbstickMagnitudeFactor(
            (float) (*m_pXInputActions)[i].State.Gamepad.sThumbRX,
            (float) (*m_pXInputActions)[i].State.Gamepad.sThumbRY,
            (int) XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,
            XINPUT_ANALOG_STICK_THROW_MAX
            );

        if (((*m_pXInputActions)[i].State.Gamepad.sThumbLX != 0) || ((*m_pXInputActions)[i].State.Gamepad.sThumbLY != 0))
        {
            playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_MOVE;

            // Normalized value of stick press in X direction
            playerInput.X =
                (float) (*m_pXInputActions)[i].State.Gamepad.sThumbLX * stickLPressMagnitude / XINPUT_ANALOG_STICK_THROW_MAX;

            // Normalized value of stick press in Y direction
            playerInput.Y =
                (float) (*m_pXInputActions)[i].State.Gamepad.sThumbLY * stickLPressMagnitude / XINPUT_ANALOG_STICK_THROW_MAX;
            
            playerInput.NormalizedInputValue = 1.f; // Use this field to indicate positive action.
            AddPlayerActionToMap(&playerInput);
        }

        if (((*m_pXInputActions)[i].State.Gamepad.sThumbRX != 0) || ((*m_pXInputActions)[i].State.Gamepad.sThumbRY != 0))
        {
            // Normalized value of stick press in X direction
            playerInput.X =
                (float) (*m_pXInputActions)[i].State.Gamepad.sThumbRX * stickLPressMagnitude / XINPUT_ANALOG_STICK_THROW_MAX;

            // Normalized value of stick press in Y direction
            playerInput.Y =
                (float) (*m_pXInputActions)[i].State.Gamepad.sThumbRY * stickLPressMagnitude / XINPUT_ANALOG_STICK_THROW_MAX;

            playerInput.NormalizedInputValue = 1.f; // Use this field to indicate positive action.
            AddPlayerActionToMap(&playerInput);
        }
        
    }
}

// Compute the normalized magnitude for any analog stick operation, including both 
// the XInput analog sticks and the touch screen virtual analog stick, and normalize
// the input values.
float InputManager::ComputeThumbstickMagnitudeFactor(
    _In_ float const stickX, 
    _In_ float const stickY, 
    _In_ int   const deadZone, 
    _In_ float const maxThrow)
{
    // Determine how far the controller is pushed.
    float magnitude = sqrt(stickX*stickX + stickY*stickY);

    // Avoid dividing by zero.
    if (magnitude == 0) return magnitude;

    float normalizedMagnitude = 0;

    // Check if the controller is outside a circular dead zone.
    if (magnitude > deadZone)
    {
        // Clip the magnitude at its expected maximum value.
        if (magnitude > maxThrow) magnitude = maxThrow;

        // Adjust magnitude relative to the end of the dead zone.
        magnitude -= deadZone;
        normalizedMagnitude = magnitude / (maxThrow - deadZone);
    }
    else // If the controller is in the deadzone zero out the magnitude:
    {
        magnitude = 0.0f;
        normalizedMagnitude = 0.0f;
    }

    return normalizedMagnitude;
}

// 
// ** END XINPUT PROCESSING METHODS **
//
#pragma endregion



//
// ** BEGIN COREWINDOW EVENT PROCESSING **
//
#pragma region MouseTouchKeyboardInputProcessing


// accepts the pass-through values
void InputManager::OnPointerEvent(
    _In_ CoreWindow^ sender, 
    _In_ PointerEventArgs^ args, 
    INPUT_EVENT_TYPE type)
{
    // Initialize a new pointer input action element.
    PointerControllerAction pointerAction;
    ZeroMemory(&pointerAction, sizeof(PointerControllerAction));

    // Perform processing on the pointer data returned by the
    // delegate arguments.
    ProcessPointerData(args, &pointerAction);

    // Updating pointer map, lock for access.
    std::lock_guard<std::mutex> lock(m_stateMutex);

    if (type == INPUT_EVENT_TYPE::INPUT_EVENT_TYPE_DOWN)
    {
        // Set the pointer ID and coordinate tuple for the touch down point (i.e. where the
        // press event started).
        XMFLOAT2 touchDownCoords = XMFLOAT2(0, 0);

        touchDownCoords.x = pointerAction.CurrentX;
        touchDownCoords.y = pointerAction.CurrentY;

        // Add the pointer action to the persistent state map.
        m_pTouchDownMap->emplace(std::pair<unsigned int, XMFLOAT2>(pointerAction.PointerId, touchDownCoords));
    }

    if (pointerAction.IsTouchEvent)
    {
        // Add the pointer action to the pointer action map.
        m_pTouchActions->emplace(std::pair<unsigned int, PointerControllerAction>(pointerAction.PointerId, pointerAction));
    }
    else
    {
        // Add the pointer action to the pointer action map.
        m_pMouseActions->emplace(std::pair<unsigned int, PointerControllerAction>(pointerAction.PointerId, pointerAction));
    }
}


// Extracts data from the pointer event, and determines whether it's a touch or mouse event.
void InputManager::ProcessPointerData(
    _In_ PointerEventArgs^ pointerArgs,
    _Out_ PointerControllerAction * pointerAction
    )
{
    PointerPoint^ point     = pointerArgs->CurrentPoint;
    uint32 pointerId        = point->PointerId;
    Point pointerPosition   = point->Position;
    auto pointerDevice      = point->PointerDevice;
    auto pointerDeviceType  = pointerDevice->PointerDeviceType;

    PointerPointProperties^ pointProperties = point->Properties;
    
    XMFLOAT2 position        = XMFLOAT2(pointerPosition.X, pointerPosition.Y); // Math type
    pointerAction->PointerId = pointerId;
    pointerAction->CurrentX  = position.x;
    pointerAction->CurrentY  = position.y;
    
    // Check event type.
    if (pointerDeviceType == Windows::Devices::Input::PointerDeviceType::Mouse)
    {
        // The event is a mouse event.
        pointerAction->IsMouseEvent = true;
        pointerAction->IsTouchEvent = false;

        // Mouse buttons.
        pointerAction->IsRightButtonPressed  = (pointProperties->IsRightButtonPressed)  ? true : false;
        pointerAction->IsMiddleButtonPressed = (pointProperties->IsMiddleButtonPressed) ? true : false;
        pointerAction->IsLeftButtonPressed   = (pointProperties->IsLeftButtonPressed)   ? true : false;
    }
    else 
    {
        // The event is a touch event.
        pointerAction->IsTouchEvent = true;
        pointerAction->IsMouseEvent = false;

        pointerAction->IsLeftButtonPressed  = (pointProperties->IsLeftButtonPressed)   ? true : false;
        pointerAction->IsRightButtonPressed = (pointProperties->IsBarrelButtonPressed) ? true : false;
    }
}

//
// Touch region methods
//

// If the touch coordinates are in a region, returns an index into the region 
// they're in. If the touch coordinates are not in a region, this method 
// returns INVALID_TOUCH_REGION_ID (-1).
int InputManager::IsTouchdownInRegion(
    _In_ XMFLOAT2 touchDownPoint
    )
{
    int i = 0;

    auto iter = m_pTouchControlRegions->begin();
    while (iter != m_pTouchControlRegions->end())
    {
        // Simple collision test checks whether a pointer is in a defined touch region.
        // NOTE TO DEVELOPERS: You can replace this with a call to your own collision testing function.
        XMFLOAT2 tl = iter->UpperLeftCoords;
        XMFLOAT2 br = iter->LowerRightCoords;

        if ((touchDownPoint.x > br.x) ||
            (touchDownPoint.x < tl.x) ||
            (touchDownPoint.y > br.y) ||
            (touchDownPoint.y < tl.y))
        {
            // no collision
            ++i;
            ++iter;
            continue;
        }
        else
        {
            // collision
            return i;
        }
    }

    return INVALID_TOUCH_REGION_ID;
}

void InputManager::EnableTouchRegion(
    _In_ unsigned int regionId
    )
{
    if (regionId <= m_pTouchControlRegions->size())
        (*m_pTouchControlRegions)[regionId].IsEnabled = true;
}

void InputManager::DisableTouchRegion(
    _In_ unsigned int regionId
    )
{
    if (regionId <= m_pTouchControlRegions->size())
        (*m_pTouchControlRegions)[regionId].IsEnabled = false;
}


// Public method to set a touch region.
// Note: maxLogicalSize SHOULD be set to m_deviceResources->GetLogicalSize().
DWORD InputManager::SetDefinedTouchRegion(
    _In_ const TouchControlRegion * newRegion,
    _Out_ unsigned int& regionId
    )
{
    // First, validate all parameters. Bad touch regions can create weird artifacts!
    
    // Are all coordinates 0.f or greater?
    if ((newRegion->UpperLeftCoords.x  < 0.f) || (newRegion->UpperLeftCoords.y  < 0.f) ||
        (newRegion->LowerRightCoords.x < 0.f) || (newRegion->LowerRightCoords.y < 0.f))
    {
        return INVALID_TOUCH_REGION_OFFSCREEN;
    }

    // Make sure the upper left coord is in fact the upper left coord, and vice versa.
    if ((newRegion->UpperLeftCoords.x > newRegion->LowerRightCoords.x) ||
        (newRegion->UpperLeftCoords.y > newRegion->LowerRightCoords.y))
    {
        return INVALID_TOUCH_REGION_INVERTED;
    }

    std::vector<TouchControlRegion>::iterator iter = m_pTouchControlRegions->begin();
    while (iter != m_pTouchControlRegions->end())
    {
        // check for overlap
        // Note to developers: You can replace this with your own collision function.
        if (!((newRegion->UpperLeftCoords.x > iter->LowerRightCoords.x) || (newRegion->LowerRightCoords.x < iter->UpperLeftCoords.x) ||
            (newRegion->UpperLeftCoords.y < iter->LowerRightCoords.y) || (newRegion->LowerRightCoords.y > iter->UpperLeftCoords.y)))
        {
            return INVALID_TOUCH_REGION_OVERLAPS;
        }

        ++iter;
    }

    m_pTouchControlRegions->push_back(*newRegion);
    regionId = m_pTouchControlRegions->size() - 1;
        
    return 0;
}

void InputManager::ClearTouchRegions(void)
{
    m_pTouchControlRegions->clear();
}

// Converts raw pointer input received from touch events into 
// specific player gameplay input actions.
void InputManager::TranslateTouchPointerActionsToPlayerActionsMap()
{
    std::unordered_map<unsigned int, PointerControllerAction>::iterator iter = m_pTouchActions->begin();

    // Prevents multiple touch points from providing stick input.
    bool virtualStickProcessedThisFrame[PLAYER_ACTION_TYPES::INPUT_MAX];
    ZeroMemory(virtualStickProcessedThisFrame, sizeof(bool) * PLAYER_ACTION_TYPES::INPUT_MAX);

    // Iterate over the set of pointer actions added for this frame.
    while (iter != m_pTouchActions->end())
    {
        // Enable the player ID for the default pointer player assignment.
        m_playersConnected |= (1 << DEFAULT_POINTER_PLAYER_ID);

        unsigned int pointerId = iter->first;
        PointerControllerAction pointerAction = iter->second;

        int id = IsTouchdownInRegion(XMFLOAT2(pointerAction.CurrentX, pointerAction.CurrentY));

        // Touching the screen provides data for player 1.
        PlayerInputData playerInput;
        playerInput.ID = DEFAULT_POINTER_PLAYER_ID;
        playerInput.IsTouchAction = true;

        if ((id == INVALID_TOUCH_REGION_ID) || (!(*m_pTouchControlRegions)[id].IsEnabled))
        {
            // Any touch input at all has to be recognized. This enables the 
            // virtual controller to display itself when the player touches 
            // the display.

            // Construct dummy event
            playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_NONE;

            // Process and add the player action to the player gameplay action vector.
            AddPlayerActionToMap(&playerInput);

            ++iter;
            continue;
        }

        // Check for analog stick regions.
        if ((*m_pTouchControlRegions)[id].RegionType == TOUCH_CONTROL_REGION_ANALOG_STICK)
        {
            // Check for thumbstick input already received for this region.
            // This prevents random touch points from interfering with the thumbstick.
            if (virtualStickProcessedThisFrame[(*m_pTouchControlRegions)[id].DefinedAction])
            {
                ++iter;
                continue;
            }
            else
            {
                virtualStickProcessedThisFrame[(*m_pTouchControlRegions)[id].DefinedAction] = true;
            }

            float centerX, centerY;

            XMFLOAT2 pointerTD = (*m_pTouchDownMap)[pointerId];

            // Check to see whether we are already watching this pointer.
            std::unordered_map<unsigned int, PointerControllerAction>::iterator val = m_pTouchActionsLastFrame->find(pointerId);
            if (val == m_pTouchActionsLastFrame->end())
            {
                // We are not watching this touch point, so we need to store the initial touch coordinates.
                // Store the center position for the virtual analog stick display.
                centerX = pointerAction.CurrentX;
                centerY = pointerAction.CurrentY;
            }
            else
            {
                // We are currently watching this touch point. Update state based on event type.
                if (!pointerAction.IsLeftButtonPressed)
                {
                    // Pointer is up, so remove it from the processing state.
                    auto temp = iter;
                    ++iter;
                    m_pTouchActions->erase(temp);
                    m_pTouchActionsLastFrame->erase(val);
                    continue;
                }
                else
                {
                    // Pointer has moved.

                    // Get back the initial touch coordinates.
                    centerX = pointerTD.x;
                    centerY = pointerTD.y;

                    // Already processed.
                    m_pTouchActionsLastFrame->erase(val);
                }
            }

            // Calculate the delta between this action and the pointer touch down action recorded
            // previously for this pointer ID. Assume it is a touch controller or drag operation.
            float xDelta = pointerAction.CurrentX - centerX;
            float yDelta = pointerAction.CurrentY - centerY;

            // Enforce a circular limit for the controller stick.
            float magnitude = sqrtf(xDelta*xDelta + yDelta*yDelta);

            // Move the thumbstick with the user's thumb.
            if (magnitude > POINTER_VIRTUAL_STICK_THROW_MAX)
            {
                // Compute how far out the user dragged the joystick.
                float magnitudeFactor = POINTER_VIRTUAL_STICK_THROW_MAX / magnitude;
                float temp1 = xDelta * magnitudeFactor;
                float temp2 = yDelta * magnitudeFactor;

                // Move the center by the same amount.
                centerX = (*m_pTouchDownMap)[pointerId].x += xDelta - temp1;
                centerY = (*m_pTouchDownMap)[pointerId].y += yDelta - temp2;

                // Reduce the "thrown" values to their max size.
                xDelta = temp1;
                yDelta = temp2;
            }

            // Compute the value we need to normalize the x/y values to a total distance of 1.0f.
            float normalizedMagnitude = ComputeThumbstickMagnitudeFactor(
                xDelta,
                yDelta,
                (int) POINTER_VIRTUAL_STICK_DEADZONE,
                POINTER_VIRTUAL_STICK_THROW_MAX
                );

            // Updated the coordinate data.

            // Store the stick's center position for virtual controller rendering.
            playerInput.PointerRawX = centerX;
            playerInput.PointerRawY = centerY;

            // Store the stick's current "thrown" position for virtual controller rendering.
            playerInput.PointerThrowX = centerX + xDelta;
            playerInput.PointerThrowY = centerY + yDelta;

            // Normalized input data. This corresponds to the normalized axis values you'd 
            // get from a physical joystick.
            playerInput.X = normalizedMagnitude * xDelta / POINTER_VIRTUAL_STICK_THROW_MAX;
            // Y-value must be reversed to map pixel coordinate system to Xbox controller behaviors
            playerInput.Y = -(normalizedMagnitude * yDelta / POINTER_VIRTUAL_STICK_THROW_MAX); 

            // Move actions store the normalized axis values in X and Y, so the 
            // NormalizedInputValue field is unused.
            // You can use this for any additional calibration or smoothing data, like an
            // interpolation modifier.
            playerInput.NormalizedInputValue = 1.f;

            // Store the action type for this touch region.
            switch ((*m_pTouchControlRegions)[id].DefinedAction)
            {
            default:
            case PLAYER_ACTION_TYPES::INPUT_MOVE:
                playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_MOVE;
                break;

            case PLAYER_ACTION_TYPES::INPUT_AIM:
                playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_AIM;
                break;
            }

            // Process the X and Y data and use it to update the player gameplay action vector.
            AddPlayerActionToMap(&playerInput);
        }
        else if ((*m_pTouchControlRegions)[id].RegionType == TOUCH_CONTROL_REGION_BUTTON)
        {
            // Virtual button was pressed.

            // Include updated coordinates.
            playerInput.PointerRawX = playerInput.X = pointerAction.CurrentX;
            playerInput.PointerRawY = playerInput.Y = pointerAction.CurrentY;

            // The action type is defined with the touch region.
            playerInput.PlayerAction = (*m_pTouchControlRegions)[id].DefinedAction;
            playerInput.NormalizedInputValue = 1.f;

            // Process and add the player action to the player gameplay action vector.
            AddPlayerActionToMap(&playerInput);
        }

        // NOTE TO DEVELOPERS: Add your own code for handling slide controls, etc. by adding more cases here.

        ++iter;
    }
}


// Converts raw pointer input received from mouse events into 
// specific player gameplay input actions.
void InputManager::TranslateMousePointerActionsToPlayerActionsMap()
{
    std::unordered_map<unsigned int, PointerControllerAction>::iterator iter = m_pMouseActions->begin();

    // Iterate over the set of pointer actions added for this frame.
    while (iter != m_pMouseActions->end())
    {
        // Enable the player ID for the default keyboard player assignment.
        m_playersConnected |= (1 << DEFAULT_POINTER_PLAYER_ID);

        unsigned int pointerId = iter->first;
        PointerControllerAction pointerAction = iter->second;

        PlayerInputData playerInput;
        playerInput.ID = DEFAULT_POINTER_PLAYER_ID;

        // Include updated coordinates.
        // For mouse updates, the raw input coords and the returned x and y values are the same.
        //  This is different for "virtual" controls like the touch analog stick.
        playerInput.PointerRawX = playerInput.X = pointerAction.CurrentX;
        playerInput.PointerRawY = pointerAction.CurrentY = pointerAction.CurrentY;
            
        // Check to see whether we are already watching this pointer.
        std::unordered_map<unsigned int, PointerControllerAction>::iterator pointerLastFrame = m_pMouseActionsLastFrame->find(pointerId);
        if (pointerLastFrame != m_pMouseActionsLastFrame->end())
        {
            playerInput.PlayerAction = (pointerAction.IsLeftButtonPressed) ? PLAYER_ACTION_TYPES::INPUT_FIRE_DOWN : PLAYER_ACTION_TYPES::INPUT_FIRE_UP;
            playerInput.NormalizedInputValue = 1.0f;
                
            // if the pointer isn't providing any state, discard it
            if (!(pointerAction.IsLeftButtonPressed || pointerAction.IsMiddleButtonPressed || pointerAction.IsRightButtonPressed))
            {
                if ((pointerAction.CurrentX == pointerLastFrame->second.CurrentX) && (pointerAction.CurrentY == pointerLastFrame->second.CurrentY))
                {
                    auto temp = iter;
                    ++iter;

                    // Erase it: we already processed it, this pointer is done for now.
                    m_pMouseActions->erase(temp);
                    m_pMouseActionsLastFrame->erase(pointerLastFrame);

                    continue;
                }
                else
                {
                    // Return 0.f as the normalized pointerLastFrameue. The coordinate data is what 
                    // the game will be processing in this case.
                    playerInput.NormalizedInputValue = 0.f;
                    playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_COORDINATES_ONLY;
                }
            }

            // Process and add the player action to the player gameplay action vector.
            AddPlayerActionToMap(&playerInput);

            // Already processed
            m_pMouseActionsLastFrame->erase(pointerLastFrame);
        }
        else
        {
            // This must be a pointer we're not currently tracking.

            if (pointerAction.IsLeftButtonPressed)
            {
                // New action detected with left-click.
                playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_FIRE_DOWN;
                playerInput.NormalizedInputValue = 1.f;
            }
            else
            {
                // Return 0.f as the normalized value. The coordinate data is what 
                // the game will be processing in this case.
                playerInput.NormalizedInputValue = 0.f;
                playerInput.PlayerAction = PLAYER_ACTION_TYPES::INPUT_COORDINATES_ONLY;
            }

            // Process and add the X and Y data to the player gameplay action vector.
            AddPlayerActionToMap(&playerInput);
        }

        ++iter;
    }

    iter = m_pMouseActionsLastFrame->begin();

    // Look for leftover pointer actions. This indicates a mouse button is still down, but the mouse has not moved.
    while (iter != m_pMouseActionsLastFrame->end())
    {
        unsigned int pointerId = iter->first;
        PointerControllerAction pointerAction = iter->second;

        if (pointerAction.IsMouseEvent)
        {
            PlayerInputData playerInput;
            playerInput.ID = DEFAULT_POINTER_PLAYER_ID;

            // Include updated coordinates.
            playerInput.PointerRawX = playerInput.X = pointerAction.CurrentX;
            playerInput.PointerRawY = pointerAction.CurrentY = pointerAction.CurrentY;

            playerInput.PlayerAction = (pointerAction.IsLeftButtonPressed) ? PLAYER_ACTION_TYPES::INPUT_FIRE_DOWN : PLAYER_ACTION_TYPES::INPUT_FIRE_UP;
            playerInput.NormalizedInputValue = 1.0f;

            // Process and add the player action to the player gameplay action vector.
            AddPlayerActionToMap(&playerInput);
        }
        
        ++iter;
    }
}


// Processes a key event. Called by the inner class (ref wrapper) whenever it 
// receives a keyboard event.
void InputManager::OnKeyEvent(
    _In_ CoreWindow^ sender,
    _In_ KeyEventArgs^ args,
    INPUT_EVENT_TYPE type)
{
    switch (type)
    {
    case INPUT_EVENT_TYPE::INPUT_EVENT_TYPE_DOWN:
        {
            // Updating keyboard input map, lock for access until we're done updating the map.
            std::lock_guard<std::mutex> lock(m_stateMutex);

            // Set the player ID to the default keyboard player assignment.
            unsigned int ID = DEFAULT_KEYBOARD_PLAYER_ID;
            unsigned int keyPress = (unsigned int) args->VirtualKey;

            // If the keypress hasn't already registered, add it. Otherwise, ignore it.
            if (m_pKeyboardActions->find(keyPress) == m_pKeyboardActions->end())
            {
                m_pKeyboardActions->insert(std::pair<unsigned int, unsigned int>(keyPress, ID));
            }
        }
        break;

    case INPUT_EVENT_TYPE::INPUT_EVENT_TYPE_UP:
        {
            // Updating keyboard input map, lock for access until we're done updating the map.
            std::lock_guard<std::mutex> lock(m_stateMutex);

            unsigned int keyPress = (unsigned int) args->VirtualKey;

            // key is released, remove from input map.
            m_pKeyboardActions->erase(keyPress);
        }
        break;
    }
}


// Convert keyboard keypresses and releases into player gameplay actions.
void InputManager::TranslateKeyboardToPlayerActionsMap()
{
    if ((m_pKeyboardActions != nullptr) && (m_pKeyboardActions->size() > 0))
    {
        // Enable the player ID for the default keyboard player assignment.
        m_playersConnected |= (1 << DEFAULT_KEYBOARD_PLAYER_ID);

        // 
        // NOTE TO DEVELOPERS: This code processes keyboard actions and translates
        // them into player gameplay actions. You can add to or update this 
        // code to support the gameplay actions you define for your game.
        //
        if (m_pKeyboardActions->find((unsigned int) VirtualKey::Escape) != m_pKeyboardActions->end()) // found
        {
            PlayerInputData * playerInput = new PlayerInputData();
            
            playerInput->ID                   = DEFAULT_KEYBOARD_PLAYER_ID;
            playerInput->PlayerAction         = PLAYER_ACTION_TYPES::INPUT_EXIT;
            playerInput->NormalizedInputValue = 1.0f;

            AddPlayerActionToMap(playerInput);
        }

        if (m_pKeyboardActions->find((unsigned int) VirtualKey::Space) != m_pKeyboardActions->end()) // non-zero
        {
            PlayerInputData * playerInput = new PlayerInputData();

            playerInput->ID = DEFAULT_KEYBOARD_PLAYER_ID;
            playerInput->PlayerAction         = PLAYER_ACTION_TYPES::INPUT_FIRE_DOWN;
            playerInput->NormalizedInputValue = 1.0f;

            AddPlayerActionToMap(playerInput);
        }
           
        if (m_pKeyboardActions->find((unsigned int) VirtualKey::Control) != m_pKeyboardActions->end()) // found
        {
            PlayerInputData * playerInput = new PlayerInputData();

            playerInput->ID = DEFAULT_KEYBOARD_PLAYER_ID;
            playerInput->PlayerAction = PLAYER_ACTION_TYPES::INPUT_JUMP_DOWN;
            playerInput->NormalizedInputValue = 1.0f;

            AddPlayerActionToMap(playerInput);
        }

        if (m_pKeyboardActions->find((unsigned int) VirtualKey::Left) != m_pKeyboardActions->end()) // found
        {
            PlayerInputData * playerInput = new PlayerInputData();

            playerInput->ID = DEFAULT_KEYBOARD_PLAYER_ID;
            playerInput->PlayerAction = PLAYER_ACTION_TYPES::INPUT_MOVE;
            playerInput->NormalizedInputValue = -1.f;
            playerInput->X = -1.f; // The movement was in the left direction on the x axis.
            playerInput->Y = 0.f; // ...and no y-axis motion.

            AddPlayerActionToMap(playerInput);
        }

        if (m_pKeyboardActions->find((unsigned int) VirtualKey::Right) != m_pKeyboardActions->end()) // found
        {
            PlayerInputData * playerInput = new PlayerInputData();

            playerInput->ID = DEFAULT_KEYBOARD_PLAYER_ID;
            playerInput->PlayerAction = PLAYER_ACTION_TYPES::INPUT_MOVE;
            playerInput->NormalizedInputValue = 1.f;
            playerInput->X = 1.f; // The movement was in the right direction on the x axis.
            playerInput->Y = 0.f; // ...and no y-axis motion.

            AddPlayerActionToMap(playerInput);
        }

        if (m_pKeyboardActions->find((unsigned int) VirtualKey::Up) != m_pKeyboardActions->end()) // found
        {
            PlayerInputData * playerInput = new PlayerInputData();

            playerInput->ID = DEFAULT_KEYBOARD_PLAYER_ID;
            playerInput->PlayerAction = PLAYER_ACTION_TYPES::INPUT_MOVE;
            playerInput->NormalizedInputValue = 1.f;
            playerInput->Y = 1.f; // The movement was in the up direction on the y axis.
            playerInput->X = 0.f; // ...and no x-axis motion.

            AddPlayerActionToMap(playerInput);
        }

        if (m_pKeyboardActions->find((unsigned int) VirtualKey::Down) != m_pKeyboardActions->end()) // found
        {
            PlayerInputData * playerInput = new PlayerInputData();

            playerInput->ID = DEFAULT_KEYBOARD_PLAYER_ID;
            playerInput->PlayerAction = PLAYER_ACTION_TYPES::INPUT_MOVE;
            playerInput->NormalizedInputValue = -1.f;
            playerInput->Y = -1.f; // The movement was in the down direction on the y axis.
            playerInput->X = 0.f; // ...and no x-axis motion.

            AddPlayerActionToMap(playerInput);
        }

        // NOTE: Add if statements here for other keys. Diagonals can be processed by checking for two keypresses
        // (e.g. Up + Right) and setting the X and Y properties of the PlayerInputData accordingly
        // (e.g. X = 1.f, Y = 1.f).
    }
}

#pragma endregion

#pragma endregion

//
// ** START REF WRAPPER **
//
#pragma region InputManagerRefWrapperClass

// ctor
InputManager::InputManagerRefWrapper::InputManagerRefWrapper(
    _In_ InputManager* const manager,
    _In_ std::function<void(InputManager* const, CoreWindow^, PointerEventArgs^, INPUT_EVENT_TYPE)> pointerCallback,
    _In_ std::function<void(InputManager* const, CoreWindow^, KeyEventArgs^, INPUT_EVENT_TYPE)> keyCallback
    ) :
    m_manager(manager),
    m_pointerEventCallback(pointerCallback),
    m_keyEventCallback(keyCallback)
{};

void InputManager::InputManagerRefWrapper::Initialize(_In_ CoreWindow^ window)
{
    m_coreWindow = window;

    // Hook up to the pointer events.
    m_coreWindow->PointerPressed  += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &InputManager::InputManagerRefWrapper::OnPointerPressed);
    m_coreWindow->PointerMoved    += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &InputManager::InputManagerRefWrapper::OnPointerMoved);
    m_coreWindow->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &InputManager::InputManagerRefWrapper::OnPointerReleased);
    m_coreWindow->PointerExited   += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &InputManager::InputManagerRefWrapper::OnPointerExited);

    // Hook up to the keyboard events.
    m_coreWindow->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &InputManager::InputManagerRefWrapper::OnKeyDown);
    m_coreWindow->KeyUp   += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &InputManager::InputManagerRefWrapper::OnKeyUp);


    // There is a separate handler for mouse only relative mouse movement events; uncomment and use if you need the relative mouse data.
    /*
    MouseDevice::GetForCurrentView()->MouseMoved +=
    ref new TypedEventHandler<MouseDevice^, MouseEventArgs^>(this, &InputManager::OnMouseMoved);
    */
}

//
//  ** START MOUSE EVENTS AND METHODS **
//
void InputManager::InputManagerRefWrapper::OnPointerPressed(
    _In_ CoreWindow^ sender,
    _In_ PointerEventArgs^ args
    )
{
    m_pointerEventCallback(m_manager, sender, args, INPUT_EVENT_TYPE_DOWN);
}

void InputManager::InputManagerRefWrapper::OnPointerMoved(
    _In_ CoreWindow^ sender,
    _In_ PointerEventArgs^ args
    )
{
    m_pointerEventCallback(m_manager, sender, args, INPUT_EVENT_TYPE_MOVED);
}

void InputManager::InputManagerRefWrapper::OnPointerReleased(
    _In_ CoreWindow^ sender,
    _In_ PointerEventArgs^ args
    )
{
    m_pointerEventCallback(m_manager, sender, args, INPUT_EVENT_TYPE_UP);
}

void InputManager::InputManagerRefWrapper::OnPointerExited(
    _In_ CoreWindow^ sender,
    _In_ PointerEventArgs^ args
    )
{
    m_pointerEventCallback(m_manager, sender, args, INPUT_EVENT_TYPE_EXITED);
}


//
// ** START KEYBOARD DELEGATES AND METHODS **
//
void InputManager::InputManagerRefWrapper::OnKeyDown(
    _In_ CoreWindow^ sender,
    _In_ KeyEventArgs^ args
    )
{
    m_keyEventCallback(m_manager, sender, args, INPUT_EVENT_TYPE_DOWN);
}

void InputManager::InputManagerRefWrapper::OnKeyUp(
    _In_ CoreWindow^ sender,
    _In_ KeyEventArgs^ args
    )
{
    m_keyEventCallback(m_manager, sender, args, INPUT_EVENT_TYPE_UP);
}

#pragma endregion
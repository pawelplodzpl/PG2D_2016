//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include <string>
#include "Common/DeviceResources.h"
#include "Common/StepTimer.h"
#include "Common/InputManager.h"
#include "Common/OverlayManager.h"

namespace SimpleSample_DirectXTK_UWP
{
    // Displays a virtual stick and buttons for the virtual controller.
    // Derived from the Overlay class and handled by the OverlayManger class.
    class SampleVirtualControllerRenderer : public Overlay
    {
    public:
        SampleVirtualControllerRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        void CreateDeviceDependentResources();
        void ReleaseDeviceDependentResources();
        void Update(DX::StepTimer const& timer);
        void Update(std::vector<PlayerInputData>* playerInput);
        void Render();

        HRESULT AddTouchControlRegion(TouchControlRegion& touchControlRegion);
        void ClearTouchControlRegions();

    private:
        // Holds the Touch Control region and the controller state.
        typedef struct _touchControl
        {
            TouchControlRegion Region;
            bool        ButtonPressed;
            float       PointerRawX;
            float       PointerThrowX;
            float       PointerRawY;
            float       PointerThrowY;

            // Constructor
            _touchControl(
                TouchControlRegion region,
                bool buttonPressed,
                float pointerRawX,
                float pointerThrowX,
                float pointerRawY,
                float pointerThrowY
                ) :
                Region(region),
                ButtonPressed(buttonPressed),
                PointerRawX(pointerRawX),
                PointerThrowX(pointerThrowX),
                PointerRawY(pointerRawY),
                PointerThrowY(pointerThrowY)
            {
            }

            // Constructor
            _touchControl() :
                Region(TouchControlRegion()),
                ButtonPressed(false),
                PointerRawX(0),
                PointerThrowX(0),
                PointerRawY(0),
                PointerThrowY(0)
            {
            }
        } TouchControl;

        std::unordered_map<PLAYER_ACTION_TYPES, TouchControl>     m_touchControls;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>    m_whiteBrush;
        Microsoft::WRL::ComPtr<ID2D1DrawingStateBlock>  m_stateBlock;

        float m_buttonFadeTimer;
        float m_stickFadeTimer;
    };
}
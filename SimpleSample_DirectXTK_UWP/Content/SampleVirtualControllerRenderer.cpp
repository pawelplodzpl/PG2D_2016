//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "SampleVirtualControllerRenderer.h"
#include "Common/DirectXHelper.h"

using namespace SimpleSample_DirectXTK_UWP;


// Initializes D2D resources used for rendering.
SampleVirtualControllerRenderer::SampleVirtualControllerRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
Overlay(deviceResources), m_buttonFadeTimer(9.f), m_stickFadeTimer(1.f)
{
    DX::ThrowIfFailed(
        m_deviceResources->GetD2DFactory()->CreateDrawingStateBlock(&m_stateBlock)
        );

    CreateDeviceDependentResources();
}

// Adds a touch control region for rendering.
HRESULT SampleVirtualControllerRenderer::AddTouchControlRegion(TouchControlRegion &touchControlRegion)
{
    Windows::Foundation::Size logicalSize = m_deviceResources->GetLogicalSize();

    TouchControl touchControl(touchControlRegion, false, -1.f, 270.f, -1.f, logicalSize.Height - 270.f);

    m_touchControls.erase(touchControlRegion.DefinedAction);
    m_touchControls.emplace(touchControlRegion.DefinedAction, touchControl);
    
    return S_OK;
}

// Clears touch control regions.
void SampleVirtualControllerRenderer::ClearTouchControlRegions()
{
    m_touchControls.clear();
}

// Updates any time-based rendering resources (currently none).
// This method must be implemented for the Overlay class.
void SampleVirtualControllerRenderer::Update(DX::StepTimer const& timer)
{
    // Update the timers for fading out unused touch inputs.
    float frameTime = static_cast<float>(timer.GetElapsedSeconds());
    if (m_stickFadeTimer > 0)  m_stickFadeTimer -= frameTime;
    if (m_buttonFadeTimer > 0) m_buttonFadeTimer -= frameTime;
}

// Updates the display based on this frame's input.
// This method is not called by the OverlayManager class.
void SampleVirtualControllerRenderer::Update(std::vector<PlayerInputData>* playerInput)
{
    m_touchControls[PLAYER_ACTION_TYPES::INPUT_MOVE].PointerRawX = -1;
    m_touchControls[PLAYER_ACTION_TYPES::INPUT_FIRE_DOWN].ButtonPressed = false;
    m_touchControls[PLAYER_ACTION_TYPES::INPUT_JUMP_DOWN].ButtonPressed = false;

    for (unsigned int i = 0; i < playerInput->size(); i++)
    {
        PlayerInputData playerAction = (*playerInput)[i];

        if (!playerAction.IsTouchAction)
            continue;

        // Any valid touch on the screen should display the virtual controller.
        m_buttonFadeTimer = 6.f;

        if (m_touchControls.count(playerAction.PlayerAction))
        {
            switch (m_touchControls[playerAction.PlayerAction].Region.RegionType)
            {
            case TOUCH_CONTROL_REGION_TYPES::TOUCH_CONTROL_REGION_ANALOG_STICK:
                m_touchControls[playerAction.PlayerAction].PointerThrowX = playerAction.PointerThrowX;
                m_touchControls[playerAction.PlayerAction].PointerThrowY = playerAction.PointerThrowY;
                m_touchControls[playerAction.PlayerAction].PointerRawX = playerAction.PointerRawX;
                m_touchControls[playerAction.PlayerAction].PointerRawY = playerAction.PointerRawY;

                // The stick location is dynamic; a quick fade helps the transition.
                m_stickFadeTimer = 0.25f;
                break;

            case TOUCH_CONTROL_REGION_TYPES::TOUCH_CONTROL_REGION_BUTTON:
                m_touchControls[playerAction.PlayerAction].ButtonPressed = true;

            default:
                break;
            }
        }
    }
}

// Renders a frame to the screen.
void SampleVirtualControllerRenderer::Render()
{
    ID2D1DeviceContext* context = m_deviceResources->GetD2DDeviceContext();

    context->SaveDrawingState(m_stateBlock.Get());
    context->BeginDraw();

    std::unordered_map<PLAYER_ACTION_TYPES, TouchControl>::iterator iter;
    for (iter = m_touchControls.begin(); iter != m_touchControls.end(); ++iter)
    {
        PLAYER_ACTION_TYPES player_action = iter->first;
        TouchControl touchControl = iter->second;
        TouchControlRegion touchControlRegion = touchControl.Region;

        switch (touchControlRegion.RegionType)
        {
        case TOUCH_CONTROL_REGION_TYPES::TOUCH_CONTROL_REGION_ANALOG_STICK:
            {
                // Draw a virtual analog stick                                                              
                D2D1_ELLIPSE outerStickEllipse;
                
                if (touchControl.PointerRawX > 0)
                {
                    // If there is analog stick input, center on the raw location.
                    outerStickEllipse = D2D1::Ellipse(D2D1::Point2F(touchControl.PointerRawX, touchControl.PointerRawY), 100, 100);
                }
                else
                {
                    // If there is no analog stick input, center on the last throw location.
                    outerStickEllipse = D2D1::Ellipse(D2D1::Point2F(touchControl.PointerThrowX, touchControl.PointerThrowY), 100, 100);
                }
                D2D1_ELLIPSE innerStickEllipse = D2D1::Ellipse(D2D1::Point2F(touchControl.PointerThrowX, touchControl.PointerThrowY), 75, 75);

                // Get opacity based on the time since the user has used the virtual analog stick.
                float opacity = m_stickFadeTimer / 0.25f;

                // save current opacity.
                float previousOpacity = m_whiteBrush->GetOpacity();

                m_whiteBrush->SetOpacity(opacity);

                context->DrawEllipse(outerStickEllipse, m_whiteBrush.Get());
                context->FillEllipse(innerStickEllipse, m_whiteBrush.Get());

                m_whiteBrush->SetOpacity(previousOpacity);
            }
            break;

        case TOUCH_CONTROL_REGION_TYPES::TOUCH_CONTROL_REGION_BUTTON:
            {
                // Draw a button.
                float radiusx = (touchControlRegion.LowerRightCoords.x - touchControlRegion.UpperLeftCoords.x) * 0.5f;
                float radiusy = (touchControlRegion.LowerRightCoords.y - touchControlRegion.UpperLeftCoords.y) * 0.5f;

                D2D1_POINT_2F buttonLoc = D2D1::Point2F(
                    touchControlRegion.UpperLeftCoords.x + radiusx,
                    touchControlRegion.UpperLeftCoords.y + radiusy
                    );

                D2D1_ELLIPSE buttonEllipse = D2D1::Ellipse(buttonLoc, radiusx, radiusy);

                // Get opacity based on the time since the user has used the touch screen.
                float opacity = m_buttonFadeTimer > 3.f ? 1.f : m_buttonFadeTimer / 3.f;

                // Save the opacity.
                float previousOpacity = m_whiteBrush->GetOpacity();

                m_whiteBrush->SetOpacity(opacity);

                if (touchControl.ButtonPressed)
                {
                    context->FillEllipse(buttonEllipse, m_whiteBrush.Get());
                }
                else
                {
                    context->DrawEllipse(buttonEllipse, m_whiteBrush.Get());
                }

                // Set opacity to it's previous value.
                m_whiteBrush->SetOpacity(previousOpacity);
            }
            break;

        default:
            break;
        }
    }

    // Ignore D2DERR_RECREATE_TARGET here. This error indicates that the device
    // is lost. It will be handled during the next call to Present.
    HRESULT hr = context->EndDraw();
    if (hr != D2DERR_RECREATE_TARGET)
    {
        DX::ThrowIfFailed(hr);
    }

    context->RestoreDrawingState(m_stateBlock.Get());
}

// Creates D2D device resources.
void SampleVirtualControllerRenderer::CreateDeviceDependentResources()
{
    DX::ThrowIfFailed(
        m_deviceResources->GetD2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_whiteBrush)
        );
}

// Releases D2D device resources.
void SampleVirtualControllerRenderer::ReleaseDeviceDependentResources()
{
    m_whiteBrush.Reset();
}
//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "SampleDebugTextRenderer.h"
#include "Common/DirectXHelper.h"

using namespace SimpleSample_DirectXTK_UWP;

// Initializes D2D resources used for text rendering.
SampleDebugTextRenderer::SampleDebugTextRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
Overlay(deviceResources)
{
    ZeroMemory(&m_textMetrics, sizeof(DWRITE_TEXT_METRICS) * XINPUT_MAX_CONTROLLERS);
    ZeroMemory(&m_textMetricsFPS, sizeof(DWRITE_TEXT_METRICS));

    for (unsigned int i = 0; i < 4; i++)
    {
        m_text[i] = L"";
    }
    // Create device-independent resources.
    DX::ThrowIfFailed(
        m_deviceResources->GetDWriteFactory()->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_LIGHT,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        32.0f,
        L"en-US",
        &m_textFormat
        )
        );

    DX::ThrowIfFailed(
        m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR)
        );

    DX::ThrowIfFailed(
        m_deviceResources->GetD2DFactory()->CreateDrawingStateBlock(&m_stateBlock)
        );


    // Generate static input text height.
    unsigned int lines = 6; // Increase this if you need to display more than 5 separate action types
    std::wstring inputText = L"";
    for (unsigned int i = 0; i < lines; i++) inputText += L"\n";

    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    DX::ThrowIfFailed(
        m_deviceResources->GetDWriteFactory()->CreateTextLayout(
        inputText.c_str(),
        (uint32) inputText.length(),
        m_textFormat.Get(),
        DEBUG_INPUT_TEXT_MAX_WIDTH,
        DEBUG_INPUT_TEXT_MAX_HEIGHT,
        &layout
        )
        );
    DWRITE_TEXT_METRICS metrics;
    DX::ThrowIfFailed(
        layout->GetMetrics(&metrics)
        );
    m_inputTextHeight = metrics.height;

    CreateDeviceDependentResources();
}

void SampleDebugTextRenderer::Update(DX::StepTimer const& timer)
{
    // Update display text.
    uint32 fps = timer.GetFramesPerSecond();

    m_textFPS = (fps > 0) ? std::to_wstring(fps) + L" FPS" : L" - FPS";

    DX::ThrowIfFailed(
        m_deviceResources->GetDWriteFactory()->CreateTextLayout(
        m_textFPS.c_str(),
        (uint32) m_textFPS.length(),
        m_textFormat.Get(),
        240.0f, // Max width of the FPS text.
        50.0f, // Max height of the FPS text.
        &m_textLayoutFPS
        )
        );

    DX::ThrowIfFailed(
        m_textLayoutFPS->GetMetrics(&m_textMetricsFPS)
        );
}

// Updates the text to be displayed.
void SampleDebugTextRenderer::Update(std::vector<PlayerInputData>* playerInputs, unsigned int playersAttached)
{
    m_playersAttached = playersAttached;

    for (unsigned int i = 0; i < XINPUT_MAX_CONTROLLERS; i++)
    {
        std::wstring inputText = L"";

        unsigned int playerAttached = (playersAttached & (1 << i));

        if (!playerAttached)
            continue;

        for (unsigned int j = 0; j < playerInputs->size(); j++)
        {
            PlayerInputData playerAction = (*playerInputs)[j];

            if (playerAction.ID != i) continue;

            switch (playerAction.PlayerAction)
            {
            case PLAYER_ACTION_TYPES::INPUT_FIRE_PRESSED:
                inputText += L"\n FirePressed(" + std::to_wstring(playerAction.NormalizedInputValue) + L") ";
                break;
            case PLAYER_ACTION_TYPES::INPUT_FIRE_DOWN:
                inputText += L"\n FireDown(" + std::to_wstring(playerAction.NormalizedInputValue) + L") ";
                break;
			case PLAYER_ACTION_TYPES::INPUT_FIRE_RELEASED:
				inputText += L"\n FireReleased(" + std::to_wstring(playerAction.NormalizedInputValue) + L") ";
				break;

            case PLAYER_ACTION_TYPES::INPUT_JUMP_PRESSED:
                inputText += L"\n JumpPressed(" + std::to_wstring(playerAction.NormalizedInputValue) + L") ";
                break;
            case PLAYER_ACTION_TYPES::INPUT_JUMP_DOWN:
                inputText += L"\n JumpDown(" + std::to_wstring(playerAction.NormalizedInputValue) + L") ";
                break;
            case PLAYER_ACTION_TYPES::INPUT_JUMP_RELEASED:
                inputText += L"\n JumpReleased(" + std::to_wstring(playerAction.NormalizedInputValue) + L") ";
                break;

            case PLAYER_ACTION_TYPES::INPUT_MOVE:
				inputText += L"\n MoveX(" + std::to_wstring(playerAction.X) + L") ";
				inputText += L"\n MoveY(" + std::to_wstring(playerAction.Y) + L") ";
                break;
            case PLAYER_ACTION_TYPES::INPUT_AIM:
				inputText += L"\n AimX(" + std::to_wstring(playerAction.X) + L") ";
				inputText += L"\n AimY(" + std::to_wstring(playerAction.Y) + L") ";
                break;
            case PLAYER_ACTION_TYPES::INPUT_BRAKE:
                inputText += L"\n Brake(" + std::to_wstring(playerAction.NormalizedInputValue) + L") ";
                break;

            default:
                break;
            }
        }

        wchar_t intStringBuffer[8];
        size_t sizeInWords = sizeof(intStringBuffer) / 2;
        _itow_s(i + 1, intStringBuffer, sizeInWords, 10);
        std::wstring playerIdString(intStringBuffer);

        m_text[i] = L"Input Player" + playerIdString += L": " + inputText;

        DX::ThrowIfFailed(
            m_deviceResources->GetDWriteFactory()->CreateTextLayout(
            m_text[i].c_str(),
            (uint32) m_text[i].length(),
            m_textFormat.Get(),
            DEBUG_INPUT_TEXT_MAX_WIDTH,
            DEBUG_INPUT_TEXT_MAX_HEIGHT,
            &m_textLayout[i]
            )
            );

        DX::ThrowIfFailed(
            m_textLayout[i]->GetMetrics(&m_textMetrics[i])
            );
    }
}

// Renders a frame to the screen.
void SampleDebugTextRenderer::Render()
{
    ID2D1DeviceContext* context = m_deviceResources->GetD2DDeviceContext();
    Windows::Foundation::Size logicalSize = m_deviceResources->GetLogicalSize();

    context->SaveDrawingState(m_stateBlock.Get());
    context->BeginDraw();

    // Position the controllers in quadrants.
    for (unsigned int i = 0; i < XINPUT_MAX_CONTROLLERS; i++)
    {
        unsigned int playerAttached = (m_playersAttached & (1 << i));

        if (!playerAttached)
            continue;

        float x = logicalSize.Width;
        float y = logicalSize.Height - 50.f; // size of FPS text

        x -= (i % 2) ? m_textMetrics[i].layoutWidth : (2 * m_textMetrics[i].layoutWidth);
        y -= (i > 1) ? m_inputTextHeight : (2 * m_inputTextHeight);

        // Translate the origin (used to position in different quadrants)
        D2D1::Matrix3x2F screenTranslation = D2D1::Matrix3x2F::Translation(x, y);

        context->SetTransform(screenTranslation * m_deviceResources->GetOrientationTransform2D());

        DX::ThrowIfFailed(
            m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING)
            );

        context->DrawTextLayout(
            D2D1::Point2F(0.f, 0.f),
            m_textLayout[i].Get(),
            m_whiteBrush.Get()
            );

    }

    // Position FPS on the bottom right corner.
    D2D1::Matrix3x2F screenTranslation = D2D1::Matrix3x2F::Translation(
        logicalSize.Width - m_textMetricsFPS.layoutWidth,
        logicalSize.Height - m_textMetricsFPS.height
        );

    context->SetTransform(screenTranslation * m_deviceResources->GetOrientationTransform2D());

    DX::ThrowIfFailed(
        m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING)
        );

    context->DrawTextLayout(
        D2D1::Point2F(0.f, 0.f),
        m_textLayoutFPS.Get(),
        m_whiteBrush.Get()
        );

    // Ignore D2DERR_RECREATE_TARGET here. This error indicates that the device
    // is lost. It will be handled during the next call to Present.
    HRESULT hr = context->EndDraw();
    if (hr != D2DERR_RECREATE_TARGET)
    {
        DX::ThrowIfFailed(hr);
    }

    context->RestoreDrawingState(m_stateBlock.Get());
}

void SampleDebugTextRenderer::CreateDeviceDependentResources()
{
    DX::ThrowIfFailed(
        m_deviceResources->GetD2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_whiteBrush)
        );
}
void SampleDebugTextRenderer::ReleaseDeviceDependentResources()
{
    m_whiteBrush.Reset();
}
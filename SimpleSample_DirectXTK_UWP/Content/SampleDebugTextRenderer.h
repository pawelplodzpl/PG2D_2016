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
    // Renders the current  value in the bottom right corner of the screen using Direct2D and DirectWrite.
    class SampleDebugTextRenderer : public Overlay
    {
    public:
        SampleDebugTextRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        void CreateDeviceDependentResources();
        void ReleaseDeviceDependentResources();
        void Update(DX::StepTimer const& timer);
        void Update(std::vector<PlayerInputData>* playerInput, unsigned int playersAttached);
        void Render();

    private:
        // Resources related to text rendering for player input data.
        std::wstring                                    m_text[XINPUT_MAX_CONTROLLERS];
        Microsoft::WRL::ComPtr<IDWriteTextLayout>       m_textLayout[XINPUT_MAX_CONTROLLERS];
        DWRITE_TEXT_METRICS                             m_textMetrics[XINPUT_MAX_CONTROLLERS];

        // Resources related to rendering the FPS text.
        std::wstring                                    m_textFPS;
        Microsoft::WRL::ComPtr<IDWriteTextLayout>       m_textLayoutFPS;
        DWRITE_TEXT_METRICS                             m_textMetricsFPS;

        // Cached height of one input text block.
        float m_inputTextHeight;

        // Cached player metadata.
        unsigned int m_playersAttached;

        // Max width of the input text.
        const float DEBUG_INPUT_TEXT_MAX_WIDTH = 640.f;

        // Max height of the input text.
        const float DEBUG_INPUT_TEXT_MAX_HEIGHT = 240.0f;

        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>    m_whiteBrush;
        Microsoft::WRL::ComPtr<ID2D1DrawingStateBlock>  m_stateBlock;
        Microsoft::WRL::ComPtr<IDWriteTextFormat>       m_textFormat;
    };
}
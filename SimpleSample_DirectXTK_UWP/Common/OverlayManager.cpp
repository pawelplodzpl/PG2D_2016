//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "OverlayManager.h"

#include "Common/DirectXHelper.h"

using namespace SimpleSample;

// Initializes D2D resources.
OverlayManager::OverlayManager(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
m_deviceResources(deviceResources)
{
    CreateDeviceDependentResources();
}

// Updates each Overlay class in order of display from bottom to top.
void OverlayManager::Update(DX::StepTimer const& timer)
{
    for (unsigned int i = 0; i < m_overlays.size(); i++)
    {
        m_overlays[i]->Update(timer);
    }
}

// Renders a frame to the screen for each Overlay class in order of display from bottom to top.
void OverlayManager::Render()
{
    for (unsigned int i = 0; i < m_overlays.size(); i++)
    {
        m_overlays[i]->Render();
    }
}

// Creates device resources for each Overlay class in order of display from bottom to top.
void OverlayManager::CreateDeviceDependentResources()
{
    for (unsigned int i = 0; i < m_overlays.size(); i++)
    {
        m_overlays[i]->CreateDeviceDependentResources();
    }
}

// Releases device resources for each Overlay class in order of display from bottom to top.
void OverlayManager::ReleaseDeviceDependentResources()
{
    for (unsigned int i = 0; i < m_overlays.size(); i++)
    {
        m_overlays[i]->ReleaseDeviceDependentResources();
    }
}

// Sets the set of Overlay classes to be displayed.  Overlays are displayed in order.
HRESULT OverlayManager::SetOverlays(std::vector<std::shared_ptr<Overlay>> overlays)
{
    m_overlays = overlays;

    return S_OK;
}
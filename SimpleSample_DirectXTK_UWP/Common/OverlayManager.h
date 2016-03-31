//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include <string>
#include <vector>
#include "Common\DeviceResources.h"
#include "Common\StepTimer.h"

namespace SimpleSample_DirectXTK_UWP
{
    // Renders an overlay to the screen.
    // Abstract class.
    class Overlay
    {
    public:
        Overlay(const std::shared_ptr<DX::DeviceResources>& deviceResources) { m_deviceResources = deviceResources; };
        virtual void CreateDeviceDependentResources() PURE;
        virtual void ReleaseDeviceDependentResources() PURE;
        virtual void Update(DX::StepTimer const& timer) PURE;
        virtual void Render() PURE;

    protected:
        // Cached pointer to device resources.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;
    };

    // Renders renders a set of Overlay classes to the screen each frame.
    class OverlayManager
    {
    public:
        OverlayManager(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        HRESULT SetOverlays(std::vector<std::shared_ptr<Overlay>> overlays);
        void CreateDeviceDependentResources();
        void ReleaseDeviceDependentResources();
        void Update(DX::StepTimer const& timer);
        void Render();

    private:
        // Cached pointer to device resources.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;

        std::vector <std::shared_ptr<Overlay>> m_overlays;
    };
}
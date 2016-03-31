//--------------------------------------------------------------------------------------
// File: DirectXTK3DSceneRenderer.h
//
// This is a simple Windows Store app for Windows 8.1 showing use of DirectXTK
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <future>

#include "..\Common\DeviceResources.h"
#include "..\Common\StepTimer.h"

#include "Audio.h"
//#include "CommonStates.h"
//#include "Effects.h"
//#include "GeometricPrimitive.h"
//#include "Model.h"
//#include "PrimitiveBatch.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
//#include "VertexTypes.h"
#include "AnimatedTexture.h"
#include "ScrollingBackground.h"
#include "Player.h"
#include "Wall.h"
#include "Enemy.h"
#include "GamePad.h"

#include "DirectXTK\Inc\SimpleMath.h"

namespace SimpleSample
{
    // This class renders a scene using DirectXTK
	class DirectXTK3DSceneRenderer
	{
	public:
		DirectXTK3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
        void CreateAudioResources();
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();

        // Signals a new audio device is available
        void NewAudioDevice();

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

        // Not usefull right now
		//void XM_CALLCONV DrawGrid(DirectX::FXMVECTOR xAxis, DirectX::FXMVECTOR yAxis, DirectX::FXMVECTOR origin, size_t xdivs, size_t ydivs, DirectX::GXMVECTOR color);

        std::unique_ptr<DirectX::SpriteBatch>                                   m_sprites;
		std::unique_ptr<DirectX::SpriteBatch>                                   spriteBatchT1;
		std::unique_ptr<DirectX::SpriteBatch>                                   spriteBatchT2;

        std::unique_ptr<DirectX::SpriteFont>                                    m_font;

		//Sound
		std::unique_ptr<DirectX::AudioEngine>                                   m_audEngine;
        std::unique_ptr<DirectX::WaveBank>                                      m_waveBank;
        std::unique_ptr<DirectX::SoundEffect>                                   m_soundEffect;
        std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect1;
        std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect2;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>						enemyTexture;
		std::unique_ptr<AnimatedTexture>										animation;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        pipeTexture;
		
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>						backgroundTexture;
		std::unique_ptr<ScrollingBackground>									background;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>						cloudsTexture;
		std::unique_ptr<ScrollingBackground>									clouds;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>						cloudsTexture2;
		std::unique_ptr<ScrollingBackground>									clouds2;
		std::unique_ptr<Player>													player;
		
		std::unique_ptr<GamePad>												gamePad;
		std::vector<Wall>														wallsVector;
		std::vector<Enemy>														enemiesVector;

		std::wstring															collisionString;

		// Variables used with the rendering loop.
        uint32_t                                                                m_audioEvent;
        float                                                                   m_audioTimerAcc;

        bool                                                                    m_retryDefault;
	};
}


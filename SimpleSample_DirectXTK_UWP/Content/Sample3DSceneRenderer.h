#pragma once

#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "AnimatedTexture.h"
#include "ScrollingBackground.hpp"
#include "Player.hpp"
#include "Wall.hpp"
#include "Enemy.hpp"

#include "SimpleMath.h"
#include "Audio.h"
#include "GamePad.h"


#include "..\Common\DeviceResources.h"
//#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"

namespace SimpleSample_DirectXTK_UWP
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void CreateAudioResources();

		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();
		
		//void StartTracking();
		//void TrackingUpdate(float positionX);
		//void StopTracking();
		//bool IsTracking() { return m_tracking; }

		// Signals a new audio device is available
		void NewAudioDevice();

	private:
		//void Rotate(float radians);

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		//// Direct3D resources for cube geometry.
		//Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
		//Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		//Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
		//Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
		//Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
		//Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBuffer;

		//// System resources for cube geometry.
		//ModelViewProjectionConstantBuffer	m_constantBufferData;
		//uint32	m_indexCount;

		//// Variables used with the rendering loop.
		bool	m_loadingComplete;
		//float	m_degreesPerSecond;
		//bool	m_tracking;

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


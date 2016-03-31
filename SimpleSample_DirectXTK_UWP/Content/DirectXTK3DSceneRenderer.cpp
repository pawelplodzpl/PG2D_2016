//--------------------------------------------------------------------------------------
// File: DirectXTK3DSceneRenderer.cpp
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

#include "pch.h"
#include "DirectXTK3DSceneRenderer.h"

#include "DDSTextureLoader.h"

#include "..\Common\DirectXHelper.h"	// For ThrowIfFailed and ReadDataAsync

using namespace SimpleSample;

using namespace DirectX;
using namespace Windows::Foundation;

DirectXTK3DSceneRenderer::DirectXTK3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
    m_deviceResources(deviceResources)
{
	CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
    CreateAudioResources();
}

// Initializes view parameters when the window size changes.
void DirectXTK3DSceneRenderer::CreateWindowSizeDependentResources()
{
    Size outputSize = m_deviceResources->GetOutputSize(); 
    float aspectRatio = outputSize.Width / outputSize.Height; // If aspectRatio < 1.0f app is usually in snapped view (mode). Act accordingly



	// MUST BE DONE FOR EVERY SPRITEBATCH
	m_sprites->SetRotation( m_deviceResources->ComputeDisplayRotation() ); // necessary for the sprites to be in correct rotation when the
																			//screen rotation changes (portrait/landscape)

	spriteBatchT1->SetRotation(m_deviceResources->ComputeDisplayRotation());
	spriteBatchT2->SetRotation(m_deviceResources->ComputeDisplayRotation());

}

void DirectXTK3DSceneRenderer::CreateAudioResources()
{
    // Create DirectXTK for Audio objects
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

    m_audEngine.reset(new AudioEngine(eflags));

    m_audioEvent = 0;
    m_audioTimerAcc = 10.f;
    m_retryDefault = false;

    m_waveBank.reset(new WaveBank(m_audEngine.get(), L"assets\\adpcmdroid.xwb"));
    m_soundEffect.reset(new SoundEffect(m_audEngine.get(), L"assets\\MusicMono_adpcm.wav"));
    m_effect1 = m_soundEffect->CreateInstance();
    m_effect2 = m_waveBank->CreateInstance(10);

    //m_effect1->Play(true);
    //m_effect2->Play();
}

//This is the UPDATE Function
void DirectXTK3DSceneRenderer::Update(DX::StepTimer const& timer)
{

    m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
    if (m_audioTimerAcc < 0)
    {
        if (m_retryDefault)
        {
            m_retryDefault = false;
            if (m_audEngine->Reset())
            {
                // Restart looping audio
                m_effect1->Play(true);
            }
        }
        else
        {
            m_audioTimerAcc = 4.f;

            m_waveBank->Play(m_audioEvent++);

            if (m_audioEvent >= 11)
                m_audioEvent = 0;
        }
    }

    if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }

#pragma region Gamepad
	//GamePad
	auto statePlayerOne = gamePad->GetState(0);
	if (statePlayerOne.IsConnected())
	{
		XMFLOAT2 tempPos = player->getPosition();
		if (statePlayerOne.IsDPadUpPressed()){
			tempPos.y -= 10; //CHANGE TO PROPER OFFSET CALCULATION - USING TIME 
		}

		if (statePlayerOne.IsDPadDownPressed()){
			tempPos.y += 10; ////CHANGE TO PROPER OFFSET CALCULATION - USING TIME 
		}
		
		if (statePlayerOne.IsDPadLeftPressed()){
			tempPos.x -= 10; //CHANGE TO PROPER OFFSET CALCULATION - USING TIME 
		}
		if (statePlayerOne.IsDPadRightPressed()){
			tempPos.x += 10; //CHANGE TO PROPER OFFSET CALCULATION - USING TIME 	
		}
		player->setPosition(tempPos);
	}
#pragma endregion Handling the Gamepad Input

#pragma region Paralaxing background
	//Update Background
	background->Update((float)timer.GetElapsedSeconds() * 100);
	clouds->Update((float)timer.GetElapsedSeconds() * 300);
	clouds2->Update((float)timer.GetElapsedSeconds() * 900);
#pragma endregion Handling the paralaxing backgrounds
	
	//auto test = timer.GetElapsedSeconds();


	//update the animation
	//animation->Update((float)timer.GetElapsedSeconds());
	player->Update((float)timer.GetElapsedSeconds());

#pragma region Enemy AI
	// TODO: handle enemy AI using promises and Lambdas
	std::vector<std::future<DirectX::XMFLOAT2>> futures;
	
	for (auto enemy : enemiesVector)
	{
		futures.push_back( std::async(std::launch::async,
			[]()
		{
			DirectX::XMFLOAT2 tempPos;
			//TODO: Write code for very complicated AI here
			
			
			return tempPos;

		}) );
	}

	for (auto &future : futures)
	{
		//TODO:get results
		
		auto enemiesIterator = enemiesVector.begin();
		
		DirectX::XMFLOAT2 tempPos;
		tempPos = future.get();
		//(*enemiesIterator).setPosition(tempPos);
		//enemiesIterator++;
	}

#pragma endregion Handling Enemy AI using std::async and std::Future. Also using C++11 Lambdas

#pragma region Collisions
	collisionString = L"There is no collision";
	gamePad->SetVibration(0, 0.f, 0.f);
	for (auto wallsIterator = wallsVector.begin(); wallsIterator < wallsVector.end(); wallsIterator++)
	{

		(*wallsIterator).Update((float)timer.GetElapsedSeconds());
		if ((*wallsIterator).isCollidingWith(player->rectangle)){
			collisionString = L"There is a collision with the wall";

			gamePad->SetVibration(0, 0.75f, 0.75f);
		}
	}
#pragma endregion Handling collision detection + simple GamePad rumble on crash

}

void DirectXTK3DSceneRenderer::NewAudioDevice()
{
    if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
}
// Drawing Grid on screen using primitives (PrimitiveBatch) - not usefull right now
//void XM_CALLCONV DirectXTK3DSceneRenderer::DrawGrid(FXMVECTOR xAxis, FXMVECTOR yAxis, FXMVECTOR origin, size_t xdivs, size_t ydivs, GXMVECTOR color)
//{
//    auto context = m_deviceResources->GetD3DDeviceContext();
//    m_batchEffect->Apply(context);
//
//    context->IASetInputLayout(m_batchInputLayout.Get());
//
//    m_batch->Begin();
//
//    xdivs = std::max<size_t>(1, xdivs);
//    ydivs = std::max<size_t>(1, ydivs);
//
//    for (size_t i = 0; i <= xdivs; ++i)
//    {
//        float fPercent = float(i) / float(xdivs);
//        fPercent = (fPercent * 2.0f) - 1.0f;
//        XMVECTOR vScale = XMVectorScale(xAxis, fPercent);
//        vScale = XMVectorAdd(vScale, origin);
//
//        VertexPositionColor v1(XMVectorSubtract(vScale, yAxis), color);
//        VertexPositionColor v2(XMVectorAdd(vScale, yAxis), color);
//        m_batch->DrawLine(v1, v2);
//    }
//
//    for (size_t i = 0; i <= ydivs; i++)
//    {
//        FLOAT fPercent = float(i) / float(ydivs);
//        fPercent = (fPercent * 2.0f) - 1.0f;
//        XMVECTOR vScale = XMVectorScale(yAxis, fPercent);
//        vScale = XMVectorAdd(vScale, origin);
//
//        VertexPositionColor v1(XMVectorSubtract(vScale, xAxis), color);
//        VertexPositionColor v2(XMVectorAdd(vScale, xAxis), color);
//        m_batch->DrawLine(v1, v2);
//    }
//
//   m_batch->End();
//}

//this is the DRAW function
void DirectXTK3DSceneRenderer::Render()
{
	auto context = m_deviceResources->GetD3DDeviceContext();

	// Set render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	D3D11_TEXTURE2D_DESC pDesc;
	Microsoft::WRL::ComPtr<ID3D11Resource> res;
	//1.) -----------------
	//m_texture->GetResource(&res);
	//((ID3D11Texture2D*)res.Get())->GetDesc(&pDesc); // Usually dangerous!

	//2.) -----------------
	m_texture->GetResource(res.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11Texture2D> text2D;
	res.As(&text2D);
	text2D->GetDesc(&pDesc);

	auto height = pDesc.Height; //texture height
	auto width = pDesc.Width; //texture width

	auto windowSize = m_deviceResources->GetOutputSize(); // physical screen resolution
	auto logicalSize = m_deviceResources->GetLogicalSize(); //DPI dependent resolution

	// Draw sprites
	m_sprites->Begin();

	background->Draw(m_sprites.get());
	clouds->Draw(m_sprites.get());

	//Drawing walls

	for (auto wall : wallsVector)
	{
		wall.Draw(m_sprites.get());
	}

	//wall->Draw(m_sprites.get());
	//wall2->Draw(m_sprites.get());
	player->Draw(m_sprites.get());

	clouds2->Draw(m_sprites.get());

	m_font->DrawString(m_sprites.get(), collisionString.c_str(), XMFLOAT2(100, 10), Colors::Yellow);
	m_sprites->End();


}

//this is the LOAD function
void DirectXTK3DSceneRenderer::CreateDeviceDependentResources()
{
	// Create DirectXTK objects
	auto device = m_deviceResources->GetD3DDevice();

	auto context = m_deviceResources->GetD3DDeviceContext();


	auto windowSize = m_deviceResources->GetOutputSize(); // physical screen resolution
	auto logicalSize = m_deviceResources->GetLogicalSize(); //DPI dependent resolution


	m_sprites.reset(new SpriteBatch(context));
	spriteBatchT1.reset(new SpriteBatch(context));
	spriteBatchT2.reset(new SpriteBatch(context));

	m_font.reset(new SpriteFont(device, L"assets\\italic.spritefont"));

	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"assets\\shipanimated.dds", nullptr, m_texture.ReleaseAndGetAddressOf())
		);
	player.reset(new Player(m_texture.Get()));

	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"assets\\background.dds", nullptr, backgroundTexture.ReleaseAndGetAddressOf())
		);
	background.reset(new ScrollingBackground);
	background->Load(backgroundTexture.Get());


	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"assets\\clouds.dds", nullptr, cloudsTexture.ReleaseAndGetAddressOf())
		);
	clouds.reset(new ScrollingBackground);
	clouds->Load(cloudsTexture.Get());

	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"assets\\clouds2.dds", nullptr, cloudsTexture2.ReleaseAndGetAddressOf())
		);
	clouds2.reset(new ScrollingBackground);
	clouds2->Load(cloudsTexture2.Get());

	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"assets\\enemyanimated.dds", nullptr, enemyTexture.ReleaseAndGetAddressOf())
		);
	//TODO: Instatiate enemies here
	Enemy enemyTemp(enemyTexture.Get());
	enemiesVector.push_back(enemyTemp);

	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"assets\\pipe.dds", nullptr, pipeTexture.ReleaseAndGetAddressOf())
		);
	
	//Adding walls to vector
	//wallsVector.push_back(Wall(logicalSize, XMFLOAT2(300, 0), pipeTexture.Get()));
	wallsVector.emplace_back(Wall(logicalSize, XMFLOAT2(logicalSize.Width, 0), pipeTexture.Get()));
	

	//set windows size for drawing the background
	background->SetWindow(logicalSize.Width, logicalSize.Height);
	clouds->SetWindow(logicalSize.Width, logicalSize.Height);
	clouds2->SetWindow(logicalSize.Width, logicalSize.Height);


	//Gamepad
	gamePad.reset(new GamePad);

}

void DirectXTK3DSceneRenderer::ReleaseDeviceDependentResources()
{
	//TODO:
    m_sprites.reset();
    m_font.reset();
    m_texture.Reset();
	backgroundTexture.Reset();
	cloudsTexture.Reset();
	cloudsTexture2.Reset();
	pipeTexture.Reset();
	enemyTexture.Reset();
}
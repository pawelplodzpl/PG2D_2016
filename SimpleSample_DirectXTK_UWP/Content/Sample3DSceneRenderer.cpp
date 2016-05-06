#include "pch.h"
#include "Sample3DSceneRenderer.h"



#include "..\Common\DirectXHelper.h"

using namespace SimpleSample_DirectXTK_UWP;

using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	//m_degreesPerSecond(45),
	//m_indexCount(0),
	//m_tracking(false),
	m_deviceResources(deviceResources)
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	//float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		//fovAngleY *= 2.0f;
	}
	//XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	// MUST BE DONE FOR EVERY SPRITEBATCH
	m_sprites->SetRotation(m_deviceResources->ComputeDisplayRotation()); // necessary for the sprites to be in correct rotation when the

	spriteBatchT1->SetRotation(m_deviceResources->ComputeDisplayRotation());
	spriteBatchT2->SetRotation(m_deviceResources->ComputeDisplayRotation());


	// Note that the OrientationTransform3D matrix is post-multiplied here
	//// in order to correctly orient the scene to match the display orientation.
	//// This post-multiplication step is required for any draw calls that are
	//// made to the swap chain render target. For draw calls to other targets,
	//// this transform should not be applied.

	//// This sample makes use of a right-handed coordinate system using row-major matrices.
	//XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
	//	fovAngleY,
	//	aspectRatio,
	//	0.01f,
	//	100.0f
	//	);

	

	//XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	//XMStoreFloat4x4(
	//	&m_constantBufferData.projection,
	//	XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
	//	);

	//// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	//static const XMVECTORF32 eye = { 0.0f, 0.7f, 1.5f, 0.0f };
	//static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	//static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	//XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
}


void Sample3DSceneRenderer::CreateAudioResources()
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

	m_waveBank.reset(new WaveBank(m_audEngine.get(), L"Assets\\ADPCMdroid.xwb"));
	m_soundEffect.reset(new SoundEffect(m_audEngine.get(), L"Assets\\musicmono_adpcm.wav"));
	m_effect1 = m_soundEffect->CreateInstance();
	m_effect2 = m_waveBank->CreateInstance(10);

	//m_effect1->Play(true);
	//m_effect2->Play();
}

// Called once per frame
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	//if (!m_tracking)
	//{
	//	// Convert degrees to radians, then convert seconds to rotation angle
	//	float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
	//	double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
	//	float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

	//	Rotate(radians);
	//}

	//m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
	//if (m_audioTimerAcc < 0)
	//{
	//	if (m_retryDefault)
	//	{
	//		m_retryDefault = false;
	//		if (m_audEngine->Reset())
	//		{
	//			// Restart looping audio
	//			m_effect1->Play(true);
	//		}
	//	}
	//	else
	//	{
	//		m_audioTimerAcc = 4.f;

	//		m_waveBank->Play(m_audioEvent++);

	//		if (m_audioEvent >= 11)
	//			m_audioEvent = 0;
	//	}
	//}

	//if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
	//{
	//	// Setup a retry in 1 second
	//	m_audioTimerAcc = 1.f;
	//	m_retryDefault = true;
	//}
	auto windowSize = m_deviceResources->GetOutputSize(); // physical screen resolution
	auto logicalSize = m_deviceResources->GetLogicalSize(); //DPI dependent resolution



#pragma region Handling Adding Enemies

	//Enemy enemyTemp(enemyTexture.Get());
	//enemiesVector.push_back(enemyTemp);

	if (enemiesVector.size() < 5)
	{
		std::random_device rd;
		std::default_random_engine random(rd());

		std::uniform_int_distribution<int> dist(0, (int)windowSize.Height); //Choose distribution of the result (inclusive,inclusive)
		std::uniform_int_distribution<int> dist2(5, 25); //Choose distribution of the result (inclusive,inclusive)

		Enemy enemyTemp(enemyTexture.Get());
		XMFLOAT2 tempPos{ 0,0 };
		tempPos.x = windowSize.Width;
		tempPos.y = dist(random);
		enemyTemp.setFlightSpeed(dist2(random));
		enemyTemp.setPosition(tempPos);
		enemiesVector.push_back(enemyTemp);
	}

#pragma endregion


#pragma region Gamepad
	//GamePad
	auto statePlayerOne = gamePad->GetState(0);
	if (statePlayerOne.IsConnected())
	{
		XMFLOAT2 tempPos = player->getPosition();
		if (statePlayerOne.IsDPadUpPressed()) {
			tempPos.y -= 10; //CHANGE TO PROPER OFFSET CALCULATION - USING TIME 
		}

		if (statePlayerOne.IsDPadDownPressed()) {
			tempPos.y += 10; ////CHANGE TO PROPER OFFSET CALCULATION - USING TIME 
		}

		if (statePlayerOne.IsDPadLeftPressed()) {
			tempPos.x -= 10; //CHANGE TO PROPER OFFSET CALCULATION - USING TIME 
		}
		if (statePlayerOne.IsDPadRightPressed()) {
			tempPos.x += 10; //CHANGE TO PROPER OFFSET CALCULATION - USING TIME 	
		}
		player->setPosition(tempPos);
	}
#pragma endregion Handling the Gamepad Input


#pragma region Keyboard
	std::unique_ptr<Keyboard::KeyboardStateTracker> tracker(new Keyboard::KeyboardStateTracker);



	auto keyboardState = Keyboard::Get().GetState();

	tracker->Update(keyboardState);
	XMFLOAT2 tempPos = player->getPosition();
	if (tracker->pressed.S)
	{

		tempPos.y += 10;
	}

	if (tracker->pressed.W)
	{

		tempPos.y -= 10;
	}

	if (tracker->pressed.A)
	{
		tempPos.x -= 10;
	}
	if (tracker->pressed.D)
	{
		tempPos.x += 10;
	}


	player->setPosition(tempPos);


#pragma endregion Handling Keyboard input




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



#pragma	region Updating Enemies without AI
	for (auto & enemy : enemiesVector)
	{
		
		XMFLOAT2 tempPos = enemy.getPosition();
		tempPos.x -= enemy.getFlightSpeed(); //CHANGE TO PROPER POSITIONING USING TIME
		enemy.setPosition(tempPos);
		if (tempPos.x < 0)
		{
			enemy.setVisibility(false);
		}

	}

#pragma endregion



#pragma region Updating Enemies with AI
	// TODO: handle enemy AI using promises and Lambdas
	std::vector<std::future<DirectX::XMFLOAT2>> futures;

	for (auto& enemy : enemiesVector)
	{
		futures.push_back(std::async(std::launch::async,
			[&]()
		{
			
			Enemy & currentEnemy = enemy;
			DirectX::XMFLOAT2 enemyPos{ 0,0 };
			DirectX::XMFLOAT2 playerPos = player->getPosition();
			//TODO: Write code for very complicated AI here

			return enemyPos;

		}));
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
	
	// Collisions of Player with walls
	for (auto wallsIterator = wallsVector.begin(); wallsIterator < wallsVector.end(); wallsIterator++)
	{

		(*wallsIterator).Update((float)timer.GetElapsedSeconds());
		if ((*wallsIterator).isCollidingWith(player->rectangle)) {
			collisionString = L"There is a collision with the wall";

			gamePad->SetVibration(0, 0.75f, 0.75f);
		}
	}

	//Collisions of Enemies with Player

	for (auto &enemy : enemiesVector)
	{
		if (enemy.isCollidingWith(player->rectangle))
		{
			enemy.setVisibility (false);
		}

	}


	//Collisions with Enemies with Walls




#pragma endregion Handling collision detection + simple GamePad rumble on crash
	
#pragma region	Final update for enemies

	for (auto it = enemiesVector.begin(); it < enemiesVector.end();)
	{
		if (it->isVisible() == false)
		{
			it = enemiesVector.erase(it);
		}
		else
		{
			it->Update((float)timer.GetElapsedSeconds());
			it++;
		}

	}
#pragma endregion




}

void Sample3DSceneRenderer::NewAudioDevice()
{
	if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
	{
		// Setup a retry in 1 second
		m_audioTimerAcc = 1.f;
		m_retryDefault = true;
	}
}

void Sample3DSceneRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	//if (!m_loadingComplete)
	//{
	//	return;
	//}

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

	for (auto& wall : wallsVector)
	{
		wall.Draw(m_sprites.get());
	}

	//wall->Draw(m_sprites.get());
	//wall2->Draw(m_sprites.get());
	player->Draw(m_sprites.get());

	for (auto& enemy : enemiesVector)
	{
		enemy.Draw(m_sprites.get());

	}


	clouds2->Draw(m_sprites.get());

	m_font->DrawString(m_sprites.get(), collisionString.c_str(), XMFLOAT2(100, 10), Colors::Yellow);
	m_sprites->End();


}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
	// Create DirectXTK objects
	auto device = m_deviceResources->GetD3DDevice();

	auto context = m_deviceResources->GetD3DDeviceContext();


	auto windowSize = m_deviceResources->GetOutputSize(); // physical screen resolution
	auto logicalSize = m_deviceResources->GetLogicalSize(); //DPI dependent resolution


	m_sprites.reset(new SpriteBatch(context));
	spriteBatchT1.reset(new SpriteBatch(context));
	spriteBatchT2.reset(new SpriteBatch(context));

	m_font.reset(new SpriteFont(device, L"Assets\\italic.spritefont"));


	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"Assets\\shipanimated.dds", nullptr, m_texture.ReleaseAndGetAddressOf())
		);
	player.reset(new Player(m_texture.Get()));

	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"Assets\\background.dds", nullptr, backgroundTexture.ReleaseAndGetAddressOf())
		);
	background.reset(new ScrollingBackground);
	background->Load(backgroundTexture.Get());


	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"Assets\\clouds.dds", nullptr, cloudsTexture.ReleaseAndGetAddressOf())
		);
	clouds.reset(new ScrollingBackground);
	clouds->Load(cloudsTexture.Get());

	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"Assets\\clouds2.dds", nullptr, cloudsTexture2.ReleaseAndGetAddressOf())
		);
	clouds2.reset(new ScrollingBackground);
	clouds2->Load(cloudsTexture2.Get());

	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"Assets\\enemyanimated.dds", nullptr, enemyTexture.ReleaseAndGetAddressOf())
		);
	
	DX::ThrowIfFailed(
		CreateWICTextureFromFile(device, L"Assets\\ships-0.png", nullptr, ships1Texture.ReleaseAndGetAddressOf())
		);
	DX::ThrowIfFailed(
		CreateWICTextureFromFile(device, L"Assets\\ships-1.png", nullptr, ships2Texture.ReleaseAndGetAddressOf())
		);
	DX::ThrowIfFailed(
		CreateWICTextureFromFile(device, L"Assets\\nebulas.png", nullptr, nebulasTexture.ReleaseAndGetAddressOf())
		);
	


	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"Assets\\pipe.dds", nullptr, pipeTexture.ReleaseAndGetAddressOf())
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
	
	// Load shaders asynchronously.
	//auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	//auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	//auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
	//	DX::ThrowIfFailed(
	//		m_deviceResources->GetD3DDevice()->CreateVertexShader(
	//			&fileData[0],
	//			fileData.size(),
	//			nullptr,
	//			&m_vertexShader
	//			)
	//		);

	//	static const D3D11_INPUT_ELEMENT_DESC vertexDesc [] =
	//	{
	//		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	//		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	//	};

	//	DX::ThrowIfFailed(
	//		m_deviceResources->GetD3DDevice()->CreateInputLayout(
	//			vertexDesc,
	//			ARRAYSIZE(vertexDesc),
	//			&fileData[0],
	//			fileData.size(),
	//			&m_inputLayout
	//			)
	//		);
	//});

}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	//m_vertexShader.Reset();
	//m_inputLayout.Reset();
	//m_pixelShader.Reset();
	//m_constantBuffer.Reset();
	//m_vertexBuffer.Reset();
	//m_indexBuffer.Reset();


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
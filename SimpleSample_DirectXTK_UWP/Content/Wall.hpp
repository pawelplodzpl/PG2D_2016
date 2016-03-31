//For educational use only
//NOT TO BE USED IN COMMERCIAL OR SCHOOL PROJECTS

#pragma once

#include <wrl.h>
#include <SpriteBatch.h>

#include <DirectXMath.h>
#include <DirectXTK\Inc\SimpleMath.h>

#include <random>

using namespace DirectX;

class Wall
{

public:

	Wall(Windows::Foundation::Size screenResolution, XMFLOAT2 position, ID3D11ShaderResourceView* pipeTexture)
		: screenSize(screenResolution),
		m_origin(0, 0),
		gapMinHeight(256),
		moveSpeed(10)
	{

		m_mainTexture = pipeTexture;

		Microsoft::WRL::ComPtr<ID3D11Resource> res;

		pipeTexture->GetResource(res.GetAddressOf());
		Microsoft::WRL::ComPtr<ID3D11Texture2D> text2D;
		
		res.As(&text2D);
		text2D->GetDesc(&mainTextureDescription);

		//set up the main rectangle
		wallRect.X = position.x;
		wallRect.Y = position.y;
		wallRect.Width = mainTextureDescription.Width;
		wallRect.Height = screenSize.Height;

		//Initialize randomness
		randomizeGap();

	}


	void Update(float elapsedTime)
	{
		wallRect.X -= moveSpeed;
		upper.X = lower.X = gap.X = wallRect.X;
		if (wallRect.X + wallRect.Width < 0)
		{
			randomizeGap();
			wallRect.X = screenSize.Width;
		}

	}

	void Draw(DirectX::SpriteBatch *batch)
	{

		XMVECTOR origin = XMLoadFloat2(&m_origin);

		//Draw upper part of the wall
		batch->Draw(m_mainTexture.Get(), XMLoadFloat2(&XMFLOAT2(upper.X, upper.Y)), nullptr,
			Colors::White, 0.f, origin, XMLoadFloat2(&upperScalingFactor), SpriteEffects_None, 0.f);

		//Draw lower part of the wall
		batch->Draw(m_mainTexture.Get(), XMLoadFloat2(&XMFLOAT2(lower.X, lower.Y)), nullptr,
		Colors::White, 0.f, origin, XMLoadFloat2(&lowerScalingFactor), SpriteEffects_None, 0.f);

	}

	bool isCollidingWith(Windows::Foundation::Rect rect)
	{
		if (upper.IntersectsWith(rect))
		{
			return true;
		}

		else if (lower.IntersectsWith(rect))
		{
			return true;
		}

		else
		{
			return false;
		}
	}

private:
	//functions
	void randomizeGap()
	{
		//set up the rectangle for gap
		//RANDOMIZE, DO NOT USE rand() !!!!!!!!!
		
		
		std::random_device rd;
		std::mt19937 mt;
		mt.seed(rd());

		std::uniform_int_distribution<int> dist(0, (int)screenSize.Height - gapMinHeight); //Choose distribution of the result (inclusive,inclusive)

		gap.X = wallRect.X;
		gap.Y = dist(mt);
		gap.Height = gapMinHeight;
		gap.Width = mainTextureDescription.Width;

		//set up rectangle for textures
		upper.X = wallRect.X;
		upper.Y = 0;
		upper.Width = mainTextureDescription.Width;
		upper.Height = gap.Y;
		upperScalingFactor.x = 1;
		upperScalingFactor.y = upper.Height / mainTextureDescription.Height;

		lower.X = wallRect.X;
		lower.Y = gap.Y + gap.Height;
		lower.Width = mainTextureDescription.Width;
		lower.Height = screenSize.Height - (upper.Height + gap.Height);
		lowerScalingFactor.x = 1;
		lowerScalingFactor.y = lower.Height / mainTextureDescription.Height;



	}


	//bounding box of the Wall
	Windows::Foundation::Rect							wallRect;
	Windows::Foundation::Rect							gap;
	Windows::Foundation::Rect							upper;
	XMFLOAT2											upperScalingFactor;

	Windows::Foundation::Rect							lower;
	XMFLOAT2											lowerScalingFactor;

	Windows::Foundation::Size							screenSize;

	int													gapMinHeight;


	float												moveSpeed;

	//texture of the wall
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_mainTexture;
	D3D11_TEXTURE2D_DESC								mainTextureDescription;

	XMFLOAT2											m_origin;

	//Randomizer
	//std::random_device rd; // create random device - will generate random number
	//std::mt19937 mt; // use random number to seed Mersenne Twister 19937 generator

};
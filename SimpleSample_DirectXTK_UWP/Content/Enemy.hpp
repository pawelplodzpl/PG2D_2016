//For educational use only
//NOT TO BE USED IN COMMERCIAL OR SCHOOL PROJECTS

#pragma once

#include <wrl.h>


//#include "..\Common\DirectXHelper.h"	// For ThrowIfaFailed and ReadDataAsync

#include <Content\AnimatedTexture.h>
#include <SpriteBatch.h>

#include <DirectXMath.h>
#include <SimpleMath.h>

#include "Wall.hpp"


class Enemy
{
public:
	Enemy(ID3D11ShaderResourceView* enemySpriteSheet) : framesOfAnimation{ 4 }, framesToBeShownPerSecond{ 4 }, visible{ true }, flightSpeed {0}
	{
		position = DirectX::XMFLOAT2(512, 512);

		//Instantiate animation here
		texture = enemySpriteSheet;
		float rotation = 0.f;
		float scale = 3.f;
		animation.reset(new AnimatedTexture(DirectX::XMFLOAT2(0.f, 0.f), rotation, scale, 0.5f));
		animation->Load(texture.Get(), framesOfAnimation, framesToBeShownPerSecond);

		width = textureWidth = animation->getFrameWidth();
		height = textureHeight = animation->getFrameHeight();

		rectangle.X = position.x;
		rectangle.Y = position.y;
		rectangle.Height = height;
		rectangle.Width = width;

	}

	void setPosition(DirectX::XMFLOAT2 newPosition)
	{
		//set the position
		position = newPosition;
		updateBoundingRect();
	}

	void setPosition(float posX, float posY)
	{
		position.x = posX;
		position.y = posY;
		updateBoundingRect();
	}

	DirectX::XMFLOAT2 getPosition()
	{
		return position;
	}

	bool isVisible()
	{
		return this->visible;
	}
	void setVisibility(bool visibility)
	{
		this->visible = visibility;
	}

	void setFlightSpeed(int speed)
	{
		this->flightSpeed = speed;
	}

	int getFlightSpeed()
	{
		return this->flightSpeed;
	}



	bool isCollidingWith(Windows::Foundation::Rect rect)
	{
		if (rectangle.IntersectsWith(rect))
		{
			return true;
		}

		else
		{
			return false;
		}
	}

	void Update(float elapsed)
	{
		animation->Update(elapsed);
	}

	void Draw(DirectX::SpriteBatch* batch)
	{
		animation->Draw(batch, position);
	}

public:
	Windows::Foundation::Rect							rectangle;

private:
	void updateBoundingRect()
	{
		//TODO: proper updating when rotating object
		rectangle.X = position.x;
		rectangle.Y = position.y;
		rectangle.Height = height;
		rectangle.Width = width;
	}

	DirectX::XMFLOAT2									position;

	int													width;
	int													height;
	int													textureWidth;
	int													textureHeight;
	int													framesOfAnimation;
	int													framesToBeShownPerSecond;
	bool												visible;
	int													flightSpeed;

	//Texture and animation
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	texture;
	std::shared_ptr<AnimatedTexture>					animation;

};
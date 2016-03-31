//--------------------------------------------------------------------------------------
// File: ScrollingBackground.hpp
//
// C++ version of the C# example on how to make a scrolling background with SpriteBatch
// http://msdn.microsoft.com/en-us/library/bb203868.aspx
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#include <exception>
#include <SpriteBatch.h>
#include <thread>
#include <wrl.h>

using namespace DirectX;

class ScrollingBackground
{
public:
	ScrollingBackground() :
		mScreenHeight(0),
		mScreenWidth(0),
		mTextureWidth(0),
        mTextureHeight(0),
        mScreenPos( 0, 0 ),
        mTextureSize( 0, 0 ),
        mOrigin( 0, 0 )
    {
    }

    void Load( ID3D11ShaderResourceView* texture )
    {
        mTexture = texture;

        if ( texture )
        {
            Microsoft::WRL::ComPtr<ID3D11Resource> resource;
            texture->GetResource( resource.GetAddressOf() );

            D3D11_RESOURCE_DIMENSION dim;
            resource->GetType( &dim );

            if ( dim != D3D11_RESOURCE_DIMENSION_TEXTURE2D )
                throw std::exception( "ScrollingBackground expects a Texture2D" );

            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2D;
            resource.As( &tex2D );

            D3D11_TEXTURE2D_DESC desc;
            tex2D->GetDesc( &desc );

            mTextureWidth = desc.Width;
            mTextureHeight = desc.Height;

            mTextureSize.x = float( desc.Width );
            //mTextureSize.y = float( desc.Height );
			mTextureSize.y = 0.f; //Wrong - loss of usefull data

            //mOrigin.x = desc.Width / 2.f;
			mOrigin.x = 0.f;
			mOrigin.y = 0.f;
			//mOrigin.y = desc.Width / 2.f;
        }
    }

    void SetWindow( int screenWidth, int screenHeight )
    {
        mScreenHeight = screenHeight;
		mScreenWidth = screenWidth;

        //mScreenPos.x = float( screenWidth ) / 2.f;
		mScreenPos.x = 0.f;
        //mScreenPos.y = float( screenHeight ) / 2.f;
		mScreenPos.y = 0.f;

		scalingFactor.x = (float)mScreenWidth / (float)mTextureWidth;
		scalingFactor.y = (float)mScreenHeight / (float)mTextureHeight;
    }

    void Update( float deltaY )
    {
		mScreenPos.x -= deltaY;
		mScreenPos.x = fmodf(mScreenPos.x, float(mTextureWidth*scalingFactor.x));
    }

    void Draw( DirectX::SpriteBatch* batch )
    {
        
        XMVECTOR screenPos = XMLoadFloat2( &mScreenPos );
        XMVECTOR origin = XMLoadFloat2( &mOrigin );
		XMVECTOR scale = XMLoadFloat2(&scalingFactor);


            batch->Draw( mTexture.Get(), screenPos, nullptr,
                         Colors::White, 0.f, origin, scale, SpriteEffects_None, 0.f );


        XMVECTOR textureSize = XMLoadFloat2( &mTextureSize ); //TODO:edit the vector to zero one dimmension, but not lose data
		
        batch->Draw( mTexture.Get(), XMLoadFloat2(&XMFLOAT2(mScreenPos.x+((float)mTextureWidth*scalingFactor.x),0)), nullptr,
                     Colors::White, 0.f, origin, scale, SpriteEffects_None, 0.f );

    }

private:
    int                                                 mScreenHeight;
	int													mScreenWidth;
    int                                                 mTextureWidth;
    int                                                 mTextureHeight;
    DirectX::XMFLOAT2                                   mScreenPos;
    DirectX::XMFLOAT2                                   mTextureSize;
    DirectX::XMFLOAT2                                   mOrigin;
	DirectX::XMFLOAT2									scalingFactor;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    mTexture;
};

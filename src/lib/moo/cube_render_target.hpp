/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef CUBE_RENDER_TARGET_HPP
#define CUBE_RENDER_TARGET_HPP

#include <iostream>
#include "base_texture.hpp"

namespace Moo
{
	/**
	 *	This class implements functionality to handle a cubic 
	 *	environment render target.
	 */
	class CubeRenderTarget : public BaseTexture, public DeviceCallback
	{
	public:
		CubeRenderTarget( const std::string& identifier );
		~CubeRenderTarget();

		bool					create( uint32 cubeDimensions, 
										const Colour & clearColour = (DWORD)0x00000000 );
		HRESULT					release();

		bool					pushRenderSurface( D3DCUBEMAP_FACES face );
		void					setCubeViewMatrix( D3DCUBEMAP_FACES face, 
													const Vector3& centre );
		void					pop( void );

		void					setupProj();
		void					restoreProj();

		// Virtual methods from BaseTexture
		virtual DX::BaseTexture*pTexture( );
		virtual uint32			width( ) const;
		virtual uint32			height( ) const;
		virtual D3DFORMAT		format( ) const;
		virtual uint32			textureMemoryUsed( );

		//methods inherited from DeviceCallback
		virtual void			deleteUnmanagedObjects( );
		virtual void			createUnmanagedObjects( );

	private:
		std::string				identifier_;
		uint32					cubeDimensions_;
		D3DFORMAT				pixelFormat_;
		Matrix					originalProj_;
		Camera					originalCamera_;
		Colour					clearColour_;

		ComObjectWrap< DX::CubeTexture > pRenderTarget_;
		ComObjectWrap< DX::Surface > pDepthStencilTarget_;

		CubeRenderTarget(const CubeRenderTarget&);
		CubeRenderTarget& operator=(const CubeRenderTarget&);

		friend std::ostream& operator<<(std::ostream&, const CubeRenderTarget&);
	};

	typedef SmartPointer< class CubeRenderTarget > CubeRenderTargetPtr;

}

#ifdef CODE_INLINE
#include "cube_render_target.ipp"
#endif

#endif
/*cube_render_target.hpp*/

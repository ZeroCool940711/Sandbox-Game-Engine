/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

#include "moo_math.hpp"
#include "moo_dx.hpp"

#include "camera.hpp"
#include "com_object_wrap.hpp"
#include "light_container.hpp"
#include "cstdmf/vectornodest.hpp"
#include "cstdmf/aligned.hpp"
#include "cstdmf/concurrency.hpp"
#include "math/matrix.hpp"
#include <stack>

#ifndef D3DRS_MAX
// Must be larger than the maximum valid value of the enum D3DRENDERSTATETYPE
#define D3DRS_MAX       210
#endif D3DRS_MAX

#ifndef D3DTSS_MAX
// Must be larger than the maximum valid value of the enum D3DTEXTURESTAGESTATETYPE
#define D3DTSS_MAX      33
#endif D3DTSS_MAX

#ifndef D3DSAMP_MAX
// Must be larger than the maximum valid value of the enum D3DSAMPLERSTATETYPE
#define D3DSAMP_MAX		14
#endif D3DSAMP_MAX

namespace Moo
{
	class Material;
	class OcclusionQuery;
	class DynamicIndexBufferInterface;

	/**
	 * Flags for various code paths for different video cards.
	 */
	enum CompatibilityFlag
	{
		COMPATIBILITYFLAG_NOOVERWRITE = 1 << 0,
		COMPATIBILITYFLAG_NVIDIA =		1 << 1,
		COMPATIBILITYFLAG_ATI =			1 << 2
	};

	/**
	 * This structure holds interesting information about a graphics device. 
	 * The RenderContext holds one of these for each device on the
	 * system.
	 */
	struct DeviceInfo
	{
		D3DADAPTER_IDENTIFIER9			identifier_;
		D3DCAPS9						caps_;
		uint32							adapterID_;
		bool							windowed_;
		D3DDISPLAYMODE					windowedDisplayMode_;
		std::vector< D3DDISPLAYMODE >	displayModes_;
		uint32							compatibilityFlags_;
	};

extern THREADLOCAL( bool ) g_renderThread;

	/**
	 * Render state cache entry.
	 */
	struct RSCacheEntry
	{
		D3DRENDERSTATETYPE				state;
		uint32							currentValue;
	};

	/**
	 * Texture stage state cache entry.
	 */
	struct TSSCacheEntry
	{
		uint16							stage;
		uint16							type;
		uint32							currentValue;
	};

	/**
	 * Sample cache entry.
	 */
	struct SampCacheEntry
	{
		uint16							stage;
		uint16							type;
		uint32							currentValue;
	};

	/**
	 * Texture cache entry.
	 */
	struct TextureCacheEntry
	{
		uint16							stage;
		DX::BaseTexture*				pCurrentTexture;
	};

	/*
	 * Profiling data struct for drawcalls
	 */
	struct DrawcallProfilingData
	{
		uint32 nDrawcalls_;
		uint32 nPrimitives_;
	};

	/**
	 *	This class creates and manages the d3d device, backbuffer etc, it's
	 *	the current state of rendering in that it contains the current lights,
	 *	matrices, lod state, zoom factor and so forth. Anything that is of global
	 *	importance to rendering should be contained in the render context.
	 */
	class RenderContext : public Aligned
	{
	public:
		RenderContext();
		~RenderContext();

		bool init();
		void fini();
		bool isValid() const { return isValid_; }

		uint32						nDevices( ) const;
		const DeviceInfo&			deviceInfo( uint32 i ) const;

		bool						checkDevice(bool *reset = NULL);
		bool						checkDeviceYield(bool *reset = NULL);

		bool						createDevice( HWND hWnd,
													uint32 deviceIndex           = 0,
													uint32 modeIndex             = 0,
													bool windowed                = true,
													bool wantStencil             = false,
													const Vector2 & windowedSize = Vector2(0, 0),
													bool hideCursor              = true,
													bool forceRef				 = false );
		void						releaseDevice( void );

		bool						changeMode( 
										uint32 modeIndex, 
										bool   windowed, 
										bool   testCooperative = false,
										uint32 backBufferWidthOverride = 0 );

		bool						resetDevice();

		void						releaseUnmanaged();
		void						createUnmanaged();

		void						clearBindings();

		DX::Device*					device() const;

		void						restoreCursor( bool state );

		bool						windowed( ) const;
		uint32						deviceIndex( ) const;
		uint32						modeIndex( ) const;
		bool						stencilAvailable( ) const;
		DWORD						maxVertexIndex( ) const;

		uint						getAvailableTextureMem( ) const;

		bool						mixedVertexProcessing() const { return mixedVertexProcessing_; }
		bool						mrtSupported() const { return mrtSupported_; }

		D3DFORMAT					getMatchingZBufferFormat( D3DFORMAT colourFormat, bool stencilWanted, bool & stencilAvailable );

		bool						pushRenderTarget();
		bool						popRenderTarget();

		const Camera&				camera( ) const;
		Camera&						camera( );
		void						camera( const Camera& cam );

		const Matrix&				projection( ) const;
		Matrix&						projection( );
		void						projection( const Matrix& m );
		void						scaleOffsetProjection( const float scale, const float x, const float y );

		const Matrix&				view( ) const;
		Matrix&						view( );
		void						view( const Matrix& m );

		const Matrix&				world() const;
		Matrix&						world();
		void						world( const Matrix& m );

		const Matrix&				lastViewProjection( ) const;
		void						lastViewProjection( const Matrix& m  );

		const Matrix&				viewProjection( ) const;
		const Matrix&				invView( ) const;

		void						preMultiply( const Matrix& m );
		void						postMultiply( const Matrix& m );

		void						push( );
		void						pop( );
		void						reset( );
		void						reset( const Matrix& m );

		void						updateProjectionMatrix( );
		void						updateViewTransforms( );

		LightContainerPtr			lightContainer( ) const;
		void						lightContainer( LightContainerPtr pLC );

		LightContainerPtr			specularLightContainer( ) const;
		void						specularLightContainer( LightContainerPtr pLC );

		const D3DSURFACE_DESC&		backBufferDesc( ) const;
		const D3DPRESENT_PARAMETERS& presentParameters( ) const;

		uint32						frameTimestamp( ) const;
		bool						frameDrawn( uint32& frame ) const;
		void						nextFrame( );

		void						fogEnabled( bool enabled );
		bool						fogEnabled() const;
		void						fogColour( Colour colour );
		Colour						fogColour() const;
		void						fogNear( float fogNear );
		float						fogNear( ) const;
		void						fogFar( float fogFar );
		float						fogFar( ) const;
	
		float						screenWidth() const;
		float						screenHeight() const;
		float						halfScreenWidth() const;
		float						halfScreenHeight() const;
		void						screenWidth( int width );
		void						screenHeight( int height );
		void						backBufferWidthOverride( uint32 backBufferWidthOverride );
		uint32						backBufferWidthOverride() const;
		
		const Vector2 &				windowedModeSize() const;

		void						fullScreenAspectRatio( float ratio );
		float						fullScreenAspectRatio() const;

		void						objectAlphaOverride( float alpha );
		uint32						objectAlphaOverride( void ) const;

		void						depthOnly( bool depthOnly );
		bool						depthOnly() const;

		const std::string			screenShot( const std::string& fileExt = "bmp", const std::string& fileName = "shot", bool autoNumber = true );
		DX::Surface*				getScreenCopy();

		float						lodValue() const;
		void						lodValue( float value );

		float						lodFar() const;
		void						lodFar( float value );

		float						lodPower() const;
		void						lodPower( float value );

		float						zoomFactor() const;

		float						gammaCorrection() const;
		void						gammaCorrection( float gammaCorrection );

		bool						waitForVBL() const;
		void						waitForVBL( bool wait );
		bool						tripleBuffering() const { return tripleBuffering_; }
		void						tripleBuffering( bool enabled ) { tripleBuffering_ = enabled; }

		bool						supportsTextureFormat( D3DFORMAT fmt ) const;

		uint16						psVersion() const;
		uint16						vsVersion() const;
		uint16						maxSimTextures() const;
		uint16						maxAnisotropy() const;

		float						currentObjectID() const { return currentObjectID_; }
		void						currentObjectID(float id) { currentObjectID_=id; }

		bool						memoryCritical() const { return memoryCritical_; }

		/*
		 *	pause/resume methods to free/realloc resources. Useful in windowed
		 *	apps that need to be friendly with other DX apps.
		 */
		void						pause();
		void						resume();
		bool						paused() const;

		/*
		 * State caching
		 */

		void						setViewport( DX::Viewport* viewport );
		void						getViewport( DX::Viewport* viewport ) const;

		uint32						setRenderState(D3DRENDERSTATETYPE state, uint32 value);
		uint32						getRenderState(D3DRENDERSTATETYPE state ) const;
		uint32						setTextureStageState( uint32 stage, D3DTEXTURESTAGESTATETYPE type, uint32 value );
		uint32						setSamplerState( uint32 stage, D3DSAMPLERSTATETYPE type, uint32 value);
		uint32						setTexture(DWORD stage, DX::BaseTexture* pTexture);

		uint32						setVertexShader( DX::VertexShader* pVS );
		uint32						setPixelShader( DX::PixelShader* pPS );
		HRESULT						setIndices( DX::IndexBuffer* pIB );
		ComObjectWrap<DX::IndexBuffer>	getIndices() const;
		uint32						setVertexDeclaration( DX::VertexDeclaration* pVD );
		uint32						setFVF( uint32 fvf );

		uint32						setWriteMask( uint i, uint32 value );

		uint32						drawIndexedPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType,
															UINT minIndex,
															UINT numVertices,
															UINT primitiveCount,
															CONST void* pIndexData,
															D3DFORMAT indexDataFormat,
															CONST void* pVertexStreamZeroData,
															UINT vertexStreamZeroStride );
		uint32						drawIndexedPrimitive( D3DPRIMITIVETYPE type,
														  INT baseVertexIndex,
														  UINT minIndex,
														  UINT numVertices,
														  UINT startIndex,
														  UINT primitiveCount );
		uint32						drawPrimitive( D3DPRIMITIVETYPE primitiveType,
												   UINT startVertex,
												   UINT primitiveCount );
		uint32						drawPrimitiveUP( D3DPRIMITIVETYPE primitiveType,
													 UINT primitiveCount,
													 CONST void* pVertexStreamZeroData,
													 UINT vertexStreamZeroStride );

		void						getNearPlaneRect( Vector3 & corner,
										Vector3 & xAxis, Vector3 & yAxis ) const;

		HRESULT						beginScene();
		HRESULT						endScene();
		HRESULT						present();

		HRESULT						setRenderTarget(uint32 index, ComObjectWrap<DX::Surface> surface);
		ComObjectWrap<DX::Surface>	getRenderTarget(uint32 index);
		uint32						renderTargetCount() const;

		ComObjectWrap<DX::Texture>	createTexture( UINT Width, UINT Height, UINT Levels, DWORD Usage,
										D3DFORMAT Format, D3DPOOL Pool, const char* allocator = "texture/unknown" );
		ComObjectWrap<DX::Texture>	createTextureFromFileInMemoryEx(
										LPCVOID pSrcData, UINT SrcDataSize, UINT Width, UINT Height,
										UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
										DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, D3DXIMAGE_INFO* pSrcInfo,
										PALETTEENTRY* pPalette, const char* allocator = "texture/unknown" );
		ComObjectWrap<DX::Texture>	createTextureFromFileEx( LPCTSTR pSrcFile, UINT Width, UINT Height, UINT MipLevels,
										DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter,
										D3DCOLOR ColorKey, D3DXIMAGE_INFO * pSrcInfo, PALETTEENTRY * pPalette,
										const char* allocator = "texture/unknown" );
		ComObjectWrap<DX::CubeTexture>	createCubeTextureFromFileInMemoryEx(
											LPCVOID pSrcData, UINT SrcDataSize, UINT Size, UINT MipLevels, DWORD Usage,
											D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey,
											D3DXIMAGE_INFO* pSrcInfo, PALETTEENTRY* pPalette, const char* allocator = "texture/unknown" );

		bool						mirroredTransform() const { return mirroredTransform_; }
		void						mirroredTransform(bool mirrored ) { mirroredTransform_ = mirrored; }

		bool						reflectionScene() const { return reflectionScene_; }
		void						reflectionScene(bool reflections ) { reflectionScene_ = reflections; }

		OcclusionQuery*				createOcclusionQuery();
		void						destroyOcclusionQuery(OcclusionQuery*);

		void						beginQuery(OcclusionQuery*);
		void						endQuery(OcclusionQuery*);
		bool						getQueryResult(int& visiblePixels, OcclusionQuery*, bool wait);

		SimpleMutex&				getD3DXCreateMutex();
		HWND						windowHandle();

		DynamicIndexBufferInterface & dynamicIndexBufferInterface() const;

		void addPreloadResource(IDirect3DResource9* preloadObject);
		void preloadDeviceResources( uint32 timeLimitMs );

		const DrawcallProfilingData&	lastFrameProfilingData() const { return lastFrameProfilingData_; }
		const DrawcallProfilingData&	liveProfilingData() const { return liveProfilingData_; }

	private:

		/**
		 *	This class is a helper class for the RenderContext to push and pop the
		 *	current rendertarget.
		 */
		struct RenderTargetStack
		{
		public:
			bool push( class RenderContext* rc );
			bool pop( class RenderContext* rc );
			int nStackItems();

			void clear() { stackItems_.clear(); }
		private:
			class StackItem : public Aligned
			{
			public:
				StackItem() :
					cam_( 0.5f, 200.f, MATH_PI * 0.5f, 1.f ),
					zbufferSurface_( NULL )
				{
				}
				~StackItem()
				{
				}
				std::vector<ComObjectWrap< DX::Surface >> renderSurfaces_;
				ComObjectWrap< DX::Surface >	zbufferSurface_;
				DX::Viewport					viewport_;
				Matrix							view_;
				Matrix							projection_;
				Camera							cam_;
			};

			typedef std::avector< StackItem > StackItems;
			StackItems stackItems_;
		};

		D3DSURFACE_DESC				backBufferDesc_;

		D3DPRESENT_PARAMETERS		presentParameters_;
		RECT						windowedRect_;
		Vector2						windowedSize_;
		HWND						windowHandle_;
		bool						windowed_;
		bool						hideCursor_;
		LONG						windowedStyle_;
		bool						stencilWanted_;
		bool						stencilAvailable_;
		D3DDEVTYPE					deviceType_;
		uint32						deviceIndex_;
		uint32						modeIndex_;
		uint32						backBufferWidthOverride_;
		float						fullScreenAspectRatio_;
		std::vector< DeviceInfo >	devices_;

		bool						mrtSupported_;
		bool						mixedVertexProcessing_;

		bool						memoryCritical_;

		float						currentObjectID_;

		ComObjectWrap< DX::Interface >	d3d_;
		ComObjectWrap< DX::Device >		device_;

		ComObjectWrap< DX::Surface >	screenCopySurface_;

		ComObjectWrap< DX::Surface >	renderTarget_[2];
		RenderTargetStack			renderTargetStack_;
		uint32						renderTargetCount_;

		Camera						camera_;
		Matrix						projection_;
		Matrix						view_;
		Matrix						viewProjection_;
		Matrix						lastViewProjection_;
		Matrix						invView_;

		float						projScale_;
		float						projXOffset_;
		float						projYOffset_;

		AVectorNoDestructor< Matrix > world_;
	
		bool						fogEnabled_;
		Colour						fogColour_;
		float						fogNear_;
		float						fogFar_;
	
		LightContainerPtr			lightContainer_;
		LightContainerPtr			specularLightContainer_;

		uint32						primitiveGroupCount_;
		uint32						primitiveCount_;
		uint32						currentFrame_;

		float						halfScreenWidth_;
		float						halfScreenHeight_;

		bool						changeModePriv(
										uint32 modeIndex,
										bool   windowed,
										bool   testCooperative,
										uint32 backBufferWidthOverride);
										
		void						fillPresentationParameters( );
		void						updateDeviceInfo( );

		void						initRenderStates();

		uint32						alphaOverride_;
		bool						depthOnly_;
		float						lodValue_;
		float						lodPower_;
		float						lodFar_;
		float						zoomFactor_;

		bool						waitForVBL_;
		bool						tripleBuffering_;

		uint16						psVersion_;
		uint16						vsVersion_;
		uint16                      maxSimTextures_;
		uint16                      maxAnisotropy_;
		float						gammaCorrection_;
		void						setGammaCorrection();

		float						maxZ_;

		RSCacheEntry				rsCache_[D3DRS_MAX];
		TSSCacheEntry				tssCache_[64][D3DTSS_MAX];
		SampCacheEntry				sampCache_[64][D3DSAMP_MAX];
		TextureCacheEntry			textureCache_[64];

		DX::VertexDeclaration*		vertexDeclaration_;
		uint32						fvf_;

		int							beginSceneCount_;

		bool mirroredTransform_, reflectionScene_;

		bool						paused_;

		std::vector<OcclusionQuery*> queryList_;

		// Has this been initialised successfully?
		bool						isValid_;
		
		// Used to prevent certain D3DXCreate operations in loading thread
		// from causing Direct3D9 error:
		// "Already in the state record mode. BeginStateBlock failed.".
		SimpleMutex					d3dxCreateMutex_;

		DynamicIndexBufferInterface * pDynamicIndexBufferInterface_;

		RenderContext(const RenderContext&);
		RenderContext& operator=(const RenderContext&);

		void releaseQueryObjects(void);
		void createQueryObjects(void);

		typedef std::list<IDirect3DResource9*> PreloadResourceList;
		PreloadResourceList preloadResourceList_;
		SimpleMutex			preloadResourceMutex_;

		static const float VP_MAXZ;

		DrawcallProfilingData		liveProfilingData_;
		DrawcallProfilingData		lastFrameProfilingData_;
	};

	// Accessor function
	RenderContext& rc();
};

#ifdef CODE_INLINE
#include "render_context.ipp"
#endif




#endif // RENDER_CONTEXT_HPP

//**************************************************************************//
// Start of an OBJ loader.  By no means an end.  This just creates			//
// triangles.																//
//																			//
// Look for the Nigel style comments, like these, for the bits you need to  //
// look at.																	//
//**************************************************************************//

//**************************************************************************//
// Modifications to the MS sample code is copyright of Dr Nigel Barlow,		//
// lecturer in computing, University of Plymouth, UK.						//
// email: nigel@soc.plymouth.ac.uk.											//
//																			//
// You may use, modify and distribute this (rather cack-handed in places)	//
// code subject to the following conditions:								//
//																			//
//	1:	You may not use it, or sell it, or use it in any adapted form for	//
//		financial gain, without my written premission.						//
//																			//
//	2:	You must not remove the copyright messages.							//
//																			//
//	3:	You should correct at least 10% of the typig abd spekking errirs.   //
//**************************************************************************//

//--------------------------------------------------------------------------------------
// File: Tutorial07 - Textures and Constant buffers.cpp
//
// This application demonstrates texturing
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include <fstream>		// Files.  The ones without ".h" are the new (not
#include <string>		// so new now) standard library headers.
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <vector>
#include <stdio.h>
#include "resource.h"


//**************************************************************************//
// Nothing is easy in DirectX.  Before we can even create a single vertex,	//
// we need to define what it looks like.									//
//																			//
// The data types seems to be inhereted from XNA.							//
// An XMFLOAT3 is a float containing 3 numbers, an x, y, x position here.	//
// An XMFLOAT4 is a float containing 4 values, an RGBA colour.	Not that	//
// alpha effects work without additional effort.							//
//**************************************************************************//
struct SimpleVertex
{
    XMFLOAT3 Pos;	//Why not a float4?  See the shader strucrure.  Any thoughts?  Nigel
	XMFLOAT3 VecNormal;
    XMFLOAT2 TexUV;
};




//**************************************************************************//
// A sort of mesh subset, basically an array of vertices and indexes.		//
//**************************************************************************//
struct SortOfMeshSubset
{
	SimpleVertex *vertices;
	USHORT       *indexes;
	USHORT       numVertices;
	USHORT       numIndices;
};
int              g_numIndices = 0;	// Need to record the number of indices
									// for drawing mesh.

struct CBMeshMaterial
{
	XMFLOAT4 materialColour;
	XMVECTOR vecLight;
};

//**************************************************************************//
// Light vector never moves; and colour never changes.  I have done it .	//
// this way to show how constant buffers can be used so that you don't		//
// upsate stuff you don't need to.											//
// Beware of constant buffers that aren't in multiples of 16 bytes..		//
//**************************************************************************//
struct CBNeverChanges
{
    XMFLOAT4 materialColour;
	XMVECTOR vecLight;			// Must be 4, we only use the first 3.
};

struct CBChangeOnResize
{
    XMMATRIX matProjection;
};

//**************************************************************************//
// Note we do it properly here and pass the WVP matrix, rather than world,	//
// view and projection matrices separately.									//
//																			//
// We still need the world matrix to transform the normal vectors.			//
//**************************************************************************//
struct CBChangesEveryFrame
{
    XMMATRIX matWorld;
	XMMATRIX matWorldViewProjection;
};


//**************************************************************************//
// Global Variables.  There are many global variables here (we aren't OO	//
// yet.  I doubt  Roy Tucker (Comp Sci students will know him) will			//
// approve pf this either.  Sorry, Roy.										//
//**************************************************************************//
HINSTANCE                           g_hInst = NULL;
HWND                                g_hWnd = NULL;
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*                       g_pd3dDevice = NULL;
ID3D11DeviceContext*                g_pImmediateContext = NULL;
IDXGISwapChain*                     g_pSwapChain = NULL;
ID3D11RenderTargetView*             g_pRenderTargetView = NULL;
ID3D11Texture2D*                    g_pDepthStencil = NULL;
ID3D11DepthStencilView*             g_pDepthStencilView = NULL;
ID3D11VertexShader*                 g_pVertexShader = NULL;
ID3D11PixelShader*                  g_pPixelShader = NULL;
ID3D11InputLayout*                  g_pVertexLayout = NULL;
ID3D11Buffer*                       g_pVertexBuffer = NULL;
ID3D11Buffer*                       g_pIndexBuffer = NULL;
ID3D11SamplerState*                 g_pSamplerLinear = NULL;
XMMATRIX                            g_MatProjection;



//**************************************************************************//
// The texture; just one here.												//
//**************************************************************************//
ID3D11ShaderResourceView           *g_pTextureResourceView   = NULL;


//**************************************************************************//
// Now a global instance of each constant buffer.							//
//**************************************************************************//
ID3D11Buffer                       *g_pCBNeverChanges      = NULL;
ID3D11Buffer                       *g_pCBChangeOnResize    = NULL;
ID3D11Buffer                       *g_pCBChangesEveryFrame = NULL;






//**************************************************************************//
// Forward declarations.													//
//																			//
// If you are not used to "C" you will find that functions (or methods in	//
// "C++" must have templates defined in advance.  It is usual to define the //
// prototypes in a header file, but we'll put them here for now to keep		//
// things simple.															//
//**************************************************************************//
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();
void charStrToWideChar(WCHAR *dest, char *source);
void XMFLOAT3normalise(XMFLOAT3 *toNormalise);
SortOfMeshSubset *LoadMesh(LPSTR filename);
void ParseMtlFile(WCHAR *mtlPath);




//**************************************************************************//
// A Windows program always kicks off in WinMain.							//
// Initializes everything and goes into a message processing				//
// loop. Idle time is used to render the scene.								//
//																			//
// In other words, run the computer flat out.  Is this good?				//
//**************************************************************************//
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }



	//**************************************************************************//
    // Main Windoze message loop.												//
	//																			//
	// Gamers will see this as a game loop, though you will find something like //
	// this main loop deep within any Windows application.						//
	//**************************************************************************//
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, 640, 480 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"TutorialWindowClass", L"Direct3D 11 Tutorial 7", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}


//**************************************************************************//
// Compile the shader file.  These files aren't pre-compiled (well, not		//
// here, and are compiled on he fly).										//
//**************************************************************************//
HRESULT CompileShaderFromFile( WCHAR* szFileName,		// File Name
							  LPCSTR szEntryPoint,		// Namee of shader
							  LPCSTR szShaderModel,		// Shader model
							  ID3DBlob** ppBlobOut )	// Blob returned
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( szFileName, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
   if( FAILED(hr) )
    {
		WCHAR errorCharsW[200];
        if( pErrorBlob != NULL )
		{
			charStrToWideChar(errorCharsW, (char *)pErrorBlob->GetBufferPointer());
            MessageBox( 0, errorCharsW, L"Error", 0 );
		}
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}




//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &g_pDepthStencil );
    if( FAILED( hr ) )
        return hr;

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

    
 	

	
	
	//**********************************************************************//
	// Compile the shader file.  These files aren't pre-compiled (well, not //
	// here, and are compiles on he fly).									//
	//																		//
	// This is DirectX11, but what shader model do you see here?			//
	// Change to shader model 5 in Babbage209 and it should still work.		//
	//**********************************************************************//
    ID3DBlob* pVSBlob = NULL;
    hr = CompileShaderFromFile( L"Start of OBJ loader VS.hlsl", "VS_obj", "vs_4_0", &pVSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

	//**********************************************************************//
    // Create the vertex shader.											//
	//**********************************************************************//
	hr = g_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader );
    if( FAILED( hr ) )
    {    
        pVSBlob->Release();
        return hr;
    }

	//**********************************************************************//
    // Create the pixel shader.												//
    //**********************************************************************//
	ID3DBlob* pPSBlob = NULL;
    hr = CompileShaderFromFile( L"Start of OBJ loader PS.hlsl", "PS_TexturesNoLighting", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }
    hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader );
    pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;


	//**********************************************************************//
    // Define the input layout.  I won't go too much into this except that  //
	// the vertex defined here MUST be consistent with the vertex shader	//
	// input you use in your shader file and the constand buffer structure  //
	// at the top of this module.											//
	//																		//
	// Here a vertex has a position a normal vector (used for lighting) and //
	// a single texture UV coordinate.										//
	//**********************************************************************//
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
    UINT numElements = ARRAYSIZE( layout );

    // Create the input layout
    hr = g_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
    pVSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    // Set the input layout
    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );

	
	//**************************************************************************//
	// Load the obj mesh, NOT COMPLETE.											//
	//**************************************************************************//	
	//SortOfMeshSubset *sortOfMesh = LoadMesh("Media\\Cup\\Cup.obj");
	//SortOfMeshSubset *sortOfMesh = LoadMesh("Media\\Textured_triangulated_Cube\\cube.obj");
	SortOfMeshSubset *sortOfMesh = LoadMesh("Media\\pig\\pig.obj");

	//**************************************************************************//
	// Create the vertex buffer.												//
	//**************************************************************************//
    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( SimpleVertex ) * sortOfMesh->numVertices;	//From sortOfMesh
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = sortOfMesh->vertices;						//From sortOfMesh

    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Set vertex buffer
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );



 	//**************************************************************************//
	// Now define some triangles.  That's all DirectX allows us to draw.  This  //
	// is called an index buffer, and it indexes the vertices to make triangles.//
	//																			//
	// This is called an index buffer.											//
	//**************************************************************************//

    bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( USHORT ) * sortOfMesh->numIndices;   //From sortOfMesh
	
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
	InitData.pSysMem = sortOfMesh->indexes;					//From sortOfMesh

    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer );
    if( FAILED( hr ) )
        return hr;

	g_numIndices = sortOfMesh->numIndices;

    // Set index buffer
    g_pImmediateContext->IASetIndexBuffer( g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

    // Set primitive topology
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );


	//**************************************************************************//
	// Can this be true?  Nigel is caught cleaning up after himself.  Once the  //
	// vertex and index buffers are created, this structure is no longer needed.//
	//**************************************************************************//
	delete sortOfMesh->indexes;		// Delete the two arrays.
	delete sortOfMesh->vertices;
	delete sortOfMesh;				// Then delete  sortOfMesh


 	//**************************************************************************//
	// Create the 3 constant buffers.											//
	//**************************************************************************//
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CBNeverChanges);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBNeverChanges );
    if( FAILED( hr ) )
        return hr;
    
    bd.ByteWidth = sizeof(CBChangeOnResize);
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBChangeOnResize );
    if( FAILED( hr ) )
        return hr;
    
    bd.ByteWidth = sizeof(CBChangesEveryFrame);
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBChangesEveryFrame );
    if( FAILED( hr ) )
        return hr;



   	//**************************************************************************//
	// Load the texture into "ordinary" RAM.									//
	//**************************************************************************//
	hr = D3DX11CreateShaderResourceViewFromFile( g_pd3dDevice, 
												 L"Media\\woodtexture.jpg", 
												 NULL, NULL, 
												 &g_pTextureResourceView,		// This is returned.
												 NULL );
    if( FAILED( hr ) )
        return hr;


	//**************************************************************************//
	// Put the texture in shader (video) memory.  As there is only one texture,	//
	// we can do this only once.  Try to minimise the number of times you put	//
	// textures into video memory, there is quite an overhead in doing so.		//
	//**************************************************************************//
	//g_pImmediateContext->PSSetShaderResources( 0, 1, &g_pTextureResourceView );


    // Create the sample state
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerLinear );
    if( FAILED( hr ) )
        return hr;

 

    //**************************************************************************//
	// Update the constant buffer for stuff (the light vector and material		//
	// colour in this case) that never change.  This is faster; don't update	//
	// stuff if you don't have to.												//
	//**************************************************************************//
	CBNeverChanges cbNeverChanges;
	cbNeverChanges.materialColour = XMFLOAT4(1, 1, 1, 1);		//Alpha does nothing.
	cbNeverChanges.vecLight       = XMVectorSet(1, 1, -2, 0);	//4th value unused.
	cbNeverChanges.vecLight       = XMVector3Normalize(cbNeverChanges.vecLight);
    g_pImmediateContext->UpdateSubresource( g_pCBNeverChanges, 
											0, NULL, 
											&cbNeverChanges, 
											0, 0 );



    //**************************************************************************//
	// Creatre the projection matrix.  Generally you will only want to create	//
	// this matrix once and then forget it.										//
	//**************************************************************************//
 	g_MatProjection = XMMatrixPerspectiveFovLH( XM_PIDIV4,			// Field of view (pi / 4 radians, or 45 degrees
		                                     width / (FLOAT)height, // Aspect ratio.
											 0.01f,					// Near clipping plane.
											 100.0f );				// Far clipping plane.
   
    CBChangeOnResize cbChangesOnResize;
    cbChangesOnResize.matProjection = XMMatrixTranspose( g_MatProjection );
    g_pImmediateContext->UpdateSubresource( g_pCBChangeOnResize, 0, NULL, &cbChangesOnResize, 0, 0 );


    return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();

    if( g_pSamplerLinear ) g_pSamplerLinear->Release();
    if( g_pTextureResourceView ) g_pTextureResourceView->Release();
    if( g_pCBNeverChanges ) g_pCBNeverChanges->Release();
    if( g_pCBChangeOnResize ) g_pCBChangeOnResize->Release();
    if( g_pCBChangesEveryFrame ) g_pCBChangesEveryFrame->Release();
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pIndexBuffer ) g_pIndexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if( g_pDepthStencil ) g_pDepthStencil->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}


//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
	//**************************************************************************//
    // Update our time.  This block is supposed to make the movement frame rate //
	// independent, as the frame rate we get depends of the performance of our	//
	// computer.  We may even be in reference (software emulation) mode, which	//
	// is painfully slow.														//
	//**************************************************************************//
    static float t = 0.0f;
    if( g_driverType == D3D_DRIVER_TYPE_REFERENCE )
    {
        t += ( float )XM_PI * 0.0125f;
    }
    else
    {
        static DWORD dwTimeStart = 0;
        DWORD dwTimeCur = GetTickCount();
        if( dwTimeStart == 0 )
            dwTimeStart = dwTimeCur;
        t = ( dwTimeCur - dwTimeStart ) / 1000.0f;
    }

    // Rotate cube around the origin
    XMMATRIX matCubeWorld = XMMatrixRotationY( t );

 
    //
    // Clear the back buffer
    //
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, ClearColor );

    //
    // Clear the depth buffer to 1.0 (max depth)
    //
    g_pImmediateContext->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );



	//**************************************************************************//
    // Initialize the view matrix.  What you do to the viewer matrix moves the  //
	// viewer, or course.														//
	//																			//
	// The viewer matrix is created every frame here, which looks silly as the	//
	// viewer never moves.  However in general your viewer does move.			//
	//**************************************************************************//
    XMVECTOR Eye = XMVectorSet( 0.0f, 1.0f, -3.0f, 0.0f );
    XMVECTOR At = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
    XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	
	XMMATRIX matView = XMMatrixLookAtLH( Eye,	// The eye, or viewer's position.
										 At,	// The look at point.
										 Up );	// Which way is up.



	XMMATRIX matWVP = matCubeWorld * matView * g_MatProjection;
    //
    // Update variables that change once per frame
    //
    CBChangesEveryFrame cb;
    cb.matWorld                = XMMatrixTranspose( matCubeWorld );
	cb.matWorldViewProjection  = XMMatrixTranspose( matWVP);
    g_pImmediateContext->UpdateSubresource( g_pCBChangesEveryFrame, 0, NULL, &cb, 0, 0 );



    //
    // Render the cube
    //
    g_pImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
    g_pImmediateContext->PSSetConstantBuffers( 0, 1, &g_pCBNeverChanges );		//Note this one belongs to the pixel shader.
    g_pImmediateContext->VSSetConstantBuffers( 0, 1, &g_pCBChangeOnResize );	// Paremeter 1 relates to pisition in 
    g_pImmediateContext->VSSetConstantBuffers( 1, 1, &g_pCBChangesEveryFrame );	// constant buffers.
    g_pImmediateContext->PSSetShader( g_pPixelShader, NULL, 0 );
    g_pImmediateContext->PSSetConstantBuffers( 2, 1, &g_pCBChangesEveryFrame );
     g_pImmediateContext->PSSetSamplers( 0, 1, &g_pSamplerLinear );
	 g_pImmediateContext->DrawIndexed( g_numIndices, 0, 0 );

    //
    // Present our back buffer to our front buffer
    //
    g_pSwapChain->Present( 0, 0 );
}




//**************************************************************************//
// Convert an old chracter (char *) string to a WCHAR * string.  There must//
// be something built into Visual Studio to do this for me, but I can't		//
// find it - Nigel.															//
//**************************************************************************//
void charStrToWideChar(WCHAR *dest, char *source)
{
	int length = strlen(source);
	for (int i = 0; i <= length; i++)
		dest[i] = (WCHAR) source[i];
}



//**************************************************************************//
// Normalise an XMFLOAT3 as if it were a vector, i.e. return a vector of	//
// unit magnitude.  By Nigel.												//
//**************************************************************************//
void XMFLOAT3normalise(XMFLOAT3 *toNormalise)
{
	float magnitude = (toNormalise->x * toNormalise->x 
		             + toNormalise->y * toNormalise->y
					 + toNormalise->z * toNormalise->z);

	magnitude = sqrt(magnitude);
	toNormalise->x /= magnitude;	
	toNormalise->y /= magnitude;	
	toNormalise->z /= magnitude;
}



//**************************************************************************//
// A couple of functions to trim the start and and of a C++ string, which	//
// strangely doesn't have a trim() method.									//
//																			//
// I nicked them from the Internet, which is why they are incomprehensible.	//
//**************************************************************************//
std::wstring TrimStart(std::wstring s) 
{
        s.erase(s.begin(), std::find_if(s.begin(), 
			    s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

std::wstring TrimEnd(std::wstring s) 
{
        s.erase(std::find_if(s.rbegin(), s.rend(), 
			std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

struct VertexXY
{
	float x, y;
};

//**************************************************************************//
// A simple structure to hold a single vertex.								//
//**************************************************************************//
struct VertexXYZ
{
	float x, y, z;
};



//**************************************************************************//
// Load the obj file into an array.  Everything here is wide (unicode)		//
//strings, hence the "w"s on the front of everything.						//
//																			//
// However, my sample does not:												//
// Read normal vectors.														// 
// Read texture UV coordinates												//
// Read the materials (the lighting properties) from the associated ".mtl"	//
//     file																	//
// Create any textures (which are indicated in the associated ".mtl" file)	//
// Handle meshes with subsets (meshes within meshes or groups in ".obj"		//
//     jargon)																//
//																			//
// ...And only works with meshes in exactly the right format.				//
//**************************************************************************//
SortOfMeshSubset *LoadMesh(LPSTR objFilePath)
{
	std::wifstream          fileStream;
	std::wstring            line;
	std::vector <VertexXYZ> vectorVertices(0);
	std::vector <VertexXY> vectorTextureVertices(0);
	std::vector <VertexXYZ> vectorNormal(0);
	std::vector <USHORT>    vectorIndices(0);

	fileStream.open(objFilePath);
	bool isOpen = fileStream.is_open();		//debugging only.


	while(std::getline(fileStream, line))
	{
		line = TrimStart(line);
		
		//Get name of .mtl file
		if (line.compare(0, 7, L"mtllib ") == 0)
		{
			WCHAR first[7];
			WCHAR mtlFileName[200];

			WCHAR oldStyleStr[200];
			wcscpy(oldStyleStr, line.c_str());
			swscanf(oldStyleStr, L"%6s%s", first, mtlFileName);
			
			//get length of mtlFileName
			int mtlNameLength = 0;
			for (int i = 0; i < 200; i++)
			{
				if (mtlFileName[i] != NULL)
				{
					mtlNameLength++;
				}
				else
				{
					break;
				}
			}

			//Get length of the obj file path
			int objFilePathLength = strlen(objFilePath);

			//get length of obj file name
			int fileNameLength = 0;
			for (int i = objFilePathLength -1; i >= 0; i--)
			{
				if (objFilePath[i] == '\\')
				{
					fileNameLength = objFilePathLength - (i + 1);
					break;
				}
			}

			//Get the path to parent folder of the obj file
			WCHAR objParentPath[200];

			for (int i = 0; i < 200; i++)
			{
				if (i < objFilePathLength - fileNameLength)
				{
					objParentPath[i] = objFilePath[i];
				}
				else
				{
					objParentPath[i] = NULL;
					break;
				}
				
			}

			//I hate c++ strings
			//Parse .mtl file
			WCHAR mtlFullPath[200];
			wcscpy(mtlFullPath, objParentPath);
			wcscat(mtlFullPath, mtlFileName);
			ParseMtlFile(mtlFullPath);
		}

		//******************************************************************//
		// If true, we have found a vertex.  Read it in as a 2 character	//
		// string, followed by 3 decimal numbers.	Suprisingly the C++		//
		// string has no split() method.   I am using really old stuff,		//
		// fscanf.  There  must be a better way, use regular expressions?	//
		//******************************************************************//
		if (line.compare(0, 2, L"v ") == 0)  //"v space"
		{
			WCHAR first[5];
			float x, y, z;

			WCHAR oldStyleStr[200];
			wcscpy(oldStyleStr, line.c_str());
			swscanf(oldStyleStr, L"%2s%f%f%f", first, &x, &y, &z); 

			VertexXYZ v;
			v.x = x; v.y = y; v.z = z;
			vectorVertices.push_back(v);
		}

		//Vertex textures
		if (line.compare(0, 3, L"vt ") == 0)  //"vt space"
		{
			WCHAR first[5];
			float x, y;

			WCHAR oldStyleStr[200];
			wcscpy(oldStyleStr, line.c_str());
			swscanf(oldStyleStr, L"%2s%f%f%f", first, &x, &y);

			VertexXY v;
			v.x = x; v.y = y;
			vectorTextureVertices.push_back(v);
		}

		//Vector Normal
		if (line.compare(0, 3, L"vn ") == 0)  //"vn space"
		{
			WCHAR first[5];
			float x, y, z;

			WCHAR oldStyleStr[200];
			wcscpy(oldStyleStr, line.c_str());
			swscanf(oldStyleStr, L"%2s%f%f%f", first, &x, &y, &z);

			VertexXYZ v;
			v.x = x; v.y = y; v.z = z;
			vectorNormal.push_back(v);
		}


		//******************************************************************//
		// If true, we have found a face.   Read it in as a 2 character		//
		// string, followed by 3 decimal numbers.	Suprisingly the C++		//
		// string has no split() method.   I am using really old stuff,		//
		// fscanf.  There must be a better way, use regular expressions?	//
		//																	//
		// It assumes the line is in the format								//
		// f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ...							//
		//******************************************************************// 
		if (line.compare(0, 2, L"f ") == 0)  //"f space"
		{
			WCHAR first[5];
			WCHAR slash1[5];
			WCHAR slash2[5];
			WCHAR slash3[5];
			WCHAR slash4[5];
			WCHAR slash5[5];
			WCHAR slash6[5];

			UINT v1, vt1, vn1, v2, vt2, vn2, v3, vt3, vn3; 

			WCHAR oldStyleStr[200];
			wcscpy(oldStyleStr, line.c_str());
			swscanf(oldStyleStr, L"%2s%d%1s%d%1s%d%d%1s%d%1s%d%d%1s%d%1s%d", first, 
										&v1, slash1, &vt1, slash2, &vn1, 
				                        &v2, slash3, &vt2, slash4, &vn2, 
										&v3, slash5, &vt3, slash6, &vn3); 

			vectorIndices.push_back(v1-1);	// Check this carefully; see below
			vectorIndices.push_back(v2-1);
			vectorIndices.push_back(v3-1);
		}
	}


	
	//******************************************************************//
	// Now build up the arrays.											//
	//																	// 
	// Nigel to address this bit; it is WRONG.  OBJ meshes assume index //
	// numbers start from 1; C arrays have indexes that start at 0.		//
	//																	//
	// See abobe wih the -1s.  Sorted?									//
	//******************************************************************//
	SortOfMeshSubset *mesh  = new SortOfMeshSubset;
	
	mesh->numVertices = (USHORT) vectorVertices.size();
	mesh->vertices    = new SimpleVertex[mesh->numVertices];
	for (int i = 0; i < mesh->numVertices; i++)
	{
		mesh->vertices[i].Pos.x			= vectorVertices[i].x;
		mesh->vertices[i].Pos.y			= vectorVertices[i].y;
		mesh->vertices[i].Pos.z			= vectorVertices[i].z;
		mesh->vertices[i].VecNormal.x	= vectorNormal[i].x;
		mesh->vertices[i].VecNormal.y	= vectorNormal[i].y;
		mesh->vertices[i].VecNormal.z	= vectorNormal[i].z;
		mesh->vertices[i].TexUV.x		= vectorTextureVertices[i].x;
		mesh->vertices[i].TexUV.y		= vectorTextureVertices[i].y;
	}

	mesh->numIndices = (USHORT) vectorIndices.size();
	mesh->indexes    = new USHORT[mesh->numIndices];
	for (int i = 0; i < mesh->numIndices; i++)
	{
		mesh->indexes[i] = vectorIndices[i];
	}
	
	return mesh;
}

void ParseMtlFile(WCHAR *mtlPath)
{
	D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice,
		L"Media\\pig\\pig_d.jpg",
		NULL, NULL,
		&g_pTextureResourceView,		// This is returned.
		NULL);

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureResourceView);
}
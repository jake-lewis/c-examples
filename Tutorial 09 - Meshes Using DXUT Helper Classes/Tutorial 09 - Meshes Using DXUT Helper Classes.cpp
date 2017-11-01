//**************************************************************************//
// This is a modified version of the Microsoft sample code and loads a mesh.//
// it uses the helper class CDXUTSDKMesh, as there is no longer any built in//
// support for meshes in DirectX 11.										//
//																			//
// The CDXUTSDKMesh is NOT DorectX, not is the file format it uses, the		//
// .sdkmesh, a standard file format. You will hnot find the .sdkmesh format	//
// outside the MS sample code.  Both these things are provided as an entry	//
// point only.																//
//																			//
// Look for the Nigel style comments, like these, for the bits you need to  //
// look at.																	//
//																			//
// You may notice that this sample tries to create a DirectX11 rendering	//
// device, and if it can't do that creates a DirectX 9 device.  I'm not		//
// using DirectX9.															//
//**************************************************************************//


//**************************************************************************//
// Modifications to the MS sample code is copyright of Dr Nigel Barlow,		//
// lecturer in computing, University of Plymouth, UK.						//
// email: nigel@soc.plymouth.ac.uk.											//
//																			//
// Sdkmesh added to MS sample Tutorial09.									//
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
// File: Tutorial 09 - Meshes Using DXUT Helper Classes.cpp
//
// This sample shows a simple example of the Microsoft Direct3D's High-Level 
// Shader Language (HLSL) using the Effect interface. 
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include <xnamath.h>
#include "resource.h"



//**************************************************************************//
// Global Variables.  There are many global variables here (we aren't OO	//
// yet.  I doubt  Roy Tucker (Comp Sci students will know him) will			//
// approve pf this either.  Sorry, Roy.										//
//**************************************************************************//
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_D3DSettingsDlg;       // Device settings dialog
CDXUTDialog                 g_HUD;                  // manages the 3D   
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls
CModelViewerCamera          g_Camera;				// Not used by Nigel.
CDXUTDirectionWidget        g_LightControl;			// Not used by Nigel.


float                       g_fLightScale;
int                         g_nNumActiveLights;
int                         g_nActiveLight;
bool                        g_bShowHelp = false;    // If true, it renders the UI control text
int							g_width  = 800;
int							g_height = 600;;

//**************************************************************************//
// Meshes here.																//
//**************************************************************************//
CDXUTSDKMesh                g_MeshTiger;			// Wot, not a pointer type?
CDXUTSDKMesh				g_MeshWingLH;
CDXUTSDKMesh				g_MeshWingRH;
CDXUTSDKMesh				g_MeshFloorTile;
CDXUTSDKMesh				g_MeshCloudBox;

XMMATRIX					g_MatProjection;

ID3D11InputLayout          *g_pVertexLayout11 = NULL;
ID3D11Buffer               *g_pVertexBuffer   = NULL;
ID3D11Buffer               *g_pIndexBuffer    = NULL;
ID3D11VertexShader         *g_pVertexShader   = NULL;
ID3D11PixelShader          *g_pPixelShader    = NULL;
ID3D11PixelShader		   *g_pPixelShaderNoLighting = NULL;
ID3D11SamplerState         *g_pSamLinear      = NULL;

//**********************************************************************//
// Variables to control the movement of the tiger.						//
// The only one I have coded is rotate about Y, we need an x, y, z		//
// position and maybe rotates about other axes.							//
//**********************************************************************//
struct TIGER
{
	float g_f_TigerRY;

	float g_f_TigerX;
	float g_f_TigerY;
	float g_f_TigerZ;

	float g_f_TigerSpeed;

	XMVECTOR g_vecTigerInitDir;

	TIGER() : 
		g_f_TigerRY(XMConvertToRadians(15)), 
		g_f_TigerX(0), 
		g_f_TigerY(0), 
		g_f_TigerZ(0), 
		g_f_TigerSpeed(0), 
		g_vecTigerInitDir(XMVectorSet(0, 0, -1, 0)) {}
};

TIGER g_Tiger;

bool		 g_b_LeftArrowDown      = false;	//Status of keyboard.  Thess are set
bool		 g_b_RightArrowDown     = false;	//in the callback KeyboardProc(), and 
bool		 g_b_UpArrowDown	    = false;	//are used in onFrameMove().
bool		 g_b_DownArrowDown	    = false;


//**************************************************************************//
// This is M$ code, but is usuig old D3DX from DirectX9.  I'm glad to see   //
// that M$ are having issues updating their sample code, same as me - Nigel.//
//**************************************************************************//
CDXUTTextHelper*            g_pTxtHelper = NULL;


//**************************************************************************//
// This is a structure we pass to the vertex shader.  						//
// Note we do it properly here and pass the WVP matrix, rather than world,	//
// view and projection matrices separately.									//
//**************************************************************************//
struct CB_VS_PER_OBJECT
{
	XMMATRIX matWorldViewProj;
    XMMATRIX matWorld;				// needed to transform the normals.
};




//**************************************************************************//
// These are structures we pass to the pixel shader.  						//
// Note we do it properly here and pass the WVP matrix, rather than world,	//
// view and projection matrices separately.									//
//																			//
// These structures must be identical to those defined in the shader that	//
// you use.  So much for encapsulation; Roy	Tucker (Comp Sci students will  //
// know him) will not approve.												//
//**************************************************************************//
struct CB_PS_PER_OBJECT
{
    XMFLOAT4 m_vObjectColor;
};
UINT                        g_iCBPSPerObjectBind = 0;

struct CB_PS_PER_FRAME
{
    XMVECTOR m_vLightDirAmbient;	// Vector pointing at the light
};


struct MexhVertexStructure
{
	XMFLOAT4 pos;
	XMFLOAT3 normal;
	XMFLOAT2 TextureUV;
};

UINT                        g_iCBPSPerFrameBind = 1;



//**************************************************************************//
// Now a global instance of each constant buffer.							//
//**************************************************************************//
ID3D11Buffer               *g_pcbVSPerObject = NULL;
ID3D11Buffer               *g_pcbPSPerObject = NULL;
ID3D11Buffer               *g_pcbPSPerFrame  = NULL;



//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           3
#define IDC_CHANGEDEVICE        4





//**************************************************************************//
// If you are not used to "C" you will find that functions (or methods in	//
// "C++" must have templates defined in advance.  It is usual to define the //
// prototypes in a header file, but we'll put them here for now to keep		//
// things simple.															//
//**************************************************************************//
//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

extern bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
                                             bool bWindowed, void* pUserContext );
extern HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice,
                                            const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
extern HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                           void* pUserContext );
extern void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime,
                                        void* pUserContext );
extern void CALLBACK OnD3D9LostDevice( void* pUserContext );
extern void CALLBACK OnD3D9DestroyDevice( void* pUserContext );

bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext );

void InitApp();
void RenderText();
void charStrToWideChar(WCHAR *dest, char *source);
void RenderMesh (ID3D11DeviceContext* pd3dImmediateContext, CDXUTSDKMesh *toRender);
void RenderMesh(ID3D11DeviceContext* pd3dImmediateContext, CDXUTSDKMesh *toRender, ID3D11PixelShader *pixelShader);




//**************************************************************************//
// A Windows program always kicks off in WinMain.							//
// Initializes everything and goes into a message processing				//
// loop.																	//
//																			//
// This version uses DXUT, and is much more complicated than previous		//
// versions you have seen.  This allows you to sync the frame rate to your  //
// monitor's vertical sync event.											//
//**************************************************************************//
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D11) 
    // that is available on the system depending on which D3D callbacks are set below

    
	//**************************************************************************//
	// Set DXUT callbacks.														//
    //**************************************************************************//
	DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );


    DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnD3D9CreateDevice );
    DXUTSetCallbackD3D9DeviceReset( OnD3D9ResetDevice );
    DXUTSetCallbackD3D9FrameRender( OnD3D9FrameRender );
    DXUTSetCallbackD3D9DeviceLost( OnD3D9LostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnD3D9DestroyDevice );


    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    InitApp();
    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"Tutorial 09 - Meshes Using DXUT Helper Classes" );
    DXUTCreateDevice (D3D_FEATURE_LEVEL_9_2, true, 800, 600 );
    //DXUTCreateDevice(true, 640, 480);
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{

    // Initialize dialogs
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 23 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 23, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 23, VK_F2 );

    g_SampleUI.SetCallback( OnGUIEvent ); iY = 10;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // Uncomment this to get debug information from D3D11
    //pDeviceSettings->d3d11.CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }
    }

    return true;
}






//**************************************************************************//
// Handle updates to the scene.  This is called regardless of which D3D		//
// API is used (we are only using Dx11).									//
//**************************************************************************//
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	if (g_b_LeftArrowDown)  g_Tiger.g_f_TigerRY -= fElapsedTime*3;	// Frame rate 
	if (g_b_RightArrowDown) g_Tiger.g_f_TigerRY += fElapsedTime*3;	// independent.
	if (g_b_UpArrowDown)	g_Tiger.g_f_TigerSpeed += fElapsedTime;
	if (g_b_DownArrowDown)	g_Tiger.g_f_TigerSpeed -= fElapsedTime;

	//If no speed modification
	if (!g_b_UpArrowDown && !g_b_DownArrowDown)
	{
		//If moving forward
		if (g_Tiger.g_f_TigerSpeed >= 0)
		{
			//Halt if slow
			if (g_Tiger.g_f_TigerSpeed - (fElapsedTime / 2) < 0)
			{
				g_Tiger.g_f_TigerSpeed = 0;
			}
			else //Slow down
			{
				g_Tiger.g_f_TigerSpeed -= (fElapsedTime / 2);
			}
		}
		else //Moving Backwards
		{
			//Halt if slow
			if (g_Tiger.g_f_TigerSpeed + (fElapsedTime / 2) > 0)
			{
				g_Tiger.g_f_TigerSpeed = 0;
			}
			else // Slow down
			{
				g_Tiger.g_f_TigerSpeed += (fElapsedTime / 2);
			}
		}
		
	}
}




//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
    UINT nBackBufferHeight = ( DXUTIsAppRenderingWithD3D9() ) ? DXUTGetD3D9BackBufferSurfaceDesc()->Height :
            DXUTGetDXGIBackBufferSurfaceDesc()->Height;

    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    // Draw help
    if( g_bShowHelp )
    {
        g_pTxtHelper->SetInsertionPos( 2, nBackBufferHeight - 20 * 6 );
        g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        g_pTxtHelper->DrawTextLine( L"Controls:" );

        g_pTxtHelper->SetInsertionPos( 20, nBackBufferHeight - 20 * 5 );
        g_pTxtHelper->DrawTextLine( L"Rotate model: Left /right arrows\n"
                                    L"And extend this yourself\n" );

        g_pTxtHelper->SetInsertionPos( 550, nBackBufferHeight - 20 * 5 );
        g_pTxtHelper->DrawTextLine( L"Hide help: F1\n"
                                    L"Quit: ESC\n" );
    }
    else
    {
        g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        g_pTxtHelper->DrawTextLine( L"Press F1 for help" );
    }

    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;


    return 0;
}







//**************************************************************************//
// Handle key presses.														//
//**************************************************************************//
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    if( bKeyDown )
    {
        switch( nChar )
        {
            case VK_F1:
                g_bShowHelp = !g_bShowHelp; break;

      }
    }

	
	//**************************************************************//
	// Nigel code to rotate the tiger.								//
	//**************************************************************//
	switch( nChar )
	{		       
		case VK_LEFT:  g_b_LeftArrowDown  = bKeyDown; break;
		case VK_RIGHT: g_b_RightArrowDown = bKeyDown; break;
		case VK_UP:    g_b_UpArrowDown    = bKeyDown; break;
		case VK_DOWN:  g_b_DownArrowDown  = bKeyDown; break;
       }
}



//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() ); break;
    }

}


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
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

    // find the file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( str, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
		WCHAR errorCharsW[200];
        if( pErrorBlob != NULL )
		{
			charStrToWideChar(errorCharsW, (char *)pErrorBlob->GetBufferPointer());
            MessageBox( 0, errorCharsW, L"Error", 0 );
		}
        if( pErrorBlob ) pErrorBlob->Release();
        return hr;
    }
    SAFE_RELEASE( pErrorBlob );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );





	//**********************************************************************//
    // Compile the pixel and vertex shaders.  If your computer doesn't		//
	// support shader model 5, try shader model 4.  There is nothing we are //
	// using here that needs shader model 5.								//
	//**********************************************************************..
    ID3DBlob* pVertexShaderBuffer = NULL;
    V_RETURN( CompileShaderFromFile( L"Tutorial 09 - Meshes Using DXUT Helper Classes_VS.hlsl", "VS_DXUTSDKMesh", "vs_4_0", &pVertexShaderBuffer ) );

    ID3DBlob* pPixelShaderBuffer = NULL;
    V_RETURN( CompileShaderFromFile( L"Tutorial 09 - Meshes Using DXUT Helper Classes_PS.hlsl", "PS_DXUTSDKMesh", "ps_4_0", &pPixelShaderBuffer ) );

	ID3DBlob* pPixelNoLightingBuffer = NULL;
	V_RETURN(CompileShaderFromFile(L"Tutorial 09 - Meshes Using DXUT Helper Classes_PS.hlsl", "PS_NOLIGHTING_DXUTSDKMesh", "ps_4_0", &pPixelNoLightingBuffer));
    
	
	//**********************************************************************//
    // Create the pixel and vertex shaders.									//
	//**********************************************************************//
	V_RETURN( pd3dDevice->CreateVertexShader( pVertexShaderBuffer->GetBufferPointer(),
                                              pVertexShaderBuffer->GetBufferSize(), NULL, &g_pVertexShader ) );
    DXUT_SetDebugName( g_pVertexShader, "VS_DXUTSDKMesh" );
    V_RETURN( pd3dDevice->CreatePixelShader( pPixelShaderBuffer->GetBufferPointer(),
                                             pPixelShaderBuffer->GetBufferSize(), NULL, &g_pPixelShader ) );
    DXUT_SetDebugName( g_pPixelShader, "PS_DXUTSDKMesh" );
	V_RETURN(pd3dDevice->CreatePixelShader(pPixelNoLightingBuffer->GetBufferPointer(),
		pPixelNoLightingBuffer->GetBufferSize(), NULL, &g_pPixelShaderNoLighting));
	DXUT_SetDebugName(g_pPixelShaderNoLighting, "PS_NoLighting_DXUTSDKMesh");

	
	
	//**********************************************************************//
    // Define the input layout.  I won't go too much into this except that  //
	// the vertex defined here MUST be consistent with the vertex shader	//
	// input you use in your shader file and the constand buffer structure  //
	// at the top of this module.											//
	//																		//
	// Normal vectors are used by lighting.									//
	//**********************************************************************//
     const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    V_RETURN( pd3dDevice->CreateInputLayout( layout, ARRAYSIZE( layout ), pVertexShaderBuffer->GetBufferPointer(),
                                             pVertexShaderBuffer->GetBufferSize(), &g_pVertexLayout11 ) );
    DXUT_SetDebugName( g_pVertexLayout11, "Primary" );

    SAFE_RELEASE( pVertexShaderBuffer );
    SAFE_RELEASE( pPixelShaderBuffer );



    //**************************************************************************//
	// Initialize the projection matrix.  Generally you will only want to create//
	// this matrix once and then forget it.										//
	//**************************************************************************//
	g_MatProjection = XMMatrixPerspectiveFovLH( XM_PIDIV2,				// Field of view (pi / 2 radians, or 90 degrees
											 g_width / (FLOAT) g_height, // Aspect ratio.
											 0.01f,						// Near clipping plane.
											 100.0f );					// Far clipping plane.


	//**************************************************************************//
    // Load the mesh.															//
	//**************************************************************************//
    V_RETURN( g_MeshTiger.Create( pd3dDevice, L"Media\\tiger\\tiger.sdkmesh", true ) );
	V_RETURN( g_MeshWingLH.Create(pd3dDevice, L"Media\\wing\\wing.sdkmesh", true));
	V_RETURN(g_MeshWingRH.Create(pd3dDevice, L"Media\\wing\\wing.sdkmesh", true));
	V_RETURN(g_MeshFloorTile.Create(pd3dDevice, L"Media\\floor\\seafloor.sdkmesh", true));
	V_RETURN(g_MeshCloudBox.Create(pd3dDevice, L"Media\\cloudbox\\skysphere.sdkmesh", true));


	// Create a sampler state
    D3D11_SAMPLER_DESC SamDesc;
    SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    SamDesc.MipLODBias = 0.0f;
    SamDesc.MaxAnisotropy = 1;
    SamDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    SamDesc.BorderColor[0] = SamDesc.BorderColor[1] = SamDesc.BorderColor[2] = SamDesc.BorderColor[3] = 0;
    SamDesc.MinLOD = 0;
    SamDesc.MaxLOD = D3D11_FLOAT32_MAX;
    V_RETURN( pd3dDevice->CreateSamplerState( &SamDesc, &g_pSamLinear ) );
    DXUT_SetDebugName( g_pSamLinear, "Primary" );

    
	//**************************************************************************//
	// Create the 3 constant bufers, using the same buffer descriptor to create //
	// all three.																//
	//**************************************************************************//
    D3D11_BUFFER_DESC Desc;
 	ZeroMemory( &Desc, sizeof(Desc) );
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = 0;
    Desc.MiscFlags = 0;
	
    Desc.ByteWidth = sizeof( CB_VS_PER_OBJECT );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbVSPerObject ) );
    DXUT_SetDebugName( g_pcbVSPerObject, "CB_VS_PER_OBJECT" );

    Desc.ByteWidth = sizeof( CB_PS_PER_OBJECT );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbPSPerObject ) );
    DXUT_SetDebugName( g_pcbPSPerObject, "CB_PS_PER_OBJECT" );

    Desc.ByteWidth = sizeof( CB_PS_PER_FRAME );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbPSPerFrame ) );
    DXUT_SetDebugName( g_pcbPSPerFrame, "CB_PS_PER_FRAME" );


    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

	g_width  = pBackBufferSurfaceDesc->Width;
	g_height = pBackBufferSurfaceDesc->Height;

	g_HUD.SetLocation( g_width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
	g_SampleUI.SetLocation( g_width - 170, g_height - 300 );
    g_SampleUI.SetSize( 170, 300 );

    return S_OK;
}




//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext )
{
    HRESULT hr;

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Clear the render target and depth stencil
    float ClearColor[4] = { 0.0f, 0.25f, 0.25f, 0.55f };
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

	
	//******************************************************************//
	// Create the world matrix for the tiger: just a rotate around	    //
	// the Y axis of 45 degrees.  DirectX does all angles in radians,	//
	// hence the conversion.  And a translate.							//
	//******************************************************************//


	float rotation = sin(timeGetTime() / 500.0);
	float rotation2 = cos(timeGetTime() / 500.0);
	float tigerScaleConstant = 1.5f;
	int arseHeightOffset = 2;
	float rotation = sin(timeGetTime() / 300.0);
	float rotation2 = cos(timeGetTime() / 300.0);

	//Tiger Movement
	XMMATRIX matTigerRotate = XMMatrixRotationY(g_Tiger.g_f_TigerRY);

	XMVECTOR vecNewDir;
	vecNewDir = XMVector3TransformCoord(g_Tiger.g_vecTigerInitDir, matTigerRotate);
	vecNewDir = XMVector3Normalize(vecNewDir);

	XMVECTOR vecArse = vecNewDir * -3;

	vecNewDir *= g_Tiger.g_f_TigerSpeed * fElapsedTime;
	g_Tiger.g_f_TigerX += XMVectorGetX(vecNewDir); // Weird syntax; can't just
	g_Tiger.g_f_TigerY += XMVectorGetY(vecNewDir); // use vector.x.
	g_Tiger.g_f_TigerZ += XMVectorGetZ(vecNewDir);

	XMMATRIX matTigerTranslate = XMMatrixTranslation(g_Tiger.g_f_TigerX, g_Tiger.g_f_TigerY, g_Tiger.g_f_TigerZ);
	XMMATRIX matTigerScale     = XMMatrixScaling(tigerScaleConstant, tigerScaleConstant, tigerScaleConstant);
	XMMATRIX matTigerWorld     = matTigerRotate * matTigerTranslate * matTigerScale;
    
	XMMATRIX matWorldViewProjection;

	//**************************************************************************//
	// Initialize the view matrix.  What you do to the viewer matrix moves the  //
	// viewer, or course.														//
	//																			//
	// The viewer matrix is created every frame here, which looks silly as the	//
	// viewer never moves.  However in general your viewer does move.			//
	//**************************************************************************//
	XMVECTOR Eye = XMVectorSet(
		g_Tiger.g_f_TigerX + XMVectorGetX(vecArse), 
		g_Tiger.g_f_TigerY + XMVectorGetY(vecArse) + arseHeightOffset,
		g_Tiger.g_f_TigerZ + XMVectorGetZ(vecArse),
		0) * tigerScaleConstant;
	XMVECTOR At = XMVectorSet(g_Tiger.g_f_TigerX, g_Tiger.g_f_TigerY, g_Tiger.g_f_TigerZ, 0.0f) * tigerScaleConstant;
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX matView;
	matView = XMMatrixLookAtLH(Eye,	// The eye, or viewer's position.
		At,		// The look at point.
		Up);	// Which way is up.

	matWorldViewProjection = matTigerWorld * matView * g_MatProjection;

	//Create world matrix for wings

	XMMATRIX matWingRotationZ = XMMatrixRotationZ(rotation);
	XMMATRIX matWingScale = XMMatrixScaling(0.7f, 0.7f, 0.7f);

	//LH Wing
	XMMATRIX matWingLHTranslate = XMMatrixTranslation(0.1, 0.6, -0.8);
	XMMATRIX matWingLHWorld = matWingRotationZ * matWingLHTranslate * matWingScale * matTigerWorld;

	XMMATRIX matWingLHWorldViewProjection;
	matWingLHWorldViewProjection = matWingLHWorld * matView * g_MatProjection;

	//RH Wing
	XMMATRIX matWingRHTranslate = XMMatrixTranslation(-0.1, 0.4, -0.6);
	XMMATRIX matWingRHRotateY = XMMatrixRotationY(XMConvertToRadians(180));
	XMMATRIX matWingWorldRH = matWingRotationZ * matWingRHRotateY * matWingScale * matWingRHTranslate * matTigerWorld;

	XMMATRIX matWingRHWorldViewProjection;
	matWingRHWorldViewProjection = matWingWorldRH * matView * g_MatProjection;

	//Floor
	XMMATRIX matFloorScale = XMMatrixScaling(1, 1, 1);
	XMMATRIX matFloorWorld = matFloorScale;

	XMMATRIX matFloorWorldViewProjection = matFloorWorld * matView * g_MatProjection;

	//Skybox
	XMMATRIX matSkyboxScale		= XMMatrixScaling(0.03f, 0.03f, 0.03f);
	XMMATRIX matSkyboxWorld		= matSkyboxScale;

	XMMATRIX matSkyboxWorldViewProjection = matSkyboxWorld * matView * g_MatProjection;

	//******************************************************************//
	// Lighting.  Ambient light and a light direction, above, to the	//
	// left and two paces back, I think.  Then normalise the light		//
	// vector.  It is kind-a-silly doing this every frame unless the	//
	// light moves.														//
	//******************************************************************//
    float    fAmbient                = 0.1f;
	XMVECTOR vectorLightDirection    = XMVectorSet(-1, 1, -2, 0);  // 4th value unused.
	vectorLightDirection = XMVector3Normalize(vectorLightDirection);
										


	CB_PS_PER_FRAME CBPerFrame;
	CBPerFrame.m_vLightDirAmbient = vectorLightDirection;
	pd3dImmediateContext->UpdateSubresource( g_pcbPSPerFrame, 0, NULL, &CBPerFrame, 0, 0 );
	pd3dImmediateContext->PSSetConstantBuffers( 1, 1, &g_pcbPSPerFrame );


	CB_PS_PER_OBJECT CBPerObject;
	CBPerObject.m_vObjectColor = XMFLOAT4(1, 1, 1, 1);
	pd3dImmediateContext->UpdateSubresource( g_pcbPSPerObject, 0, NULL, &CBPerObject, 0, 0 );
	pd3dImmediateContext->PSSetConstantBuffers( 0, 1, &g_pcbPSPerObject );


    pd3dImmediateContext->PSSetSamplers( 0, 1, &g_pSamLinear );

	//******************************************************************//    
	// Update shader variables.  We must update these for every mesh	//
	// that we draw (well, actually we need only update the position	//
	// for each mesh, think hard about this - Nigel						//
	//																	//
	// We pass the parameters to it in a constant buffer.  The buffer	//
	// we define in this module MUST match the constant buffer in the	//
	// shader.															//
	//																	//
	// It would seem that the constant buffer we pass to the shader must//
	// be global, well defined on the heap anyway.  Not a local variable//
	// it would seem.													//
	//******************************************************************//

	CB_VS_PER_OBJECT CBMatrices;
	CBMatrices.matWorld = XMMatrixTranspose(matTigerWorld);
	CBMatrices.matWorldViewProj = XMMatrixTranspose(matWorldViewProjection);
	pd3dImmediateContext->UpdateSubresource(g_pcbVSPerObject, 0, NULL, &CBMatrices, 0, 0);
	pd3dImmediateContext->VSSetConstantBuffers(0, 1, &g_pcbVSPerObject);

	
	//**************************************************************************//
	// Render the mesh.															//
	//**************************************************************************//
	RenderMesh (pd3dImmediateContext, &g_MeshTiger);
	
	CBMatrices.matWorld = XMMatrixTranspose(matWingLHWorld);
	CBMatrices.matWorldViewProj = XMMatrixTranspose(matWingLHWorldViewProjection);
	pd3dImmediateContext->UpdateSubresource(g_pcbVSPerObject, 0, NULL, &CBMatrices, 0, 0);
	pd3dImmediateContext->VSSetConstantBuffers(0, 1, &g_pcbVSPerObject); 

	RenderMesh(pd3dImmediateContext, &g_MeshWingLH);

	CBMatrices.matWorld = XMMatrixTranspose(matWingWorldRH);
	CBMatrices.matWorldViewProj = XMMatrixTranspose(matWingRHWorldViewProjection);
	pd3dImmediateContext->UpdateSubresource(g_pcbVSPerObject, 0, NULL, &CBMatrices, 0, 0);
	pd3dImmediateContext->VSSetConstantBuffers(0, 1, &g_pcbVSPerObject);

	RenderMesh(pd3dImmediateContext, &g_MeshWingRH);
    
	CBMatrices.matWorld = XMMatrixTranspose(matFloorWorld);
	CBMatrices.matWorldViewProj = XMMatrixTranspose(matFloorWorldViewProjection);
	pd3dImmediateContext->UpdateSubresource(g_pcbVSPerObject, 0, NULL, &CBMatrices, 0, 0);
	pd3dImmediateContext->VSSetConstantBuffers(0, 1, &g_pcbVSPerObject);

	RenderMesh(pd3dImmediateContext, &g_MeshFloorTile);

	CBMatrices.matWorld = XMMatrixTranspose(matSkyboxWorld);
	CBMatrices.matWorldViewProj = XMMatrixTranspose(matSkyboxWorldViewProjection);
	pd3dImmediateContext->UpdateSubresource(g_pcbVSPerObject, 0, NULL, &CBMatrices, 0, 0);
	pd3dImmediateContext->VSSetConstantBuffers(0, 1, &g_pcbVSPerObject);

	RenderMesh(pd3dImmediateContext, &g_MeshCloudBox, g_pPixelShaderNoLighting);
	
	//**************************************************************************//
	// Render what is rather grandly called the head up display.				//
	//**************************************************************************//
	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    g_HUD.OnRender( fElapsedTime );
    g_SampleUI.OnRender( fElapsedTime );
    RenderText();
    DXUT_EndPerfEvent();
}

//Render a mesh using the default Pixel Shader
void RenderMesh(ID3D11DeviceContext *pContext,
				CDXUTSDKMesh         *toRender)
{
	RenderMesh(pContext, toRender, g_pPixelShader);
}


//**************************************************************************//
// Overloaded: Render a CDXUTSDKMesh, using the Device Context specified	//
//and specific Pixel Shader.												//
//**************************************************************************//
void RenderMesh (ID3D11DeviceContext *pContext, 
				 CDXUTSDKMesh         *toRender,
				 ID3D11PixelShader	*pixelShader)
{
	//Get the mesh
    //IA setup
    pContext->IASetInputLayout( g_pVertexLayout11 );
    UINT Strides[1];
    UINT Offsets[1];
    ID3D11Buffer* pVB[1];
    pVB[0] = toRender->GetVB11( 0, 0 );
    Strides[0] = ( UINT )toRender->GetVertexStride( 0, 0 );
    Offsets[0] = 0;
    pContext->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
    pContext->IASetIndexBuffer( toRender->GetIB11( 0 ), toRender->GetIBFormat11( 0 ), 0 );

    // Set the shaders
    pContext->VSSetShader( g_pVertexShader, NULL, 0 );
    pContext->PSSetShader( pixelShader,  NULL, 0 );
    
	for( UINT subset = 0; subset < toRender->GetNumSubsets( 0 ); ++subset )
    {
		//Render
		SDKMESH_SUBSET* pSubset = NULL;
		D3D11_PRIMITIVE_TOPOLOGY PrimType;
        
		// Get the subset
        pSubset = toRender->GetSubset( 0, subset );

        PrimType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
        pContext->IASetPrimitiveTopology( PrimType );

		//**************************************************************************//
		// At the moment we load a texture into video memory every frame, which is	//
		// HORRIBLE, we need to create more Texture2D variables.					//
		//**************************************************************************//
        ID3D11ShaderResourceView* pDiffuseRV = toRender->GetMaterial( pSubset->MaterialID )->pDiffuseRV11;
        pContext->PSSetShaderResources( 0, 1, &pDiffuseRV );

        pContext->DrawIndexed( ( UINT )pSubset->IndexCount, 0, ( UINT )pSubset->VertexStart );
    }

}



//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );

    g_MeshTiger.Destroy();
	g_MeshWingLH.Destroy();
	g_MeshWingRH.Destroy();
	g_MeshFloorTile.Destroy();
	g_MeshCloudBox.Destroy();
                
    SAFE_RELEASE( g_pVertexLayout11 );
    SAFE_RELEASE( g_pVertexBuffer );
    SAFE_RELEASE( g_pIndexBuffer );
    SAFE_RELEASE( g_pVertexShader );
    SAFE_RELEASE( g_pPixelShader );
	SAFE_RELEASE(g_pPixelShaderNoLighting);
    SAFE_RELEASE( g_pSamLinear );

    SAFE_RELEASE( g_pcbVSPerObject );
    SAFE_RELEASE( g_pcbPSPerObject );
    SAFE_RELEASE( g_pcbPSPerFrame );
}




//**************************************************************************//
// Convert an old chracter (char *) string to a WCHAR * string.  There must //
// be something built into Visual Studio to do this for me, but I can't		//
// find it - Nigel.															//
//**************************************************************************//
void charStrToWideChar(WCHAR *dest, char *source)
{
	int length = strlen(source);
	for (int i = 0; i <= length; i++)
		dest[i] = (WCHAR) source[i];
}

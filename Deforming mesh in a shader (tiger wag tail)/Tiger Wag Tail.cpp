//**************************************************************************//
// DirectX10 version!!														//
//																			//
// Tiger with wagging tail, which is dynalically deformed in the shader.	//
//																			//
// This is based on an empty sample, "simpleSample" provided for us by		//
// Microsoft.  It does all the hard work of enumerating the hardware and	//
// giving us these reasonable cool "DxUT buttons" to change things.			//
//																			//
// This version uses the fixed finction pipeline to do the rendering, the   //
// build in lighting etc. within DirectX.  There is an alternative that uses//
// a shader file, which you don't have to use.								//
//																			//
// Look for this style of comments for the bits you must work on.			//
//																			//
// OnD3D10CreateDevice():													//
//					The tiger mesh and the light are created in this one,   //
//					but are never destroyed.  Odd things happen even if you	//
//					change the screen to, say, full	screen.					//
// OnD3D10FrameRender():													//
//					The vast majority of the action takes place in here.	//
// OnFrameMove():	Movement is processed in this one, based on some global //
//					variables that indicate keyboard status.				//
// Keyboard():		Key events are processed in here.  Not proper			//
//					DirectInput, I'm afraid.								//
//																			//
// NOTE that by default, this sample synchronizes the frame rate to the		//
// vSync event.  You can turn that off with the "change device" button.		//
//																			//
// Also NOTE that this uses the Microsoft utitity class CDXUTMesh to load	//
// and render the mesh, but I am not using the Microsoft camara class		//
// CModelViewerCamera.  That class looks very nice, do use it is you want	//
// to, but also understand how the view matrix is created.					//
//																			//
// And, as usual, you should correct at least 10% of the typig abd spekking //
// errirs.	nigel@soc.plymouth.ac.uk.										//
//																			//
// Heavily based on "SimpleSample" Copyright (c) Microsoft Corporation.		//
//**************************************************************************//

//**************************************************************************//
// Note that, unlike the old common framework applications which all extend //
// the CD3DApplication, this is not a class.  It is just a program that		//
// defines several callbacks, like OnFrameRender().							//
//																			//
//**************************************************************************//

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "resource.h"
#include "nig.h"
#include <stdio.h>

//#define DEBUG_VS   // Uncomment this line to debug D3D9 vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug D3D9 pixel shaders 


//**************************************************************************//
// Convert an old chrtacter (char *) string to a WCHAR * string.  There must//
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
// Global variables originally defined by Microsoft.						//
//**************************************************************************//

CModelViewerCamera          g_Camera;               // A model viewing camera
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg             g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*            g_pTxtHelper = NULL;
CDXUTDialog                 g_HUD;                  // dialog for standard controls
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls

// Direct3D 9 resources
extern ID3DXFont*           g_pFont9;
extern ID3DXSprite*         g_pSprite9;


// Direct3D 10 resources
ID3DX10Font*                g_pFont10 = NULL;
ID3DX10Sprite*              g_pSprite10 = NULL;
ID3D10InputLayout*          g_pVertexLayout = NULL;



//**************************************************************************//
// Nigel added variables.  All in Hungariain notation, of course.  Don't	//
// feel that you have to use Hungariain notation, though, especially if you //
// aren't Hungarian.														//
//																			//
// There are many global variables here.  I suggest that at some point you	//
// 1: Don't tell Mary that you are using so many globals.					//
// 2: Consider encapsulating some of this stuff into classes of your own.	//
//**************************************************************************//

CDXUTSDKMesh		   g_MeshTiger;			//Wot, not a pointer type?  You 
											//will need one mesh per object
											//you want to draw.
ID3D10Effect		   *g_p_Effect;			//The shader file.
ID3D10InputLayout      *g_p_VertexLayout;	//The vertex structure.
ID3D10EffectTechnique  *g_p_Technique;		//The technique within the shader.



//**************************************************************************//
// This isn't really as it should be.  Lighting (light colour and direction)//
// are surely global things, but the tiger's material is not.  But it is	//
// here, I'll leave that for you to fix.									//
//**************************************************************************//

D3DXVECTOR4		g_lightDirection(0, 1, -1, 0);				//w value unused.
D3DXVECTOR4		g_lightDiffuseColour(1, 1, 1, 1);			//w value unused.
D3DXVECTOR4		g_materialDiffuseColour(1, 1, 1, 1);		//w value unused.






//**************************************************************************//
// There are all variables we will use to assess variables within the		//
// shader (.fx) file.													  //
//**********************************************************************//

ID3D10EffectMatrixVariable		    *g_p_MatWorldViewProjInShader;
ID3D10EffectMatrixVariable          *g_p_MatWorldInShader;
ID3D10EffectScalarVariable		    *g_p_fTimeInShader;
ID3D10EffectShaderResourceVariable  *g_p_txDiffuseInShader;
ID3D10EffectVectorVariable		    *g_p_LightDiffuseColourInShader;
ID3D10EffectVectorVariable	        *g_p_LightDirectionInShader;
ID3D10EffectVectorVariable	        *g_p_MaterialDiffuseColourInShader;



//**********************************************************************//
// Variables to control the movement of the tiger.						//
//**********************************************************************//

float		 g_f_TigerX,  g_f_TigerY, g_f_TigerZ;	//X, y, z position.
float		 g_f_TigerRX, g_f_TigerRY,g_f_TigerRZ;	//Rotate about X, Y, Z.

bool		 g_b_LeftArrowDown      = false;	//Status of keyboard.  Thess are set
bool		 g_b_RightArrowDown     = false;	//in the callback KeyboardProc(), and 
bool		 g_b_UpArrowDown	    = false;	//are used in onFrameMove().
bool		 g_b_DownArrowDown	    = false;



//**********************************************************************//
// Extra variables to make the tail wag.  A separate technique, and a   //
// float to pass the tail angle in.  Shaders are (well, were) stateless //
// so if we want something to be passed from frame to frame you need to //
// pass it as a parameter.                                              //
//**********************************************************************//

ID3D10EffectTechnique       *g_p_TechniqueWagTail;		
ID3D10EffectScalarVariable	*g_p_fTailAngleInShader;
float                       g_f_TailAngle = 0;


//**********************************************************************//
// XBOX controller macros and structures.								//
//**********************************************************************//

#define MAX_CONTROLLERS 4  // XInput handles up to 4 controllers 
#define INPUT_DEADZONE  ( 0.24f * FLOAT(0x7FFF) )  // Default to 24% of the +/- 32767 range.   This is a reasonable default value but can be altered if needed.

struct CONTROLER_STATE
{
    XINPUT_STATE state;
    bool connected;
};

CONTROLER_STATE g_Controllers[MAX_CONTROLLERS];
WCHAR g_szMessage[4][1024] = {0};
HWND    g_hWnd;
bool    g_bDeadZoneOn = true;




//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           2
#define IDC_CHANGEDEVICE        3
#define IDC_TOGGLEWARP          4


//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );

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

bool CALLBACK IsD3D10DeviceAcceptable( UINT Adapter, UINT Output, D3D10_DRIVER_TYPE DeviceType,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D10CreateDevice( ID3D10Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext );
HRESULT CALLBACK OnD3D10ResizedSwapChain( ID3D10Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D10FrameRender( ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK OnD3D10ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D10DestroyDevice( void* pUserContext );

void InitApp();
void RenderText();
void renderMesh(ID3D10Device* pd3dDevice, ID3D10EffectTechnique *technique,	CDXUTSDKMesh *mesh);
void UpdateControllerState();




//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D10) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnD3D9CreateDevice );
    DXUTSetCallbackD3D9DeviceReset( OnD3D9ResetDevice );
    DXUTSetCallbackD3D9DeviceLost( OnD3D9LostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnD3D9DestroyDevice );
    DXUTSetCallbackD3D9FrameRender( OnD3D9FrameRender );

    DXUTSetCallbackD3D10DeviceAcceptable( IsD3D10DeviceAcceptable );
    DXUTSetCallbackD3D10DeviceCreated( OnD3D10CreateDevice );
    DXUTSetCallbackD3D10SwapChainResized( OnD3D10ResizedSwapChain );
    DXUTSetCallbackD3D10SwapChainReleasing( OnD3D10ReleasingSwapChain );
    DXUTSetCallbackD3D10DeviceDestroyed( OnD3D10DestroyDevice );
    DXUTSetCallbackD3D10FrameRender( OnD3D10FrameRender );

    InitApp();
    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"Starting point for Coursework" );
    DXUTCreateDevice( true, 640, 480 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    g_SettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3 );
    g_HUD.AddButton( IDC_TOGGLEWARP, L"Toggle WARP (F4)", 35, iY += 24, 125, 22, VK_F4 );

    g_SampleUI.SetCallback( OnGUIEvent ); iY = 10;


	//**********************************************************************//
	// Zero the mamory block used by the controller.						//
	//**********************************************************************//
    ZeroMemory( g_Controllers, sizeof( CONTROLER_STATE ) * MAX_CONTROLLERS );

}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 5, 5 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );



	//**********************************************************************//
	// Write controller related stuff.  As usual this is not good from a    //
	// performance point of view as we are calling this code every frame.   //
	//**********************************************************************//

	WCHAR strBuffer[50];
	int numControllers = 0;
	for (int i = 0; i < MAX_CONTROLLERS; i++)
		if (g_Controllers[i].connected) numControllers++;

	swprintf(strBuffer, L"Num Controllers found: %d", numControllers);
	g_pTxtHelper->DrawTextLine(strBuffer);

    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Reject any D3D10 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D10DeviceAcceptable( UINT Adapter, UINT Output, D3D10_DRIVER_TYPE DeviceType,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10CreateDevice( ID3D10Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr;

    V_RETURN( D3DX10CreateSprite( pd3dDevice, 500, &g_pSprite10 ) );
    V_RETURN( g_DialogResourceManager.OnD3D10CreateDevice( pd3dDevice ) );
    V_RETURN( g_SettingsDlg.OnD3D10CreateDevice( pd3dDevice ) );
    V_RETURN( D3DX10CreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                L"Arial", &g_pFont10 ) );
    g_pTxtHelper = new CDXUTTextHelper( NULL, NULL, g_pFont10, g_pSprite10, 15 );





	//**********************************************************************//
	// We use the camera object to store the View and Projection matrices.  //
	// The projection matrix is created in OnD3D10ResizedSwapChain().		//
	// Setup the camera's view parameters here, which creates the view		//
	// matrix.																//
	//**********************************************************************//
    
	D3DXVECTOR3 vecEye( 0.0f, 1.0f, -5.0f );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, 0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );





	//**********************************************************************//
	// Create the effect (shader file).  This is quite a business; do ask!  //
	//**********************************************************************//
	
	ID3D10Blob *errors;

	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
	#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3D10_SHADER_DEBUG;
    #endif

	
	// first, see if the effect file exists.
	WCHAR path[MAX_PATH];
    hr =  DXUTFindDXSDKMediaFileCch( path, MAX_PATH, L"TigerWagTail.fx" );
	V(hr);


	hr = D3DX10CreateEffectFromFile( path, NULL, NULL, "fx_4_0", 
									 dwShaderFlags, 0, pd3dDevice, NULL,
                                     NULL, &g_p_Effect, &errors, NULL );



	//**********************************************************************//
	// This code is simply to extract any errors that occur when compiling  //
	// the ".fx" fine and displaying them in a message box so that we can   //
	// see them.															//
	//**********************************************************************//

	if (FAILED(hr))
	{
		WCHAR errorWStr[500];
		char  *errorCharStr;
		char  errorCharStrFull[400];

		if (errors == NULL) V(hr);		//Probably file not found.

		errorCharStr = (char *) errors->GetBufferPointer();

		strcpy(errorCharStrFull, "Error compiling FX file\n\n");
		strcat(errorCharStrFull, errorCharStr);

		charStrToWideChar(errorWStr, errorCharStrFull);
		ShowMessage(errorWStr);
	}




	//**********************************************************************//
	// Obtain the technique and other shader variables.						//
	//**********************************************************************//

    g_p_Technique          = g_p_Effect->GetTechniqueByName( "RenderScene" );
    g_p_txDiffuseInShader  = g_p_Effect->GetVariableByName( "g_MeshTexture" )->AsShaderResource();
    
	g_p_MatWorldViewProjInShader = g_p_Effect->GetVariableByName( "g_mWorldViewProjection" )->AsMatrix();
    g_p_MatWorldInShader         = g_p_Effect->GetVariableByName( "g_mWorld" )->AsMatrix();
    g_p_fTimeInShader            = g_p_Effect->GetVariableByName( "g_fTime" )->AsScalar();
	
	g_p_LightDiffuseColourInShader     = g_p_Effect->GetVariableByName ("g_LightDiffuse")->AsVector();
	g_p_LightDirectionInShader		   = g_p_Effect->GetVariableByName ("g_LightDir")->AsVector();
	g_p_MaterialDiffuseColourInShader  = g_p_Effect->GetVariableByName ("g_MaterialDiffuseColor")->AsVector();


	g_p_TechniqueWagTail	= g_p_Effect->GetTechniqueByName( "RenderTigerWagTail" );
	g_p_fTailAngleInShader  = g_p_Effect->GetVariableByName( "g_fTailAngle")->AsScalar();



	//**********************************************************************//
	// Vertex format for our mesh.  The vertices are not coloured, but 		//
	// there is a texture u and v here.										//
	//**********************************************************************//

	const D3D10_INPUT_ELEMENT_DESC layout[] =
	{
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	};
  	   
	UINT numElements = sizeof( layout ) / sizeof( layout[0] );

	
  // Create the input layout
    D3D10_PASS_DESC PassDesc;
    g_p_Technique->GetPassByIndex( 0 )->GetDesc( &PassDesc );

	//Also for the wag tail technique.
	g_p_TechniqueWagTail->GetPassByIndex( 0 )->GetDesc( &PassDesc );


    V_RETURN( pd3dDevice->CreateInputLayout( layout, numElements, PassDesc.pIAInputSignature,
                                             PassDesc.IAInputSignatureSize, &g_pVertexLayout ) );

    // Set the input layout
    pd3dDevice->IASetInputLayout( g_pVertexLayout );




    pd3dDevice->CreateInputLayout( layout, numElements, PassDesc.pIAInputSignature,
                                     PassDesc.IAInputSignatureSize, (ID3D10InputLayout**) &g_p_VertexLayout);



    // Set the input layout
    pd3dDevice->IASetInputLayout( g_p_VertexLayout );

    
	
	//**********************************************************************//
	// Assuming all your meshes have the same vertex structure as the tiger,//
	// you can just load them here.											//
	//**********************************************************************//

	V( g_MeshTiger.Create( pd3dDevice, L"Media\\Tiger\\Tiger.sdkmesh", true ) );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10ResizedSwapChain( ID3D10Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D10ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_SettingsDlg.OnD3D10ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );



 	//**********************************************************************//
	// Create the projection matrix.	Again, the projection matrix is		//
	// contained within the camera.											//
	//**********************************************************************//
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI / 4, fAspectRatio, 0.1f, 1000.0f );


    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300 );
    g_SampleUI.SetSize( 170, 300 );

    return S_OK;
}






//**************************************************************************//
// Render the scene using the D3D10 device.									//
// As this skeleton is in DirectX10, this is the one we are using.			//
//**************************************************************************//

void CALLBACK OnD3D10FrameRender( ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    D3DXMATRIX matWorldViewProjection;
    D3DXMATRIX matView;
    D3DXMATRIX matProj;


	D3DXMATRIX matTigerWorld;
	D3DXMATRIX matTigerRotY;
	D3DXMATRIX matTigerRotX;


    float ClearColor[4] = { 0.176f, 0.196f, 0.667f, 0.0f };
    ID3D10RenderTargetView* pRTV = DXUTGetD3D10RenderTargetView();
    pd3dDevice->ClearRenderTargetView( pRTV, ClearColor );

    // Clear the depth stencil
    ID3D10DepthStencilView* pDSV = DXUTGetD3D10DepthStencilView();
    pd3dDevice->ClearDepthStencilView( pDSV, D3D10_CLEAR_DEPTH, 1.0, 0 );

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }



	//****************************************************************************//
	// Move the Tiger and then render it.   At the moment, all we do is rotate it,//
	// you should translate it as well.	 									      //
	//****************************************************************************//

	D3DXMatrixRotationX( &matTigerRotX, g_f_TigerRX );
	D3DXMatrixRotationY( &matTigerRotY, g_f_TigerRY );

	matTigerWorld = matTigerRotX * matTigerRotY;	//I.e. just rotations, no position,
													//In C++, the "*" and "=" operators
													//have been overloaded for us by 
													//Microsoft.
	
	//****************************************************************************//
	// Get the projection & view matrix, which are now hidden in the camera class.//
	//****************************************************************************//
    
    matProj = *g_Camera.GetProjMatrix();
    matView = *g_Camera.GetViewMatrix();
    matWorldViewProjection = matTigerWorld * matView * matProj;



    // Update the effect's variables.  Instead of using strings, it would 
    // be more efficient to cache a handle to the parameter by calling 
    // ID3DXEffect::GetParameterByName


    g_p_MatWorldViewProjInShader->SetMatrix( ( float* )&matWorldViewProjection );
    g_p_MatWorldInShader->SetMatrix(         ( float* )&matTigerWorld );
    g_p_fTimeInShader->SetFloat(             ( float )fTime );


	g_p_LightDirectionInShader->SetFloatVector(      (float *) &g_lightDirection);
	g_p_LightDiffuseColourInShader->SetFloatVector(  (float *) &g_lightDiffuseColour);
	g_p_MaterialDiffuseColourInShader->SetFloatVector((float *) &g_materialDiffuseColour);

	g_f_TailAngle = sin((double) timeGetTime()/200);     //Set tail angle from real time clock.
	g_p_fTailAngleInShader->SetFloat( (float) g_f_TailAngle); //Then tell the shader about it.

	renderMesh(pd3dDevice, g_p_TechniqueWagTail, &g_MeshTiger);


	//****************************************************************************//
	// And finally, render the DXUT buttons and the thing which grandly calls	  //
	// itself a HUD (Head Up Display).											  //
	//****************************************************************************//

    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    RenderText();
    g_HUD.OnRender( fElapsedTime );
    g_SampleUI.OnRender( fElapsedTime );
    DXUT_EndPerfEvent();
}





//****************************************************************************//
// Render the mesh.															  //
//																			  //
// I have extracted this from the onFrameRender() function to make the program//
// look a little more readable.												  //
//                                                                            //
// And in the wag tail version, we must specify the technique to be used as   //
// well.                                                                      //
//****************************************************************************//
 
void renderMesh(ID3D10Device* pd3dDevice,
	            ID3D10EffectTechnique *technique,
				CDXUTSDKMesh *mesh)
{
 
	pd3dDevice->IASetInputLayout( g_p_VertexLayout );

	UINT Strides[1];
    UINT Offsets[1];
    ID3D10Buffer* pVB[1];
    pVB[0] = mesh->GetVB10( 0, 0 );
    Strides[0] = ( UINT )mesh->GetVertexStride( 0, 0 );
    Offsets[0] = 0;
    pd3dDevice->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
    pd3dDevice->IASetIndexBuffer( mesh->GetIB10( 0 ), mesh->GetIBFormat10( 0 ), 0 );

    D3D10_TECHNIQUE_DESC techDesc;
    
	//************************************************************************//
	// Ah.  We are using another technique here, do be aware of that.         //
	//************************************************************************//
	technique->GetDesc( &techDesc );
    SDKMESH_SUBSET* pSubset = NULL;
    ID3D10ShaderResourceView* pDiffuseRV = NULL;
    D3D10_PRIMITIVE_TOPOLOGY PrimType;


	
	for( UINT p = 0; p < techDesc.Passes; ++p )
    {
        for( UINT subset = 0; subset < mesh->GetNumSubsets( 0 ); ++subset )
        {
            pSubset = mesh->GetSubset( 0, subset );

            PrimType = mesh->GetPrimitiveType10( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
            pd3dDevice->IASetPrimitiveTopology( PrimType );

            pDiffuseRV = mesh->GetMaterial( pSubset->MaterialID )->pDiffuseRV10;
            g_p_txDiffuseInShader->SetResource( pDiffuseRV );

			technique->GetPassByIndex( p )->Apply( 0 );
            pd3dDevice->DrawIndexed( ( UINT )pSubset->IndexCount, 0, ( UINT )pSubset->VertexStart );
        }
  
	}
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D10ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D10DestroyDevice();
    g_SettingsDlg.OnD3D10DestroyDevice();
    SAFE_RELEASE( g_pFont10 );
    SAFE_RELEASE( g_p_Effect );
    SAFE_RELEASE( g_pVertexLayout );
    SAFE_RELEASE( g_pSprite10 );
    SAFE_DELETE( g_pTxtHelper );
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    if( pDeviceSettings->ver == DXUT_D3D9_DEVICE )
    {
        IDirect3D9* pD3D = DXUTGetD3D9Object();
        D3DCAPS9 Caps;
        pD3D->GetDeviceCaps( pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &Caps );

        // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
        // then switch to SWVP.
        if( ( Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) == 0 ||
            Caps.VertexShaderVersion < D3DVS_VERSION( 1, 1 ) )
        {
            pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }

        // Debugging vertex shaders requires either REF or software vertex processing 
        // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
        if( pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF )
        {
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
            pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }
#endif
#ifdef DEBUG_PS
        pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
    }

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF ) ||
            ( DXUT_D3D10_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE ) )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
    }

    return true;
}




//**************************************************************************//
// Handle updates to the scene.  This is called regardless of which D3D API //
// is used.																	//
//**************************************************************************//

void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );

	UpdateControllerState();



	//**********************************************************************//
	// Process key presses.													//
	//**********************************************************************//
	
	if (g_b_LeftArrowDown)  g_f_TigerRY -= fElapsedTime*2;	//Rotate about y in a frame rate independent
	if (g_b_RightArrowDown) g_f_TigerRY += fElapsedTime*2;	//way.  Case is not used as it is possible 	
	if (g_b_UpArrowDown)    g_f_TigerRX += fElapsedTime*2;	//that several keys could be down at once.
	if (g_b_DownArrowDown)  g_f_TigerRX -= fElapsedTime*2;

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
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//****************************************************************************//
// This doesn't give us the lovely remapable keys that the old common		  //
// framework classes give us.  And you need to look in the header file		  //
// "WinUser.h" to see what the keys mean.  Right click on, say, VK_F1, to open//
// the header file.															  //
//																			  //
// The "bKeyDown" parameter tells us wheather the key was pressed or released.//
//****************************************************************************//

void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
switch( nChar )
    {
        
		//******************************************************************//
		// Nigel code to rotate the tiger.									//
		//******************************************************************//

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
        case IDC_TOGGLEWARP:
            DXUTToggleWARP(); break;
        case IDC_CHANGEDEVICE:
            g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;
    }
}



//****************************************************************************//
// Polls the Xbox controllers (if present) and uptates their state into a	  //
// global array g_Controllers.												  //
//****************************************************************************//

void UpdateControllerState()
{
    DWORD dwResult;
    for( DWORD i = 0; i < MAX_CONTROLLERS; i++ )
    {
        // Simply get the state of the controller from XInput.
        dwResult = XInputGetState( i, &g_Controllers[i].state );

        if( dwResult == ERROR_SUCCESS )
            g_Controllers[i].connected = true;
        else
            g_Controllers[i].connected = false;
    }


	//**********************************************************************//
	// We only use controller[0].	Look in the state.GamePad variable to   //
	// see what is available.												//
	//**********************************************************************//

	if (g_Controllers[0].connected)
	{
		float ry = g_Controllers[0].state.Gamepad.sThumbLX;	//Left Joypad 
		float rx = g_Controllers[0].state.Gamepad.sThumbLY; //Left Joypad 

		//**********************************************************************//
		// Joypads have a dead zone.  To prevent noise, cero the reading if the //
		// answer is within the dead zone.										//
		//**********************************************************************//

		if ((ry < INPUT_DEADZONE) && (ry > -INPUT_DEADZONE )) ry = 0;
		if ((rx < INPUT_DEADZONE) && (rx > -INPUT_DEADZONE )) rx = 0;

		//**********************************************************************//
		// Scale the result.  Full scale is + / - 32767.  Multiply by 90  for a //
		// max ov 90 degrees and convert to radians.							//
		//**********************************************************************//

		g_f_TigerRY = D3DXToRadian( ry / 32767.0 * 90);
		g_f_TigerRX = D3DXToRadian( rx / 32767.0 * 90);
	}
}

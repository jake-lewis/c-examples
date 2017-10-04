//**************************************************************************//
// VERTEX SHADER for the DXUT meshes sample.								//
//																			//
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
// File: BasicHLSL11_VS.hlsl
//
// The vertex shader file for the BasicHLSL11 sample.  
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------



//**************************************************************************//
// First constant buffer, goes at position 0.  We will need to know the		//
// position in the sample that uses this code.  And the constant buffer's	//
// structure must be identical to the structure defined in the program that	//
// uses this shader.														//
//**************************************************************************//
cbuffer cbPerObject : register( b0 )
{
	matrix		g_mWorldViewProjection;	
	matrix		g_mWorld;				
};



//**************************************************************************//
// Vertex shader input structure.	The semantics (the things after the		//
// colon) look a little weird.  The semantics are used (so Microsoft tell	//
// us) used by the compiler to link shader inputs and outputs. 				//
//																			//
// For this to work, you must ensure that the vertex structure you use in	//
// any program that uses this shader is the same as below, vertex position,	//
// normal vector and texture U, V, in that order!							//
//**************************************************************************//
struct VS_INPUT
{
	float4 vPosition	: POSITION;
	float3 vNormal		: NORMAL;
	float2 vTexcoord	: TEXCOORD0;
};


//**************************************************************************//
// Vertex shader output structure.  This is usually plugges into the Pixel	//
// shader input, so the pixel shader input  structure.						//
//																			//
// NOTE: Pos has a different samentic to the structure above; get it wrong	//
// and nothing works.  That's because the pixel shader is in a different	//
// stage in the rendering pipeline.											//
//**************************************************************************//
struct VS_OUTPUT
{
	float3 vNormal		: NORMAL;
	float2 vTexcoord	: TEXCOORD0;
	float4 vPosition	: SV_POSITION;
};



//**************************************************************************//
// Vertex Shader.															//
//**************************************************************************//
VS_OUTPUT VS_DXUTSDKMesh( VS_INPUT Input )
{
	VS_OUTPUT Output;
	//**********************************************************************//
	// Multiply every vertex vy the WVP matrix (we do it "properly here		//
	// unlike the cubes sample.												//
	//**********************************************************************//
	Output.vPosition = mul( Input.vPosition, g_mWorldViewProjection );

	//**********************************************************************//
	// Whatever we do to the tiger, we must also do to its normal vector.	//
	//**********************************************************************//
	Output.vNormal = mul( Input.vNormal, (float3x3)g_mWorld );

	//**********************************************************************//
	// And finally, just copu the texture Us and Vs to the output structure	//
	//**********************************************************************//
	Output.vTexcoord = Input.vTexcoord;
	
	return Output;
}




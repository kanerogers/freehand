#include "Graphics.h"
#include <d3dcompiler.h>
#include <iterator>

namespace wrl = Microsoft::WRL;
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

Graphics::Graphics(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = hWnd;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	// Create device, front/back buffers, swapchain and rendering context
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		D3D11_CREATE_DEVICE_DEBUG,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&sd,
		&pSwap,
		&pDevice,
		nullptr,
		&pContext
	);
	if (hr != 0)
	{
		throw "oh no";
	}

	// Get a reference to the backbuffer of the swapchain
	wrl::ComPtr<ID3D11Resource> pBackBuffer;
	hr = pSwap->GetBuffer(0, __uuidof(ID3D11Resource), &pBackBuffer);
	if (hr != 0)
	{
		throw "oh no";
	}
	
	pDevice->CreateRenderTargetView(
		pBackBuffer.Get(),
		nullptr,
		&pTarget
	);
}

void Graphics::EndFrame()
{
	pSwap->Present(1u, 0u);
}

void Graphics::DrawTriangle()
{
	namespace wrl = Microsoft::WRL;
	HRESULT hr;

	struct Vertex {
		float x;
		float y;
	};

	// an triangle
	const Vertex vertices[] =
	{
		{ 0.0f, 0.5f },
		{ 0.5f, -0.5f },
		{ -0.5f, -0.5f }
	};

	// an vertex buffer
	wrl::ComPtr<ID3D11Buffer> pVertexBuffer;
	D3D11_BUFFER_DESC bd = {};
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags = 0u;
	bd.MiscFlags = 0u;
	bd.ByteWidth = sizeof(vertices);
	bd.StructureByteStride = sizeof(Vertex);
	
	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = vertices;
	
	hr = pDevice->CreateBuffer(&bd, &sd, &pVertexBuffer);
	if (hr != 0) 
	{
		throw "oh no";
	}

	const UINT stride = sizeof(Vertex);
	const UINT offset = 0u;

	// note the use of GetAddressOf instead of & to avoid releasing pVertexBuffer
	pContext->IASetVertexBuffers(0u, 1u, pVertexBuffer.GetAddressOf(), &stride, &offset);
	
	// Pixel shader
	wrl::ComPtr<ID3D11PixelShader> pPixelShader;
	wrl::ComPtr<ID3DBlob> pBlob;
	hr = D3DReadFileToBlob(L"PixelShader.cso", &pBlob);
	if (hr != 0) 
	{
		throw "oh no";
	}

	hr = pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader);
	if (hr != 0) 
	{
		throw "oh no";
	}

	pContext->PSSetShader(pPixelShader.Get(), nullptr, 0u);
	
	// Vertex Shader
	wrl::ComPtr<ID3D11VertexShader> pVertexShader;
	hr = D3DReadFileToBlob(L"VertexShader.cso", &pBlob);
	if (hr != 0) 
	{
		throw "oh no";
	}

	hr = pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader);
	if (hr != 0) 
	{
		throw "oh no";
	}

	pContext->VSSetShader(pVertexShader.Get(), nullptr, 0u);

	// Vertex layout
	wrl::ComPtr<ID3D11InputLayout> pInputLayout;
	const D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	hr = pDevice->CreateInputLayout(
		ied,
		(UINT)std::size(ied),
		pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(),
		&pInputLayout
	);
	if (hr != 0) 
	{
		throw "oh no";
	}

	pContext->IASetInputLayout(pInputLayout.Get());

	// Render target
	pContext->OMSetRenderTargets(1u, pTarget.GetAddressOf(), nullptr);

	// Input topology
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Viewport
	D3D11_VIEWPORT vp = {};
	vp.Width = 800;
	vp.Height = 600;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	pContext->RSSetViewports(1u, &vp);

	// DRAW
	pContext->Draw((UINT)std::size(vertices), 0u);
}

void Graphics::ClearBuffer(float red, float green, float blue) noexcept
{
	const float color[] = { red, green, blue, 1.0f };
	pContext->ClearRenderTargetView(pTarget.Get(), color);
}

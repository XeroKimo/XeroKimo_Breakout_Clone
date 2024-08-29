#include <dxgi1_6.h>
#include <d3d11.h>
#include <utility>
#include <tuple>
#include <d3d11sdklayers.h>
#include <array>
#include <span>
#include <wrl/client.h>
#include <d3dcompiler.h>

module DeluEngine:ExperimentalRenderer;
import TypedDXGI;
import TypedD3D11;
using namespace TypedD3D;

namespace DeluEngine
{
	ExperimentalRenderer::ExperimentalRenderer(HWND window)
	{
		TypedDXGI::Wrapper<IDXGIFactory2> factory = TypedDXGI::CreateFactory1<IDXGIFactory2>();
		std::tie(m_device, m_deviceContext) = TypedD3D11::CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, D3D_FEATURE_LEVEL_11_0, D3D11_SDK_VERSION);
		m_swapChain = factory->CreateSwapChainForHwnd<IDXGISwapChain1>(
			m_device, 
			window,
			DXGI_SWAP_CHAIN_DESC1
			{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.SampleDesc
				{
					.Count = 1
				},
				.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT,
				.BufferCount = 2,
				.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
				.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
			},
			nullptr, 
			nullptr);

		m_debugDevice = TypedD3D::Cast<ID3D11Debug>(m_device.AsComPtr());

		auto backBuffer = m_swapChain->GetBuffer<ID3D11Resource>(0);

		m_backBuffer = m_device->CreateRenderTargetView(backBuffer);
	}

	ExperimentalRenderer::~ExperimentalRenderer()
	{
		if(m_debugDevice)
			m_debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	}

	void ExperimentalRenderer::Draw()
	{
		Texture t;

		TypedD3D11::Wrapper<ID3D11Resource> resource;
	}

	void ExperimentalRenderer::ClearBuffer()
	{
		std::array clearColor{ 1.f, 1.f, 1.f, 1.f };
		m_deviceContext->ClearRenderTargetView(m_backBuffer, clearColor);

		m_deviceContext->OMSetRenderTargets(m_backBuffer, nullptr);
	}

	void ExperimentalRenderer::Present()
	{
		m_swapChain->Present(0, 0);
	}

	struct Vertex
	{
		xk::Math::Vector<float, 3> pos;
		xk::Math::Vector<float, 2> uv;
	};

	constexpr Vertex bl{ { -0.5f, -0.5f}, { 0, 0 } };
	constexpr Vertex tl{ { -0.5f, 0.5f}, { 0, 1 } };
	constexpr Vertex tr{ { 0.5f, 0.5f}, { 1, 1 } };
	constexpr Vertex br{ { 0.5f, -0.5f}, { 1, 0 } };

	constexpr std::array<Vertex, 6> vertexData
	{
		bl, tl, tr,
		tr, br, bl
	};

	std::array<Vertex, 6> TransformVertex(xk::Math::Aliases::Matrix4x4 transform)
	{
		xk::Math::Aliases::Vector4 blPos = transform * xk::Math::Aliases::Vector4{ -0.5f, -0.5f, 0, 1 };
		xk::Math::Aliases::Vector4 tlPos = transform * xk::Math::Aliases::Vector4{ -0.5f, 0.5f, 0, 1 };
		xk::Math::Aliases::Vector4 trPos = transform * xk::Math::Aliases::Vector4{ 0.5f, 0.5f, 0, 1 };
		xk::Math::Aliases::Vector4 brPos = transform * xk::Math::Aliases::Vector4{ 0.5f, -0.5f, 0, 1 };

		Vertex bl{ blPos.Swizzle<0, 1, 2>(), { 0, 0 } };
		Vertex tl{ tlPos.Swizzle<0, 1, 2>(), { 0, 1 } };
		Vertex tr{ trPos.Swizzle<0, 1, 2>(), { 1, 1 } };
		Vertex br{ brPos.Swizzle<0, 1, 2>(), { 1, 0 } };

		return
		{
			bl, tl, tr,
			tr, br, bl
		};
	}


	ExperimentalSpriteRenderer::ExperimentalSpriteRenderer(TypedD3D11::Wrapper<ID3D11Device> device, TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext) :
		m_device{ device },
		m_deviceContext{ deviceContext }
	{
		Microsoft::WRL::ComPtr<ID3DBlob> vertexBlob;

		TypedD3D::ThrowIfFailed(D3DCompileFromFile(L"../Engine/Shaders/VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertexBlob, nullptr));
		m_vertexShader = device->CreateVertexShader(*vertexBlob.Get(), nullptr);
		std::array inputElement
		{
			D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "Position",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 0,
				.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
			D3D11_INPUT_ELEMENT_DESC{
				.SemanticName = "TexCoord",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = 0,
				.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0,
			},
		};

		m_layout = m_device->CreateInputLayout(inputElement, *vertexBlob.Get());

		Microsoft::WRL::ComPtr<ID3DBlob> pixelBlob;
		TypedD3D::ThrowIfFailed(D3DCompileFromFile(L"../Engine/Shaders/PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &pixelBlob, nullptr));
		m_pixelShader = device->CreatePixelShader(*pixelBlob.Get(), nullptr);
		
		{
			D3D11_BUFFER_DESC bufferDesc
			{
				.ByteWidth = sizeof(float) * 5 * 6,
				.Usage = D3D11_USAGE_DYNAMIC,
				.BindFlags = D3D11_BIND_VERTEX_BUFFER,
				.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
				.MiscFlags = 0,
				.StructureByteStride = 0
			};



			D3D11_SUBRESOURCE_DATA data{};
			data.pSysMem = vertexData.data();
			m_vertexBuffer = m_device->CreateBuffer(bufferDesc, nullptr);
		}

		{
			D3D11_RASTERIZER_DESC desc
			{
				.FillMode = D3D11_FILL_SOLID,
				.CullMode = D3D11_CULL_BACK,
				.FrontCounterClockwise = false,
				.DepthBias = 0,
				.DepthBiasClamp = 0,
				.SlopeScaledDepthBias = 0,
				.DepthClipEnable = true,
				.ScissorEnable = true,
				.MultisampleEnable = false,
				.AntialiasedLineEnable = true
			};
			m_rasterizerState = m_device->CreateRasterizerState(desc);
		}

		{
			D3D11_BUFFER_DESC bufferDesc
			{
				.ByteWidth = sizeof(xk::Math::Aliases::Matrix4x4),
				.Usage = D3D11_USAGE_DYNAMIC,
				.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
				.MiscFlags = 0,
				.StructureByteStride = 0
			};
			m_cameraBuffer = m_device->CreateBuffer(bufferDesc);
		}
	}

	void SpriteRenderInterface::Draw(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture, xk::Math::Aliases::Matrix4x4 transform)
	{
		UpdateConstantBuffer(m_renderer.m_deviceContext, m_renderer.m_vertexBuffer, [&transform](D3D11_MAPPED_SUBRESOURCE data)
		{
			auto vertices = TransformVertex(transform);
			std::memcpy(data.pData, &vertices, sizeof(vertices));
		});

		m_renderer.m_deviceContext->IASetVertexBuffers(0, m_renderer.m_vertexBuffer.Get(), sizeof(float) * 5, 0);
		m_renderer.m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D11_VIEWPORT viewports;
		viewports.TopLeftX = 0;
		viewports.TopLeftY = 0;
		viewports.MinDepth = 0;
		viewports.MaxDepth = 1;
		viewports.Width = 1600;
		viewports.Height = 900;

		D3D11_RECT rects;
		rects.left = 0;
		rects.top = 0;
		rects.right = 1600;
		rects.bottom = 900;
		m_renderer.m_deviceContext->RSSetViewports({&viewports, 1});
		//m_renderer.m_deviceContext->RSSetScissorRects({&rects, 1});
		//m_renderer.m_deviceContext->PSSetShaderResources(0, std::span{&texture, 1});

		m_renderer.m_deviceContext->Draw(6, 0);
	}

	//void SpriteRenderInterface::DrawMultiple(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture, std::span<xk::Math::Aliases::Matrix4x4> transform)
	//{
	//	m_renderer.m_deviceContext->PSSetShaderResources(0, texture);
	//	for(auto& t : transform)
	//	{
	//		UpdateConstantBuffer(m_renderer.m_deviceContext, m_renderer.m_constantBuffer, [&transform](D3D11_MAPPED_SUBRESOURCE data)
	//			{
	//				std::memcpy(data.pData, &transform, sizeof(transform));
	//			});
	//		m_renderer.m_deviceContext->VSSetConstantBuffers(0, m_renderer.m_constantBuffer);
	//		m_renderer.m_deviceContext->Draw(6, 0);
	//	}
	//}
}
#include <dxgi1_6.h>
#include <d3d11.h>
#include <utility>
#include <tuple>
#include <d3d11sdklayers.h>
#include <array>

module DeluEngine:ExperimentalRenderer;
import TypedDXGI;
import TypedD3D11;
using namespace TypedD3D;
using namespace TypedDXGI;
using namespace TypedD3D11;
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
	}

	void ExperimentalRenderer::ClearBuffer()
	{
		std::array clearColor{ 1.f, 1.f, 1.f, 1.f };
		m_deviceContext->ClearRenderTargetView(m_backBuffer, clearColor);
	}

	void ExperimentalRenderer::Present()
	{
		m_swapChain->Present(0, 0);
	}
}
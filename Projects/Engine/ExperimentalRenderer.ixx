module;

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

export module DeluEngine:ExperimentalRenderer;
import xk.Math.Matrix;
import TypedD3D11;
import TypedDXGI;

using Microsoft::WRL::ComPtr;

namespace DeluEngine
{
	export struct Texture
	{
		TypedD3D11::Wrapper<ID3D11Texture2D> texture;
		TypedD3D11::Wrapper<ID3D11RenderTargetView> rt;
	};

	export class ExperimentalRenderer
	{
	private:
		TypedD3D11::Wrapper<ID3D11Device> m_device;
		ComPtr<ID3D11Debug> m_debugDevice;
		TypedD3D11::Wrapper<ID3D11DeviceContext> m_deviceContext;
		TypedDXGI::Wrapper<IDXGISwapChain1> m_swapChain;

	public:
		ExperimentalRenderer(HWND window);
		void Draw();
		void Present();
	};
};
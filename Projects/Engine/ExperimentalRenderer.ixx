module;

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <d3d11sdklayers.h>

export module DeluEngine:ExperimentalRenderer;
import xk.Math.Matrix;
import TypedD3D11;
import TypedDXGI;

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
		Microsoft::WRL::ComPtr<ID3D11Debug> m_debugDevice;

		TypedD3D11::Wrapper<ID3D11DeviceContext> m_deviceContext;
		TypedDXGI::Wrapper<IDXGISwapChain1> m_swapChain;
		TypedD3D11::Wrapper<ID3D11RenderTargetView> m_backBuffer;
	public:
		ExperimentalRenderer(HWND window);
		~ExperimentalRenderer();
		void Draw();
		void ClearBuffer();
		void Present();
	};
};
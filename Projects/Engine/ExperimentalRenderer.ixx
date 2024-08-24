module;

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <d3d11sdklayers.h>
#include <span>

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

	public:
		void Draw();
		void ClearBuffer();
		void Present();

	public:
		TypedD3D11::Wrapper<ID3D11Device> GetDevice() const { return m_device; }
		TypedD3D11::Wrapper<ID3D11DeviceContext> GetDeviceContext() const { return m_deviceContext; }
	};

	export class ExperimentalSpriteRenderer;
	export class SpriteRenderInterface
	{
	private:
		ExperimentalSpriteRenderer& m_renderer;

	public:
		SpriteRenderInterface(ExperimentalSpriteRenderer& renderer) : m_renderer{ renderer } {}

		void Draw(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture, xk::Math::Aliases::Matrix4x4 transform);
		void DrawMultiple(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture, std::span<xk::Math::Aliases::Matrix4x4> transform);
	};

	class ExperimentalSpriteRenderer
	{
		friend SpriteRenderInterface;

		TypedD3D11::Wrapper<ID3D11Device> m_device;
		TypedD3D11::Wrapper<ID3D11DeviceContext> m_deviceContext;
		TypedD3D11::Wrapper<ID3D11Buffer> m_constantBuffer;
		TypedD3D11::Wrapper<ID3D11Buffer> m_vertexBuffer;
		TypedD3D11::Wrapper<ID3D11RasterizerState> m_rasterizerState;

		TypedD3D11::Wrapper<ID3D11InputLayout> m_layout;
		TypedD3D11::Wrapper<ID3D11VertexShader> m_vertexShader;
		TypedD3D11::Wrapper<ID3D11PixelShader> m_pixelShader;

	public:
		ExperimentalSpriteRenderer(TypedD3D11::Wrapper<ID3D11Device> device, TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext);

		template<std::invocable<SpriteRenderInterface> Ty>
		void DrawPass(Ty func)
		{
			//m_deviceContext->RSSetState(m_rasterizerState);
			m_deviceContext->IASetInputLayout(m_layout);
			m_deviceContext->VSSetShader(m_vertexShader, {});
			m_deviceContext->PSSetShader(m_pixelShader, {});

			func(SpriteRenderInterface{ *this });
		}
	};
};
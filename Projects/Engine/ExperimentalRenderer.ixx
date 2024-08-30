module;

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <d3d11sdklayers.h>
#include <span>
#include <concepts>

export module DeluEngine:ExperimentalRenderer;
import xk.Math.Matrix;
import xk.Math.Angles;
import TypedD3D11;
import TypedDXGI;

using namespace TypedD3D;
namespace DeluEngine
{
	export struct Texture
	{
		TypedD3D11::Wrapper<ID3D11Texture2D> texture;
		TypedD3D11::Wrapper<ID3D11RenderTargetView> rt;
	};

	export template<std::invocable<D3D11_MAPPED_SUBRESOURCE> Func>
	void UpdateConstantBuffer(TypedD3D11::Wrapper<ID3D11DeviceContext> context, TypedD3D11::Wrapper<ID3D11Resource> resource, Func func)
	{
		D3D11_MAPPED_SUBRESOURCE data = context->Map(resource, 0, D3D11_MAP_WRITE_DISCARD, 0);
		func(data);
		context->Unmap(resource, 0);
	}

	export template<std::invocable<D3D11_MAPPED_SUBRESOURCE> Func>
	void UpdateConstantBufferNoOverwrite(TypedD3D11::Wrapper<ID3D11DeviceContext> context, TypedD3D11::Wrapper<ID3D11Resource> resource, Func func)
	{
		D3D11_MAPPED_SUBRESOURCE data = context->Map(resource, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0);
		func(data);
		context->Unmap(resource, 0);
	}

	export class ExperimentalRenderer;

	export template<class Ty>
	concept RenderPipeline = requires (Ty pipeline, ExperimentalRenderer& renderer)
	{
		pipeline.Bind(renderer);
		requires std::is_class_v<decltype(pipeline.MakeRenderInterface(renderer))> ;
	};

	class ExperimentalRenderer
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
		template<RenderPipeline Ty, std::invocable<decltype(std::declval<Ty>().MakeRenderInterface(std::declval<ExperimentalRenderer&>()))> Func>
		void BindPipeline(Ty& pipeline, Func func)
		{
			pipeline.Bind(*this);
			func(pipeline.MakeRenderInterface(*this));
		}

		void ClearBuffer();
		void Present();

	public:
		TypedD3D11::Wrapper<ID3D11RenderTargetView> GetSwapChainBackBuffer() { return m_backBuffer; }
		TypedD3D11::Wrapper<ID3D11Device> GetDevice() const { return m_device; }
		TypedD3D11::Wrapper<ID3D11DeviceContext> GetDeviceContext() const { return m_deviceContext; }
	};

	export class ExperimentalSpritePipeline;
	export struct Camera
	{
		xk::Math::Matrix<float, 4, 4> viewPerspectiveTransform;

		Camera(xk::Math::Vector<float, 3> position, xk::Math::Degree<float> angle, xk::Math::Matrix<float, 4, 4> perspective);
	};

	export class SpriteRenderInterface
	{
	private:
		ExperimentalRenderer& m_renderer;
		ExperimentalSpritePipeline& m_spriteRenderer;

	public:
		SpriteRenderInterface(ExperimentalRenderer& renderer, ExperimentalSpritePipeline& spriteRenderer) : 
			m_renderer{ renderer },
			m_spriteRenderer{ spriteRenderer }
		{
		}

		template<std::invocable<Camera> Func>
		void CameraPass(const Camera& camera, Func func);

		void Draw(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture, xk::Math::Aliases::Matrix4x4 transform);
		void DrawMultiple(TypedD3D11::Wrapper<ID3D11ShaderResourceView> texture, std::span<xk::Math::Aliases::Matrix4x4> transform);
	};

	struct ExperimentalSpritePipeline
	{
		TypedD3D11::Wrapper<ID3D11Buffer> cameraBuffer;
		TypedD3D11::Wrapper<ID3D11Buffer> vertexBuffer;
		TypedD3D11::Wrapper<ID3D11Buffer> instanceBuffer;
		TypedD3D11::Wrapper<ID3D11RasterizerState> rasterizerState;

		TypedD3D11::Wrapper<ID3D11InputLayout> layout;
		TypedD3D11::Wrapper<ID3D11VertexShader> vertexShader;
		TypedD3D11::Wrapper<ID3D11PixelShader> pixelShader;

	public:
		static constexpr UINT VSPerFrameCBufferSlot = 0;
		static constexpr UINT VSPerCameraCBufferSlot = 1;
		static constexpr UINT VSPerMaterialCBufferSlot = 2;
		static constexpr UINT VSPerObjectCBufferSlot = 3;


	public:
		ExperimentalSpritePipeline(TypedD3D11::Wrapper<ID3D11Device> device, TypedD3D11::Wrapper<ID3D11DeviceContext> deviceContext);

		void Bind(ExperimentalRenderer& renderer)
		{
			renderer.GetDeviceContext()->IASetInputLayout(layout);
			renderer.GetDeviceContext()->VSSetShader(vertexShader, {});
			renderer.GetDeviceContext()->PSSetShader(pixelShader, {});

			D3D11_TEXTURE2D_DESC desc = TypedD3D::Cast<ID3D11Texture2D>(renderer.GetSwapChainBackBuffer()->GetResource())->GetDesc();
			D3D11_VIEWPORT viewports;
			viewports.TopLeftX = 0;
			viewports.TopLeftY = 0;
			viewports.MinDepth = 0;
			viewports.MaxDepth = 1;
			viewports.Width = desc.Width;
			viewports.Height = desc.Height;
			renderer.GetDeviceContext()->RSSetViewports(viewports);
			renderer.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}		
		
		SpriteRenderInterface MakeRenderInterface(ExperimentalRenderer& renderer)
		{
			return { renderer, *this };
		}
	};

	template<std::invocable<Camera> Func>
	void SpriteRenderInterface::CameraPass(const Camera& camera, Func func)
	{
		UpdateConstantBuffer(m_renderer.GetDeviceContext(), m_spriteRenderer.cameraBuffer, [&camera](D3D11_MAPPED_SUBRESOURCE data)
			{
				std::memcpy(data.pData, &camera.viewPerspectiveTransform, sizeof(camera.viewPerspectiveTransform));
			});
		m_renderer.GetDeviceContext()->VSSetConstantBuffers(ExperimentalSpritePipeline::VSPerCameraCBufferSlot, m_spriteRenderer.cameraBuffer);
		func(camera);
	}
};
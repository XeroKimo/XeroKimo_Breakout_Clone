#pragma once

#include "Vector.h"
#include "../DX11/Device.h"
#include "Debug Classes/Exceptions/HRESULTException.h"
#include "SDL_config_windows.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include <Windows.h>
#include <wrl/client.h>
#include <string_view>
#include <memory>

struct IDXGISwapChain4;
struct ID3D11RenderTargetView1;
struct SDL_Window;


namespace InsanityEngine::DX11
{
    class Device;
}

namespace InsanityEngine::DX11
{
    template<class Renderer>
    class Window
    {
    private:
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

    private:
        static constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW;

    private:
        std::unique_ptr<SDL_Window, void (*)(SDL_Window*)> m_handle;
        //DX11::Device* m_device = nullptr;

        Renderer m_renderer;
    public:
        Window(std::string_view windowName, Math::Types::Vector2f windowSize, DX11::Device& device) :
            m_handle(InitializeWindow(windowName, windowSize)),
            m_renderer(device, InitializeSwapChain(device))
        {
            //InitializeSwapChain();
            //InitializeBackBuffer();
        }
        //~Window();

    //public:
    //    void Present();
    //    void Draw();

    //public:
        //ID3D11RenderTargetView1* GetBackBuffer() const { return m_backBuffer.Get(); }
        Math::Types::Vector2f GetWindowSize() const
        {
            int width;
            int height;
            SDL_GetWindowSize(m_handle.get(), &width, &height);
            return Math::Types::Vector2f(width, height);
        }
        Renderer& GetRenderer() { return m_renderer; }


    private:
        std::unique_ptr<SDL_Window, void (*)(SDL_Window*)> InitializeWindow(std::string_view windowName, Math::Types::Vector2f windowSize)
        {
            return std::unique_ptr<SDL_Window, void (*)(SDL_Window*)>(SDL_CreateWindow(windowName.data(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN), &SDL_DestroyWindow);
        }
        ComPtr<IDXGISwapChain4> InitializeSwapChain(DX11::Device& device) {
            ComPtr<IDXGIFactory2> factory;

            HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));

            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = 0;
            swapChainDesc.Height = 0;
            swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            swapChainDesc.Stereo = false;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
            swapChainDesc.BufferCount = 2;
            swapChainDesc.Scaling = DXGI_SCALING_NONE;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
            fullscreenDesc.RefreshRate.Denominator = fullscreenDesc.RefreshRate.Numerator = 0;
            fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            fullscreenDesc.Windowed = true;

            ComPtr<IDXGISwapChain1> tempSwapChain;
            //ComPtr<ID3D11Device> device;

            SDL_SysWMinfo info;
            SDL_VERSION(&info.version);
            SDL_GetWindowWMInfo(m_handle.get(), &info);

            if(HRESULT hr = factory->CreateSwapChainForHwnd(
                device.GetDevice(),
                info.info.win.window,
                &swapChainDesc,
                &fullscreenDesc,
                nullptr,
                tempSwapChain.GetAddressOf());
                FAILED(hr))
            {
                throw Debug::Exceptions::HRESULTException("Failed to create swap chain. HRESULT: ", hr);
            }

            ComPtr<IDXGISwapChain4> swapChain;
            if(HRESULT hr = tempSwapChain.As(&swapChain); FAILED(hr))
            {
                throw Debug::Exceptions::HRESULTException("Failed to query swap chain. HRESULT: ", hr);
            }

            return swapChain;
        }
        //void InitializeBackBuffer();
    };


}
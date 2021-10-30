//#include "Window.h"
//
//using namespace InsanityEngine::Math::Types;
//using namespace InsanityEngine::Debug::Exceptions;
//
//namespace InsanityEngine::DX11
//{
//    Window::Window(std::string_view windowName, Vector2f windowSize, DX11::Device& device, BaseRenderer& renderer) :
//        m_device(&device),
//        m_handle(SDL_CreateWindow(windowName.data(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN), &SDL_DestroyWindow),
//        m_renderer(&renderer)
//    {
//        InitializeWindow(windowName, windowSize);
//        InitializeSwapChain();
//        InitializeBackBuffer();
//    }
//
//    Window::~Window()
//    {
//        m_swapChain->SetFullscreenState(false, nullptr);
//
//    }
//
//    void Window::Present()
//    {
//        m_swapChain->Present(1, 0);
//    }
//
//    void Window::Draw()
//    {
//        m_renderer->Draw(m_backBuffer);
//        m_swapChain->Present(1, 0);
//    }
//
//
//    Vector2f Window::GetWindowSize() const
//    {
//        int width;
//        int height;
//        SDL_GetWindowSize(m_handle.get(), &width, &height);
//        return Vector2f(width, height);
//    }
//
//    void Window::InitializeWindow(std::string_view windowName, Math::Types::Vector2f windowSize)
//    {
//        //m_handle = std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>();
//    }
//
//    void Window::InitializeSwapChain()
//   
//
//    void Window::InitializeBackBuffer()
//    {
//        ComPtr<ID3D11Texture2D> backBuffer;
//        HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
//
//        if(FAILED(hr))
//            throw HRESULTException("Failed to get back buffer. HRESULT: ", hr);
//
//        D3D11_RENDER_TARGET_VIEW_DESC1 rtvDesc = {};
//        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
//        rtvDesc.Texture2D.MipSlice = 0;
//        rtvDesc.Texture2D.PlaneSlice = 0;
//
//        hr = m_device->GetDevice()->CreateRenderTargetView1(backBuffer.Get(), &rtvDesc, &m_backBuffer);
//
//        if(FAILED(hr))
//            throw HRESULTException("Failed to create back buffer. HRESULT: ", hr);
//    }
//}
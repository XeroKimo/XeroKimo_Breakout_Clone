#pragma once
#include "../Window.h"
#include "../Resources.h"
#include "Handle.h"
#include "Camera.h"
#include "MeshObject.h"
#include <array>
#include <unordered_map>

namespace InsanityEngine::DX11
{
    class Renderer;

    //using CameraHandle = Handle<CameraObject>;
    using StaticMeshHandle = StaticMesh::MeshHandle;


    class Renderer
    {
        static constexpr size_t positionNormalUVLayout = 0;

        template<class T>
        friend struct ManagedHandleDeleter;

    private:
        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        DX11::Device* m_device = nullptr;
        ComPtr<IDXGISwapChain4> m_swapChain;
        ComPtr<ID3D11RenderTargetView1> m_backBuffer;
        ComPtr<ID3D11DepthStencilView> m_depthStencilView;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;

        std::array<ComPtr<ID3D11InputLayout>, 1> m_inputLayouts;

        std::unique_ptr<CameraObject> m_camera;
        std::vector<std::unique_ptr<StaticMesh::MeshObject>> m_meshes;
    public:
        Renderer(DX11::Device& device, ComPtr<IDXGISwapChain4> swapChain);

    public:
        //CameraHandle CreateCamera(CameraData data);
        StaticMeshHandle CreateMesh(DX11::StaticMesh::MeshObjectData data);

    public:
        void Draw();

        CameraData& GetCamera();

    private:
        //void Destroy(CameraObject* object);
        void Destroy(StaticMesh::MeshObject* object);

    private:
        ComPtr<ID3D11RenderTargetView1> InitializeBackBuffer();
        ComPtr<ID3D11DepthStencilView> InitializeDepthStencilView();
        ComPtr<ID3D11DepthStencilState> InitializeDepthStencilState();
    };

    //template<class T>
    //ManagedHandle<T>::~ManagedHandle()
    //{
    //    if(m_renderer != nullptr)
    //    {
    //        m_renderer->Destroy(m_object);
    //    }
    //}
}
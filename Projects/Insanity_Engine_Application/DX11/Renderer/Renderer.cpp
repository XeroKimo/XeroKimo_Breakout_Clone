#include "Renderer.h"
#include "../Device.h"
#include "../Helpers.h"
#include "../CommonInclude.h"
#include "../InputLayouts.h"
#include "../ShaderConstants.h"
#include "Extensions/MatrixExtension.h"
#include <assert.h>
#include <algorithm>

using namespace InsanityEngine;
using namespace InsanityEngine::Math::Types;



static void SetMaterial(const InsanityEngine::DX11::Device& device, const DX11::Resources::StaticMesh::Material& mat);
static void SetMesh(const InsanityEngine::DX11::Device& device, const DX11::Resources::Mesh& mesh);
static void DrawMesh(const InsanityEngine::DX11::Device& device, const DX11::StaticMesh::MeshObject& mesh);

namespace InsanityEngine::DX11
{
    Renderer::Renderer(DX11::Device& device, ComPtr<IDXGISwapChain4> swapChain) :
        m_device(&device),
        m_swapChain(swapChain),
        m_backBuffer(InitializeBackBuffer()),
        m_depthStencilView(InitializeDepthStencilView()),
        m_depthStencilState(InitializeDepthStencilState())
    {
        CameraData data(m_backBuffer, m_depthStencilView, m_depthStencilState);
        data.position.z() = -5;
        StaticMesh::Constants::Camera constants{ .viewProjMatrix = Math::Matrix::ViewProjectionMatrix(data.GetViewMatrix(), data.GetPerspectiveMatrix()) };

        ComPtr<ID3D11Buffer> constantBuffer;
        Helpers::CreateConstantBuffer(m_device->GetDevice(), &constantBuffer, true, constants);
        m_camera = std::make_unique<CameraObject>(constantBuffer, std::move(data));
        m_inputLayouts[positionNormalUVLayout] = DX11::InputLayouts::PositionNormalUV::CreateInputLayout(m_device->GetDevice());
    }

    //CameraHandle Renderer::CreateCamera(CameraData data)
    //{
    //    StaticMesh::Constants::Camera constants{ .viewProjMatrix = Math::Matrix::ViewProjectionMatrix(data.GetViewMatrix(), data.GetPerspectiveMatrix()) };

    //    ComPtr<ID3D11Buffer> constantBuffer;
    //    Helpers::CreateConstantBuffer(m_device->GetDevice(), &constantBuffer, true, constants);
    //    m_cameras.push_back(std::make_unique<CameraObject>(constantBuffer, std::move(data)));
    //    return CameraHandle(*this, *m_cameras.back().get());
    //}

    StaticMeshHandle Renderer::CreateMesh(DX11::StaticMesh::MeshObjectData data)
    {
        StaticMesh::Constants::VSMesh constants{ .worldMatrix = data.GetObjectMatrix() };

        ComPtr<ID3D11Buffer> constantBuffer;
        Helpers::CreateConstantBuffer(m_device->GetDevice(), &constantBuffer, true, constants);

        m_meshes.push_back(std::make_unique<StaticMesh::MeshObject>(constantBuffer, std::move(data)));
        return StaticMeshHandle(*this, *m_meshes.back().get());
    }

    void Renderer::Draw()
    {
        DX11::Helpers::ClearRenderTargetView(m_device->GetDeviceContext(), m_backBuffer.Get(), Vector4f{ 0, 0.3f, 0.7f, 1 });

        //Updates all mesh and materials (might dupe for now but who cares) constant buffers
        for(const auto& mesh : m_meshes)
        {
            //Mesh constants updated
            D3D11_MAPPED_SUBRESOURCE subresource;
            m_device->GetDeviceContext()->Map(mesh->GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

            StaticMesh::Constants::VSMesh constants{ .worldMatrix = mesh->data.GetObjectMatrix() };
            std::memcpy(subresource.pData, &constants, sizeof(constants));
            m_device->GetDeviceContext()->Unmap(mesh->GetConstantBuffer(), 0);

            //Material constants updated
            m_device->GetDeviceContext()->Map(mesh->data.GetMaterial()->GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

            StaticMesh::Constants::PSMaterial psConstants{ .color = mesh->data.GetMaterial()->GetColor() };
            std::memcpy(subresource.pData, &psConstants, sizeof(psConstants));
            m_device->GetDeviceContext()->Unmap(mesh->data.GetMaterial()->GetConstantBuffer(), 0);
        }


        auto& camera = m_camera;
        //for(const auto& camera : m_cameras)
        //{
            //Update camera constant buffers
        D3D11_MAPPED_SUBRESOURCE subresource;
        m_device->GetDeviceContext()->Map(camera->GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

        StaticMesh::Constants::Camera constants{ .viewProjMatrix = Math::Matrix::ViewProjectionMatrix(camera->data.GetViewMatrix(), camera->data.GetPerspectiveMatrix()) };
        std::memcpy(subresource.pData, &constants, sizeof(constants));
        m_device->GetDeviceContext()->Unmap(camera->GetConstantBuffer(), 0);

        //Clear render targets
        if(camera->data.GetRenderTargetView() != m_backBuffer.Get())
            DX11::Helpers::ClearRenderTargetView(m_device->GetDeviceContext(), camera->data.GetRenderTargetView(), Vector4f{ 0, 0.3f, 0.7f, 1 });

        if(camera->data.GetDepthStencilView() != nullptr)
            m_device->GetDeviceContext()->ClearDepthStencilView(camera->data.GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

        //Setting the cameras
        Vector2f resolution = camera->data.GetRenderTargetResolution();

        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(resolution.x());
        viewport.Height = static_cast<float>(resolution.y());
        viewport.MaxDepth = 1;
        viewport.MinDepth = 0;

        D3D11_RECT rect = {};
        rect.right = static_cast<LONG>(resolution.x());
        rect.bottom = static_cast<LONG>(resolution.y());


        std::array renderTargets{ static_cast<ID3D11RenderTargetView*>(camera->data.GetRenderTargetView()) };
        m_device->GetDeviceContext()->OMSetRenderTargets(static_cast<UINT>(renderTargets.size()), renderTargets.data(), camera->data.GetDepthStencilView());
        m_device->GetDeviceContext()->OMSetDepthStencilState(camera->data.GetDepthStencilState(), 0);
        m_device->GetDeviceContext()->RSSetViewports(1, &viewport);
        m_device->GetDeviceContext()->RSSetScissorRects(1, &rect);

        //Mesh rendering set up
        std::array vsCameraBuffer{ camera->GetConstantBuffer() };
        m_device->GetDeviceContext()->VSSetConstantBuffers(StaticMesh::Registers::VS::cameraConstants, static_cast<UINT>(vsCameraBuffer.size()), vsCameraBuffer.data());

        m_device->GetDeviceContext()->IASetInputLayout(m_inputLayouts[positionNormalUVLayout].Get());
        m_device->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        //Draw meshes
        for(const auto& mesh : m_meshes)
        {
            SetMaterial(*m_device, *mesh->data.GetMaterial());
            SetMesh(*m_device, *mesh->data.GetMesh());
            DrawMesh(*m_device, *mesh);
        }
        //}

        m_swapChain->Present(1, 0);
    }

    CameraData& Renderer::GetCamera()
    {
        return m_camera->data;
    }

    //void Renderer::Destroy(CameraObject* object)
    //{
    //    auto it = std::remove_if(m_cameras.begin(), m_cameras.end(), [=](std::unique_ptr<CameraObject>& o) { return o.get() == object; });
    //    if(it != m_cameras.end())
    //        m_cameras.erase(it, m_cameras.end());
    //}

    void Renderer::Destroy(StaticMesh::MeshObject* object)
    {
        auto it = std::remove_if(m_meshes.begin(), m_meshes.end(), [=](std::unique_ptr<StaticMesh::MeshObject>& o) { return o.get() == object; });

        if(it != m_meshes.end())
            m_meshes.erase(it, m_meshes.end());
    }

    ComPtr<ID3D11DepthStencilView> Renderer::InitializeDepthStencilView()
    {
        ComPtr<ID3D11Resource> resource;
        m_backBuffer->GetResource(&resource);

        ComPtr<ID3D11Texture2D> texture;
        resource.As(&texture);

        D3D11_TEXTURE2D_DESC textureDesc = {};
        texture->GetDesc(&textureDesc);

        textureDesc.MipLevels = 0;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ComPtr<ID3D11Texture2D> depthStencilTexture;
        HRESULT hr = m_device->GetDevice()->CreateTexture2D(&textureDesc, nullptr, &depthStencilTexture);

        if(FAILED(hr))
        {
            throw Debug::Exceptions::HRESULTException("Failed to create depth stencil texture", hr);
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDesc;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = 0;
        depthStencilDesc.Texture2D.MipSlice = 0;

        ComPtr<ID3D11DepthStencilView> depthStencilView;
        hr = m_device->GetDevice()->CreateDepthStencilView(depthStencilTexture.Get(), &depthStencilDesc, &depthStencilView);

        if(FAILED(hr))
        {
            throw Debug::Exceptions::HRESULTException("Failed to create depth stencil view", hr);
        }

        return ComPtr<ID3D11DepthStencilView>();
    }

    ComPtr<ID3D11DepthStencilState> Renderer::InitializeDepthStencilState()
    {
        D3D11_DEPTH_STENCIL_DESC desc{};

        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_LESS;
        desc.StencilEnable = false;
        desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
        desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace = desc.FrontFace;

        ComPtr<ID3D11DepthStencilState> depthStencilState;
        m_device->GetDevice()->CreateDepthStencilState(&desc, &depthStencilState);

        return depthStencilState;
    }

    ComPtr<ID3D11RenderTargetView1> Renderer::InitializeBackBuffer()
    {
        ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

        if(FAILED(hr))
            throw Debug::Exceptions::HRESULTException("Failed to get back buffer. HRESULT: ", hr);

        D3D11_RENDER_TARGET_VIEW_DESC1 rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
        rtvDesc.Texture2D.PlaneSlice = 0;

        ComPtr<ID3D11RenderTargetView1> renderTarget;
        hr = m_device->GetDevice()->CreateRenderTargetView1(backBuffer.Get(), &rtvDesc, &renderTarget);

        if(FAILED(hr))
            throw Debug::Exceptions::HRESULTException("Failed to create back buffer. HRESULT: ", hr);

        return renderTarget;
    }

}


void SetMaterial(const InsanityEngine::DX11::Device& device, const DX11::Resources::StaticMesh::Material& mat)
{
    device.GetDeviceContext()->VSSetShader(mat.GetShader()->GetVertexShader(), nullptr, 0);
    device.GetDeviceContext()->PSSetShader(mat.GetShader()->GetPixelShader(), nullptr, 0);

    std::array samplers{ mat.GetAlbedo()->GetSamplerState() };
    std::array textures{ mat.GetAlbedo()->GetView() };
    std::array constantBuffers{ mat.GetConstantBuffer() };
    device.GetDeviceContext()->PSSetSamplers(DX11::StaticMesh::Registers::PS::albedoSampler, 1, samplers.data());
    device.GetDeviceContext()->PSSetShaderResources(DX11::StaticMesh::Registers::PS::albedoTexture, 1, textures.data());
    device.GetDeviceContext()->PSSetConstantBuffers(DX11::StaticMesh::Registers::PS::materialConstants, static_cast<UINT>(constantBuffers.size()), constantBuffers.data());
}

void SetMesh(const InsanityEngine::DX11::Device& device, const DX11::Resources::Mesh& mesh)
{
    std::array vertexBuffers{ mesh.GetVertexBuffer() };

    UINT stride = sizeof(DX11::StaticMesh::VertexData);
    UINT offset = 0;

    device.GetDeviceContext()->IASetVertexBuffers(0, static_cast<UINT>(vertexBuffers.size()), vertexBuffers.data(), &stride, &offset);
    device.GetDeviceContext()->IASetIndexBuffer(mesh.GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
}

void DrawMesh(const InsanityEngine::DX11::Device& device, const DX11::StaticMesh::MeshObject& mesh)
{
    std::array constantBuffers{ mesh.GetConstantBuffer() };
    device.GetDeviceContext()->VSSetConstantBuffers(DX11::StaticMesh::Registers::VS::objectConstants, static_cast<UINT>(constantBuffers.size()), constantBuffers.data());
    device.GetDeviceContext()->DrawIndexed(mesh.data.GetMesh()->GetIndexCount(), 0, 0);
}
//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

#include <DirectXMath.h>
#include <BufferHelpers.h>
#include <VertexTypes.h>
#include <Effects.h>
#include <DirectXHelpers.h>

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    static const VertexPositionColor s_vertexData[3] =
    {
        { XMFLOAT3{ 0.0f,   0.5f,  0.5f }, XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f } },  // Top / Red
        { XMFLOAT3{ 0.5f,  -0.5f,  0.5f }, XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f } },  // Right / Green
        { XMFLOAT3{ -0.5f, -0.5f,  0.5f }, XMFLOAT4{ 0.0f, 0.0f, 1.0f, 1.0f } }   // Left / Blue
    };

    ComPtr<ID3D11Buffer> g_VertexBuffer;
    ComPtr<ID3D11Buffer> g_IndexBuffer;
    std::unique_ptr<BasicEffect> g_BasicEffect;
    ComPtr<ID3D11InputLayout> g_InputLayout;
    XMFLOAT4X4 g_World;
    XMFLOAT4X4 g_View;
    XMFLOAT4X4 g_Projection;
}

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    // TODO: Provide parameters for swapchain format, depth/stencil format, and backbuffer count.
    //   Add DX::DeviceResources::c_AllowTearing to opt-in to variable rate displays.
    //   Add DX::DeviceResources::c_EnableHDR for HDR10 display.
    m_deviceResources->RegisterDeviceNotify(this);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */

    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    // TMP Start
    {
        DX::ThrowIfFailed(CreateStaticBuffer(m_deviceResources->GetD3DDevice(), s_vertexData, 3, D3D11_BIND_VERTEX_BUFFER, &g_VertexBuffer));
        const unsigned int indices[] = {
            0, 1, 2,
            0, 2, 3,
        };
        DX::ThrowIfFailed(CreateStaticBuffer(m_deviceResources->GetD3DDevice(), indices, 6, D3D11_BIND_INDEX_BUFFER, &g_IndexBuffer));

         g_BasicEffect = std::make_unique<BasicEffect>(m_deviceResources->GetD3DDevice());

        DX::ThrowIfFailed(CreateInputLayoutFromEffect<VertexPositionColor>(
            m_deviceResources->GetD3DDevice(), g_BasicEffect.get(), &g_InputLayout));

        XMStoreFloat4x4(&g_World, XMMatrixIdentity());
        XMStoreFloat4x4(&g_Projection, XMMatrixIdentity());
        XMStoreFloat4x4(&g_View, XMMatrixIdentity());
    }
   // TMP End
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
    elapsedTime;

    const auto state = m_keyboard->GetState();
    if (state.Escape)
    {
        ExitGame();
    }

    const XMFLOAT3 eyePosition_(0.0f, 5.0f, 10.0f);
    const XMVECTOR eyePosition = XMLoadFloat3(&eyePosition_);
    const XMFLOAT3 focusPosition_(0.0f, 0.0f, 0.0f);
    const XMVECTOR focusPosition = XMLoadFloat3(&focusPosition_);
    const XMFLOAT3 upDirection_(0.0f, 1.0f, 0.0f);
    const XMVECTOR upDirection = XMLoadFloat3(&upDirection_);
    const XMMATRIX view = XMMatrixLookAtLH(eyePosition, focusPosition, upDirection);
    const RECT outSize = m_deviceResources->GetOutputSize();
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), float(outSize.right) / float(outSize.bottom), 0.1f, 100.0f);

    const XMMATRIX world = XMLoadFloat4x4(&g_World);
    g_BasicEffect->SetMatrices(world, view, proj);
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();

    // TODO: Add your rendering code here.
    g_BasicEffect->Apply(m_deviceResources->GetD3DDeviceContext());
    unsigned int strides = sizeof(VertexPositionColor);
    unsigned int offsets = 0;
    context->IASetVertexBuffers(0, 1, &g_VertexBuffer, &strides, &offsets);
    context->IASetIndexBuffer(g_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(g_InputLayout.Get());
    context->DrawIndexed(6, 0, 0);

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto const viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 800;
    height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // TODO: Initialize device dependent objects here (independent of window size).
    device;
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

#include "epe_render_d3d12.hpp"

#include <Windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <stdexcept>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;

namespace epe {
namespace render {

namespace {

struct Impl {
  ComPtr<ID3D12Device> device;
  ComPtr<ID3D12CommandQueue> queue;
  ComPtr<IDXGISwapChain3> swapchain;
  ComPtr<ID3D12DescriptorHeap> rtv_heap;
  ComPtr<ID3D12Resource> targets[2];
  ComPtr<ID3D12CommandAllocator> allocator;
  ComPtr<ID3D12GraphicsCommandList> list;
  ComPtr<ID3D12Fence> fence;
  HANDLE fence_event = nullptr;
  UINT64 fence_value = 0;
  UINT frame_index = 0;
  UINT rtv_stride = 0;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
};

HRESULT WaitGpu(Impl* i) {
  const UINT64 v = ++i->fence_value;
  HRESULT hr = i->queue->Signal(i->fence.Get(), v);
  if (FAILED(hr)) return hr;
  if (i->fence->GetCompletedValue() < v) {
    hr = i->fence->SetEventOnCompletion(v, i->fence_event);
    if (FAILED(hr)) return hr;
    WaitForSingleObject(i->fence_event, INFINITE);
  }
  return S_OK;
}

}  // namespace

D3D12Surface::~D3D12Surface() { Shutdown(); }

Status D3D12Surface::Init(HWND hwnd, std::uint32_t width, std::uint32_t height) {
  Shutdown();
  if (!hwnd || width == 0 || height == 0) return Status::InvalidArg;

  auto* i = new Impl();
  HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&i->device));
  if (FAILED(hr)) {
    delete i;
    return Status::FailClosed;
  }

  D3D12_COMMAND_QUEUE_DESC qd{};
  qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  hr = i->device->CreateCommandQueue(&qd, IID_PPV_ARGS(&i->queue));
  if (FAILED(hr)) {
    delete i;
    return Status::FailClosed;
  }

  ComPtr<IDXGIFactory4> factory;
  hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
  if (FAILED(hr)) {
    delete i;
    return Status::FailClosed;
  }

  DXGI_SWAP_CHAIN_DESC1 scd{};
  scd.BufferCount = 2;
  scd.Width = width;
  scd.Height = height;
  scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  scd.SampleDesc.Count = 1;

  ComPtr<IDXGISwapChain1> sc1;
  hr = factory->CreateSwapChainForHwnd(i->queue.Get(), hwnd, &scd, nullptr, nullptr, &sc1);
  if (FAILED(hr)) {
    delete i;
    return Status::FailClosed;
  }
  hr = sc1.As(&i->swapchain);
  if (FAILED(hr)) {
    delete i;
    return Status::FailClosed;
  }

  D3D12_DESCRIPTOR_HEAP_DESC hd{};
  hd.NumDescriptors = 2;
  hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  hr = i->device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&i->rtv_heap));
  if (FAILED(hr)) {
    delete i;
    return Status::FailClosed;
  }
  i->rtv_stride = i->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  D3D12_CPU_DESCRIPTOR_HANDLE rtv = i->rtv_heap->GetCPUDescriptorHandleForHeapStart();
  for (UINT n = 0; n < 2; ++n) {
    hr = i->swapchain->GetBuffer(n, IID_PPV_ARGS(&i->targets[n]));
    if (FAILED(hr)) {
      delete i;
      return Status::FailClosed;
    }
    i->device->CreateRenderTargetView(i->targets[n].Get(), nullptr, rtv);
    rtv.ptr += i->rtv_stride;
  }

  hr = i->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&i->allocator));
  if (FAILED(hr)) {
    delete i;
    return Status::FailClosed;
  }
  hr = i->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, i->allocator.Get(), nullptr,
                                   IID_PPV_ARGS(&i->list));
  if (FAILED(hr)) {
    delete i;
    return Status::FailClosed;
  }
  i->list->Close();

  hr = i->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&i->fence));
  if (FAILED(hr)) {
    delete i;
    return Status::FailClosed;
  }
  i->fence_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (!i->fence_event) {
    delete i;
    return Status::FailClosed;
  }

  i->width = width;
  i->height = height;
  i->frame_index = i->swapchain->GetCurrentBackBufferIndex();
  impl_ = i;
  ready_ = true;
  return Status::Ok;
}

Status D3D12Surface::Resize(std::uint32_t width, std::uint32_t height) {
  if (!ready_) return Status::NotReady;
  auto* i = static_cast<Impl*>(impl_);
  WaitGpu(i);
  for (auto& t : i->targets) t.Reset();
  HRESULT hr = i->swapchain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
  if (FAILED(hr)) return Status::DeviceLost;
  D3D12_CPU_DESCRIPTOR_HANDLE rtv = i->rtv_heap->GetCPUDescriptorHandleForHeapStart();
  for (UINT n = 0; n < 2; ++n) {
    hr = i->swapchain->GetBuffer(n, IID_PPV_ARGS(&i->targets[n]));
    if (FAILED(hr)) return Status::DeviceLost;
    i->device->CreateRenderTargetView(i->targets[n].Get(), nullptr, rtv);
    rtv.ptr += i->rtv_stride;
  }
  i->frame_index = i->swapchain->GetCurrentBackBufferIndex();
  i->width = width;
  i->height = height;
  return Status::Ok;
}

Status D3D12Surface::ClearAndPresent(const float rgba[4], bool present) {
  if (!ready_) return Status::NotReady;
  auto* i = static_cast<Impl*>(impl_);
  HRESULT hr = i->allocator->Reset();
  if (FAILED(hr)) return Status::DeviceLost;
  hr = i->list->Reset(i->allocator.Get(), nullptr);
  if (FAILED(hr)) return Status::DeviceLost;

  D3D12_RESOURCE_BARRIER bar{};
  bar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  bar.Transition.pResource = i->targets[i->frame_index].Get();
  bar.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  bar.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  bar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  i->list->ResourceBarrier(1, &bar);

  D3D12_CPU_DESCRIPTOR_HANDLE rtv = i->rtv_heap->GetCPUDescriptorHandleForHeapStart();
  rtv.ptr += static_cast<SIZE_T>(i->frame_index) * i->rtv_stride;
  i->list->ClearRenderTargetView(rtv, rgba, 0, nullptr);

  bar.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  bar.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  i->list->ResourceBarrier(1, &bar);
  hr = i->list->Close();
  if (FAILED(hr)) return Status::DeviceLost;

  ID3D12CommandList* lists[] = {i->list.Get()};
  i->queue->ExecuteCommandLists(1, lists);
  if (present) {
    hr = i->swapchain->Present(1, 0);
    if (FAILED(hr)) return Status::DeviceLost;
  }
  WaitGpu(i);
  i->frame_index = i->swapchain->GetCurrentBackBufferIndex();
  return present ? Status::Ok : Status::OmittedPresent;
}

Status D3D12Surface::Recover() {
  // Alfa: re-Init externo con mismo HWND.
  return ready_ ? Status::Ok : Status::NotReady;
}

void D3D12Surface::Shutdown() {
  if (!impl_) {
    ready_ = false;
    return;
  }
  auto* i = static_cast<Impl*>(impl_);
  if (i->queue && i->fence) WaitGpu(i);
  if (i->fence_event) CloseHandle(i->fence_event);
  delete i;
  impl_ = nullptr;
  ready_ = false;
}

}  // namespace render
}  // namespace epe

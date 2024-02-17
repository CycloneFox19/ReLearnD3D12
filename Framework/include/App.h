#pragma once

//--------------------------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------------------------
#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>


//--------------------------------------------------------------------------------------------------------
// Linker
//--------------------------------------------------------------------------------------------------------
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")


//--------------------------------------------------------------------------------------------------------
// Type Alias
//--------------------------------------------------------------------------------------------------------
template<typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transform structure
//////////////////////////////////////////////////////////////////////////////////////////////////////////
struct alignas(256) Transform
{
	DirectX::XMMATRIX World; // world matrix
	DirectX::XMMATRIX View; // view matrix
	DirectX::XMMATRIX Proj; // projection matrix
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// ConstantBufferView structure
//////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T> struct ConstantBufferView
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC Desc; // configuration of constant buffer
	D3D12_CPU_DESCRIPTOR_HANDLE HandleCPU; // CPU descriptor handle
	D3D12_GPU_DESCRIPTOR_HANDLE HandleGPU; // GPU descriptor handle
	T* pBuffer; // pointer to the start of buffer
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// App class
//////////////////////////////////////////////////////////////////////////////////////////////////////////
class App
{
	//====================================================================================================
	// List of friend classes and methods
	//====================================================================================================
	/* NOTHING */

public:
	//====================================================================================================
	// Public variables
	//====================================================================================================
	/* NOTHING */

	//====================================================================================================
	// Public methods
	//====================================================================================================
	App(uint32_t width, uint32_t height);
	virtual ~App();
	void Run();

private:
	//====================================================================================================
	// Private variables
	//====================================================================================================
	static const uint32_t FrameCount = 2; // number of frame buffer

	HINSTANCE m_hInst; // Instance handle
	HWND m_hWnd; // Window handle
	uint32_t m_Width; // Width of the window
	uint32_t m_Height; // Height of the window

	ComPtr<ID3D12Device8> m_pDevice; // device
	ComPtr<ID3D12CommandQueue> m_pQueue; // command queue
	ComPtr<IDXGISwapChain3> m_pSwapChain; // swap chain
	ComPtr<ID3D12Resource> m_pColorBuffer[FrameCount]; // color buffer
	ComPtr<ID3D12CommandAllocator> m_pCmdAllocator[FrameCount]; // command allocator
	ComPtr<ID3D12GraphicsCommandList6> m_pCmdList; // command list
	ComPtr<ID3D12DescriptorHeap> m_pHeapRTV; // descriptor heap for render target view
	ComPtr<ID3D12Fence> m_pFence; // fence
	ComPtr<ID3D12DescriptorHeap> m_pHeapRes; // descriptor heap for constant buffer view
	ComPtr<ID3D12Resource> m_pVB; // vertex buffer
	ComPtr<ID3D12Resource> m_pIB; // index buffer
	ComPtr<ID3D12Resource> m_pCB[FrameCount]; // constant buffer
	ComPtr<ID3D12RootSignature> m_pRootSignature; // root signature
	ComPtr<ID3D12PipelineState> m_pPSO; // pipeline state object

	HANDLE m_FenceEvent; // fence event
	uint64_t m_FenceCounter[FrameCount]; // fence counter
	uint32_t m_FrameIndex; // index of frame
	D3D12_CPU_DESCRIPTOR_HANDLE m_HandleRTV[FrameCount]; // CPU descriptor for render target view
	D3D12_VIEWPORT m_Viewport; // viewport
	D3D12_RECT m_Scissor; // scissor rectangle
	ConstantBufferView<Transform> m_CBV[FrameCount]; // constant buffer view
	float m_RotateAngle; // angle of rotation

	//====================================================================================================
	// Private methods
	//====================================================================================================
	bool InitApp();
	void TermApp();
	bool InitWnd();
	void TermWnd();
	void MainLoop();
	bool InitD3D();
	void TermD3D();
	void Render();
	void WaitGPU();
	void Present(uint32_t interval);
	bool OnInit();
	void OnTerm();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
};

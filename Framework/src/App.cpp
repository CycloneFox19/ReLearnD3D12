//--------------------------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------------------------
#include <App.h>
#include "ResourceUploadBatch.h"
#include "DDSTextureLoader.h"
#include "VertexTypes.h"
#include "FileUtil.h"
#include <cassert>


namespace /* anonymous */ {

	//----------------------------------------------------------------------------------------------------
	// Constant Values
	//----------------------------------------------------------------------------------------------------
	const auto ClassName = TEXT("SampleWindowClass");

} // namespace /* anonymous */


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// App class
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------------
//	 constructor
//--------------------------------------------------------------------------------------------------------
App::App(uint32_t width, uint32_t height)
	: m_hInst(nullptr)
	, m_hWnd(nullptr)
	, m_Width(width)
	, m_Height(height)
	, m_FrameIndex(0)
	, m_RotateAngle(0.f)
{
	for (uint32_t i = 0u; i < FrameCount; ++i)
	{
		m_pColorBuffer[i] = nullptr;
		m_pCmdAllocator[i] = nullptr;
		m_FenceCounter[i] = 0;
	}
}

//--------------------------------------------------------------------------------------------------------
//	 destructor
//--------------------------------------------------------------------------------------------------------
App::~App()
{
	/* DO_NOTHING */
}

//--------------------------------------------------------------------------------------------------------
//	 run
//--------------------------------------------------------------------------------------------------------
void App::Run()
{
	if (InitApp())
	{
		MainLoop();
	}

	TermApp();
}

//--------------------------------------------------------------------------------------------------------
//	 initialization
//--------------------------------------------------------------------------------------------------------
bool App::InitApp()
{
	// initialize window
	if (!InitWnd())
	{
		return false;
	}

	// initialize Direct3D 12
	if (!InitD3D())
	{
		return false;
	}

	// other processing on initialization
	if (!OnInit())
	{
		return false;
	}

	// finish normally
	return true;
}

//--------------------------------------------------------------------------------------------------------
//	 terminate application
//--------------------------------------------------------------------------------------------------------
void App::TermApp()
{
	// end processing of application-specific
	OnTerm();

	// end processing of Direct3D 12
	TermD3D();

	// terminate window
	TermWnd();
}

//--------------------------------------------------------------------------------------------------------
//	 initialization of window
//--------------------------------------------------------------------------------------------------------
bool App::InitWnd()
{
	// get instance handle
	HINSTANCE hInst = GetModuleHandle(nullptr);
	if (hInst == nullptr)
	{
		return false;
	}

	// settings of window
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hIcon = LoadIcon(hInst, IDI_APPLICATION);
	wc.hCursor = LoadCursor(hInst, IDC_ARROW);
	wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
	wc.lpszMenuName	 = nullptr;
	wc.lpszClassName = ClassName;
	wc.hIconSm = LoadIcon(hInst, IDI_APPLICATION);

	// register window
	if (!RegisterClassEx(&wc))
	{
		return false;
	}

	// set intance handle
	m_hInst = hInst;

	// set the size of the window
	RECT rc = {};
	rc.right = static_cast<LONG>(m_Width);
	rc.bottom = static_cast<LONG>(m_Height);

	// adjust window size
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rc, style, WS_SYSMENU);

	// generate window
	m_hWnd = CreateWindowEx(
		0,
		ClassName,
		TEXT("Sample"),
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		m_hInst,
		nullptr);

	if (m_hWnd == nullptr)
	{
		return false;
	}

	// show window
	ShowWindow(m_hWnd, SW_SHOWNORMAL);

	// update window
	UpdateWindow(m_hWnd);

	// set the focus on the window
	SetFocus(m_hWnd);

	// finish normally
	return true;
}

//--------------------------------------------------------------------------------------------------------
//	 terminate window
//--------------------------------------------------------------------------------------------------------
void App::TermWnd()
{
	// unregister the window
	if (m_hInst != nullptr)
	{
		UnregisterClass(ClassName, m_hInst);
	}

	m_hInst = nullptr;
	m_hWnd = nullptr;
}

//--------------------------------------------------------------------------------------------------------
//	 main loop
//--------------------------------------------------------------------------------------------------------
void App::MainLoop()
{
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) == TRUE)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}
}

//--------------------------------------------------------------------------------------------------------
//	 initialization of Direct3D
//--------------------------------------------------------------------------------------------------------
bool App::InitD3D()
{
#if defined(DEBUG) || defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debug;
		auto hr = D3D12GetDebugInterface(IID_PPV_ARGS(debug.GetAddressOf()));

		// enable debug layer
		if (SUCCEEDED(hr))
		{
			debug->EnableDebugLayer();
		}
	}
#endif

	// generate device
	HRESULT hr = D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(m_pDevice.GetAddressOf()));
	if (FAILED(hr))
	{
		return false;
	}

	// generate command queue
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		hr = m_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(m_pQueue.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}
	}

	// generate swap chain
	{
		// generate DXGI factory
		ComPtr<IDXGIFactory4> pFactory = nullptr;
		hr = CreateDXGIFactory1(IID_PPV_ARGS(pFactory.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}

		// settings of swap chain
		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferDesc.Width = m_Width;
		desc.BufferDesc.Height = m_Height;
		desc.BufferDesc.RefreshRate.Numerator = 60;
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = FrameCount;
		desc.OutputWindow = m_hWnd;
		desc.Windowed = TRUE;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		// generate swap chain
		ComPtr<IDXGISwapChain> pSwapChain = nullptr;
		hr = pFactory->CreateSwapChain(m_pQueue.Get(), &desc, pSwapChain.GetAddressOf());
		if (FAILED(hr))
		{
			return false;
		}

		// get IDXGISwapChain3
		hr = pSwapChain.As(&m_pSwapChain);
		if (FAILED(hr))
		{
			return false;
		}

		// get index of back buffer
		m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

		// release unnecessary things
		pFactory.Reset();
		pSwapChain.Reset();
	}

	// generate command allocator
	{
		for (uint32_t i = 0u; i < FrameCount; ++i)
		{
			hr = m_pDevice->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(m_pCmdAllocator[i].GetAddressOf()));
			if (FAILED(hr))
			{
				return false;
			}
		}
	}

	// generate command list
	{
		hr = m_pDevice->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_pCmdAllocator[m_FrameIndex].Get(),
			nullptr,
			IID_PPV_ARGS(m_pCmdList.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}
	}

	// generate render target view
	{
		// settings of descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = FrameCount;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;

		// generate descriptor heap
		hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pHeapRTV.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pHeapRTV->GetCPUDescriptorHandleForHeapStart();
		uint32_t incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (uint32_t i = 0u; i < FrameCount; ++i)
		{
			hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pColorBuffer[i].GetAddressOf()));
			if (FAILED(hr))
			{
				return false;
			}

			D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
			viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MipSlice = 0;
			viewDesc.Texture2D.PlaneSlice = 0;

			// generate render target view
			m_pDevice->CreateRenderTargetView(m_pColorBuffer[i].Get(), &viewDesc, handle);

			m_HandleRTV[i] = handle;
			handle.ptr += incrementSize;
		}
	}

	// generate depth stencil buffer
	{
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_DEFAULT;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resDesc.Alignment = 0;
		resDesc.Width = m_Width;
		resDesc.Height = m_Height;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_D32_FLOAT;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.f;
		clearValue.DepthStencil.Stencil = 0;

		hr = m_pDevice->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(m_pDepthBuffer.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}

		// configuration of descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;

		hr = m_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_pHeapDSV.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pHeapDSV->GetCPUDescriptorHandleForHeapStart();
		UINT incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
		viewDesc.Format = DXGI_FORMAT_D32_FLOAT;
		viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipSlice = 0;
		viewDesc.Flags = D3D12_DSV_FLAG_NONE;

		m_pDevice->CreateDepthStencilView(m_pDepthBuffer.Get(), &viewDesc, handle);

		m_HandleDSV = handle;
	}

	// generate fence
	{
		// reset fence counter
		for (uint32_t i = 0u; i < FrameCount; ++i)
		{
			m_FenceCounter[i] = 0;
		}

		// generate fence
		hr = m_pDevice->CreateFence(
			m_FenceCounter[m_FrameIndex],
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(m_pFence.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}

		m_FenceCounter[m_FrameIndex]++;

		// generate event
		m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_FenceEvent == nullptr)
		{
			return false;
		}
	}

	// close command list
	m_pCmdList->Close();

	return true;
}

//--------------------------------------------------------------------------------------------------------
//	 termination for Direct3D
//--------------------------------------------------------------------------------------------------------
void App::TermD3D()
{
	// wait for completion of GPU processing
	WaitGPU();

	// abandon event
	if (m_FenceEvent != nullptr)
	{
		CloseHandle(m_FenceEvent);
		m_FenceEvent = nullptr;
	}

	// abandon fence
	m_pFence.Reset();

	// abandon render target
	m_pHeapRTV.Reset();
	for (uint32_t i = 0u; i < FrameCount; ++i)
	{
		m_pColorBuffer[i].Reset();
	}

	// abandon depth stencil view
	m_pHeapDSV.Reset();
	m_pDepthBuffer.Reset();

	// abandon command list
	m_pCmdList.Reset();

	for (uint32_t i = 0u; i < FrameCount; ++i)
	{
		m_pCmdAllocator[i].Reset();
	}

	// abandon swap chain
	m_pSwapChain.Reset();

	// abandon command queue
	m_pQueue.Reset();

	// abandon device
	m_pDevice.Reset();
}

//--------------------------------------------------------------------------------------------------------
//	 rendering
//--------------------------------------------------------------------------------------------------------
void App::Render()
{
	// update parameters
	{
		m_RotateAngle += 0.025f;
		m_CBV[m_FrameIndex].pBuffer->World = DirectX::XMMatrixRotationY(m_RotateAngle);
	}

	// initiate recording command
	m_pCmdAllocator[m_FrameIndex]->Reset();
	m_pCmdList->Reset(m_pCmdAllocator[m_FrameIndex].Get(), nullptr);

	// settings of resource barrier
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_pColorBuffer[m_FrameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// resource barrier
	m_pCmdList->ResourceBarrier(1, &barrier);

	// set render target
	m_pCmdList->OMSetRenderTargets(1, &m_HandleRTV[m_FrameIndex], FALSE, &m_HandleDSV);

	// set clear color
	float clearColor[] = { 0.25f, 0.25f, 0.25f, 1.0f };

	// clear render target view
	m_pCmdList->ClearRenderTargetView(m_HandleRTV[m_FrameIndex], clearColor, 0, nullptr);

	// clear depth stencil view
	m_pCmdList->ClearDepthStencilView(m_HandleDSV, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	// rendering
	{
		m_pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
		m_pCmdList->SetDescriptorHeaps(1, m_pHeapCBV.GetAddressOf());
		m_pCmdList->SetGraphicsRootConstantBufferView(0, m_CBV[m_FrameIndex].Desc.BufferLocation);
		m_pCmdList->SetGraphicsRootDescriptorTable(1, m_Texture.HandleGPU);
		m_pCmdList->SetPipelineState(m_pPSO.Get());

		m_pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pCmdList->IASetVertexBuffers(0, 1, &m_VBV);
		m_pCmdList->IASetIndexBuffer(&m_IBV);
		m_pCmdList->RSSetViewports(1, &m_Viewport);
		m_pCmdList->RSSetScissorRects(1, &m_Scissor);

		m_pCmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}

	// settings of resource barrier
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_pColorBuffer[m_FrameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	// resource barrier
	m_pCmdList->ResourceBarrier(1, &barrier);

	// finish recording command
	m_pCmdList->Close();

	// execute command
	ID3D12CommandList* ppCmdLists[] = { m_pCmdList.Get()};
	m_pQueue->ExecuteCommandLists(1, ppCmdLists);

	// show on screen
	Present(1);
}

//--------------------------------------------------------------------------------------------------------
//	 wait for GPU to complete processing
//--------------------------------------------------------------------------------------------------------
void App::WaitGPU()
{
	assert(m_pQueue != nullptr);
	assert(m_pFence != nullptr);
	assert(m_FenceEvent != nullptr);

	// signal
	m_pQueue->Signal(m_pFence.Get(), m_FenceCounter[m_FrameIndex]);

	// set event on completion of GPU processing
	m_pFence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);

	// wait for signal
	WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);

	// increment counter
	m_FenceCounter[m_FrameIndex]++;
}

//--------------------------------------------------------------------------------------------------------
//	 show render target on screen and prepare for next frame
//--------------------------------------------------------------------------------------------------------
void App::Present(uint32_t interval)
{
	// show on screen
	m_pSwapChain->Present(interval, 0);

	// signal
	const uint64_t currentValue = m_FenceCounter[m_FrameIndex];
	m_pQueue->Signal(m_pFence.Get(), currentValue);

	// update back buffer index
	m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// wait if preparation for next frame hasn't been done
	if (m_pFence->GetCompletedValue() < m_FenceCounter[m_FrameIndex])
	{
		m_pFence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);
		WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
	}

	// increment fence counter of next frame
	m_FenceCounter[m_FrameIndex] = currentValue + 1;
}

//--------------------------------------------------------------------------------------------------------
// processing on initialization
//--------------------------------------------------------------------------------------------------------
bool App::OnInit()
{
	// generate vertex buffer
	{
		// vertex data
		DirectX::VertexPositionTexture vertices[] = {
			DirectX::VertexPositionTexture(DirectX::XMFLOAT3(-1.f,  1.f, 0.f), DirectX::XMFLOAT2(0.f, 0.f)),
			DirectX::VertexPositionTexture(DirectX::XMFLOAT3( 1.f,  1.f, 0.f), DirectX::XMFLOAT2(1.f, 0.f)),
			DirectX::VertexPositionTexture(DirectX::XMFLOAT3( 1.f, -1.f, 0.f), DirectX::XMFLOAT2(1.f, 1.f)),
			DirectX::VertexPositionTexture(DirectX::XMFLOAT3(-1.f, -1.f, 0.f), DirectX::XMFLOAT2(0.f, 1.f))
		};

		// heap property
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		// configuration of resource
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = sizeof(vertices);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// generate resource
		HRESULT hr = m_pDevice->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_pVB.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}

		// memory mapping
		void* ptr = nullptr;
		hr = m_pVB->Map(0, nullptr, &ptr); // translate physical address to logical address
		if (FAILED(hr))
		{
			return false;
		}

		// set vertex data to mapping destination
		memcpy(ptr, vertices, sizeof(vertices));

		// unmap memory
		m_pVB->Unmap(0, nullptr);

		// configuration of vertex buffer view
		m_VBV.BufferLocation = m_pVB->GetGPUVirtualAddress();
		m_VBV.SizeInBytes = static_cast<UINT>(sizeof(vertices));
		m_VBV.StrideInBytes = static_cast<UINT>(sizeof(DirectX::VertexPositionTexture));
	}

	// generate index buffer
	{
		uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };

		// heap property
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		// settings of resource
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = sizeof(indices);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// generate resurce
		HRESULT hr = m_pDevice->CreateCommittedResource(
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_pIB.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}

		// mapping
		void* ptr = nullptr;
		hr = m_pIB->Map(0, nullptr, &ptr);
		if (FAILED(hr))
		{
			return false;
		}

		// set index data to mapping destination
		memcpy(ptr, indices, sizeof(indices));

		// unmap memory
		m_pIB->Unmap(0, nullptr);

		// settings of index buffer view
		m_IBV.BufferLocation = m_pIB->GetGPUVirtualAddress();
		m_IBV.Format = DXGI_FORMAT_R32_UINT;
		m_IBV.SizeInBytes = sizeof(indices);
	}

	// generate descriptor heap for CBV/SRV/UAV
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = FrameCount + 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NodeMask = 0;

		HRESULT hr = m_pDevice->CreateDescriptorHeap(
			&desc,
			IID_PPV_ARGS(m_pHeapCBV.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}
	}

	// generate constant buffer
	{
		// heap properties
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		// configuration of resource
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = sizeof(Transform);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		size_t incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		for (uint32_t i = 0; i < FrameCount; ++i)
		{
			// generate resource
			HRESULT hr = m_pDevice->CreateCommittedResource(
				&prop,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(m_pCB[i].GetAddressOf()));
			if (FAILED(hr))
			{
				return false;
			}

			D3D12_GPU_VIRTUAL_ADDRESS address = m_pCB[i]->GetGPUVirtualAddress();
			D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = m_pHeapCBV->GetCPUDescriptorHandleForHeapStart();
			D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = m_pHeapCBV->GetGPUDescriptorHandleForHeapStart();

			handleCPU.ptr += incrementSize * i;
			handleGPU.ptr += incrementSize * i;

			// settings of constant buffer view
			m_CBV[i].HandleCPU = handleCPU;
			m_CBV[i].HandleGPU = handleGPU;
			m_CBV[i].Desc.BufferLocation = address;
			m_CBV[i].Desc.SizeInBytes = sizeof(Transform);

			// create constant buffer view
			m_pDevice->CreateConstantBufferView(&m_CBV[i].Desc, handleCPU);

			// mapping
			hr = m_pCB[i]->Map(0, nullptr, reinterpret_cast<void**>(&m_CBV[i].pBuffer));
			if (FAILED(hr))
			{
				return false;
			}

			DirectX::XMVECTOR eyePos = DirectX::XMVectorSet(0.f, 0.f, 5.f, 0.f);
			DirectX::XMVECTOR targetPos = DirectX::XMVectorZero();
			DirectX::XMVECTOR upward = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);

			float fovY = DirectX::XMConvertToRadians(37.5f);
			float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);

			// settings of transformation matrix
			m_CBV[i].pBuffer->World = DirectX::XMMatrixIdentity();
			m_CBV[i].pBuffer->View = DirectX::XMMatrixLookAtRH(eyePos, targetPos, upward);
			m_CBV[i].pBuffer->Proj = DirectX::XMMatrixPerspectiveFovRH(fovY, aspect, 1.f, 1000.f); // why right handed?
		}
	}

	// generate root signature
	{
		D3D12_ROOT_SIGNATURE_FLAGS flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
		flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
		flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		// configuration of root parameter
		D3D12_ROOT_PARAMETER param[2] = {};
		param[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		param[0].Descriptor.ShaderRegister = 0;
		param[0].Descriptor.RegisterSpace = 0;
		param[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		D3D12_DESCRIPTOR_RANGE range = {};
		range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		range.NumDescriptors = 1;
		range.BaseShaderRegister = 0;
		range.RegisterSpace = 0;
		range.OffsetInDescriptorsFromTableStart = 0;

		param[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param[1].DescriptorTable.NumDescriptorRanges = 1;
		param[1].DescriptorTable.pDescriptorRanges = &range;
		param[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// configuration of static sampler
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.MipLODBias = D3D12_DEFAULT_MIP_LOD_BIAS;
		sampler.MaxAnisotropy = 1;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = -D3D12_FLOAT32_MAX;
		sampler.MaxLOD = +D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		// configuration of root signature
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = 2;
		desc.NumStaticSamplers = 1;
		desc.pParameters = param;
		desc.pStaticSamplers = &sampler;
		desc.Flags = flag;

		ComPtr<ID3DBlob> pBlob;
		ComPtr<ID3DBlob> pErrorBlob;

		// serialize
		HRESULT hr = D3D12SerializeRootSignature(
			&desc,
			D3D_ROOT_SIGNATURE_VERSION_1_0,
			pBlob.GetAddressOf(),
			pErrorBlob.GetAddressOf());
		if (FAILED(hr))
		{
			return false;
		}

		// generate root signature
		hr = m_pDevice->CreateRootSignature(
			0,
			pBlob->GetBufferPointer(),
			pBlob->GetBufferSize(),
			IID_PPV_ARGS(m_pRootSignature.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}
	}

	// generate pipeline state
	{
		// configuration of input layout
		D3D12_INPUT_ELEMENT_DESC elements[2];
		elements[0].SemanticName = "POSITION";
		elements[0].SemanticIndex = 0;
		elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		elements[0].InputSlot = 0;
		elements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		elements[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		elements[0].InstanceDataStepRate = 0;

		elements[1].SemanticName = "TEXCOORD";
		elements[1].SemanticIndex = 0;
		elements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		elements[1].InputSlot = 0;
		elements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		elements[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		elements[1].InstanceDataStepRate = 0;

		// configuration of rasterizer state
		D3D12_RASTERIZER_DESC descRS = {};
		descRS.FillMode = D3D12_FILL_MODE_SOLID;
		descRS.CullMode = D3D12_CULL_MODE_NONE;
		descRS.FrontCounterClockwise = FALSE;
		descRS.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		descRS.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		descRS.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		descRS.DepthClipEnable = FALSE;
		descRS.MultisampleEnable = FALSE;
		descRS.AntialiasedLineEnable = FALSE;
		descRS.ForcedSampleCount = 0;
		descRS.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		// blend settings of render target
		D3D12_RENDER_TARGET_BLEND_DESC descRTBS = {
			FALSE,
			FALSE,
			D3D12_BLEND_ONE,
			D3D12_BLEND_ZERO,
			D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE,
			D3D12_BLEND_ZERO,
			D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL
		};

		// configuration of blend state
		D3D12_BLEND_DESC descBS;
		descBS.AlphaToCoverageEnable = FALSE;
		descBS.IndependentBlendEnable = FALSE;
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			descBS.RenderTarget[i] = descRTBS;
		}

		// configuration of depth stencil state
		D3D12_DEPTH_STENCIL_DESC descDSS = {};
		descDSS.DepthEnable = TRUE;
		descDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		descDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		descDSS.StencilEnable = FALSE;

		ComPtr<ID3DBlob> pVSBlob;
		ComPtr<ID3DBlob> pPSBlob;

		std::wstring vsPath;
		std::wstring psPath;

		if (!SearchFilePath(L"SimpleTexVS.cso", vsPath))
		{
			return false;
		}

		if (!SearchFilePath(L"SimpleTexPS.cso", psPath))
		{
			return false;
		}

		// read vertex shader
		HRESULT hr = D3DReadFileToBlob(vsPath.c_str(), pVSBlob.GetAddressOf());
		if (FAILED(hr))
		{
			return false;
		}

		// read pixel shader
		hr = D3DReadFileToBlob(psPath.c_str(), pPSBlob.GetAddressOf());
		if (FAILED(hr))
		{
			return false;
		}

		// configuration of pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
		desc.InputLayout = { elements, _countof(elements) };
		desc.pRootSignature = m_pRootSignature.Get();
		desc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
		desc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
		desc.RasterizerState = descRS;
		desc.BlendState = descBS;
		desc.DepthStencilState = descDSS;
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		// generate pipeline state
		hr = m_pDevice->CreateGraphicsPipelineState(
			&desc,
			IID_PPV_ARGS(m_pPSO.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}
	}

	// generate texture
	{
		// search for file path
		std::wstring texturePath;
		if (!SearchFilePath(L"res/SampleTexture.dds", texturePath))
		{
			return false;
		}

		DirectX::ResourceUploadBatch batch(m_pDevice.Get());
		batch.Begin();

		// generate resource
		HRESULT hr = DirectX::CreateDDSTextureFromFile(
			m_pDevice.Get(),
			batch,
			texturePath.c_str(),
			m_Texture.pResource.GetAddressOf(),
			true);
		if (FAILED(hr))
		{
			return false;
		}

		// execute command
		std::future<void> future = batch.End(m_pQueue.Get());

		// wait for completion of command
		future.wait();

		// get increment size
		UINT incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// get CPU and GPU descriptor handle from descriptor heap
		D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = m_pHeapCBV->GetCPUDescriptorHandleForHeapStart();
		D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = m_pHeapCBV->GetGPUDescriptorHandleForHeapStart();

		// distribute descriptor to texture
		handleCPU.ptr += incrementSize * FrameCount;
		handleGPU.ptr += incrementSize * FrameCount;

		m_Texture.HandleCPU = handleCPU;
		m_Texture.HandleGPU = handleGPU;

		// get descriptor of texture
		D3D12_RESOURCE_DESC textureDesc = m_Texture.pResource->GetDesc();

		// configuration of shader resource view
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Format = textureDesc.Format;
		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		viewDesc.Texture2D.MostDetailedMip = 0;
		viewDesc.Texture2D.MipLevels = textureDesc.MipLevels;
		viewDesc.Texture2D.PlaneSlice = 0;
		viewDesc.Texture2D.ResourceMinLODClamp = 0.f;

		// generate shader resource view
		m_pDevice->CreateShaderResourceView(
			m_Texture.pResource.Get(),
			&viewDesc,
			handleCPU);
	}

	// configuration of viewport and scissor rect
	{
		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = static_cast<float>(m_Width);
		m_Viewport.Height = static_cast<float>(m_Height);
		m_Viewport.MinDepth = 0.f;
		m_Viewport.MaxDepth = 1.f;

		m_Scissor.left = 0;
		m_Scissor.right = m_Width;
		m_Scissor.top = 0;
		m_Scissor.bottom = m_Height;
	}

	return true;
}

//--------------------------------------------------------------------------------------------------------
// processing on termination
//--------------------------------------------------------------------------------------------------------
void App::OnTerm()
{
	for (uint32_t i = 0; i < FrameCount; ++i)
	{
		if (m_pCB[i].Get() != nullptr)
		{
			m_pCB[i]->Unmap(0, nullptr);
			memset(&m_CBV[i], 0, sizeof(m_CBV[i]));
		}
		m_pCB[i].Reset();
	}

	m_pIB.Reset();
	m_pVB.Reset();
	m_pPSO.Reset();
	m_pHeapCBV.Reset();

	m_VBV.BufferLocation = 0;
	m_VBV.SizeInBytes = 0;
	m_VBV.StrideInBytes = 0;

	m_IBV.BufferLocation = 0;
	m_IBV.Format = DXGI_FORMAT_UNKNOWN;
	m_IBV.SizeInBytes = 0;

	m_pRootSignature.Reset();

	m_Texture.pResource.Reset();
	m_Texture.HandleCPU.ptr = 0;
	m_Texture.HandleGPU.ptr = 0;
}

//--------------------------------------------------------------------------------------------------------
//	 window procedure
//--------------------------------------------------------------------------------------------------------
LRESULT App::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	break;

	default:
	{
		/* DO_NOTHING */
	}
	break;
	}

	return DefWindowProc(hWnd, msg, wp, lp);
}

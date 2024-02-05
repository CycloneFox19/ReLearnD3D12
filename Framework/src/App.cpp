//--------------------------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------------------------
#include <App.h>
#include <cassert>


namespace /* anonymous */ {

	//----------------------------------------------------------------------------------------------------
	// Constant Values
	//----------------------------------------------------------------------------------------------------
	const auto ClassName = TEXT("SampleWindowClass");


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Vertex structure
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	struct Vertex
	{
		DirectX::XMFLOAT3 Position; // position coordinates
		DirectX::XMFLOAT4 Color; // color of vertex
	};

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
	, m_pDevice(nullptr)
	, m_pQueue(nullptr)
	, m_pSwapChain(nullptr)
	, m_pCmdList(nullptr)
	, m_pHeapRTV(nullptr)
	, m_pFence(nullptr)
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
		m_CBV[2 * m_FrameIndex + 0].pBuffer->World = DirectX::XMMatrixRotationZ(m_RotateAngle + DirectX::XMConvertToRadians(45.f));
		m_CBV[2 * m_FrameIndex + 1].pBuffer->World = DirectX::XMMatrixRotationY(m_RotateAngle) * DirectX::XMMatrixScaling(2.f, 0.5f, 1.f);
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
	m_pCmdList->OMSetRenderTargets(1, &m_HandleRTV[m_FrameIndex], FALSE, nullptr);

	// set clear color
	float clearColor[] = { 0.25f, 0.25f, 0.25f, 1.0f };

	// clear render target view
	m_pCmdList->ClearRenderTargetView(m_HandleRTV[m_FrameIndex], clearColor, 0, nullptr);

	// rendering
	{
		m_pCmdList->SetGraphicsRootSignature(m_pRootSignature.Get());
		m_pCmdList->SetDescriptorHeaps(1, m_pHeapCBV.GetAddressOf());
		m_pCmdList->SetPipelineState(m_pPSO.Get());

		m_pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pCmdList->IASetVertexBuffers(0, 1, &m_VBV);
		m_pCmdList->IASetIndexBuffer(&m_IBV);
		m_pCmdList->RSSetViewports(1, &m_Viewport);
		m_pCmdList->RSSetScissorRects(1, &m_Scissor);

		m_pCmdList->SetGraphicsRootConstantBufferView(0, m_CBV[2 * m_FrameIndex + 0].Desc.BufferLocation);
		m_pCmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		m_pCmdList->SetGraphicsRootConstantBufferView(0, m_CBV[2 * m_FrameIndex + 1].Desc.BufferLocation);
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
		Vertex vertices[] = {
			{ DirectX::XMFLOAT3(-1.f,  1.f, 0.f), DirectX::XMFLOAT4(1.f, 0.f, 0.f, 1.f) },
			{ DirectX::XMFLOAT3( 1.f,  1.f, 0.f), DirectX::XMFLOAT4(0.f, 1.f, 0.f, 1.f) },
			{ DirectX::XMFLOAT3( 1.f, -1.f, 0.f), DirectX::XMFLOAT4(0.f, 0.f, 1.f, 1.f) },
			{ DirectX::XMFLOAT3(-1.f, -1.f, 0.f), DirectX::XMFLOAT4(1.f, 0.f, 1.f, 1.f) }
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
		m_VBV.StrideInBytes = static_cast<UINT>(sizeof(Vertex));
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

	// generate descriptor heap for constant buffer
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = FrameCount * 2;
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

		for (uint32_t i = 0; i < FrameCount * 2; ++i)
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
		D3D12_ROOT_PARAMETER param = {};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		param.Descriptor.ShaderRegister = 0;
		param.Descriptor.RegisterSpace = 0;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		// configuration of root signature
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.NumParameters = 1;
		desc.NumStaticSamplers = 0;
		desc.pParameters = &param;
		desc.pStaticSamplers = nullptr;
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

		elements[1].SemanticName = "COLOR";
		elements[1].SemanticIndex = 0;
		elements[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		elements[1].InputSlot = 0;
		elements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		elements[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		elements[1].InstanceDataStepRate = 0;

		// configuration of rasterizer state
		D3D12_RASTERIZER_DESC descRS = {};
		descRS.FillMode = D3D12_FILL_MODE_SOLID;
		descRS.CullMode = D3D12_CULL_MODE_NONE; // I'd change here later - just as the experiment
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

		ComPtr<ID3DBlob> pVSBlob;
		ComPtr<ID3DBlob> pPSBlob;

		// read vertex shader
		HRESULT hr = D3DReadFileToBlob(L"SimpleVS.cso", pVSBlob.GetAddressOf());
		if (FAILED(hr))
		{
			return false;
		}

		hr = D3DReadFileToBlob(L"SimplePS.cso", pPSBlob.GetAddressOf());
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
		desc.DepthStencilState.DepthEnable = FALSE;
		desc.DepthStencilState.StencilEnable = FALSE;
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.DSVFormat = DXGI_FORMAT_UNKNOWN;
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

	// configuration of viewport and scissor rect
	{
		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = static_cast<float>(m_Width);
		m_Viewport.Height = static_cast<float>(m_Height);
		m_Viewport.MinDepth = 0.f;
		m_Viewport.MaxDepth = 0.f;

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
	for (uint32_t i = 0; i < 2 * FrameCount; ++i)
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

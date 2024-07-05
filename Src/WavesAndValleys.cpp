#include "../Include/WavesAndValleys.h"

#include <DirectXColors.h>

#include "GeometryGenerator.h"

WavesAndValleys::WavesAndValleys(HINSTANCE hInstance, const BINDU::BINDU_WINDOW_DESC& wndDesc, DXGI_MODE_DESC backBufferDisplayMode ) :
 Win32Application(hInstance,wndDesc), DX12Graphics(&m_windowHandle, backBufferDisplayMode)
{

}

bool WavesAndValleys::OnInit()
{
	this->InitDirect3D();
	this->OnResize(m_windowWidth, m_windowHeight);

	// start initialization commands
	DX::ThrowIfFailed(m_commandList->Reset(m_commandAlloc.Get(), nullptr));

	BuildShadersAndInputLayout();

	BuildCBV();

	BuildValleyGeometry();

	BuildRootSignature();

	BuildPSO();


	// close the command list
	DX::ThrowIfFailed(m_commandList->Close());

	// Execute commands up to this point
	ID3D12CommandList* commands[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(commands), commands);

	// wait for gpu to execute commands prior this point
	this->FlushCommandQueue();

	this->OnResize(m_windowWidth, m_windowHeight);

	m_timer.Reset();
	return false;
}

void WavesAndValleys::Run()
{
	m_timer.Tick();

	int fps{ 0 };
	float mspf{0.0f} ;

	if(CalculateFrameStats(fps, mspf, static_cast<float>(m_timer.TotalTime())))
	{
		std::wstring title = StringToWstring(m_windowTitle) + L" FPS: " + std::to_wstring(fps) + L"\tMSPF: " + std::to_wstring(mspf) + L"\tTOTAL TIME: " + std::to_wstring(m_timer.TotalTime());

		SetWindowTitle(title);
	}

	this->Update();
	this->Render();
}

bool WavesAndValleys::OnDestroy()
{

	return false;
}

void WavesAndValleys::Update()
{
	m_eyePosW.x = m_radius * sinf(m_phi) * cosf(m_theta);
	m_eyePosW.z = m_radius * sinf(m_phi) * sinf(m_theta);
	m_eyePosW.y = m_radius * cosf(m_phi);

	// building the new world view proj matrix
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(m_eyePosW.x, m_eyePosW.y, m_eyePosW.z, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX viewMat = DirectX::XMMatrixLookAtLH(pos, target, up);

	DirectX::XMStoreFloat4x4(&m_viewMat, viewMat);

	DirectX::XMMATRIX worldMat = DirectX::XMLoadFloat4x4(&m_worldMat);
	DirectX::XMMATRIX projMat = DirectX::XMLoadFloat4x4(&m_projMat);

	DirectX::XMMATRIX worldViewProjMat = worldMat * viewMat * projMat;

	ObjectConstants objectConstants;
	DirectX::XMStoreFloat4x4(&objectConstants.WorldViewProj, DirectX::XMMatrixTranspose(worldViewProjMat));

	m_objectCB->CopyData(0, objectConstants);
}

void WavesAndValleys::Render()
{
	// Resetting the command allocator every frame to reuse the memory
	DX::ThrowIfFailed(m_commandAlloc->Reset());
	DX::ThrowIfFailed(m_commandList->Reset(m_commandAlloc.Get(), m_PSO.Get()));

	m_commandList->RSSetViewports(1, this->GetViewPort());
	m_commandList->RSSetScissorRects(1, this->GetScissorRect());

	// Transition and prepare the current back buffer as the render target for this frame
	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_dxgiSwapChainBuffers[m_currentBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	m_commandList->ResourceBarrier(1, &resourceBarrier);

	// set command to clear the Render target view and depth stencil view
	m_commandList->ClearRenderTargetView(GetCurrentBackBufferView(), DirectX::Colors::Red, 0, nullptr);
	m_commandList->ClearDepthStencilView(this->GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, nullptr);

	// specify the buffer we are going to render to
	m_commandList->OMSetRenderTargets(1, &this->GetCurrentBackBufferView(), true,
		&this->GetDepthStencilView());

	m_commandList->SetGraphicsRootSignature(m_rootSig.Get());

	// Set the vertex and index buffers
	m_commandList->IASetVertexBuffers(0, 1, &m_geometries.GetVertexBufferView());
	m_commandList->IASetIndexBuffer(&m_geometries.GetIndexBufferView());
	m_commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_commandList->SetGraphicsRootConstantBufferView(0, m_objectCB->Resource()->GetGPUVirtualAddress());


	// Draw the object
	m_commandList->DrawIndexedInstanced(m_geometries.Submesh["grid"].IndexCount,
		1, m_geometries.Submesh["grid"].StartIndexLocation,
		m_geometries.Submesh["grid"].BaseVertexLocation, 0);


	// Transition and prepare the current back buffer as the present buffer for this frame
	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_dxgiSwapChainBuffers[m_currentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	m_commandList->ResourceBarrier(1, &resourceBarrier);

	// done recording commands for this frame
	DX::ThrowIfFailed(m_commandList->Close());

	// Execute the commands up to this point
	ID3D12CommandList* commands[] = {m_commandList.Get()};
	m_commandQueue->ExecuteCommandLists(_countof(commands), commands);

	// Present the current back buffer
	DX::ThrowIfFailed(m_dxgiSwapChain->Present(0, 0));

	m_currentBackBuffer = (m_currentBackBuffer + 1) % m_swapChainBufferCount;

	// Flush the command queue ( make the cpu wait until gpu finishes executing commands)
	this->FlushCommandQueue();
}

void WavesAndValleys::OnResize(int width, int height)
{
	DX12Graphics::OnResize(width, height);

	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * 3.1415926535f, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_projMat, P);
}

void WavesAndValleys::BuildValleyGeometry()
{
	GeometryGenerator geoGen;
	// Create a grid
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.f, 160.f, 50, 50);

	std::vector<Vertex> vertices(grid.Vertices.size());

	for(size_t i = 0; i< grid.Vertices.size(); i++)
	{
		//vertices[i].Position = grid.Vertices[i].Position;

		auto& p = grid.Vertices[i].Position;
		vertices[i].Position = p;
		vertices[i].Position.y = GetHillsHeight(p.x, p.z);

		// Color vertices based on it's height
		if(vertices[i].Position.y < -10.f)
		{
			// Sandy beach color
			vertices[i].Color = DirectX::XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
		}
		else if(vertices[i].Position.y < 5.0f)
		{
			// Light yellow green
			vertices[i].Color = DirectX::XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		}
		else if(vertices[i].Position.y < 12.0f)
		{
			// Dark Yellow green
			vertices[i].Color = DirectX::XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
		}
		else if (vertices[i].Position.y < 20.0f)
		{
			// Dark brown
			vertices[i].Color = DirectX::XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
		}
		else
		{
			// White snow
			vertices[i].Color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}


	const std::vector<std::uint16_t> indices = grid.GetIndices16();

	//indices.insert(indices.begin(), grid.GetIndices16().begin(), grid.GetIndices16().end());

	const UINT vbByteSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));
	const UINT ibByteSize = static_cast<UINT>(indices.size() * sizeof(std::uint16_t));

	m_geometries.Name = "landGeo";

	DX::ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_geometries.VertexBufferCPU));
	DX::ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_geometries.IndexBufferCPU));

	m_geometries.VertexBufferGPU = D3DUtil::CreateDefaultBuffer(m_d3dDevice.Get(), m_commandList.Get(),
		vertices.data(), vbByteSize, m_geometries.VertexBufferUploader);

	m_geometries.IndexBufferGPU = D3DUtil::CreateDefaultBuffer(m_d3dDevice.Get(), m_commandList.Get(),
		indices.data(), ibByteSize, m_geometries.IndexBufferUploader);

	m_geometries.VertexBufferByteSize = vbByteSize;
	m_geometries.VertexByteStride = sizeof(Vertex);
	m_geometries.IndexBufferByteSize = ibByteSize;
	m_geometries.IndexFormat = DXGI_FORMAT_R16_UINT;

	SubmeshGeometry submeshGeometry;
	submeshGeometry.IndexCount = static_cast<UINT>(indices.size());
	submeshGeometry.StartIndexLocation = 0;
	submeshGeometry.BaseVertexLocation = 0;

	m_geometries.Submesh["grid"] = submeshGeometry;

}

void WavesAndValleys::BuildShadersAndInputLayout()
{

	m_shaders["standardVS"] = D3DUtil::CompileShader(
		RelativeResourcePath("Shaders/color.hlsl","WavesAndValleys"),
		nullptr, "VS", "vs_5_0");

	m_shaders["standardPS"] = D3DUtil::CompileShader(
		RelativeResourcePath("Shaders/color.hlsl", "WavesAndValleys"),
		nullptr, "PS", "ps_5_0");

	D3D12_INPUT_ELEMENT_DESC inputElementDesc;
	inputElementDesc.SemanticName = "POSITION";
	inputElementDesc.SemanticIndex = 0;
	inputElementDesc.AlignedByteOffset = 0;
	inputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDesc.InputSlot = 0;
	inputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElementDesc.InstanceDataStepRate = 0;

	m_inputLayout.push_back(inputElementDesc);

	inputElementDesc.SemanticName = "COLOR";
	inputElementDesc.AlignedByteOffset = 12;
	inputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	m_inputLayout.push_back(inputElementDesc);

}

void WavesAndValleys::BuildRootSignature()
{

	CD3DX12_ROOT_PARAMETER rootParameter;
	rootParameter.InitAsConstantBufferView(0);

	CD3DX12_ROOT_SIGNATURE_DESC rootsigDesc(1, &rootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSigBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootsigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSigBlob, &errorBlob);

	if(errorBlob)
	{
		OutputDebugStringA(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
	}
	DX::ThrowIfFailed(hr);

	DX::ThrowIfFailed(m_d3dDevice->CreateRootSignature(0,
		serializedRootSigBlob->GetBufferPointer(),
		serializedRootSigBlob->GetBufferSize(), IID_PPV_ARGS(m_rootSig.ReleaseAndGetAddressOf())));
}

void WavesAndValleys::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(psoDesc));

	CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = m_depthStencilFormat;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_backBufferFormat;
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NodeMask = 0;
	psoDesc.SampleDesc.Count = m_4xMSAAState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m_4xMSAAState ? (m_4xMSAAQuality - 1) : 0;
	psoDesc.InputLayout = { m_inputLayout.data(),
		static_cast<UINT>(m_inputLayout.size()) };

	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_shaders["standardVS"]->GetBufferPointer()),
		m_shaders["standardVS"]->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_shaders["standardPS"]->GetBufferPointer())
		,m_shaders["standardPS"]->GetBufferSize()
	};
	psoDesc.pRootSignature = m_rootSig.Get();
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	DX::ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_PSO.ReleaseAndGetAddressOf())));

}

void WavesAndValleys::BuildCBV()
{
	m_objectCB = std::make_unique<UploadBuffer<ObjectConstants>>(m_d3dDevice.Get(), 
		1, true);
}

float WavesAndValleys::GetHillsHeight(float x, float z) const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

DirectX::XMFLOAT3 WavesAndValleys::GetHillsNormal(float x, float z) const
{
	return DirectX::XMFLOAT3(0.f, 0.f, 0.f);
}

void WavesAndValleys::OnMouseDown(BINDU::MouseButton btn, int x, int y)
{
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;

	SetCapture(m_windowHandle);
}

void WavesAndValleys::OnMouseMove(BINDU::MouseButton btn, int x, int y)
{
	if(btn == BINDU::LEFT)
	{
		float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - m_lastMousePos.x));
		float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - m_lastMousePos.y));

		m_theta += dx;
		m_phi += dy;

		m_phi = MathHelper::Clamp(m_phi, 0.1f, 3.1415926535f - 0.1f);
	}
	else if(btn == BINDU::RIGHT)
	{
		float dx = 0.05f * static_cast<float>(x - m_lastMousePos.x);
		float dy = 0.05f * static_cast<float>(y - m_lastMousePos.y);

		// Update the camera radius based on input.
		m_radius += dx - dy;

		// Restrict the radius.
		m_radius = MathHelper::Clamp(m_radius, 5.0f, 150.0f);
	}

	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}

void WavesAndValleys::OnMouseUp(BINDU::MouseButton btn, int x, int y)
{
	ReleaseCapture();
}

void WavesAndValleys::OnKeyboardDown(BINDU::KeyBoardKey key, bool isDown, bool repeat)
{
}

void WavesAndValleys::OnKeyboardUp(BINDU::KeyBoardKey key, bool isUp, bool repeat)
{
}


LRESULT WavesAndValleys::WindowMessageProc(UINT& msg, WPARAM& wParam, LPARAM& lParam)
{

	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_ACTIVATE:

		if (LOWORD(wParam) == WA_INACTIVE)
		{
			m_isPaused = true;
			m_timer.Stop();
		}
		else
		{
			m_isPaused = false;
			m_timer.Start();
		}

		break;

	case WM_ENTERSIZEMOVE:
		m_isPaused = true;
		m_resizing = true;
		m_timer.Stop();
		break;

	case WM_EXITSIZEMOVE:
		m_isPaused = false;
		m_resizing = false;
		m_timer.Start();
		this->OnResize(m_windowWidth, m_windowHeight);
		break;

	case WM_SIZE:

		m_windowWidth = LOWORD(lParam);
		m_windowHeight = HIWORD(lParam);

		if (m_d3dInitialized)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				m_isPaused = true;
				m_minimized = true;
				m_maximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{

				m_isPaused = false;
				m_minimized = false;
				m_maximized = true;

				this->OnResize(m_windowWidth, m_windowHeight);
			}
			else if (wParam == SIZE_RESTORED)
			{
				if (m_minimized)
				{
					m_isPaused = false;
					m_minimized = false;
					this->OnResize(m_windowWidth, m_windowHeight);
				}
				else if (m_maximized)
				{
					m_isPaused = false;
					m_maximized = false;
					this->OnResize(m_windowWidth, m_windowHeight);
				}
				else if (m_resizing)
				{

				}
				else
					this->OnResize(m_windowWidth, m_windowHeight);
			}

		}

		break;

	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		break;

	default:
		break;
	}

	Win32Input::InputMsgProc(nullptr, msg, wParam, lParam);

	return DefWindowProc(m_windowHandle, msg, wParam, lParam);
}


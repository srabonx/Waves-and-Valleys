#pragma once
#include <Bindu_Timer.h>

#include <Bindu_Graphics.h>
#include <DirectXColors.h>

#include <MathHelper.h>

#include "Win32Application.h"
#include "Win32Input.h"


class WavesAndValleys final : public BINDU::Win32Application, public BINDU::DX12Graphics, public BINDU::Win32Input
{

public:

	// Defining the Vertex structure for this project
	struct Vertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT4 Color = DirectX::XMFLOAT4(0.f, 255.f, 0.f, 255.f);
	};


	// Object constants for this project

	struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 WorldViewProj;
	};

public:
	// Constructor
	WavesAndValleys(HINSTANCE hInstance, const BINDU::BINDU_WINDOW_DESC& wndDesc, DXGI_MODE_DESC backBufferDisplayMode);

	// Destructor
	~WavesAndValleys() = default;

	// Initialization stuff goes in here
	bool OnInit() override;

	// Overriden base class method to calculate the frame stats/ called every frame
	void Run() override;
	
	// Closing stuff goes in here
	bool OnDestroy() override;

	// Window message processing is done here
	LRESULT WindowMessageProc(UINT& msg, WPARAM& wParam, LPARAM& lParam) override;

private:

	// Update and Render gets called every frame in order
	void Update() override;
	void Render() override;

	// To fix the aspect ratio when the window get resized
	void OnResize(int width, int height) override;

private:

	// Building the valley geometry
	void BuildValleyGeometry();

	// Shaders and input layout
	void BuildShadersAndInputLayout();

	// Builing the root signature
	void BuildRootSignature();

	// Building the pipeline state object
	void BuildPSO();

	// Build the constant buffer view
	void BuildCBV();

private:

	// Helper methods
	float GetHillsHeight(float x, float z) const;
	DirectX::XMFLOAT3 GetHillsNormal(float x, float z) const;


private:
	// Input methods
	void OnMouseDown(BINDU::MouseButton btn, int x, int y) override;
	void OnMouseMove(BINDU::MouseButton btn, int x, int y) override;
	void OnMouseUp(BINDU::MouseButton btn, int x, int y) override;
	void OnKeyboardDown(BINDU::KeyBoardKey key, bool isDown, bool repeat) override;
	void OnKeyboardUp(BINDU::KeyBoardKey key, bool isUp, bool repeat) override;

private:

	BINDU::Timer m_timer;

	// Container for all the geometries
	MeshGeometry m_geometries;

	// Shaders
	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_shaders;

	// Input layout
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

	// Root signature
	ComPtr<ID3D12RootSignature>	m_rootSig{ nullptr };

	// Pipeline state object
	ComPtr<ID3D12PipelineState>	m_PSO{ nullptr };

	// object Constant buffer
	std::unique_ptr<UploadBuffer<ObjectConstants>> m_objectCB = nullptr;


	// World, View, Proj matrix

	DirectX::XMFLOAT4X4 m_worldMat = MathHelper::Identity4X4();
	DirectX::XMFLOAT4X4 m_viewMat = MathHelper::Identity4X4();
	DirectX::XMFLOAT4X4 m_projMat = MathHelper::Identity4X4();

	DirectX::XMFLOAT3 m_eyePosW = { 0.f,0.f,0.f };

	float m_phi = 0.2f * DirectX::XM_PI;
	float m_theta = 1.5f * DirectX::XM_PI;
	float m_radius = 15.0f;


	POINT	m_lastMousePos {0,0};
};

#include "../Include/WavesAndValleys.h"
#include <Win32Application.h>



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	int r = -1;
	{
		BINDU::BINDU_WINDOW_DESC windowDesc;
		windowDesc.windowWidth = 800;
		windowDesc.windowHeight = 600;
		windowDesc.windowTitle = "Waves and Valleys";

		DXGI_MODE_DESC backBufferDesc = { 0 };
		backBufferDesc.Width = windowDesc.windowWidth;
		backBufferDesc.Height = windowDesc.windowHeight;

		WavesAndValleys app(hInstance, windowDesc, backBufferDesc);

		r = app.Start();

	}

#if defined(DEBUG) || defined(_DEBUG)
	_CrtDumpMemoryLeaks();
#endif

	return r;
}
#include <windows.h>
#include "BattleFireVulkan.h"
#include "Scene.h"
#pragma comment(lib,"winmm.lib")
// Returns elapsed time between frames in seconds.
float GetFrameTime() {
	static unsigned long lastTime = 0, timeSinceComputerStart = 0;
	timeSinceComputerStart = timeGetTime();
	unsigned long frameTime = lastTime == 0 ? 0 : timeSinceComputerStart - lastTime;
	lastTime = timeSinceComputerStart;
	return float(frameTime) / 1000.0f;
}
// Utility loader used for reading precompiled shader binaries (*.vsb/*.fsb).
unsigned char* LoadFileContent(const char* path, int& filesize) {
	unsigned char* fileContent = nullptr;
	filesize = 0;
	FILE* pFile = nullptr;
	errno_t ret = fopen_s(&pFile,path, "rb");
	if (ret==0) {
		fseek(pFile, 0, SEEK_END);
		int nLen = ftell(pFile);
		if (nLen > 0) {
			rewind(pFile);
			fileContent = new unsigned char[nLen + 1];
			fread(fileContent, sizeof(unsigned char), nLen, pFile);
			fileContent[nLen] = '\0';
			filesize = nLen;
		}
		fclose(pFile);
	}
	return fileContent;
}
// Window callback:
// - resize => recreate swapchain-dependent resources
// - close  => request app quit
LRESULT CALLBACK VulkanWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_SIZE: {
		RECT rect;
		GetClientRect(hwnd, &rect);
		OnViewportChanged(rect.right - rect.left, rect.bottom - rect.top);
	}
	break;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
// Application entry:
// 1) create window
// 2) initialize Vulkan backend
// 3) initialize scene resources
// 4) run render loop
INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	WNDCLASSEX wndClass;
	memset(&wndClass, 0, sizeof(WNDCLASSEX));
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hbrBackground = NULL;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hIcon = NULL;
	wndClass.hIconSm = NULL;
	wndClass.hInstance = hInstance;
	wndClass.lpfnWndProc = VulkanWndProc;
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = L"BattleFireWindow";
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	ATOM atom = RegisterClassEx(&wndClass);
	RECT rect = { 0,0,1280,720 };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	HWND hwnd = CreateWindowEx(NULL, L"BattleFireWindow", L"Vulkan Render Window", WS_OVERLAPPEDWINDOW,
		200, 200, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);
	GetGlobalConfig().mPreferedSampleCount = VK_SAMPLE_COUNT_1_BIT;
	// Backend init creates instance/device/swapchain/render pass/sync objects.
	InitVulkan(hwnd, 1280, 720);
	// Scene init builds all render resources and nodes.
	InitScene();
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	MSG msg;
	while (true) {
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Render a complete frame every tick.
		RenderOneFrame();
	}
	//OnQuit();
	return 0;
}
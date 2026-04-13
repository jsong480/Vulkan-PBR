#include <windows.h>
#include <windowsx.h>
#include <cstdio>
#include "MyVulkan.h"
#include "Scene.h"
#pragma comment(lib,"winmm.lib")

static HWND gHWND = nullptr;
static float gFPS = 0.0f;
static void UpdateTitle() {
	char buf[256];
	sprintf_s(buf, "Vulkan PBR | %s | SSAO:%s Bloom:%s Lights:%s FXAA:%s | FPS:%.1f | 1-5 toggle",
		IsDeferredEnabled() ? "Deferred" : "Forward",
		IsSSAOEnabled() ? "ON" : "OFF",
		IsBloomEnabled() ? "ON" : "OFF",
		IsPointLightsEnabled() ? "ON" : "OFF",
		IsFXAAEnabled() ? "ON" : "OFF",
		gFPS);
	SetWindowTextA(gHWND, buf);
}
float GetFrameTime() {
	static unsigned long lastTime = 0, timeSinceComputerStart = 0;
	timeSinceComputerStart = timeGetTime();
	unsigned long frameTime = lastTime == 0 ? 0 : timeSinceComputerStart - lastTime;
	lastTime = timeSinceComputerStart;
	return float(frameTime) / 1000.0f;
}
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
static bool gMouseCaptured = false;
static void CaptureMouse(HWND hwnd) {
	gMouseCaptured = true;
	RECT r;
	GetClientRect(hwnd, &r);
	POINT center = { (r.right) / 2, (r.bottom) / 2 };
	ClientToScreen(hwnd, &center);
	SetCursorPos(center.x, center.y);
	ShowCursor(FALSE);
	SetCapture(hwnd);
}
static void ReleaseMouse() {
	gMouseCaptured = false;
	ShowCursor(TRUE);
	ReleaseCapture();
}
LRESULT CALLBACK VulkanWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_SIZE: {
		RECT rect;
		GetClientRect(hwnd, &rect);
		OnViewportChanged(rect.right - rect.left, rect.bottom - rect.top);
	}
	break;
	case WM_KEYDOWN:
		if (wParam == '1') { ToggleSSAO(); UpdateTitle(); }
		if (wParam == '2') { ToggleBloom(); UpdateTitle(); }
		if (wParam == '3') { ToggleDeferred(); UpdateTitle(); }
		if (wParam == '4') { TogglePointLights(); UpdateTitle(); }
		if (wParam == '5') { ToggleFXAA(); UpdateTitle(); }
		if (wParam == VK_ESCAPE && gMouseCaptured) { ReleaseMouse(); }
		break;
	case WM_LBUTTONDOWN:
		if (!gMouseCaptured) { CaptureMouse(hwnd); }
		break;
	case WM_MOUSEMOVE:
		if (gMouseCaptured) {
			RECT r;
			GetClientRect(hwnd, &r);
			POINT center = { r.right / 2, r.bottom / 2 };
			POINT screenCenter = center;
			ClientToScreen(hwnd, &screenCenter);
			int mx = GET_X_LPARAM(lParam);
			int my = GET_Y_LPARAM(lParam);
			float dx = float(mx - center.x);
			float dy = float(my - center.y);
			if (dx != 0.0f || dy != 0.0f) {
				SetMouseDelta(dx, dy);
				SetCursorPos(screenCenter.x, screenCenter.y);
			}
		}
		break;
	case WM_KILLFOCUS:
		if (gMouseCaptured) { ReleaseMouse(); }
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
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
	gHWND = hwnd;
	GetGlobalConfig().mPreferedSampleCount = VK_SAMPLE_COUNT_1_BIT;
	InitVulkan(hwnd, 1280, 720);
	InitScene();
	UpdateTitle();
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	MSG msg;
	int frameCount = 0;
	DWORD lastFPSTime = timeGetTime();
	while (true) {
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		RenderOneFrame();
		frameCount++;
		DWORD now = timeGetTime();
		if (now - lastFPSTime >= 1000) {
			gFPS = frameCount * 1000.0f / float(now - lastFPSTime);
			frameCount = 0;
			lastFPSTime = now;
			UpdateTitle();
		}
	}
	//OnQuit();
	return 0;
}
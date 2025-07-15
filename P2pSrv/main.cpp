#include "WindowsService.h"
#include <iostream>

/**
* @brief WinMain entry point for the Windows service application
*
* This function serves as the primary entry point for the service application.
* It handles command-line arguments for service installation/uninstallation
* and starts the service control dispatcher when run as a service.
*
* @details Command-line argument handling:
* start a hidden window
* start http server 
* continue working without GUI untill not reciving a heartbeat signal from web page for 5 seconds
*/
#define TIMER_ID 1
#define TIMEOUT_MS 500000 // 500 seconds for testing

volatile bool timerElapsed = false;

// Window procedure to handle messages
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_TIMER && wParam == TIMER_ID) {
		timerElapsed = true;
		KillTimer(hwnd, TIMER_ID);
		//PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// Register a window class
	const wchar_t CLASS_NAME[] = L"HiddenTimerWindowClass";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	// Create a hidden window
	HWND hwnd = CreateWindowEx(
		0, CLASS_NAME, L"Hidden Timer Window",
		0, 0, 0, 0, 0,
		HWND_MESSAGE, NULL, hInstance, NULL);

	if (!hwnd) {
		MessageBox(NULL, L"Failed to create hidden window", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	//start http server
	CWindowsService servic;
	servic.StartHttpThrd();

	// Set the timer on the hidden window
	if (!SetTimer(hwnd, TIMER_ID, TIMEOUT_MS, NULL)) {
		MessageBox(NULL, L"Failed to set timer", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	// Message loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}


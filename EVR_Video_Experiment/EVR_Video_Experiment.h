#pragma once
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
void FullScreen(HWND hWnd);
HRESULT OpenFile(HWND hWnd, LPCWSTR pszFile);
HRESULT InitEvr(HWND hWnd);
HRESULT SetVideoPos(HWND hWnd, int nMode);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT OpenFileMadVR(LPCWSTR pszFile);
HRESULT InitMadVR();
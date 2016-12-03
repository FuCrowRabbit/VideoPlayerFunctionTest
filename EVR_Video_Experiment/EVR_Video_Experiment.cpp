#include "stdafx.h"
#include "EVR_Video_Experiment.h"




//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const LPCWSTR const VIDEOpass(L"C:\\Downloads\\munou.mp4");
//↑動画のパスをここに入力して、実行してみよう!('\'はエスケープシーケンスが働いているので注意)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------




// Unicode文字セット
#pragma comment(lib, "strmiids.lib")

#include <DShow.h>
#include <evr.h>

#define SAFE_RELEASE(x) { if (x) { x->Release(); x = NULL; } }
const LPCWSTR const CLASSname(L"DirectShow_EVR");
const LPCWSTR const WINDOWname(L"DirectShow_EVR");

// 関数プロトタイプ宣言
HRESULT OpenFile(HWND hWnd, LPCWSTR pszFileName);
HRESULT InitEvr(HWND hWnd);
HRESULT SetVideoPos(HWND hWnd, int nMode);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// 外部変数構造体
static struct {
	IBaseFilter *pEvr;
	IGraphBuilder *pGraph;
	IMediaControl *pControl;
	IMFVideoDisplayControl *pVideo;
	SIZE size;
	int nPlay;
	int Cusor;
	unsigned int Cusor_time;
	bool FullScreen_Flag;
} g;

//------------------------------------------------------------------------------
void FullScreen(HWND hWnd)
{
	MFVideoNormalizedRect mvnr = { 0.0f, 0.0f, 1.0f, 1.0f };
	RECT rcDst;
	if(!(g.FullScreen_Flag)){
		g.FullScreen_Flag = true;
		// ウィンドウスタイル変更(メニューバーなし、最前面)
		SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP);
		SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

		// 最大化する
		ShowWindow(hWnd, SW_MAXIMIZE);

		// ディスプレイサイズを取得
		int mainDisplayWidth = GetSystemMetrics(SM_CXSCREEN);
		int mainDisplayHeight = GetSystemMetrics(SM_CYSCREEN);

		// クライアント領域をディスプレーに合わせる
		SetWindowPos(hWnd, NULL,0, 0, mainDisplayWidth, mainDisplayHeight,SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE);

		SetRect(&rcDst, 0, 0, g.size.cx, g.size.cy);
		GetClientRect(hWnd, &rcDst);
		//g.pVideo->SetVideoPosition(&mvnr, &rcDst);(必要ない)
	}
	else {
		g.FullScreen_Flag = false;
		SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		//SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

		// 普通に戻す
		ShowWindow(hWnd, SW_RESTORE); 
		SetVideoPos(hWnd,1);
	}
}

//------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wcx;
	HWND hWnd;
	MSG msg = { NULL };
	HRESULT hr;
	g.FullScreen_Flag = false;//Flagの初期化
	// COMライブラリの初期化
	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		return 0;
	}

	// ウィンドウクラスの登録
	ZeroMemory(&wcx, sizeof wcx);
	wcx.cbSize = sizeof wcx;
	wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcx.lpfnWndProc = MainWndProc;
	wcx.hInstance = hInstance;
	wcx.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);	// 黒がいいかも
	wcx.lpszClassName = CLASSname;
	if (RegisterClassEx(&wcx) == 0) {
		goto Exit;
	}

	// ウィンドウの作成
	hWnd = CreateWindow(CLASSname, WINDOWname,
		WS_OVERLAPPEDWINDOW,
		//		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
		100, 100, 640, 480,
		NULL, NULL, hInstance, NULL);
	if (hWnd == NULL) {
		goto Exit;
	}

	// DirectShowフィルタの準備

	hr = OpenFile(hWnd, VIDEOpass);
	if (FAILED(hr)) {

		goto Exit;
	}

	ShowWindow(hWnd, nCmdShow);

	// 動画再生
	hr = g.pControl->Run();
	g.nPlay = 1;

	// メッセージループ
	do {
		Sleep(1);
		if (g.Cusor >= 0) {
			if (g.Cusor_time > 0) {
				g.Cusor_time--;
			}
			else{//(g.Cusor_time == 0)と同意義 
				g.Cusor = ShowCursor(false);
			}
		}
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	} while (msg.message != WM_QUIT);

	// 動画停止
	hr = g.pControl->Stop();
	Sleep(1000);
Exit:
	SAFE_RELEASE(g.pEvr);
	SAFE_RELEASE(g.pVideo);
	SAFE_RELEASE(g.pControl);
	SAFE_RELEASE(g.pGraph);
	CoUninitialize();
	return msg.wParam;
}

//------------------------------------------------------------------------------
HRESULT OpenFile(HWND hWnd, LPCWSTR pszFile)
{
	// フィルタグラフの作成
	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&g.pGraph));

	// メディアコントロールインターフェイスの取得
	if (SUCCEEDED(hr)) {

		hr = g.pGraph->QueryInterface(IID_PPV_ARGS(&g.pControl));
	}

	// ビデオの作成
	if (SUCCEEDED(hr)) {

		hr = InitEvr(hWnd);
	}
	// グラフを作成する
	if (SUCCEEDED(hr)) {
		//Graph(pszFile);
		hr = g.pGraph->RenderFile(pszFile, NULL);
	}

	// 描画領域の設定
	if (SUCCEEDED(hr)) {

		g.pVideo->GetNativeVideoSize(&g.size, NULL);
	}
	if (SUCCEEDED(hr)) {

		hr = SetVideoPos(hWnd, 1);
	}
	return hr;
}

//------------------------------------------------------------------------------
HRESULT InitEvr(HWND hWnd)
{

	// EVRの作成
	HRESULT hr = CoCreateInstance(CLSID_EnhancedVideoRenderer, NULL, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&g.pEvr));

	// フィルタグラフにEVRを追加
	if (SUCCEEDED(hr)) {
		hr = g.pGraph->AddFilter(g.pEvr, L"EVR");
	}

	IMFGetService *pService = NULL;
	if (SUCCEEDED(hr)) {
		hr = g.pEvr->QueryInterface(IID_PPV_ARGS(&pService));
	}
	if (SUCCEEDED(hr)) {
		hr = pService->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&g.pVideo));
	}
	SAFE_RELEASE(pService);

	if (SUCCEEDED(hr)) {
		hr = g.pVideo->SetVideoWindow(hWnd);
	}
	return hr;
}

//------------------------------------------------------------------------------
HRESULT SetVideoPos(HWND hWnd, int nMode)
{
	MFVideoNormalizedRect mvnr = { 0.0f, 0.0f, 1.0f, 1.0f };
	RECT rcDst;

	if (1 <= nMode && nMode <= 4) {
		SetRect(&rcDst, 0, 0, g.size.cx * nMode / 2, g.size.cy * nMode / 2);
		AdjustWindowRectEx(&rcDst, WS_OVERLAPPEDWINDOW, FALSE, 0);
		SetWindowPos(hWnd, NULL, 0, 0, rcDst.right - rcDst.left, rcDst.bottom - rcDst.top,
			SWP_NOZORDER | SWP_NOMOVE);
	}
	GetClientRect(hWnd, &rcDst);
	return g.pVideo->SetVideoPosition(&mvnr, &rcDst);
}

//------------------------------------------------------------------------------
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CHAR:
		switch (wParam) {
		case VK_SPACE:
			if (g.nPlay) {
				g.pControl->Pause();
				g.nPlay = 0;
			}
			else {
				g.pControl->Run();
				g.nPlay = 1;
			}
			break;
		case 's':
			g.pControl->StopWhenReady();
			g.nPlay = 0;
			break;
		case '1': case '2': case '3': case '4':
			SetVideoPos(hWnd, wParam - '0');
			break;
		case VK_ESCAPE:
			DestroyWindow(hWnd);
			break;
		}
		break;
	case WM_SIZE:
		SetVideoPos(hWnd, 0);
		break;	
	case WM_LBUTTONDOWN:
		if(!(g.FullScreen_Flag)){
			PostMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);//クライアントの上でウィンドウを動かす。
		}
		break;
	case WM_LBUTTONDBLCLK:
		FullScreen(hWnd);
		break;
	case WM_DESTROY:
		while (g.Cusor < 0) {
			g.Cusor = ShowCursor(true);
		}
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		g.Cusor_time = 1000;
		g.Cusor=0;
		break;
	case WM_MOUSEMOVE:
		g.Cusor_time = 1000;
		while (g.Cusor < 0) {
			g.Cusor=ShowCursor(true);
		}
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}
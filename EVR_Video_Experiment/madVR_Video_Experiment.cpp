#include "stdafx.h"
#include "atlbase.h"
#include "EVR_Video_Experiment.h"
#include "interfaces\mvrInterfaces.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const LPCWSTR VIDEOpass(L"D:\\CrowRabbit\\Videos\\poporon.mkv");
//↑動画のパスをここに入力して、実行してみよう!('\'はエスケープシーケンスが働いているので注意)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// Unicode文字セット
#pragma comment(lib, "strmiids.lib")

#include <DShow.h>
#include <evr.h>
#include "vmr9.h"

#define SAFE_RELEASE(x) { if (x) { x->Release(); x = NULL; } }
const LPCWSTR CLASSname(L"DirectShow_madVR");
const LPCWSTR WINDOWname(L"DirectShow_madVR");

// 関数プロトタイプ宣言
HRESULT OpenFile(HWND hWnd, LPCWSTR pszFileName);
HRESULT InitEvr(HWND hWnd);
HRESULT SetVideoPos(HWND hWnd, int nMode);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// 外部変数構造体
static struct {
	IBaseFilter *pMVR;
	IBasicVideo2 *pBase2;
	IVideoWindow *pWindow;
	IGraphBuilder *pGraph;
	IMediaControl *pControl;
	//IMFVideoDisplayControl *pVideo;
	ICaptureGraphBuilder2 *pCGB2;
	CComPtr<IMFVideoMixerBitmap>    pMix;
	CComPtr<IMFVideoProcessor>      pProc;
	SIZE size;
	int nPlay;
	int Cusor;
	unsigned int Cusor_time;
	bool FullScreen_Flag;
} g;

class MadVRClass {
public:
	IMadVRSubclassReplacement *pMVRSP;

	~MadVRClass() {
		pMVRSP->Release();
	}

};

MadVRClass MadVR;

//-------------------------helpers-------------------------
#pragma comment(lib, "rpcrt4.lib")//ないと未解決のエラー
BOOL GuidFromString
(
	GUID* pGuid
	, std::wstring oGuidString
)
{
	// 文字列をGUIDに変換する
	if (RPC_S_OK == ::UuidFromString((RPC_WSTR)oGuidString.c_str(), (UUID*)pGuid)) {

		// 変換できました。
		return(TRUE);
	}
	return(FALSE);
}

HRESULT AddFilter(
	IGraphBuilder *pGraph,  // Filter Graph Manager へのポインタ
	const GUID& clsid,      // 作成するフィルタの CLSID
	LPCWSTR wszName,        // フィルタの名前
	IBaseFilter **ppF)      // フィルタへのポインタが格納される
{
	*ppF = 0;
	IBaseFilter *pF = 0;
	HRESULT hr = CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, reinterpret_cast<LPVOID*>(&pF));
	if (SUCCEEDED(hr))
	{
		hr = pGraph->AddFilter(pF, wszName);
		if (SUCCEEDED(hr))
			*ppF = pF;
		else
			pF->Release();
	}
	return hr;
}


//-----------------------------------------------------------
//------------------------------------------------------------------------------
void FullScreen(HWND hWnd)
{
	MFVideoNormalizedRect mvnr = { 0.0f, 0.0f, 1.0f, 1.0f };
	RECT rcDst;
	if (!(g.FullScreen_Flag)) {
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
		SetWindowPos(hWnd, NULL, 0, 0, mainDisplayWidth, mainDisplayHeight, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE);

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
		SetVideoPos(hWnd, 1);
	}
}

//------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wcx;
	HWND madVRWindow = nullptr;
	MSG msg = { NULL };
	HRESULT hr;
	g.FullScreen_Flag = false;//Flagの初期化
							  
	hr = CoInitialize(NULL);	// COMライブラリの初期化
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
	madVRWindow = CreateWindow(CLASSname, WINDOWname,
		WS_OVERLAPPEDWINDOW,
		//		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
		100, 100, 640, 480,
		NULL, NULL, hInstance, NULL);
	if (madVRWindow == NULL) {
		goto Exit;
	}

	// DirectShowフィルタの準備

	hr = OpenFileMadVR(VIDEOpass);
	if (FAILED(hr)) {
		goto Exit;
	}

	//hr = g.pWindow->put_AutoShow(OAFALSE);
	hr = g.pWindow->put_Owner((OAHWND)madVRWindow);
	hr = g.pWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
	hr = g.pWindow->SetWindowPosition(0, 0, 640, 480);
	hr = g.pWindow->put_MessageDrain((OAHWND)madVRWindow);
	hr = g.pWindow->SetWindowForeground(OATRUE);
	hr = g.pWindow->put_Visible(OATRUE);
	/*
	//Window調達
	if(g.pWindow){
		hr = g.pWindow->get_Owner((OAHWND*)madVRWindow);
		hr = g.pWindow->put_WindowStyle(WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS);
		hr = g.pWindow->put_MessageDrain((OAHWND)madVRWindow);
	}
	*/
	//HWND hWnd = GetWindow(madVRWindow, GW_CHILD);
	//EnableWindow(hWnd, false);
	ShowWindow(madVRWindow, nCmdShow);

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
			else {//(g.Cusor_time == 0)と同意義 
				//g.Cusor = ShowCursor(false);
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
	SAFE_RELEASE(g.pMVR);
	//SAFE_RELEASE(g.pVideo);
	SAFE_RELEASE(g.pBase2);
	SAFE_RELEASE(g.pWindow);
	SAFE_RELEASE(g.pCGB2);
	SAFE_RELEASE(g.pControl);
	SAFE_RELEASE(g.pGraph);
	CoUninitialize();
	return msg.wParam;
}

//------------------------------------------------------------------------------
HRESULT OpenFileMadVR(LPCWSTR pszFile)
{
	// フィルタグラフの作成
	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&g.pGraph));

	// メディアコントロールインターフェイスの取得
	if (SUCCEEDED(hr)) {
		hr = g.pGraph->QueryInterface(IID_PPV_ARGS(&g.pControl));
	}

	if (SUCCEEDED(hr)) {
		hr = g.pGraph->QueryInterface(IID_IBasicVideo2, (void**)&g.pBase2);
	}

	if (SUCCEEDED(hr)) {
		hr = g.pGraph->QueryInterface(IID_IVideoWindow, (void**)&g.pWindow);
	}

	//ICaptureGraphBuilder2をインスタンス
	if (SUCCEEDED(hr)) {
		hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **)&g.pCGB2);
	}

	if (SUCCEEDED(hr)) {
		hr = g.pCGB2->SetFiltergraph(g.pGraph);//キャプチャ グラフ ビルダが使うフィルタ グラフを指定する。
	}

	// ビデオの作成
	if (SUCCEEDED(hr)) {

		hr = InitMadVR();
	}

	if (SUCCEEDED(hr)) {
		//hr = g.pCGB2->RenderStream(0, 0, g.pSource, 0, g.pEvr);

		IBaseFilter *pSrc = 0, *pDec = 0, *pRawfilter = 0, *pVSFilter = 0;
		IPin *pSubtitle = 0;
		GUID CLSID_LAVVideo, CLSID_ffdshow_raw, CLSID_VSFilter, CLSID_MEDIATYPE_Subtitle, CLSID_ffdshow_Video;
		HRESULT Connect;//Directshowのフィルタの接続安否(最終的には字幕の接続安否に使われる)
		const std::wstring LAV_Video(L"EE30215D-164F-4A92-A4EB-9D4C13390F9F");
		const std::wstring ffdshow_Video(L"04FE9017-F873-410E-871E-AB91661A4EF7");
		const std::wstring ffdshow_Raw(L"0B390488-D80F-4A68-8408-48DC199F0E97");
		const std::wstring VSFilter(L"93A22E7A-5091-45EF-BA61-6DA26156A5D0");
		const std::wstring MEDIATYPE_Subtitle(L"E487EB08-6B26-4BE9-9DD3-993434D313FD");

		GuidFromString(&CLSID_MEDIATYPE_Subtitle, MEDIATYPE_Subtitle);
		hr = g.pGraph->AddSourceFilter(pszFile, pszFile, &pSrc);
		if (SUCCEEDED(hr)) {
			GuidFromString(&CLSID_LAVVideo, LAV_Video);
			Connect = AddFilter(g.pGraph, CLSID_LAVVideo, L"LAV Video Decoder", &pDec);
			if (SUCCEEDED(Connect)) {
				hr = g.pCGB2->RenderStream(0, 0, pSrc, 0, pDec);
				GuidFromString(&CLSID_ffdshow_raw, ffdshow_Raw);
				Connect = AddFilter(g.pGraph, CLSID_ffdshow_raw, L"ffdshow raw video filter", &pRawfilter);
				if (SUCCEEDED(Connect)) {
					hr = g.pCGB2->RenderStream(0, 0, pDec, 0, pRawfilter);
					GuidFromString(&CLSID_VSFilter, VSFilter);
					Connect = AddFilter(g.pGraph, CLSID_VSFilter, L"VSFilter", &pVSFilter);
					if (SUCCEEDED(Connect)) {
						//Full installed.
						hr = g.pCGB2->RenderStream(0, 0, pRawfilter, 0, pVSFilter);
						hr = g.pCGB2->RenderStream(0, 0, pVSFilter, 0, g.pMVR);
						HRESULT hr_SubTitle = g.pCGB2->FindPin(pSrc,
							PINDIR_OUTPUT,
							NULL,
							&CLSID_MEDIATYPE_Subtitle,
							true,
							NULL,
							&pSubtitle
						);//字幕のピンを探す
						if (SUCCEEDED(hr_SubTitle)) {
							Connect = g.pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pVSFilter);//字幕の接続
						}
					}
					else {
						//VSFilter is not installed.
						hr = g.pCGB2->RenderStream(0, 0, pRawfilter, 0, g.pMVR);
						HRESULT hr_SubTitle = g.pCGB2->FindPin(pSrc,
							PINDIR_OUTPUT,
							NULL,
							&CLSID_MEDIATYPE_Subtitle,
							true,
							NULL,
							&pSubtitle
						);//字幕のピンを探す
						if (SUCCEEDED(hr_SubTitle)) {
							Connect = g.pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pRawfilter);//字幕の接続
						}
					}
				}
				else {
					//LAV_Filters is installed only.
					hr = g.pCGB2->RenderStream(0, 0, pDec, 0, g.pMVR);
				}
			}
			else {
				//LAV_Video_Decorderがないということなので、代案でffdshowに接続
				GuidFromString(&CLSID_ffdshow_Video, ffdshow_Video);
				Connect = AddFilter(g.pGraph, CLSID_ffdshow_Video, L"ffdshow Decoder", &pDec);
				if (SUCCEEDED(Connect)) {
					hr = g.pCGB2->RenderStream(0, 0, pSrc, 0, pDec);
					GuidFromString(&CLSID_VSFilter, VSFilter);
					Connect = AddFilter(g.pGraph, CLSID_VSFilter, L"VSFilter", &pVSFilter);
					if (SUCCEEDED(Connect)) {
						//LAV_Filter is not installed.
						hr = g.pCGB2->RenderStream(0, 0, pDec, 0, pVSFilter);
						hr = g.pCGB2->RenderStream(0, 0, pVSFilter, 0, g.pMVR);
						HRESULT hr_SubTitle = g.pCGB2->FindPin(pSrc,
							PINDIR_OUTPUT,
							NULL,
							&CLSID_MEDIATYPE_Subtitle,
							true,
							NULL,
							&pSubtitle
						);//字幕のピンを探す
						if (SUCCEEDED(hr_SubTitle)) {
							Connect = g.pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pVSFilter);//字幕の接続
						}
					}
					else {
						//ffdshow is installed only. 
						hr = g.pCGB2->RenderStream(0, 0, pDec, 0, g.pMVR);
						HRESULT hr_SubTitle = g.pCGB2->FindPin(pSrc,
							PINDIR_OUTPUT,
							NULL,
							&CLSID_MEDIATYPE_Subtitle,
							true,
							NULL,
							&pSubtitle
						);//字幕のピンを探す
						if (SUCCEEDED(hr_SubTitle)) {
							Connect = g.pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pDec);//字幕の接続
						}
					}
				}
				else {
					//LAV_Filters or ffdshow are not installed.
					hr = g.pCGB2->RenderStream(0, 0, pSrc, 0, g.pMVR);
				}
			}
		}
		//オーディオの接続
		hr = g.pCGB2->RenderStream(0, &MEDIATYPE_Audio, pSrc, 0, 0);

		SAFE_RELEASE(pSubtitle);
		SAFE_RELEASE(pSrc);
		SAFE_RELEASE(pDec);
		SAFE_RELEASE(pRawfilter);
		SAFE_RELEASE(pVSFilter);
	}

	// 描画領域の設定
	if (SUCCEEDED(hr)) {

		//g.pVideo->GetNativeVideoSize(&g.size, NULL);
	}

	return hr;
}

//------------------------------------------------------------------------------
HRESULT InitMadVR(){

	// MadVRの作成
	HRESULT hr = CoCreateInstance(CLSID_madVR, NULL, CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&g.pMVR));
	// フィルタグラフにmadVRを追加
	if (SUCCEEDED(hr)) {
		hr = g.pGraph->AddFilter(g.pMVR, L"madVR");
	}

	IMFGetService *pService = NULL;
	if (SUCCEEDED(hr)) {
		hr = g.pMVR->QueryInterface(IID_PPV_ARGS(&pService));
	}

	if (SUCCEEDED(hr)) {
		hr = g.pMVR->QueryInterface(IID_PPV_ARGS(&MadVR.pMVRSP));
	}

	if (SUCCEEDED(hr)) {
		// Hook DXVA to have status and logging.
		//CComPtr<IDirectXVideoDecoderService> pDecoderService;
		CComPtr<IDirect3DDeviceManager9>     pDeviceManager;
		CComPtr<IDirectXVideoMemoryConfiguration> pMemoryConfig;
		HANDLE hDevice = INVALID_HANDLE_VALUE;

		//hr = pService->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&g.pVideo));

		if (SUCCEEDED(hr)) {
			hr = pService->GetService(MR_VIDEO_ACCELERATION_SERVICE, IID_PPV_ARGS(&pDeviceManager));
		}

		if (SUCCEEDED(hr)) {
			hr = pService->GetService(MR_VIDEO_ACCELERATION_SERVICE, IID_PPV_ARGS(&pMemoryConfig));
		}

		if (SUCCEEDED(hr)) {
			hr = pDeviceManager->OpenDeviceHandle(&hDevice);
		}

		if (SUCCEEDED(hr)) {
			//hr = pDeviceManager->GetVideoService(hDevice, IID_PPV_ARGS(&pDecoderService));
		}
		//hr = pService->GetService(MR_VIDEO_ACCELERATION_SERVICE, IID_PPV_ARGS(&g.pVideo));
		pDeviceManager.Release();
		pMemoryConfig.Release();
	}
	SAFE_RELEASE(pService);
	/*
	if (SUCCEEDED(hr)) {
		hr = g.pVideo->SetVideoWindow(hWnd);
	}
	*/
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
		SetWindowPos(hWnd, NULL, 0, 0, 640, 480,
			SWP_NOZORDER | SWP_NOMOVE);
	}
	GetClientRect(hWnd, &rcDst);
	return g.pWindow->SetWindowPosition(0, 0, rcDst.right - rcDst.left, rcDst.bottom - rcDst.top);
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
		if (!(g.FullScreen_Flag)) {
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
		g.Cusor = 0;
		break;
	case WM_MOUSEMOVE:
		g.Cusor_time = 1000;
		while (g.Cusor < 0) {
			g.Cusor = ShowCursor(true);
		}
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	if (g.pWindow) {
		g.pWindow->NotifyOwnerMessage((OAHWND)hWnd, uMsg, wParam, lParam);
	}
	return 0;
}
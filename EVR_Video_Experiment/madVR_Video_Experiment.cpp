#include "stdafx.h"
#include "atlbase.h"
#include "EVR_Video_Experiment.h"
#include "interfaces\mvrInterfaces.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const LPCWSTR VIDEOpass(L"D:\\CrowRabbit\\Videos\\poporon.mkv");
//������̃p�X�������ɓ��͂��āA���s���Ă݂悤!('\'�̓G�X�P�[�v�V�[�P���X�������Ă���̂Œ���)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// Unicode�����Z�b�g
#pragma comment(lib, "strmiids.lib")

#include <DShow.h>
#include <evr.h>
#include "vmr9.h"

#define SAFE_RELEASE(x) { if (x) { x->Release(); x = NULL; } }
const LPCWSTR CLASSname(L"DirectShow_madVR");
const LPCWSTR WINDOWname(L"DirectShow_madVR");

// �֐��v���g�^�C�v�錾
HRESULT OpenFile(HWND hWnd, LPCWSTR pszFileName);
HRESULT InitEvr(HWND hWnd);
HRESULT SetVideoPos(HWND hWnd, int nMode);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// �O���ϐ��\����
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
#pragma comment(lib, "rpcrt4.lib")//�Ȃ��Ɩ������̃G���[
BOOL GuidFromString
(
	GUID* pGuid
	, std::wstring oGuidString
)
{
	// �������GUID�ɕϊ�����
	if (RPC_S_OK == ::UuidFromString((RPC_WSTR)oGuidString.c_str(), (UUID*)pGuid)) {

		// �ϊ��ł��܂����B
		return(TRUE);
	}
	return(FALSE);
}

HRESULT AddFilter(
	IGraphBuilder *pGraph,  // Filter Graph Manager �ւ̃|�C���^
	const GUID& clsid,      // �쐬����t�B���^�� CLSID
	LPCWSTR wszName,        // �t�B���^�̖��O
	IBaseFilter **ppF)      // �t�B���^�ւ̃|�C���^���i�[�����
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
		// �E�B���h�E�X�^�C���ύX(���j���[�o�[�Ȃ��A�őO��)
		SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP);
		SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

		// �ő剻����
		ShowWindow(hWnd, SW_MAXIMIZE);

		// �f�B�X�v���C�T�C�Y���擾
		int mainDisplayWidth = GetSystemMetrics(SM_CXSCREEN);
		int mainDisplayHeight = GetSystemMetrics(SM_CYSCREEN);

		// �N���C�A���g�̈���f�B�X�v���[�ɍ��킹��
		SetWindowPos(hWnd, NULL, 0, 0, mainDisplayWidth, mainDisplayHeight, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE);

		SetRect(&rcDst, 0, 0, g.size.cx, g.size.cy);
		GetClientRect(hWnd, &rcDst);
		//g.pVideo->SetVideoPosition(&mvnr, &rcDst);(�K�v�Ȃ�)
	}
	else {
		g.FullScreen_Flag = false;
		SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		//SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

		// ���ʂɖ߂�
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
	g.FullScreen_Flag = false;//Flag�̏�����
							  
	hr = CoInitialize(NULL);	// COM���C�u�����̏�����
	if (FAILED(hr)) {
		return 0;
	}

	// �E�B���h�E�N���X�̓o�^
	ZeroMemory(&wcx, sizeof wcx);
	wcx.cbSize = sizeof wcx;
	wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcx.lpfnWndProc = MainWndProc;
	wcx.hInstance = hInstance;
	wcx.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);	// ������������
	wcx.lpszClassName = CLASSname;
	if (RegisterClassEx(&wcx) == 0) {
		goto Exit;
	}

	// �E�B���h�E�̍쐬
	madVRWindow = CreateWindow(CLASSname, WINDOWname,
		WS_OVERLAPPEDWINDOW,
		//		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
		100, 100, 640, 480,
		NULL, NULL, hInstance, NULL);
	if (madVRWindow == NULL) {
		goto Exit;
	}

	// DirectShow�t�B���^�̏���

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
	//Window���B
	if(g.pWindow){
		hr = g.pWindow->get_Owner((OAHWND*)madVRWindow);
		hr = g.pWindow->put_WindowStyle(WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS);
		hr = g.pWindow->put_MessageDrain((OAHWND)madVRWindow);
	}
	*/
	//HWND hWnd = GetWindow(madVRWindow, GW_CHILD);
	//EnableWindow(hWnd, false);
	ShowWindow(madVRWindow, nCmdShow);

	// ����Đ�
	hr = g.pControl->Run();
	g.nPlay = 1;

	// ���b�Z�[�W���[�v
	do {
		Sleep(1);
		if (g.Cusor >= 0) {
			if (g.Cusor_time > 0) {
				g.Cusor_time--;
			}
			else {//(g.Cusor_time == 0)�Ɠ��Ӌ` 
				//g.Cusor = ShowCursor(false);
			}
		}
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	} while (msg.message != WM_QUIT);

	// �����~
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
	// �t�B���^�O���t�̍쐬
	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&g.pGraph));

	// ���f�B�A�R���g���[���C���^�[�t�F�C�X�̎擾
	if (SUCCEEDED(hr)) {
		hr = g.pGraph->QueryInterface(IID_PPV_ARGS(&g.pControl));
	}

	if (SUCCEEDED(hr)) {
		hr = g.pGraph->QueryInterface(IID_IBasicVideo2, (void**)&g.pBase2);
	}

	if (SUCCEEDED(hr)) {
		hr = g.pGraph->QueryInterface(IID_IVideoWindow, (void**)&g.pWindow);
	}

	//ICaptureGraphBuilder2���C���X�^���X
	if (SUCCEEDED(hr)) {
		hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **)&g.pCGB2);
	}

	if (SUCCEEDED(hr)) {
		hr = g.pCGB2->SetFiltergraph(g.pGraph);//�L���v�`�� �O���t �r���_���g���t�B���^ �O���t���w�肷��B
	}

	// �r�f�I�̍쐬
	if (SUCCEEDED(hr)) {

		hr = InitMadVR();
	}

	if (SUCCEEDED(hr)) {
		//hr = g.pCGB2->RenderStream(0, 0, g.pSource, 0, g.pEvr);

		IBaseFilter *pSrc = 0, *pDec = 0, *pRawfilter = 0, *pVSFilter = 0;
		IPin *pSubtitle = 0;
		GUID CLSID_LAVVideo, CLSID_ffdshow_raw, CLSID_VSFilter, CLSID_MEDIATYPE_Subtitle, CLSID_ffdshow_Video;
		HRESULT Connect;//Directshow�̃t�B���^�̐ڑ�����(�ŏI�I�ɂ͎����̐ڑ����ۂɎg����)
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
						);//�����̃s����T��
						if (SUCCEEDED(hr_SubTitle)) {
							Connect = g.pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pVSFilter);//�����̐ڑ�
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
						);//�����̃s����T��
						if (SUCCEEDED(hr_SubTitle)) {
							Connect = g.pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pRawfilter);//�����̐ڑ�
						}
					}
				}
				else {
					//LAV_Filters is installed only.
					hr = g.pCGB2->RenderStream(0, 0, pDec, 0, g.pMVR);
				}
			}
			else {
				//LAV_Video_Decorder���Ȃ��Ƃ������ƂȂ̂ŁA��Ă�ffdshow�ɐڑ�
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
						);//�����̃s����T��
						if (SUCCEEDED(hr_SubTitle)) {
							Connect = g.pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pVSFilter);//�����̐ڑ�
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
						);//�����̃s����T��
						if (SUCCEEDED(hr_SubTitle)) {
							Connect = g.pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pDec);//�����̐ڑ�
						}
					}
				}
				else {
					//LAV_Filters or ffdshow are not installed.
					hr = g.pCGB2->RenderStream(0, 0, pSrc, 0, g.pMVR);
				}
			}
		}
		//�I�[�f�B�I�̐ڑ�
		hr = g.pCGB2->RenderStream(0, &MEDIATYPE_Audio, pSrc, 0, 0);

		SAFE_RELEASE(pSubtitle);
		SAFE_RELEASE(pSrc);
		SAFE_RELEASE(pDec);
		SAFE_RELEASE(pRawfilter);
		SAFE_RELEASE(pVSFilter);
	}

	// �`��̈�̐ݒ�
	if (SUCCEEDED(hr)) {

		//g.pVideo->GetNativeVideoSize(&g.size, NULL);
	}

	return hr;
}

//------------------------------------------------------------------------------
HRESULT InitMadVR(){

	// MadVR�̍쐬
	HRESULT hr = CoCreateInstance(CLSID_madVR, NULL, CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&g.pMVR));
	// �t�B���^�O���t��madVR��ǉ�
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
			PostMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);//�N���C�A���g�̏�ŃE�B���h�E�𓮂����B
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
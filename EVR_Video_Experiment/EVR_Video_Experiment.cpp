#include "stdafx.h"
#include "EVR_Video_Experiment.h"




//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const LPCWSTR const VIDEOpass(L"C:\\Downloads\\munou.mp4");
//������̃p�X�������ɓ��͂��āA���s���Ă݂悤!('\'�̓G�X�P�[�v�V�[�P���X�������Ă���̂Œ���)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------




// Unicode�����Z�b�g
#pragma comment(lib, "strmiids.lib")

#include <DShow.h>
#include <evr.h>

#define SAFE_RELEASE(x) { if (x) { x->Release(); x = NULL; } }
const LPCWSTR const CLASSname(L"DirectShow_EVR");
const LPCWSTR const WINDOWname(L"DirectShow_EVR");

// �֐��v���g�^�C�v�錾
HRESULT OpenFile(HWND hWnd, LPCWSTR pszFileName);
HRESULT InitEvr(HWND hWnd);
HRESULT SetVideoPos(HWND hWnd, int nMode);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// �O���ϐ��\����
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
		// �E�B���h�E�X�^�C���ύX(���j���[�o�[�Ȃ��A�őO��)
		SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP);
		SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

		// �ő剻����
		ShowWindow(hWnd, SW_MAXIMIZE);

		// �f�B�X�v���C�T�C�Y���擾
		int mainDisplayWidth = GetSystemMetrics(SM_CXSCREEN);
		int mainDisplayHeight = GetSystemMetrics(SM_CYSCREEN);

		// �N���C�A���g�̈���f�B�X�v���[�ɍ��킹��
		SetWindowPos(hWnd, NULL,0, 0, mainDisplayWidth, mainDisplayHeight,SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE);

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
	g.FullScreen_Flag = false;//Flag�̏�����
	// COM���C�u�����̏�����
	hr = CoInitialize(NULL);
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
	hWnd = CreateWindow(CLASSname, WINDOWname,
		WS_OVERLAPPEDWINDOW,
		//		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
		100, 100, 640, 480,
		NULL, NULL, hInstance, NULL);
	if (hWnd == NULL) {
		goto Exit;
	}

	// DirectShow�t�B���^�̏���

	hr = OpenFile(hWnd, VIDEOpass);
	if (FAILED(hr)) {

		goto Exit;
	}

	ShowWindow(hWnd, nCmdShow);

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
			else{//(g.Cusor_time == 0)�Ɠ��Ӌ` 
				g.Cusor = ShowCursor(false);
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
	// �t�B���^�O���t�̍쐬
	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&g.pGraph));

	// ���f�B�A�R���g���[���C���^�[�t�F�C�X�̎擾
	if (SUCCEEDED(hr)) {

		hr = g.pGraph->QueryInterface(IID_PPV_ARGS(&g.pControl));
	}

	// �r�f�I�̍쐬
	if (SUCCEEDED(hr)) {

		hr = InitEvr(hWnd);
	}
	// �O���t���쐬����
	if (SUCCEEDED(hr)) {
		//Graph(pszFile);
		hr = g.pGraph->RenderFile(pszFile, NULL);
	}

	// �`��̈�̐ݒ�
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

	// EVR�̍쐬
	HRESULT hr = CoCreateInstance(CLSID_EnhancedVideoRenderer, NULL, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&g.pEvr));

	// �t�B���^�O���t��EVR��ǉ�
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
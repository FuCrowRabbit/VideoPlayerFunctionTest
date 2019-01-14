#pragma once
#include "interfaces/mvrInterfaces.h"

#define SAFE_RELEASE(x) { if (x) { x->Release(); x = NULL; } }

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

class VideoPlayer
{
   static BOOL toGuidFromString(GUID* pGuid, std::wstring oGuidString);
   static HRESULT addFilter(
      IGraphBuilder* pGraph, // Filter Graph Manager へのポインタ
      const GUID& clsid, // 作成するフィルタの CLSID
      LPCWSTR wszName, // フィルタの名前
      IBaseFilter** ppF // フィルタへのポインタが格納される(GUID* pGuid, std::wstring oGuidString);
   );

   CComPtr<IBaseFilter> pMVR;
   CComPtr<IBasicVideo2> pBase2;
   CComPtr<IGraphBuilder> pGraph;
   CComPtr<IMediaControl> pControl;
   //IMFVideoDisplayControl *pVideo;
   CComPtr<ICaptureGraphBuilder2> pCGB2;
   CComPtr<IMFVideoMixerBitmap> pMix;
   CComPtr<IMFVideoProcessor> pProc;
   CComPtr<IMadVRSubclassReplacement> pMVRSP;

   SIZE size;
   int nPlay;
   bool FullScreen_Flag;

public:
   explicit VideoPlayer(HINSTANCE hInstance);
   ~VideoPlayer();
   WNDCLASSEX wcx;
   static LPCWSTR class_name_directshow_madvr;
   static LPCWSTR window_name;
   CComPtr<IVideoWindow> pWindow;
   int Cusor;
   unsigned int Cusor_time;

   HRESULT OpenFile(HWND hWnd, LPCWSTR pszFile);
   HRESULT OpenFileMadVR(LPCWSTR pszFile);
   HRESULT playVideo();
   HRESULT stopVideo() const;

private:
   void FullScreen(HWND hWnd);
   HRESULT InitEvr(HWND hWnd);
   HRESULT SetVideoPos(HWND hWnd, int nMode);
   static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
   HRESULT InitMadVR();
};

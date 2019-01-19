#include "stdafx.h"
#include "Video_Experiment.h"
#include "interfaces\mvrInterfaces.h"

// Unicode文字セット
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "rpcrt4.lib")//ないと未解決のエラー

#include <DShow.h>
#include <evr.h>
#include "vmr9.h"

LPCWSTR VideoPlayer::class_name_directshow_madvr(L"DirectShow_madVR");
LPCWSTR VideoPlayer::window_name(L"DirectShow_madVR");


VideoPlayer::VideoPlayer(HINSTANCE hInstance) : size{}, nPlay{ 0 }, Cusor{ 0 }, Cusor_time{ 0 }, FullScreen_Flag(false)
{
   auto hr = CoInitialize(nullptr); // COMライブラリの初期化
}

VideoPlayer::~VideoPlayer()
{
   CoUninitialize();
}

void VideoPlayer::onChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
   switch (nChar)
   {
   case VK_SPACE:
      if (this->nPlay)
      {
         this->pControl->Pause();
         this->nPlay = 0;
      }
      else
      {
         this->pControl->Run();
         this->nPlay = 1;
      }
      break;
   case 's':
      this->pControl->StopWhenReady();
      this->nPlay = 0;
      break;
   case '1': case '2': case '3': case '4':
      SetVideoPos(nChar - '0');
      break;
   case VK_ESCAPE:
      DestroyWindow();
      break;
   }
}

void VideoPlayer::onPaint(CDCHandle dc)
{
   if (this->Cusor >= 0)
   {
      if (this->Cusor_time > 0)
      {
         this->Cusor_time--;
      }
      else
      {
         //(player.Cusor_time == 0)と同意義 
         //player.Cusor = ShowCursor(false);
      }
   }
}

void VideoPlayer::onSize(UINT nType, WTL::CSize size)
{
   SetVideoPos(0);
}

void VideoPlayer::onLButtonDown(UINT nFlags, WTL::CPoint point)
{
   if (!(this->FullScreen_Flag))
   {
      PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, nFlags); //クライアントの上でウィンドウを動かす。
   }
}

void VideoPlayer::onLButtonDblClk(UINT nFlags, WTL::CPoint point)
{
   FullScreen();
}

void VideoPlayer::onDestroy()
{
   while (this->Cusor < 0)
   {
      this->Cusor = ShowCursor(true);
   }
   auto pLoop = _Module.GetMessageLoop();
   pLoop->RemoveMessageFilter(this);
   pLoop->RemoveIdleHandler(this);
   PostQuitMessage(0);
}

int VideoPlayer::onCreate(LPCREATESTRUCT lpCreateStruct)
{
   auto pLoop = _Module.GetMessageLoop();
   pLoop->AddMessageFilter(this);
   pLoop->AddIdleHandler(this);
   this->Cusor_time = 1000;
   this->Cusor = 0;
   return 0;
}

void VideoPlayer::onMouseMove(UINT nFlags, WTL::CPoint point)
{
   this->Cusor_time = 1000;
   while (this->Cusor < 0)
   {
      this->Cusor = ShowCursor(true);
   }
}

BOOL VideoPlayer::toGuidFromString(GUID* pGuid, std::wstring oGuidString)
{
   // 文字列をGUIDに変換する
   if (RPC_S_OK == ::UuidFromString((RPC_WSTR)oGuidString.c_str(), (UUID*)pGuid))
   {
      // 変換できました。
      return (TRUE);
   }
   return (FALSE);
}

HRESULT VideoPlayer::addFilter(
   IGraphBuilder* pGraph, // Filter Graph Manager へのポインタ
   const GUID& clsid, // 作成するフィルタの CLSID
   LPCWSTR wszName, // フィルタの名前
   IBaseFilter** ppF) // フィルタへのポインタが格納される
{
   *ppF = 0;
   IBaseFilter* pF = 0;
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

void VideoPlayer::FullScreen()
{
   MFVideoNormalizedRect mvnr = { 0.0f, 0.0f, 1.0f, 1.0f };
   RECT rcDst;
   if (!(this->FullScreen_Flag))
   {
      this->FullScreen_Flag = true;
      // ウィンドウスタイル変更(メニューバーなし、最前面)
      SetWindowLongPtr(GWL_STYLE, WS_POPUP);
      SetWindowLongPtr(GWL_EXSTYLE, WS_EX_TOPMOST);

      // 最大化する
      ShowWindow(SW_MAXIMIZE);

      // ディスプレイサイズを取得
      int mainDisplayWidth = GetSystemMetrics(SM_CXSCREEN);
      int mainDisplayHeight = GetSystemMetrics(SM_CYSCREEN);

      // クライアント領域をディスプレーに合わせる
      SetWindowPos(NULL, 0, 0, mainDisplayWidth, mainDisplayHeight, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE);

      SetRect(&rcDst, 0, 0, this->size.cx, this->size.cy);
      GetClientRect(&rcDst);
      //this->pVideo->SetVideoPosition(&mvnr, &rcDst);(必要ない)
   }
   else
   {
      this->FullScreen_Flag = false;
      SetWindowLongPtr(GWL_STYLE, WS_OVERLAPPEDWINDOW);
      //SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

      // 普通に戻す
      ShowWindow(SW_RESTORE);
      SetVideoPos(1);
   }
}

//------------------------------------------------------------------------------
HRESULT VideoPlayer::OpenFileMadVR(LPCWSTR pszFile)
{
   // フィルタグラフの作成
   HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
      IID_PPV_ARGS(&this->pGraph));

   // メディアコントロールインターフェイスの取得
   if (SUCCEEDED(hr))
   {
      hr = this->pGraph->QueryInterface(IID_PPV_ARGS(&this->pControl));
   }

   if (SUCCEEDED(hr))
   {
      hr = this->pGraph->QueryInterface(IID_IBasicVideo2, (void**)&this->pBase2);
   }

   if (SUCCEEDED(hr))
   {
      hr = this->pGraph->QueryInterface(IID_IVideoWindow, (void**)&this->pWindow);
   }

   //ICaptureGraphBuilder2をインスタンス
   if (SUCCEEDED(hr))
   {
      hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2,
         (void **)&this->pCGB2);
   }

   if (SUCCEEDED(hr))
   {
      hr = this->pCGB2->SetFiltergraph(this->pGraph); //キャプチャ グラフ ビルダが使うフィルタ グラフを指定する。
   }

   // ビデオの作成
   if (SUCCEEDED(hr))
   {
      hr = InitMadVR();
   }

   if (SUCCEEDED(hr))
   {
      //hr = this->pCGB2->RenderStream(0, 0, this->pSource, 0, this->pEvr);

      IBaseFilter *pSrc = 0, *pDec = 0, *pRawfilter = 0, *pVSFilter = 0;
      IPin* pSubtitle = 0;
      GUID CLSID_LAVVideo, CLSID_ffdshow_raw, CLSID_VSFilter, CLSID_MEDIATYPE_Subtitle, CLSID_ffdshow_Video;
      HRESULT Connect; //Directshowのフィルタの接続安否(最終的には字幕の接続安否に使われる)
      const std::wstring LAV_Video(L"EE30215D-164F-4A92-A4EB-9D4C13390F9F");
      const std::wstring ffdshow_Video(L"04FE9017-F873-410E-871E-AB91661A4EF7");
      const std::wstring ffdshow_Raw(L"0B390488-D80F-4A68-8408-48DC199F0E97");
      const std::wstring VSFilter(L"93A22E7A-5091-45EF-BA61-6DA26156A5D0");
      const std::wstring MEDIATYPE_Subtitle(L"E487EB08-6B26-4BE9-9DD3-993434D313FD");

      toGuidFromString(&CLSID_MEDIATYPE_Subtitle, MEDIATYPE_Subtitle);
      hr = this->pGraph->AddSourceFilter(pszFile, pszFile, &pSrc);
      if (SUCCEEDED(hr))
      {
         toGuidFromString(&CLSID_LAVVideo, LAV_Video);
         Connect = addFilter(this->pGraph, CLSID_LAVVideo, L"LAV Video Decoder", &pDec);
         if (SUCCEEDED(Connect))
         {
            hr = this->pCGB2->RenderStream(0, 0, pSrc, 0, pDec);
            toGuidFromString(&CLSID_ffdshow_raw, ffdshow_Raw);
            Connect = addFilter(this->pGraph, CLSID_ffdshow_raw, L"ffdshow raw video filter", &pRawfilter);
            if (SUCCEEDED(Connect))
            {
               hr = this->pCGB2->RenderStream(0, 0, pDec, 0, pRawfilter);
               toGuidFromString(&CLSID_VSFilter, VSFilter);
               Connect = addFilter(this->pGraph, CLSID_VSFilter, L"VSFilter", &pVSFilter);
               if (SUCCEEDED(Connect))
               {
                  //Full installed.
                  hr = this->pCGB2->RenderStream(0, 0, pRawfilter, 0, pVSFilter);
                  hr = this->pCGB2->RenderStream(0, 0, pVSFilter, 0, this->pMVR);
                  HRESULT hr_SubTitle = this->pCGB2->FindPin(pSrc,
                     PINDIR_OUTPUT,
                     NULL,
                     &CLSID_MEDIATYPE_Subtitle,
                     true,
                     NULL,
                     &pSubtitle
                  ); //字幕のピンを探す
                  if (SUCCEEDED(hr_SubTitle))
                  {
                     Connect = this->pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pVSFilter); //字幕の接続
                  }
               }
               else
               {
                  //VSFilter is not installed.
                  hr = this->pCGB2->RenderStream(0, 0, pRawfilter, 0, this->pMVR);
                  HRESULT hr_SubTitle = this->pCGB2->FindPin(pSrc,
                     PINDIR_OUTPUT,
                     NULL,
                     &CLSID_MEDIATYPE_Subtitle,
                     true,
                     NULL,
                     &pSubtitle
                  ); //字幕のピンを探す
                  if (SUCCEEDED(hr_SubTitle))
                  {
                     Connect = this->pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pRawfilter); //字幕の接続
                  }
               }
            }
            else
            {
               //LAV_Filters is installed only.
               hr = this->pCGB2->RenderStream(0, 0, pDec, 0, this->pMVR);
            }
         }
         else
         {
            //LAV_Video_Decorderがないということなので、代案でffdshowに接続
            toGuidFromString(&CLSID_ffdshow_Video, ffdshow_Video);
            Connect = addFilter(this->pGraph, CLSID_ffdshow_Video, L"ffdshow Decoder", &pDec);
            if (SUCCEEDED(Connect))
            {
               hr = this->pCGB2->RenderStream(0, 0, pSrc, 0, pDec);
               toGuidFromString(&CLSID_VSFilter, VSFilter);
               Connect = addFilter(this->pGraph, CLSID_VSFilter, L"VSFilter", &pVSFilter);
               if (SUCCEEDED(Connect))
               {
                  //LAV_Filter is not installed.
                  hr = this->pCGB2->RenderStream(0, 0, pDec, 0, pVSFilter);
                  hr = this->pCGB2->RenderStream(0, 0, pVSFilter, 0, this->pMVR);
                  HRESULT hr_SubTitle = this->pCGB2->FindPin(pSrc,
                     PINDIR_OUTPUT,
                     NULL,
                     &CLSID_MEDIATYPE_Subtitle,
                     true,
                     NULL,
                     &pSubtitle
                  ); //字幕のピンを探す
                  if (SUCCEEDED(hr_SubTitle))
                  {
                     Connect = this->pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pVSFilter); //字幕の接続
                  }
               }
               else
               {
                  //ffdshow is installed only. 
                  hr = this->pCGB2->RenderStream(0, 0, pDec, 0, this->pMVR);
                  HRESULT hr_SubTitle = this->pCGB2->FindPin(pSrc,
                     PINDIR_OUTPUT,
                     NULL,
                     &CLSID_MEDIATYPE_Subtitle,
                     true,
                     NULL,
                     &pSubtitle
                  ); //字幕のピンを探す
                  if (SUCCEEDED(hr_SubTitle))
                  {
                     Connect = this->pCGB2->RenderStream(0, &CLSID_MEDIATYPE_Subtitle, pSrc, 0, pDec); //字幕の接続
                  }
               }
            }
            else
            {
               //LAV_Filters or ffdshow are not installed.
               hr = this->pCGB2->RenderStream(0, 0, pSrc, 0, this->pMVR);
            }
         }
      }
      //オーディオの接続
      hr = this->pCGB2->RenderStream(0, &MEDIATYPE_Audio, pSrc, 0, 0);

      SAFE_RELEASE(pSubtitle);
      SAFE_RELEASE(pSrc);
      SAFE_RELEASE(pDec);
      SAFE_RELEASE(pRawfilter);
      SAFE_RELEASE(pVSFilter);
   }

   // 描画領域の設定
   if (SUCCEEDED(hr))
   {
      //this->pVideo->GetNativeVideoSize(&this->size, NULL);
   }

   return hr;
}

HRESULT VideoPlayer::playVideo()
{
   const auto hr = this->pControl->Run();
   if (SUCCEEDED(hr))
   {
      this->nPlay = 1;
      return hr;
   }
   this->nPlay = 0;
   return hr;
}

HRESULT VideoPlayer::stopVideo() const
{
   return this->pControl->Stop();
}

//------------------------------------------------------------------------------
HRESULT VideoPlayer::InitMadVR()
{
   // MadVRの作成
   HRESULT hr = CoCreateInstance(CLSID_madVR, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&this->pMVR));
   // フィルタグラフにmadVRを追加
   if (SUCCEEDED(hr))
   {
      hr = this->pGraph->AddFilter(this->pMVR, L"madVR");
   }

   IMFGetService* pService = NULL;
   if (SUCCEEDED(hr))
   {
      hr = this->pMVR->QueryInterface(IID_PPV_ARGS(&pService));
   }

   if (SUCCEEDED(hr))
   {
      hr = this->pMVR->QueryInterface(IID_PPV_ARGS(&this->pMVRSP));
   }

   if (SUCCEEDED(hr))
   {
      // Hook DXVA to have status and logginthis->
      //CComPtr<IDirectXVideoDecoderService> pDecoderService;
      CComPtr<IDirect3DDeviceManager9> pDeviceManager;
      CComPtr<IDirectXVideoMemoryConfiguration> pMemoryConfig;
      HANDLE hDevice = INVALID_HANDLE_VALUE;

      //hr = pService->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&this->pVideo));

      if (SUCCEEDED(hr))
      {
         hr = pService->GetService(MR_VIDEO_ACCELERATION_SERVICE, IID_PPV_ARGS(&pDeviceManager));
      }

      if (SUCCEEDED(hr))
      {
         hr = pService->GetService(MR_VIDEO_ACCELERATION_SERVICE, IID_PPV_ARGS(&pMemoryConfig));
      }

      if (SUCCEEDED(hr))
      {
         hr = pDeviceManager->OpenDeviceHandle(&hDevice);
      }

      if (SUCCEEDED(hr))
      {
         //hr = pDeviceManager->GetVideoService(hDevice, IID_PPV_ARGS(&pDecoderService));
      }
      //hr = pService->GetService(MR_VIDEO_ACCELERATION_SERVICE, IID_PPV_ARGS(&this->pVideo));
      pDeviceManager.Release();
      pMemoryConfig.Release();
   }
   SAFE_RELEASE(pService);
   /*
   if (SUCCEEDED(hr)) {
      hr = this->pVideo->SetVideoWindow(hWnd);
   }
   */
   return hr;
}

//------------------------------------------------------------------------------

HRESULT VideoPlayer::SetVideoPos(int nMode)
{
   MFVideoNormalizedRect mvnr = { 0.0f, 0.0f, 1.0f, 1.0f };
   RECT rcDst;

   if (1 <= nMode && nMode <= 4)
   {
      SetRect(&rcDst, 0, 0, this->size.cx * nMode / 2, this->size.cy * nMode / 2);
      AdjustWindowRectEx(&rcDst, WS_OVERLAPPEDWINDOW, FALSE, 0);
      SetWindowPos(NULL, 0, 0, 640, 480, SWP_NOZORDER | SWP_NOMOVE);
   }
   GetClientRect(&rcDst);
   return this->pWindow->SetWindowPosition(0, 0, rcDst.right - rcDst.left, rcDst.bottom - rcDst.top);
}

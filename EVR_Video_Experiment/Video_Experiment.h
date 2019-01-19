#pragma once
#include "stdafx.h"
#include "interfaces/mvrInterfaces.h"

#define SAFE_RELEASE(x) { if (x) { x->Release(); x = NULL; } }

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

class VideoPlayer final : public CWindowImpl<VideoPlayer>, public CMessageFilter, public CIdleHandler
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

   void onChar(UINT nChar, UINT nRepCnt, UINT nFlags);
   void onPaint(CDCHandle dc);
   void onSize(UINT nType, WTL::CSize size);
   void onLButtonDown(UINT nFlags, WTL::CPoint point);
   void onLButtonDblClk(UINT nFlags, WTL::CPoint point);
   void onDestroy();
   int onCreate(LPCREATESTRUCT lpCreateStruct);
   void onMouseMove(UINT nFlags, WTL::CPoint point);

   // メッセージフィルタ処理
   BOOL PreTranslateMessage(MSG* pMsg) override
   {
      return FALSE;
   }

   // アイドル処理
   BOOL OnIdle() override
   {
      return FALSE;
   }

   // メッセージマップ
   BEGIN_MSG_MAP_EX(VideoPlayer)
      MSG_WM_CHAR(onChar)
      MSG_WM_PAINT(onPaint)
      MSG_WM_SIZE(onSize)
      MSG_WM_LBUTTONDOWN(onLButtonDown)
      MSG_WM_LBUTTONDBLCLK(onLButtonDblClk)
      MSG_WM_DESTROY(onDestroy)
      MSG_WM_CREATE(onCreate)
      MSG_WM_MOUSEMOVE(onMouseMove)
      END_MSG_MAP()

   static CWndClassInfo& GetWndClassInfo()
   {
      static CWndClassInfo wc =
      {
         {
            sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
            StartWindowProc, 0, 0, NULL, NULL, LoadCursor(nullptr, MAKEINTRESOURCE(IDC_ARROW)),
            reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), NULL,
            VideoPlayer::class_name_directshow_madvr, NULL
         }, // WNDCLASSEX構造体
         NULL, // 既存のウィンドウクラス名
         NULL, // 既存のウィンドウプロシージャ
         NULL, // カーソルリソース名
         // システムカーソルならばTRUE、それ以外はFALSE
         FALSE,
         0, // 登録済みウィンドウクラスの識別子
         _T("") // ATLが自動生成したウィンドウクラス名
      };
      return wc;
   }

   HRESULT OpenFile(HWND hWnd, LPCWSTR pszFile);
   HRESULT OpenFileMadVR(LPCWSTR pszFile);
   HRESULT playVideo();
   HRESULT stopVideo() const;

private:
   void FullScreen();
   HRESULT InitEvr();
   HRESULT SetVideoPos(int nMode);
   HRESULT InitMadVR();
};

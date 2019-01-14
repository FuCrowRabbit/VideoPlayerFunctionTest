#include "stdafx.h"
#include "atlbase.h"
#include "Video_Experiment.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   MSG msg = { nullptr };
   auto hr = CoInitialize(nullptr); // COMライブラリの初期化

   auto player = VideoPlayer(hInstance);

   if (FAILED(hr)) return 0;

   // ウィンドウの作成
   HWND madVRWindow = nullptr;
   madVRWindow = CreateWindow(VideoPlayer::class_name_directshow_madvr, VideoPlayer::window_name,
      WS_OVERLAPPEDWINDOW,
      //    CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
      100, 100, 640, 480,
      NULL, NULL, hInstance, NULL);

   // DirectShowフィルタの準備
   //------------------------------------------------------------------------------
   //------------------------------------------------------------------------------
   const auto video_path(L"D:\\CrowRabbit\\Videos\\poporon.mkv");
   //↑動画のパスをここに入力して、実行してみよう!('\'はエスケープシーケンスが働いているので注意)
   //------------------------------------------------------------------------------
   //------------------------------------------------------------------------------
   hr = player.OpenFileMadVR(video_path);

   //hr = player.pWindow->put_AutoShow(OAFALSE);
   hr = player.pWindow->put_Owner((OAHWND)madVRWindow);
   hr = player.pWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
   hr = player.pWindow->SetWindowPosition(0, 0, 640, 480);
   hr = player.pWindow->put_MessageDrain((OAHWND)madVRWindow);
   hr = player.pWindow->SetWindowForeground(OATRUE);
   hr = player.pWindow->put_Visible(OATRUE);
   /*
   //Window調達
   if(player.pWindow){
      hr = player.pWindow->get_Owner((OAHWND*)madVRWindow);
      hr = player.pWindow->put_WindowStyle(WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS);
      hr = player.pWindow->put_MessageDrain((OAHWND)madVRWindow);
   }
   */
   //HWND hWnd = GetWindow(madVRWindow, GW_CHILD);
   //EnableWindow(hWnd, false);
   ShowWindow(madVRWindow, nCmdShow);

   // 動画再生
   hr = player.playVideo();

   // メッセージループ
   do
   {
      Sleep(1);
      if (player.Cusor >= 0)
      {
         if (player.Cusor_time > 0)
         {
            player.Cusor_time--;
         }
         else
         {
            //(player.Cusor_time == 0)と同意義 
            //player.Cusor = ShowCursor(false);
         }
      }
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   } while (msg.message != WM_QUIT);

   // 動画停止
   hr = player.stopVideo();
   Sleep(1000);

   CoUninitialize();
}

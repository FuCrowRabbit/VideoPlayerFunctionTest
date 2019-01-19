#include "stdafx.h"
#include "Video_Experiment.h"

CAppModule _Module;  // CComModuleからCAppModuleに置き換える

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   // WTL
   _Module.Init(nullptr, hInstance);

   CMessageLoop theLoop;
   _Module.AddMessageLoop(&theLoop);

   auto hr = CoInitialize(nullptr); // COMライブラリの初期化

   auto player = VideoPlayer(hInstance);

   //------------------------------------------------------------------------------
   //------------------------------------------------------------------------------
   const auto video_path(L"D:\\CrowRabbit\\Videos\\poporon.mkv");
   //↑動画のパスをここに入力して、実行してみよう!('\'はエスケープシーケンスが働いているので注意)
   //------------------------------------------------------------------------------
   //------------------------------------------------------------------------------
   hr = player.OpenFileMadVR(video_path);

   player.Create(nullptr, CWindow::rcDefault,
      VideoPlayer::class_name_directshow_madvr, WS_OVERLAPPEDWINDOW | WS_VISIBLE);

   hr = player.pWindow->put_Owner(reinterpret_cast<OAHWND>(player.m_hWnd));
   hr = player.pWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
   hr = player.pWindow->put_MessageDrain(reinterpret_cast<OAHWND>(player.m_hWnd));
   hr = player.pWindow->SetWindowForeground(OATRUE);
   hr = player.pWindow->put_Visible(OATRUE);

   ShowWindow(player.m_hWnd, nCmdShow);

   // 動画再生
   hr = player.playVideo();

   auto nRet = theLoop.Run();

   // 動画停止
   hr = player.stopVideo();
   Sleep(1000);

   _Module.RemoveMessageLoop();

   _Module.Term();
}

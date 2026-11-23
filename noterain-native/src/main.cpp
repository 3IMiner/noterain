#include <dwmapi.h>
#include <shellapi.h>
#include <windows.h>

#include <memory>

#include "app/app.h"

namespace {

nr::App* gApp = nullptr;
float gDpiScale = 1.0f;

float queryDpiScale() {
  HDC dc = GetDC(nullptr);
  const int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSX) : 96;
  if (dc) ReleaseDC(nullptr, dc);
  return dpi > 0 ? (float)dpi / 96.0f : 1.0f;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_SIZE: {
      if (gApp && wParam != SIZE_MINIMIZED) {
        gApp->resize((int)LOWORD(lParam), (int)HIWORD(lParam), gDpiScale);
      }
      return 0;
    }
    case WM_ERASEBKGND:
      return 1; // the canvas paints every pixel
    case WM_PAINT: {
      PAINTSTRUCT ps{};
      BeginPaint(hwnd, &ps);
      EndPaint(hwnd, &ps);
      return 0;
    }
    case WM_MOUSEMOVE: {
      if (gApp) {
        gApp->onMouseMove((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
      }
      return 0;
    }
    case WM_LBUTTONDOWN: {
      SetCapture(hwnd);
      SetFocus(hwnd);
      if (gApp) {
        gApp->onMouseMove((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
        gApp->onMouseDown();
      }
      return 0;
    }
    case WM_LBUTTONUP: {
      ReleaseCapture();
      if (gApp) {
        gApp->onMouseMove((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
        gApp->onMouseUp();
      }
      return 0;
    }
    case WM_MOUSEWHEEL: {
      if (gApp) gApp->onMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
      return 0;
    }
    case WM_KEYDOWN: {
      const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
      if (gApp) gApp->onKeyDown((int)wParam, ctrl);
      return 0;
    }
    case WM_DROPFILES: {
      HDROP drop = (HDROP)wParam;
      const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
      if (gApp && count > 0) {
        wchar_t path[MAX_PATH * 2]{};
        if (DragQueryFileW(drop, 0, path, (UINT)std::size(path))) {
          gApp->onDropFile(std::wstring(path));
        }
      }
      DragFinish(drop);
      return 0;
    }
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    default:
      break;
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
  SetProcessDPIAware();
  gDpiScale = queryDpiScale();

  WNDCLASSEXW wc{};
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(101));
  wc.hIconSm = wc.hIcon;
  wc.lpszClassName = L"NoteRainNativeWindow";
  if (!RegisterClassExW(&wc)) return 1;

  RECT frame{0, 0, 1440, 900};
  AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);
  HWND hwnd = CreateWindowExW(0, L"NoteRainNativeWindow", L"NoteRain", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, frame.right - frame.left,
                              frame.bottom - frame.top, nullptr, nullptr, hInstance, nullptr);
  if (!hwnd) return 1;

  DragAcceptFiles(hwnd, TRUE);

  gApp = new nr::App();
  if (!gApp->init(hwnd)) {
    MessageBoxW(hwnd, L"Direct2D initialisation failed.", L"NoteRain", MB_ICONERROR | MB_OK);
    delete gApp;
    gApp = nullptr;
    DestroyWindow(hwnd);
    return 1;
  }

  RECT client{};
  GetClientRect(hwnd, &client);
  gApp->resize(client.right - client.left, client.bottom - client.top, gDpiScale);

  ShowWindow(hwnd, nCmdShow == 0 ? SW_SHOWDEFAULT : nCmdShow);
  UpdateWindow(hwnd);

  MSG msg{};
  bool running = true;
  while (running) {
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        running = false;
        break;
      }
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
    if (!running) break;

    if (IsIconic(hwnd)) {
      Sleep(30);
      continue;
    }
    if (gApp) {
      gApp->frame();
      DwmFlush(); // pace to the compositor (≈60 fps)
    }
  }

  if (gApp) {
    gApp->shutdown();
    delete gApp;
    gApp = nullptr;
  }
  return 0;
}

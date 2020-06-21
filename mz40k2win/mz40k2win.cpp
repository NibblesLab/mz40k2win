// mz40k2win.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include <mmsystem.h>
#include "mz40k2win.h"

#define MAX_LOADSTRING 100

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: ここにコードを挿入してください。

    // グローバル文字列を初期化する
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MZ40K2WIN, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // アプリケーション初期化の実行:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MZ40K2WIN));

    MSG msg;

    // メイン メッセージ ループ:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    HBITMAP hBitmap;
    HBRUSH hBackGround;

    hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BASE));
    hBackGround = CreatePatternBrush(hBitmap);

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MZ40K2WIN));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = hBackGround; //(HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // グローバル変数にインスタンス ハンドルを格納する

   HWND hWnd = CreateWindowW(szWindowClass, 0, WS_POPUP,
      100, 100, 800, 180, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウを描画する
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc, hBuffer;
    static HBITMAP hKeyBmp[32];
    POINT po;
    HMENU tmp, hTMenu;
    static HMENU hMenu, hSubMenu;
    MENUITEMINFOW mii;
    MIDIOUTCAPS outCaps;
    MMRESULT res;
    WCHAR str[MAXERRORLENGTH];
    static UINT opendMidiDevId;
    static HMIDIOUT hMidiOut;

    switch (message)
    {
    case WM_CREATE:
        // キーを配置
        for (UINT num = 0; num < 32; num++)
        {
            hKeyBmp[num] = (HBITMAP)LoadImage(((LPCREATESTRUCT)(lParam))->hInstance, MAKEINTRESOURCE(IDB_KEY40 + num), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE);
            UINT id = num + 200;
            CreateWindow(
                TEXT("BUTTON"), TEXT(""),
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                keyX[num], keyY[num], 40, 40, hWnd, (HMENU)id,
                ((LPCREATESTRUCT)(lParam))->hInstance, NULL
            );
        }
        // メニューの設定
        hMenu = CreatePopupMenu();
        hSubMenu = CreateMenu();
        mii.cbSize = sizeof(MENUITEMINFOW);
        mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU;
        mii.hSubMenu = hSubMenu;
        mii.fState = MFS_ENABLED;
        mii.fType = MFT_STRING;
        mii.dwTypeData = (LPWSTR)TEXT("connect MIDI...");
        mii.wID = IDM_MIDI;
        InsertMenuItemW(hMenu, 0, TRUE, &mii);
        mii.fMask = MIIM_TYPE;
        mii.fType = MFT_SEPARATOR;
        InsertMenuItemW(hMenu, 1, TRUE, &mii);
        mii.fMask = MIIM_TYPE | MIIM_ID;
        mii.fType = MFT_STRING;
        mii.dwTypeData = (LPWSTR)TEXT("About...");
        mii.wID = IDM_ABOUT;
        InsertMenuItemW(hMenu, 2, TRUE, &mii);
        mii.dwTypeData = (LPWSTR)TEXT("Exit");
        mii.wID = IDM_EXIT;
        InsertMenuItemW(hMenu, 3, TRUE, &mii);
        for (UINT i = 0; ; i++)
        {
            res = midiOutGetDevCaps(i, &outCaps, sizeof(outCaps));
            if (res == MMSYSERR_NOERROR)
            {
                wcscpy_s(str, MAXERRORLENGTH, (const WCHAR*)outCaps.szPname);
                mii.fMask = MIIM_TYPE | MIIM_ID;
                mii.dwTypeData = str;
                mii.wID = IDM_MIDIOUT + i;
                mii.fState = MFS_ENABLED;
            }
            else if (i == 0)
            {
                mii.fMask = MIIM_TYPE;
                mii.dwTypeData = (LPWSTR)TEXT("None");
                mii.fState = MFS_GRAYED;
            }
            else
            {
                break;
            }
            InsertMenuItemW(hSubMenu, i, TRUE, &mii);
        }
        // MIDI出力デバイス管理変数の初期化
        opendMidiDevId = 0;
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 選択されたメニューの解析:
            if (wmId < IDM_MIDIOUT)
            {
                switch (wmId)
                {
                case IDM_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    break;
                case IDM_EXIT:
                    // 使用中のMIDIデバイスのクローズ
                    if (opendMidiDevId != 0)
                    {
                        midiOutReset(hMidiOut);
                        midiOutClose(hMidiOut);
                    }
                    DestroyWindow(hWnd);
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
                }
            }
            else
            {
                // 既にオープンしているデバイスでなければオープンする
                if (wmId != opendMidiDevId)
                {
                    // 使用不可デバイスでなければオープンする
                    mii.cbSize = sizeof(MENUITEMINFOW);
                    mii.fMask = MIIM_STATE;
                    GetMenuItemInfo(hSubMenu, opendMidiDevId, FALSE, &mii);
                    if (!(mii.fState & MFS_GRAYED))
                    {
                        // 使用中のデバイスのクローズ
                        if (opendMidiDevId != 0)
                        {
                            midiOutReset(hMidiOut);
                            midiOutClose(hMidiOut);
                        }

                        // デバイスのオープン
                        res = midiOutOpen(&hMidiOut, wmId - IDM_MIDIOUT, 0, 0, 0);
                        if (res != MMSYSERR_NOERROR)
                        {
                            // エラー発生時、メッセージを表示する
                            midiOutGetErrorText(res, str, sizeof(str));
                            MessageBox(hWnd, str, TEXT("ERROR"), MB_ICONSTOP | MB_OK);

                            // 以前選択していたデバイスがあれば、再度オープンする
                            if (opendMidiDevId != 0)
                            { 
                                // デバイスのオープン
                                res = midiOutOpen(&hMidiOut, opendMidiDevId - IDM_MIDIOUT, 0, 0, 0);
                                if (res != MMSYSERR_NOERROR)
                                { 
                                    // なぜかエラーになるのでメニューのチェックマークを消す
                                    mii.cbSize = sizeof(MENUITEMINFOW);
                                    mii.fMask = MIIM_STATE;
                                    GetMenuItemInfo(hSubMenu, opendMidiDevId, FALSE, &mii);
                                    mii.fState = MFS_ENABLED;
                                    SetMenuItemInfo(hSubMenu, opendMidiDevId, FALSE, &mii);
                                    // 非選択状態
                                    opendMidiDevId = 0;
                                }
                            }
                        }
                        else
                        {
                            // メニューのチェックマークを消す
                            mii.fState = MFS_ENABLED;
                            SetMenuItemInfo(hSubMenu, opendMidiDevId, FALSE, &mii);

                            // メニューのオープンしたデバイスにチェックマークをつける
                            opendMidiDevId = wmId;
                            GetMenuItemInfo(hSubMenu, wmId, FALSE, &mii);
                            mii.fState = MFS_CHECKED;
                            SetMenuItemInfo(hSubMenu, wmId, FALSE, &mii);
                        }
                    }
                }
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: HDC を使用する描画コードをここに追加してください...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DRAWITEM:
        {
            int wmId = LOWORD(wParam);
            if (wmId >= IDB_KEY40 && wmId <= IDB_KEY71)
            {
                hdc = ((LPDRAWITEMSTRUCT)(lParam))->hDC;
                hBuffer = CreateCompatibleDC(hdc);
                SelectObject(hBuffer, hKeyBmp[wmId - 200]);

                if (((LPDRAWITEMSTRUCT)(lParam))->itemState & ODS_SELECTED)
                {
                    BitBlt(hdc, 0, 0, 40, 40, hBuffer, 0, 40, SRCCOPY);
                }
                else
                {
                    BitBlt(hdc, 0, 0, 40, 40, hBuffer, 0, 0, SRCCOPY);
                }

            DeleteDC(hBuffer);
            }
        }
        return TRUE;
    case WM_RBUTTONUP:
        po.x = LOWORD(lParam);
        po.y = HIWORD(lParam);
        ClientToScreen(hWnd, &po);
        TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN,
            po.x, po.y, 0, hWnd, NULL
        );
        break;
    case WM_LBUTTONDOWN:
        ReleaseCapture();
        SendMessage(hWnd, WM_SYSCOMMAND, SC_MOVE | 2, 0);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

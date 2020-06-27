// mz40k2win.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include <mmsystem.h>
#include <commctrl.h>
#include "mz40k2win.h"

#define MAX_LOADSTRING 100
#define MAX_NOTE_CNT  8

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名
WCHAR NoteCnt = 0;
WCHAR NoteBuf[MAX_NOTE_CNT];
UINT opendMidiDevId, midiCh;
HMIDIOUT hMidiOut;
HBITMAP hKeyBmp[32];
HWND hButton[32];

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
// キーオン
//
void keyOn(UINT note)
{
    if (opendMidiDevId)
    {
        midiOutShortMsg(hMidiOut, 0x007f0090 | (note << 8) + midiCh);
    }
}

//
// キーオフ
//
void keyOff(UINT note)
{
    if (opendMidiDevId)
    {
        midiOutShortMsg(hMidiOut, 0x00000090 | (note << 8) + midiCh);
    }
}

//
// ノートオフ
//    キーを押した順を記憶し、離した時の次の音を逆順で決める
//
void NoteOff(UINT note)
{
    bool flg = false;

    // バッファに最後に登録されている音(今鳴っている)か
    if (NoteBuf[NoteCnt - 1] == note)
    {
        // 今鳴らしてるのなら止めておく
        keyOff(NoteBuf[NoteCnt - 1]);
        // リストを詰める
        NoteCnt--;
    }
    else
    {
        // リストから探して削除
        for (UINT i = 0; i < NoteCnt; i++)
        {
            if (flg)
            {
                NoteBuf[i - 1] = NoteBuf[i];
            }
            if (NoteBuf[i] == note)
            {
                flg = true;
            }
        }
        if (flg)
        {
            NoteCnt--;
            return;
        }
    }

    if (NoteCnt != 0)
    {
        // 二番目に新しかった音を鳴らす
        keyOn(NoteBuf[NoteCnt - 1]);
    }
}

//
// ノートオン処理
//    後着優先で音を鳴らす
//
void NoteOn(UINT note)
{
    // バッファに登録済のノートは無視
    for (UINT i = 0; i < NoteCnt; i++)
    {
        if (NoteBuf[i] == note)
        {
            return;
        }
    }

    // バッファがいっぱい？
    if (NoteCnt == MAX_NOTE_CNT)
    {
        // 玉突き処理
        for (UINT i = 0; i < (MAX_NOTE_CNT - 1); i++) {
            NoteBuf[i] = NoteBuf[i + 1];
        }
        NoteBuf[MAX_NOTE_CNT - 1] = note;
    }
    else
    {
        NoteBuf[NoteCnt] = note;
        NoteCnt++;
    }

    // 今鳴ってる音があるなら消す
    if (NoteCnt > 1)
    {
        keyOff(NoteBuf[NoteCnt - 2]);
    }
    // 音を出す
    keyOn(note);
}

//
// キー押下処理
//
void keyDown(WPARAM wParam)
{
    HDC hdc, hBuffer;
    UINT note;
    UINT wmId = LOWORD(wParam);
    if (wmId >= 0x41 && wmId <= 0x5a)
    {
        note = keyNote[wmId - 0x41];
    }
    else if (wmId >= 0xba && wmId <= 0xbf)
    {
        note = keyNote[wmId - 0xa0];
    }
    else if (wmId == 0xe2)
    {
        note = keyNote[0x20];
    }
    else if (wmId == 0xdd)
    {
        note = keyNote[0x21];
    }
    else
    {
        return;
    }
    if (note != 0)
    {
        NoteOn(note);
        hdc = GetDC(hButton[note - 40]);
        hBuffer = CreateCompatibleDC(hdc);
        SelectObject(hBuffer, hKeyBmp[note - 40]);
        BitBlt(hdc, 0, 0, 40, 40, hBuffer, 0, 40, SRCCOPY);
        DeleteDC(hBuffer);
    }
}

//
// キー離上処理
//
void keyUp(WPARAM wParam)
{
    HDC hdc, hBuffer;
    UINT note;
    UINT wmId = LOWORD(wParam);
    if (wmId >= 0x41 && wmId <= 0x5a)
    {
        note = keyNote[wmId - 0x41];
    }
    else if (wmId >= 0xba && wmId <= 0xbf)
    {
        note = keyNote[wmId - 0xa0];
    }
    else if (wmId == 0xe2)
    {
        note = keyNote[0x20];
    }
    else if (wmId == 0xdd)
    {
        note = keyNote[0x21];
    }
    else
    {
        return;
    }
    if (note != 0)
    {
        NoteOff(note);
        hdc = GetDC(hButton[note - 40]);
        hBuffer = CreateCompatibleDC(hdc);
        SelectObject(hBuffer, hKeyBmp[note - 40]);
        BitBlt(hdc, 0, 0, 40, 40, hBuffer, 0, 0, SRCCOPY);
        DeleteDC(hBuffer);
    }
}

//
// ボタン入力のサブプロシージャ(全ボタン共通)
//
LRESULT CALLBACK keyPressProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (message)
    {
    case WM_LBUTTONDOWN:
        NoteOn(dwRefData);
        break;
    case WM_LBUTTONUP:
        NoteOff(dwRefData);
        break;
    case WM_KEYDOWN:
        keyDown(wParam);
        break;
    case WM_KEYUP:
        keyUp(wParam);
        break;
    }

    return DefSubclassProc(hWnd, message, wParam, lParam);
}

//
//  メイン ウィンドウのメッセージ処理
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc, hBuffer;
    //static HBITMAP hKeyBmp[32];
    //static HWND hButton[32];
    POINT po;
    HMENU tmp, hTMenu;
    static HMENU hMenu, hDevMenu, hChMenu;
    MENUITEMINFOW mii;
    MIDIOUTCAPS outCaps;
    MMRESULT res;
    WCHAR str[MAXERRORLENGTH];

    switch (message)
    {
    case WM_CREATE:
        // キーを配置
        for (UINT num = 0; num < 32; num++)
        {
            hKeyBmp[num] = (HBITMAP)LoadImage(((LPCREATESTRUCT)(lParam))->hInstance, MAKEINTRESOURCE(IDB_KEY40 + num), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE);
            UINT id = num + 200;
            hButton[num] = CreateWindow(
                TEXT("BUTTON"), TEXT(""),
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                keyX[num], keyY[num], 40, 40, hWnd, (HMENU)id,
                ((LPCREATESTRUCT)(lParam))->hInstance, NULL
            );
            SetWindowSubclass(hButton[num], keyPressProc, id, 40 + num);
        }
        // メニューの設定
        hMenu = CreatePopupMenu();
        hDevMenu = CreateMenu();
        hChMenu = CreateMenu();
        mii.cbSize = sizeof(MENUITEMINFOW);
        mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU;
        mii.hSubMenu = hDevMenu;
        mii.fState = MFS_ENABLED;
        mii.fType = MFT_STRING;
        mii.dwTypeData = (LPWSTR)TEXT("MIDI device");
        mii.wID = IDM_MIDIDEV;
        InsertMenuItemW(hMenu, 0, TRUE, &mii);
        mii.hSubMenu = hChMenu;
        mii.dwTypeData = (LPWSTR)TEXT("MIDI channel");
        mii.wID = IDM_MIDICH;
        InsertMenuItemW(hMenu, 1, TRUE, &mii);
        mii.fMask = MIIM_TYPE;
        mii.fType = MFT_SEPARATOR;
        InsertMenuItemW(hMenu, 2, TRUE, &mii);
        mii.fMask = MIIM_TYPE | MIIM_ID;
        mii.fType = MFT_STRING;
        mii.dwTypeData = (LPWSTR)TEXT("About...");
        mii.wID = IDM_ABOUT;
        InsertMenuItemW(hMenu, 3, TRUE, &mii);
        mii.dwTypeData = (LPWSTR)TEXT("Exit");
        mii.wID = IDM_EXIT;
        InsertMenuItemW(hMenu, 4, TRUE, &mii);
        for (UINT i = 0; ; i++)
        {
            res = midiOutGetDevCaps(i, &outCaps, sizeof(outCaps));
            if (res == MMSYSERR_NOERROR)
            {
                wcscpy_s(str, MAXERRORLENGTH, (const WCHAR*)outCaps.szPname);
                mii.fMask = MIIM_TYPE | MIIM_ID;
                mii.fType = MFT_STRING;
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
            InsertMenuItemW(hDevMenu, i, TRUE, &mii);
        }
        for (UINT i = 0; i < 16; i++)
        {
            wsprintf(str, TEXT("%d"), i + 1);
            mii.fMask = MIIM_TYPE | MIIM_ID;
            mii.fType = MFT_STRING;
            mii.dwTypeData = str;
            mii.wID = IDM_MIDINUM + i;
            mii.fState = MFS_ENABLED;
            InsertMenuItemW(hChMenu, i, TRUE, &mii);
        }
        // MIDI出力デバイス管理変数の初期化
        opendMidiDevId = 0;
        midiCh = 0;
        mii.fMask = MIIM_STATE;
        GetMenuItemInfo(hChMenu, IDM_MIDINUM, FALSE, &mii);
        mii.fState = MFS_CHECKED;
        SetMenuItemInfo(hChMenu, IDM_MIDINUM, FALSE, &mii);
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 選択されたメニューの解析:
            if (wmId < IDM_MIDINUM)
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
            else if (wmId < IDM_MIDIOUT)
            {
                if (midiCh != (wmId - IDM_MIDINUM))
                {
                    // 今選択してないチャンネルを指定されたら
                    // 選択しているチャンネルのチェックを外す
                    mii.cbSize = sizeof(MENUITEMINFOW);
                    mii.fMask = MIIM_STATE;
                    GetMenuItemInfo(hChMenu, IDM_MIDINUM + midiCh, FALSE, &mii);
                    mii.fState = MFS_ENABLED;
                    SetMenuItemInfo(hChMenu, IDM_MIDINUM + midiCh, FALSE, &mii);
                    // 選択されたチェックを付ける
                    GetMenuItemInfo(hChMenu, wmId, FALSE, &mii);
                    mii.fState = MFS_CHECKED;
                    SetMenuItemInfo(hChMenu, wmId, FALSE, &mii);
                    midiCh = wmId - IDM_MIDINUM;
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
                    GetMenuItemInfo(hDevMenu, opendMidiDevId, FALSE, &mii);
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
                                    GetMenuItemInfo(hDevMenu, opendMidiDevId, FALSE, &mii);
                                    mii.fState = MFS_ENABLED;
                                    SetMenuItemInfo(hDevMenu, opendMidiDevId, FALSE, &mii);
                                    // 非選択状態
                                    opendMidiDevId = 0;
                                }
                            }
                        }
                        else
                        {
                            // メニューのチェックマークを消す
                            mii.fState = MFS_ENABLED;
                            SetMenuItemInfo(hDevMenu, opendMidiDevId, FALSE, &mii);

                            // メニューのオープンしたデバイスにチェックマークをつける
                            opendMidiDevId = wmId;
                            GetMenuItemInfo(hDevMenu, wmId, FALSE, &mii);
                            mii.fState = MFS_CHECKED;
                            SetMenuItemInfo(hDevMenu, wmId, FALSE, &mii);
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
    case WM_KEYDOWN:
        keyDown(wParam);
        break;
    case WM_KEYUP:
        keyUp(wParam);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
//
// バージョン情報ボックス
//
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    HDC hdc, hmdc;
    PAINTSTRUCT ps;
    HBITMAP hBitmapL, hBitmapLM, hBitmapB, hBitmapBM;
    HINSTANCE hInst;

    switch (message) {
    case WM_PAINT:
        hdc = BeginPaint(hDlg, &ps);
        hInst = (HINSTANCE)GetWindowLongPtr(hDlg, GWLP_HINSTANCE);
        hBitmapLM = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LOGOM));
        hBitmapL = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LOGO));
        hBitmapBM = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BOYM));
        hBitmapB = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BOY));
        hmdc = CreateCompatibleDC(hdc);
        SelectObject(hmdc, hBitmapLM);
        BitBlt(hdc, 151, 0, 350, 99, hmdc, 0, 0, SRCAND);
        SelectObject(hmdc, hBitmapL);
        BitBlt(hdc, 151, 0, 350, 99, hmdc, 0, 0, SRCPAINT);
        SelectObject(hmdc, hBitmapBM);
        BitBlt(hdc, 0, 0, 150, 203, hmdc, 0, 0, SRCAND);
        SelectObject(hmdc, hBitmapB);
        BitBlt(hdc, 0, 0, 150, 203, hmdc, 0, 0, SRCPAINT);
        DeleteDC(hmdc);
        DeleteObject(hBitmapL);
        DeleteObject(hBitmapLM);
        DeleteObject(hBitmapB);
        DeleteObject(hBitmapBM);
        EndPaint(hDlg, &ps);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
    case WM_DESTROY:
        EndDialog(hDlg, IDOK);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

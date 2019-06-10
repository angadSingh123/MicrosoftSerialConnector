
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <iostream>

#define FILE_MENU_NEW 1
#define FILE_MENU_OPEN 2
#define FILE_MENU_EXIT 3
#define FILE_MENU_NEW_PROJECT 4
#define FILE_MENU_ACTION_TITLE 5
#define TXT_MSG_SZ 50

char ComPortName[] = "COM8";

HBITMAP hMyImage;

HMENU hMenu;

HWND btn, hImg, hCom, hBaud,  hEdit;

HANDLE hThread, hComm;

HINSTANCE hGlobalInst;

DWORD WINAPI thread(LPVOID lpar);

LRESULT CALLBACK WindowProc (HWND, UINT, WPARAM, LPARAM);

void AddMenus (HWND);

void AddControls (HWND);

void LoadImages();

inline void AppendText(char * SerialBuffer);

inline AppN(int n);

int i = 0 ;

char buffer[5];

bool running;

int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmd, int ncmdShow  ){

    WNDCLASSW windowClass = {0};

    windowClass.hbrBackground = (HBRUSH) COLOR_WINDOW;

    windowClass.hCursor = LoadCursor(NULL, IDC_CROSS);

    windowClass.hInstance = hInst;

    hGlobalInst = hInst;

    windowClass.lpszClassName = L"WindowClass";

    windowClass.lpfnWndProc = WindowProc;

    windowClass.hIcon = (HICON) LoadImage (NULL, "mc.ico", IMAGE_ICON, 48, 48, LR_LOADFROMFILE);

    if (!RegisterClassW(&windowClass)) {

        printf("Error could not register window class\n");
        return - 1;

    }

    CreateWindowW(L"WindowClass", L"Microsoft Serial Connector 1.0", WS_VISIBLE | WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU, 0, 0, 482, 490, NULL, NULL, NULL, NULL);

    MSG msg = {0};

    while (GetMessage(&msg, NULL, 0, 0)){

        TranslateMessage((&msg));

        DispatchMessage(&msg);

    }


}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wPara, LPARAM lPara) {

    switch (msg) {

    case  WM_CREATE:
        AddControls(hwnd);
        AddMenus (hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_COMMAND:

        switch (wPara){

        case FILE_MENU_NEW:
            MessageBox(hwnd, "Microsoft Serial Connector. \nCopyright 2019. HHI", "About", MB_OK);
            break;

        case FILE_MENU_ACTION_TITLE:
            if (!running) {

                hThread = CreateThread(NULL, 0, thread, NULL, 0 , NULL);
                SetWindowText(btn, "Stop");

            }
            else {

                TerminateThread(hThread, 0);
                CloseHandle(hThread);
                CloseHandle(hComm);
                SetWindowText(btn, "Start");
                running = false;
            }
            break;

        default:
                break;
        }
        break;

    default:
        DefWindowProc(hwnd, msg, wPara, lPara);
        break;

    }
}

void AddControls(HWND hwnd) {

    CreateWindowW(L"static", L"COM", WS_CHILD | WS_VISIBLE , 375, 0, 100, 20, hwnd, NULL, NULL, NULL );

    CreateWindowW(L"static", L"BAUD RATE", WS_CHILD | WS_VISIBLE , 375, 45, 100, 20, hwnd, NULL, NULL, NULL);

    LoadLibrary(("Msftedit.dll"));

	hEdit = CreateWindowExW(0, L"RICHEDIT50W", L"", WS_VSCROLL | WS_HSCROLL| ES_AUTOVSCROLL | ES_READONLY | ES_MULTILINE | WS_VISIBLE | WS_CHILD | WS_BORDER , 0, 0, 374, 440, hwnd, NULL, NULL, NULL);

	btn = CreateWindowW(L"Button", L"Start", WS_VISIBLE | WS_CHILD | SS_CENTER , 375, 85, 100, 20, hwnd, (HMENU) FILE_MENU_ACTION_TITLE ,NULL, NULL);

    hCom = CreateWindowW(L"edit", L"",  WS_CHILD | WS_OVERLAPPED | WS_VISIBLE| WS_BORDER, 375, 20, 100, 20, hwnd, NULL, hGlobalInst, NULL);

    SetWindowText(hCom, ComPortName);

    hBaud = CreateWindowW(L"edit", L"",  WS_VISIBLE | WS_OVERLAPPED | WS_CHILD| WS_BORDER, 375, 60, 100, 20, hwnd, NULL, NULL, NULL);

    SetWindowText(hBaud, "9600");

}

void AddMenus(HWND hwnd)  {

    hMenu = CreateMenu();

    AppendMenu (hMenu,MF_STRING, FILE_MENU_NEW, "About");

    SetMenu(hwnd, hMenu);

}

DWORD WINAPI thread(LPVOID lpar) {

    running  = true;

    char SerialBuffer[30];

    char tempChar;

    BOOL Status;

    DWORD dwEventMask, BytesRead;

    GetWindowText(hCom, ComPortName, 6);

    hComm = CreateFile(ComPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (hComm == INVALID_HANDLE_VALUE)

        std::cout << "Cannot open port: " << ComPortName << "\n";

    else

        std::cout << "Port opened: " << ComPortName << "\n";

    DCB serialParams = {0};

    serialParams.DCBlength = sizeof(serialParams);

    Status = GetCommState(hComm, &serialParams);

    if (!Status) {

        std::cout << "Error\n";

        return -1;

    }

    serialParams.BaudRate = CBR_9600;

    serialParams.ByteSize = 8;

    serialParams.StopBits = ONESTOPBIT;

    serialParams.Parity = (BYTE) PARITY_NONE;

    Status = SetCommState(hComm, &serialParams);

    if (!Status) {

        std::cout << "Error in structure setting\n";

    }

    COMMTIMEOUTS timeOut = {0};

    timeOut.ReadIntervalTimeout = MAXDWORD;

    timeOut.ReadTotalTimeoutConstant = 50;

    timeOut.ReadTotalTimeoutMultiplier = MAXDWORD;

    timeOut.WriteTotalTimeoutConstant = 50;

    timeOut.WriteTotalTimeoutMultiplier = 10;

    Status = SetCommTimeouts(hComm, &timeOut);

    if (!Status) {

        std::cout << "Error in setting timeouts.\n";

        CloseHandle(hComm);
    }

    Status = SetCommMask(hComm, EV_RXCHAR);

    if (!Status) {

        SetWindowText(hEdit, "Problem mask setting");
    }

    memset(&SerialBuffer, 0, 30);

    while (true)    {

        Status = WaitCommEvent(hComm, &dwEventMask, NULL);

        if (Status) {

            memset(&SerialBuffer, 0, i);

            i = 0;

                do {
                    Status = ReadFile(hComm, &tempChar, sizeof(tempChar), &BytesRead, NULL);
                    SerialBuffer[i] = tempChar;
                    i+=1;
                } while (BytesRead > 0);

            if (i >= 2) {

                for (int j = 0 ; j < i -1; j++) {

                    AppN (*(SerialBuffer + j));
                }

                AppendText("\n");
            }
        }

    }

    CloseHandle(hComm);

    return 0;

}


inline void AppendText(char * SerialBuffer) {

        int  ndx = GetWindowTextLength(hEdit);

        SendMessage (hEdit, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);

        SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) (SerialBuffer)));

}

inline AppN(int n) {

    itoa(n, buffer, 10);
    AppendText(buffer);
}

// backup_gui.cpp
// Win32 GUI for robocopy backup with Windows Task Scheduler integration

#include <windows.h>
#include <shobjidl.h>
#include <string>
#include <commctrl.h>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "Comctl32.lib")

// 전역 변수
HWND hEditSource;
HWND hEditTarget;
HWND hCheckSchedule;
HWND hDateTimePicker;

// 함수 선언
std::wstring BrowseForFolder(HWND hwndOwner);
void RunRobocopy(const std::wstring& source, const std::wstring& target);
void ScheduleWithTaskScheduler(const std::wstring& source, const std::wstring& target, const SYSTEMTIME& st);

// 폴더 브라우저
std::wstring BrowseForFolder(HWND hwndOwner) {
    std::wstring folderPath;
    IFileDialog* pDialog = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pDialog)))) {
        DWORD opts;
        pDialog->GetOptions(&opts);
        pDialog->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        if (SUCCEEDED(pDialog->Show(hwndOwner))) {
            IShellItem* pItem = nullptr;
            if (SUCCEEDED(pDialog->GetResult(&pItem))) {
                PWSTR psz;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &psz))) {
                    folderPath = psz;
                    CoTaskMemFree(psz);
                }
                pItem->Release();
            }
        }
        pDialog->Release();
    }
    return folderPath;
}

// robocopy 직접 실행
void RunRobocopy(const std::wstring& source, const std::wstring& target) {
    // 로그파일 타임스탬프
    SYSTEMTIME now;
    GetLocalTime(&now);
    wchar_t ts[32];
    swprintf_s(ts, L"%04d%02d%02d_%02d%02d%02d",
        now.wYear, now.wMonth, now.wDay,
        now.wHour, now.wMinute, now.wSecond);

    std::wstringstream cmd;
    cmd << L"robocopy \"" << source << L"\" \"" << target << L"\""
        L" /XO /COPY:DAT /PURGE /V /E /NJH /NJS /TEE /LOG+:%TEMP%\\ROBOCOPY"
        << ts << L".log";

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = L"cmd.exe";
    std::wstring params = L"/c " + cmd.str();
    sei.lpParameters = params.c_str();
    sei.nShow = SW_SHOWNORMAL;

    if (ShellExecuteExW(&sei)) {
        if (sei.hProcess) {
            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);
        }
        MessageBoxW(nullptr, L"백업이 완료되었습니다.", L"완료", MB_OK | MB_ICONINFORMATION);
    }
    else {
        MessageBoxW(nullptr, L"백업 실행 실패", L"오류", MB_OK | MB_ICONERROR);
    }
}

// Windows Task Scheduler에 예약 등록
void ScheduleWithTaskScheduler(const std::wstring& source, const std::wstring& target, const SYSTEMTIME& st) {
    // 작업 이름 생성
    wchar_t nameBuf[64];
    swprintf_s(nameBuf, L"RobocopyBackup_%04d%02d%02d_%02d%02d%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    // schtasks 명령어 조립 (MM/dd/yyyy 형식 요구됨)
    wchar_t dateBuf[16], timeBuf[8];
    swprintf_s(dateBuf, L"%02d/%02d/%04d", st.wMonth, st.wDay, st.wYear);
    swprintf_s(timeBuf, L"%02d:%02d", st.wHour, st.wMinute);

    // robocopy 명령문 따옴표 이스케이프
    std::wstringstream rc;
    rc << L"robocopy \"" << source << L"\" \"" << target << L"\""
        << L" /XO /COPY:DAT /PURGE /V /E /NJH /NJS /TEE /LOG+:%TEMP%\\ROBOCOPY"
        << dateBuf << L"_" << timeBuf << L".log";

    std::wstringstream cmd;
    cmd << L"/Create /SC ONCE /TN \"" << nameBuf << L"\" "
        L"/TR \"" << rc.str() << L"\" "
        L"/ST " << timeBuf << L" /SD " << dateBuf << L" /F";

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = L"schtasks.exe";
    std::wstring params = cmd.str();
    sei.lpParameters = params.c_str();
    sei.nShow = SW_HIDE;

    if (ShellExecuteExW(&sei)) {
        if (sei.hProcess) {
            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);
        }
        MessageBoxW(nullptr, L"스케줄러에 작업이 등록되었습니다.", L"완료", MB_OK | MB_ICONINFORMATION);
    }
    else {
        MessageBoxW(nullptr, L"작업 등록 실패", L"오류", MB_OK | MB_ICONERROR);
    }
}

// WndProc
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_DATE_CLASSES };
        InitCommonControlsEx(&icc);

        CreateWindowW(L"STATIC", L"소스 경로:", WS_VISIBLE | WS_CHILD, 10, 10, 80, 20, hwnd, nullptr, nullptr, nullptr);
        hEditSource = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 10, 400, 20, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Browse...", WS_VISIBLE | WS_CHILD, 510, 10, 80, 20, hwnd, (HMENU)1, nullptr, nullptr);

        CreateWindowW(L"STATIC", L"백업 위치:", WS_VISIBLE | WS_CHILD, 10, 40, 80, 20, hwnd, nullptr, nullptr, nullptr);
        hEditTarget = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 40, 400, 20, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Browse...", WS_VISIBLE | WS_CHILD, 510, 40, 80, 20, hwnd, (HMENU)2, nullptr, nullptr);

        hCheckSchedule = CreateWindowW(L"BUTTON", L"예약 실행", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 100, 70, 100, 20, hwnd, (HMENU)4, nullptr, nullptr);
        hDateTimePicker = CreateWindowW(DATETIMEPICK_CLASS, nullptr, WS_VISIBLE | WS_CHILD | DTS_APPCANPARSE, 210, 70, 200, 20, hwnd, nullptr, nullptr, nullptr);

        CreateWindowW(L"BUTTON", L"확인", WS_VISIBLE | WS_CHILD, 10, 100, 80, 20, hwnd, (HMENU)3, nullptr, nullptr);
    } break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            auto p = BrowseForFolder(hwnd);
            if (!p.empty()) SetWindowTextW(hEditSource, p.c_str());
        }
        else if (LOWORD(wParam) == 2) {
            auto p = BrowseForFolder(hwnd);
            if (!p.empty()) SetWindowTextW(hEditTarget, p.c_str());
        }
        else if (LOWORD(wParam) == 3) {
            wchar_t src[MAX_PATH], tgt[MAX_PATH];
            GetWindowTextW(hEditSource, src, MAX_PATH);
            GetWindowTextW(hEditTarget, tgt, MAX_PATH);
            if (!*src || !*tgt) {
                MessageBoxW(hwnd, L"경로를 모두 입력하세요.", L"오류", MB_OK | MB_ICONERROR);
                break;
            }
            if (SendMessageW(hCheckSchedule, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                SYSTEMTIME st;
                DateTime_GetSystemtime(hDateTimePicker, &st);
                ScheduleWithTaskScheduler(src, tgt, st);
            }
            else {
                RunRobocopy(src, tgt);
            }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 진입점
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nShow) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const wchar_t CLASS_NAME[] = L"BackupGuiClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Robocopy Backup Helper",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT,
        620, 160, nullptr, nullptr, hInst, nullptr);
    if (!hwnd) return 0;
    ShowWindow(hwnd, nShow);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    CoUninitialize();
    return 0;
}

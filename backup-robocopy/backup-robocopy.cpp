// backup_gui.cpp
// A simple Win32 GUI for selecting a source path for robocopy backup

#include <windows.h>
#include <shobjidl.h> 
#include <string>
#include <commctrl.h>
#include <iostream>
#include <chrono>
#include <ctime>

// 전역 변수
HWND hEditSource; // 소스 경로 편집기 핸들
HWND hEditTarget; // 타겟 경로 편집기 핸들

// 폴더 브라우저 대화상자를 여는 함수  
std::wstring BrowseForFolder(HWND hwndOwner) {  
   std::wstring folderPath; // 선택된 폴더 경로  
   IFileDialog* pFileDialog = nullptr; // IFileDialog 인터페이스 포인터  

   // COM 라이브러리 초기화  
   HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,  
       IID_PPV_ARGS(&pFileDialog));  

   // IFileDialog 인터페이스 생성  
   if (SUCCEEDED(hr)) {  
       DWORD dwOptions;  
       pFileDialog->GetOptions(&dwOptions);  
       pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);  

       hr = pFileDialog->Show(hwndOwner);  
       if (SUCCEEDED(hr)) {  
           IShellItem* pItem = nullptr;  
           hr = pFileDialog->GetResult(&pItem);  
           if (SUCCEEDED(hr)) {  
               PWSTR pszFilePath = nullptr;  
               hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);  
               if (SUCCEEDED(hr) && pszFilePath != nullptr) { // NULL 체크 추가  
                   folderPath = pszFilePath;  
                   CoTaskMemFree(pszFilePath);  
			   }
			   else
			   {
				   MessageBoxW(hwndOwner, L"폴더 경로를 가져오는 데 실패했습니다.", L"오류", MB_OK | MB_ICONERROR);
			   }
               pItem->Release();  
           }  
       }  
       pFileDialog->Release();  
   }  
   return folderPath;  
}

// 창 프로시저
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 현재 시간을 가져오기
	auto now = time(NULL);
	tm localTime;
	localtime_s(&localTime, &now);
	wchar_t timeStr[20];
	// 시간을 문자열로 변환
	swprintf_s(timeStr, 20, L"%02d%02d%02d_%02d%02d%02d",
		localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday,
		localTime.tm_hour, localTime.tm_min, localTime.tm_sec);
	// 메시지 처리
	switch (msg) {
    
	// 윈도우 생성 시
    case WM_CREATE:
		// 라벨 - 소스 경로
        CreateWindowW(L"STATIC", L"소스 경로:",
            WS_VISIBLE | WS_CHILD,
            10, 10, 80, 20,
            hwnd, nullptr, nullptr, nullptr);

		// 컨트롤 편집기 - 소스 경로
        hEditSource = CreateWindowW(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            100, 10, 420, 20,
            hwnd, nullptr, nullptr, nullptr);

		// 브라우즈 버튼 - 소스 경로
        CreateWindowW(L"BUTTON", L"Browse...",
            WS_VISIBLE | WS_CHILD,
            530, 10, 80, 20,
            hwnd, (HMENU)1, nullptr, nullptr);

		// 라벨 - 타겟 경로
		CreateWindowW(L"STATIC", L"백업 위치:",
			WS_VISIBLE | WS_CHILD,
			10, 40, 80, 20,
			hwnd, nullptr, nullptr, nullptr);

        // 컨트롤 편집기 - 타겟 경로
		hEditTarget = CreateWindowW(L"EDIT", L"",
			WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
			100, 40, 420, 20,
			hwnd, nullptr, nullptr, nullptr);

		// 브라우즈 버튼 - 타겟 경로
		CreateWindowW(L"BUTTON", L"Browse...",
			WS_VISIBLE | WS_CHILD,
			530, 40, 80, 20,
			hwnd, (HMENU)2, nullptr, nullptr);

		// 확인 버튼
		CreateWindowW(L"BUTTON", L"확인",
			WS_VISIBLE | WS_CHILD,
			10, 70, 80, 20,
			hwnd, (HMENU)3, nullptr, nullptr);

        break;
	
    case WM_COMMAND:
		if (LOWORD(wParam) == 1) { // 브라우즈 버튼 클릭 - 소스 경로
			
            // 폴더 선택 대화상자 열기
            std::wstring path = BrowseForFolder(hwnd);
            if (!path.empty()) {
                SetWindowTextW(hEditSource, path.c_str());
            }
		}
		else if (LOWORD(wParam) == 2) { // 브라우즈 버튼 클릭 - 타겟 경로
			// 폴더 선택 대화상자 열기
			std::wstring path = BrowseForFolder(hwnd);
			if (!path.empty()) {
				SetWindowTextW(hEditTarget, path.c_str());
			}
		}
		else if (LOWORD(wParam) == 3) { // 확인 버튼 클릭
			wchar_t sourcePath[MAX_PATH];
			wchar_t targetPath[MAX_PATH];
			GetWindowTextW(hEditSource, sourcePath, MAX_PATH);
			GetWindowTextW(hEditTarget, targetPath, MAX_PATH);
			// 경로가 비어있지 않은지 확인
			if (wcslen(sourcePath) == 0 || wcslen(targetPath) == 0) {
				MessageBoxW(hwnd, L"소스 경로와 타겟 경로를 모두 입력하세요.", L"오류", MB_OK | MB_ICONERROR);
				break;
			}

		    // robocopy 백업
			// robocopy 명령어 생성
			std::wstring command = L"robocopy \"" + std::wstring(sourcePath) + L"\" \"" + std::wstring(targetPath) + L"\" /XO /COPY:DAT /PURGE /V /E /NJH /NJS /TEE /LOG+:%TEMP%\\ROBOCOPY" + timeStr + L".log";
			// 관리자  권한으로 robocopy 실행
			SHELLEXECUTEINFOW sei = { sizeof(sei) };
			sei.fMask = SEE_MASK_NOCLOSEPROCESS;
			sei.hwnd = hwnd;
			sei.lpVerb = L"runas"; // 관리자 권한으로 실행
            // Fix for C26815: Use a separate variable to store the concatenated string to ensure its lifetime is valid.
            std::wstring parameters = L"/c " + command;
			sei.lpParameters = parameters.c_str(); // c_str() 메서드를 사용하여 std::wstring을 wchar_t*로 변환
			sei.lpFile = L"cmd.exe"; // cmd.exe 실행
			sei.nShow = SW_SHOWNORMAL; // 창을 보이게 함
			// 명령어 실행
			if (ShellExecuteExW(&sei)) {
				// 명령어 실행이 성공하면 대기
				if (sei.hProcess != nullptr) {
                   WaitForSingleObject(sei.hProcess, INFINITE);
                   CloseHandle(sei.hProcess);
                } else {
                   MessageBoxW(hwnd, L"프로세스 핸들이 유효하지 않습니다.", L"오류", MB_OK | MB_ICONERROR);
                }
				MessageBoxW(hwnd, L"백업이 완료되었습니다.", L"완료", MB_OK | MB_ICONINFORMATION);
			}
			else {
				MessageBoxW(hwnd, L"백업을 시작할 수 없습니다.", L"오류", MB_OK | MB_ICONERROR);
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

// wWinMain 함수에 대한 주석을 추가하여 C28251 오류를 해결합니다.
// 또한, VCR003 경고를 해결하기 위해 wWinMain 함수를 정적으로 설정합니다.

/**
* @brief 애플리케이션의 진입점 함수입니다.
* 
* @param hInstance 현재 인스턴스 핸들입니다.
* @param hPrevInstance 이전 인스턴스 핸들입니다. (사용되지 않음)
* @param lpCmdLine 명령줄 인수입니다.
* @param nCmdShow 윈도우 표시 상태입니다.
* @return 애플리케이션 종료 코드입니다.
*/
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {  
  // Initialize COM  
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);  
  if (FAILED(hr)) {  
      MessageBoxW(nullptr, L"COM 라이브러리 초기화에 실패했습니다.", L"오류", MB_OK | MB_ICONERROR);  
      return -1;  
  }  

  const wchar_t CLASS_NAME[] = L"BackupGuiClass";  
  WNDCLASSW wc = {};  
  wc.lpfnWndProc = WndProc;  
  wc.hInstance = hInstance;  
  wc.lpszClassName = CLASS_NAME;  

  RegisterClassW(&wc); // 윈도우 클래스 등록  

  // 윈도우 생성  
  HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Robocopy Backup Helper",  
      WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,  
      CW_USEDEFAULT, CW_USEDEFAULT, 640, 160,  
      nullptr, nullptr, hInstance, nullptr);  
  if (!hwnd) {  
      CoUninitialize();  
      return 0;  
  }  

  ShowWindow(hwnd, nCmdShow);  

  // Message 루프  
  MSG msg = {};  
  while (GetMessage(&msg, nullptr, 0, 0)) {  
      TranslateMessage(&msg);  
      DispatchMessage(&msg);  
  }  

  CoUninitialize();  
  return 0;  
}
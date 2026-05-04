/******************************************************************************
* This file is part of DefaultAudioChanger.
* Copyright (c) 2026 plastfw
*
* Licensed under the MIT License.
******************************************************************************/

#include "stdafx.h"
#include "resource.h"
#include "DACWindow.h"
#include "DevicesManager.h"
#include <strsafe.h>

static CDevicesManager devicesManager;
static HKEY            deviceSettingsKey = nullptr;
static HKEY            appSettingsKey    = nullptr;
static DACWindow     appWindow;

#define PIPE_NAME L"\\\\.\\pipe\\DefaultAudioChangerPipe"

// ---------------------------------------------------------------------------
// Message from a second instance: switch devices
// ---------------------------------------------------------------------------
static void SendServerSwitchMessage()
{
    HANDLE pipe = CreateFile(PIPE_NAME,
        GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(pipe, &dwMode, nullptr, nullptr);

    int msg = 1;
    DWORD written;
    WriteFile(pipe, &msg, sizeof(msg), &written, nullptr);
    CloseHandle(pipe);
}

// ---------------------------------------------------------------------------
// Background thread: listens for switch commands from second instances
// ---------------------------------------------------------------------------
static DWORD WINAPI ListeningThread(LPVOID)
{
    for (;;)
    {
        HANDLE pipe = CreateNamedPipe(PIPE_NAME,
            PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, 10, 10, 0, nullptr);

        if (pipe == INVALID_HANDLE_VALUE)
            return 0;

        if (ConnectNamedPipe(pipe, nullptr))
        {
            int msg = 0;
            DWORD bRead = 0;
            if (ReadFile(pipe, &msg, sizeof(msg), &bRead, nullptr))
            {
                DWORD lpcValues = 0, lpcMaxLen = 0;
                RegQueryInfoKey(deviceSettingsKey, nullptr, nullptr, nullptr,
                    nullptr, nullptr, nullptr,
                    &lpcValues, &lpcMaxLen, nullptr, nullptr, nullptr);

                if (lpcValues > 0)
                {
                    std::vector<std::wstring> ids;
                    for (DWORD i = 0; i < lpcValues; i++)
                    {
                        DWORD len = lpcMaxLen + 1;
                        std::wstring name(len, L'\0');
                        RegEnumValue(deviceSettingsKey, i,
                            &name[0], &len,
                            nullptr, nullptr, nullptr, nullptr);
                        name.resize(len);
                        ids.push_back(name);
                    }
                    devicesManager.SwitchDevices(ids);
                    appWindow.UpdateApplicationIcon();
                }
            }
        }

        FlushFileBuffers(pipe);
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
}

// ---------------------------------------------------------------------------
// WinMain
// ---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpstrCmdLine, int)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) return -1;

    if (FAILED(devicesManager.InitializeDeviceEnumerator()) ||
        FAILED(devicesManager.LoadAudioDevices()))
    {
        MessageBox(nullptr, L"Failed to initialize audio devices.", L"Error", MB_ICONERROR);
        return -1;
    }

    HKEY currentUser;
    if (RegOpenCurrentUser(KEY_ALL_ACCESS, &currentUser) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, L"Cannot open registry.", L"Error", MB_ICONERROR);
        return -1;
    }

    if (RegCreateKeyEx(currentUser,
            L"Software\\Zergiu.com\\DAC\\Devices", 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr,
            &deviceSettingsKey, nullptr) != ERROR_SUCCESS ||
        RegCreateKeyEx(currentUser,
            L"Software\\Zergiu.com\\DAC\\Settings", 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr,
            &appSettingsKey, nullptr) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, L"Cannot open registry keys.", L"Error", MB_ICONERROR);
        return -1;
    }

    // Single-instance check via named pipe
    HANDLE pipe = CreateNamedPipe(PIPE_NAME,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, 10, 10, 0, nullptr);

    HANDLE listenerThread = nullptr;
    if (pipe == INVALID_HANDLE_VALUE)
    {
        // Another instance is running — send switch command and exit
        SendServerSwitchMessage();
        goto Cleanup;
    }
    CloseHandle(pipe);
    listenerThread = CreateThread(nullptr, 0, ListeningThread, nullptr, 0, nullptr);

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_COOL_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // Create and show the ImGui window
    appWindow.SetDevicesManager(&devicesManager);
    appWindow.SetDeviceSettingsKey(deviceSettingsKey);
    appWindow.SetAppSettingsKey(appSettingsKey);

    if (!appWindow.Create(hInstance))
    {
        MessageBox(nullptr, L"Window creation failed.", L"Error", MB_ICONERROR);
        goto Cleanup;
    }

    if (appWindow.ShowTrayIcon())
    {
        // Show settings window only on first run (no devices configured yet)
        DWORD lpcValues = 0;
        RegQueryInfoKey(deviceSettingsKey, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr,
            &lpcValues, nullptr, nullptr, nullptr, nullptr);
        appWindow.ShowWnd(lpcValues == 0 ? SW_SHOWDEFAULT : SW_HIDE);
    }
    else
    {
        appWindow.ShowWnd(SW_SHOWDEFAULT);
    }

    // Standard message loop
    {
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

Cleanup:
    devicesManager.ReleaseDeviceEnumerator();
    if (deviceSettingsKey) RegCloseKey(deviceSettingsKey);
    if (appSettingsKey)    RegCloseKey(appSettingsKey);
    if (currentUser)       RegCloseKey(currentUser);
    if (listenerThread)    CloseHandle(listenerThread);

    CoUninitialize();
    return 0;
}

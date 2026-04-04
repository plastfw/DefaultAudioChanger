#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <d3d11.h>
#include <vector>
#include <string>

class CDevicesManager;

class DACWindow
{
public:
    static const UINT WM_TRAYICON_MSG = WM_APP + 1;
    static const UINT TRAY_ICON_ID    = 1;
    static const UINT TIMER_RENDER    = 1;
    static const UINT IDM_SWITCH      = 100;
    static const UINT IDM_OPTIONS     = 101;
    static const UINT IDM_EXIT        = 102;

    DACWindow();
    ~DACWindow();

    bool Create(HINSTANCE hInstance);
    void ShowWnd(int nCmdShow);
    bool ShowTrayIcon();
    void UpdateApplicationIcon();

    void SetDevicesManager(CDevicesManager* dm);
    void SetDeviceSettingsKey(HKEY key);
    void SetAppSettingsKey(HKEY key);

    HWND GetHwnd() const { return m_hwnd; }

    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

    // D3D11
    bool InitDX();
    void CleanupDX();
    void CreateRenderTarget();
    void CleanupRenderTarget();

    // Rendering
    void Render();
    void BuildUI();

    // State loading
    void LoadAllState();
    void LoadDeviceCheckStates();
    void LoadHotkeyState();
    void LoadStartupState();

    // Actions
    void DoSwitch();
    void DoReload();
    void DoExit();
    void SaveDeviceCheckState(const std::wstring& deviceId, bool checked);
    void ApplyHotkey(WORD hotkey);
    void UnregisterCurrentHotkey();
    void UpdateTrayTooltip();

    // Tray
    void OnTrayIcon(WPARAM wp, LPARAM lp);
    void ShowContextMenu();

    // Hotkey conversion helpers
    BYTE hkf2modf(BYTE hkf);
    BYTE modf2hkf(BYTE modf);

private:
    HWND      m_hwnd      = nullptr;
    HINSTANCE m_hInstance = nullptr;

    // D3D11
    ID3D11Device*           m_pDevice  = nullptr;
    ID3D11DeviceContext*    m_pContext  = nullptr;
    IDXGISwapChain*         m_pSwapChain = nullptr;
    ID3D11RenderTargetView* m_pRTV     = nullptr;

    // Tray
    NOTIFYICONDATA m_nid   = {};
    HMENU          m_hMenu = nullptr;

    // App dependencies
    CDevicesManager* m_devicesManager    = nullptr;
    HKEY             m_deviceSettingsKey = nullptr;
    HKEY             m_appSettingsKey    = nullptr;

    // UI state
    std::vector<bool> m_deviceChecked;
    bool  m_hotkeyEnabled  = false;
    WORD  m_hotkey         = 0;   // MAKEWORD(vkCode, hkfModifiers)
    bool  m_startupEnabled = false;
    bool  m_capturingKey   = false;  // true while waiting for hotkey input
    bool  m_showAbout      = false;
    ATOM  m_hotKeyAtom     = 0;

    // Window size
    UINT m_width  = 460;
    UINT m_height = 340;
};

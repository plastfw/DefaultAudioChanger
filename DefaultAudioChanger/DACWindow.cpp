// DACWindow.cpp - Dear ImGui + D3D11 replacement for the WTL dialog UI
// Requires ImGui source files in a subfolder: imgui/
//   imgui.h, imgui.cpp, imgui_draw.cpp, imgui_tables.cpp, imgui_widgets.cpp
//   imgui_impl_win32.h/cpp, imgui_impl_dx11.h/cpp

#include "stdafx.h"
#include "DACWindow.h"
#include "DevicesManager.h"
#include "resource.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

// As instructed by imgui_impl_win32.h — must be declared manually
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#define STARTUP_REG_KEY  _T("DefaultAudioChanger")
#define STARTUP_RUN_PATH _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run")

// ---------------------------------------------------------------------------
// Custom dark theme with blue accent
// ---------------------------------------------------------------------------
// Accent colours used throughout BuildUI
static const ImVec4 kGreen     = ImVec4(0.07f, 0.86f, 0.48f, 1.00f);
static const ImVec4 kGreenHov  = ImVec4(0.12f, 1.00f, 0.58f, 1.00f);
static const ImVec4 kGreenAct  = ImVec4(0.05f, 0.65f, 0.36f, 1.00f);
static const ImVec4 kCoral     = ImVec4(1.00f, 0.30f, 0.37f, 1.00f);
static const ImVec4 kCoralHov  = ImVec4(1.00f, 0.44f, 0.50f, 1.00f);
static const ImVec4 kCoralAct  = ImVec4(0.80f, 0.20f, 0.26f, 1.00f);

static void ApplyDACTheme()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding     = 0.0f;   // full-screen ImGui window — no rounding needed
    s.ChildRounding      = 8.0f;
    s.FrameRounding      = 7.0f;
    s.GrabRounding       = 7.0f;
    s.PopupRounding      = 8.0f;
    s.ScrollbarRounding  = 7.0f;
    s.WindowBorderSize   = 0.0f;
    s.ChildBorderSize    = 0.0f;
    s.FrameBorderSize    = 0.0f;
    s.ItemSpacing        = ImVec2(6, 5);
    s.ItemInnerSpacing   = ImVec2(5, 4);
    s.FramePadding       = ImVec2(7, 4);
    s.WindowPadding      = ImVec2(12, 10);
    s.ScrollbarSize      = 8.0f;

    ImVec4* c = s.Colors;
    // Backgrounds
    c[ImGuiCol_WindowBg]             = ImVec4(0.07f, 0.07f, 0.10f, 1.00f);
    c[ImGuiCol_ChildBg]              = ImVec4(0.11f, 0.11f, 0.16f, 1.00f);
    c[ImGuiCol_PopupBg]              = ImVec4(0.10f, 0.10f, 0.15f, 0.98f);
    // Borders
    c[ImGuiCol_Border]               = ImVec4(0.20f, 0.20f, 0.28f, 0.50f);
    c[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    // Frame (checkbox bg, input, etc.)
    c[ImGuiCol_FrameBg]              = ImVec4(0.15f, 0.15f, 0.21f, 1.00f);
    c[ImGuiCol_FrameBgHovered]       = ImVec4(0.20f, 0.20f, 0.28f, 1.00f);
    c[ImGuiCol_FrameBgActive]        = ImVec4(0.23f, 0.23f, 0.33f, 1.00f);
    // Title (unused in borderless mode, but set anyway)
    c[ImGuiCol_TitleBg]              = ImVec4(0.07f, 0.07f, 0.10f, 1.00f);
    c[ImGuiCol_TitleBgActive]        = ImVec4(0.07f, 0.07f, 0.10f, 1.00f);
    // Accent — green checkmark + slider
    c[ImGuiCol_CheckMark]            = kGreen;
    c[ImGuiCol_SliderGrab]           = kGreen;
    c[ImGuiCol_SliderGrabActive]     = kGreenAct;
    // Default button — subtle dark pill
    c[ImGuiCol_Button]               = ImVec4(0.16f, 0.16f, 0.23f, 1.00f);
    c[ImGuiCol_ButtonHovered]        = ImVec4(0.23f, 0.23f, 0.33f, 1.00f);
    c[ImGuiCol_ButtonActive]         = ImVec4(0.11f, 0.11f, 0.17f, 1.00f);
    // Selectable / header
    c[ImGuiCol_Header]               = ImVec4(0.07f, 0.86f, 0.48f, 0.18f);
    c[ImGuiCol_HeaderHovered]        = ImVec4(0.07f, 0.86f, 0.48f, 0.30f);
    c[ImGuiCol_HeaderActive]         = ImVec4(0.07f, 0.86f, 0.48f, 0.50f);
    // Separator
    c[ImGuiCol_Separator]            = ImVec4(0.20f, 0.20f, 0.28f, 0.80f);
    c[ImGuiCol_SeparatorHovered]     = ImVec4(0.07f, 0.86f, 0.48f, 0.60f);
    c[ImGuiCol_SeparatorActive]      = kGreen;
    // Text
    c[ImGuiCol_Text]                 = ImVec4(0.93f, 0.93f, 0.96f, 1.00f);
    c[ImGuiCol_TextDisabled]         = ImVec4(0.35f, 0.35f, 0.48f, 1.00f);
    // Scrollbar
    c[ImGuiCol_ScrollbarBg]          = ImVec4(0.07f, 0.07f, 0.10f, 1.00f);
    c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.22f, 0.22f, 0.30f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.42f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]  = kGreen;
}

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------
DACWindow::DACWindow()
{
}

DACWindow::~DACWindow()
{
    if (m_hwnd)
        KillTimer(m_hwnd, TIMER_RENDER);

    if (m_hotKeyAtom)
    {
        UnregisterCurrentHotkey();
        GlobalDeleteAtom(m_hotKeyAtom);
    }
    if (m_nid.cbSize)
        Shell_NotifyIcon(NIM_DELETE, &m_nid);
    if (m_hMenu)
        DestroyMenu(m_hMenu);

    if (m_imguiReady)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_imguiReady = false;
    }

    CleanupDX();

    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Window creation
// ---------------------------------------------------------------------------
bool DACWindow::Create(HINSTANCE hInstance)
{
    m_hInstance = hInstance;

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProcStatic;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"DACImGuiClass";

    ATOM atom = RegisterClassExW(&wc);
    if (!atom && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        DWORD err = GetLastError();
        WCHAR msg[256];
        swprintf_s(msg, L"RegisterClassEx failed. Error code: %lu", err);
        MessageBoxW(nullptr, msg, L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    m_hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        L"DACImGuiClass",
        L"Default Audio Changer",
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        (int)m_width, (int)m_height,
        nullptr, nullptr,
        hInstance, this);

    if (!m_hwnd)
    {
        DWORD err = GetLastError();
        WCHAR msg[256];
        swprintf_s(msg, L"CreateWindow failed. Error code: %lu", err);
        MessageBoxW(nullptr, msg, L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!InitDX())
        return false;

    // ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;          // no imgui.ini
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Font - Segoe UI if present
    ImFontConfig fc;
    fc.OversampleH = 2;
    fc.OversampleV = 2;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 13.0f, &fc);
    if (io.Fonts->Fonts.empty())
        io.Fonts->AddFontDefault();

    ApplyDACTheme();

    ImGui_ImplWin32_Init(m_hwnd);
    ImGui_ImplDX11_Init(m_pDevice, m_pContext);
    m_imguiReady = true;

    // Context menu
    m_hMenu = CreatePopupMenu();
    AppendMenuW(m_hMenu, MF_STRING, IDM_OPTIONS, L"&Options");
    AppendMenuW(m_hMenu, MF_STRING, IDM_SWITCH,  L"&Switch");
    AppendMenuW(m_hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m_hMenu, MF_STRING, IDM_EXIT,    L"E&xit");
    SetMenuDefaultItem(m_hMenu, IDM_OPTIONS, FALSE);

    LoadAllState();

    // Start render timer
    SetTimer(m_hwnd, TIMER_RENDER, 16, nullptr); // ~60 fps

    return true;
}

void DACWindow::ShowWnd(int nCmdShow)
{
    ::ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
}

// ---------------------------------------------------------------------------
// D3D11
// ---------------------------------------------------------------------------
bool DACWindow::InitDX()
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = m_width;
    sd.BufferDesc.Height                  = m_height;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = m_hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    // Try hardware first, fall back to WARP software renderer
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, levels, 2,
        D3D11_SDK_VERSION, &sd, &m_pSwapChain,
        &m_pDevice, &featureLevel, &m_pContext);

    if (FAILED(hr))
    {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            0, levels, 2,
            D3D11_SDK_VERSION, &sd, &m_pSwapChain,
            &m_pDevice, &featureLevel, &m_pContext);
    }

    if (FAILED(hr))
    {
        MessageBoxW(nullptr, L"D3D11 init failed. Your system may not support DirectX 11.",
            L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    CreateRenderTarget();
    return true;
}

void DACWindow::CreateRenderTarget()
{
    ID3D11Texture2D* pBack = nullptr;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBack));
    if (pBack)
    {
        m_pDevice->CreateRenderTargetView(pBack, nullptr, &m_pRTV);
        pBack->Release();
    }
}

void DACWindow::CleanupRenderTarget()
{
    if (m_pRTV) { m_pRTV->Release(); m_pRTV = nullptr; }
}

void DACWindow::CleanupDX()
{
    CleanupRenderTarget();
    if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = nullptr; }
    if (m_pContext)   { m_pContext->Release();   m_pContext   = nullptr; }
    if (m_pDevice)    { m_pDevice->Release();    m_pDevice    = nullptr; }
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------
void DACWindow::Render()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    BuildUI();

    ImGui::Render();

    const float clear[4] = { 0.11f, 0.11f, 0.13f, 1.0f };
    m_pContext->OMSetRenderTargets(1, &m_pRTV, nullptr);
    m_pContext->ClearRenderTargetView(m_pRTV, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    m_pSwapChain->Present(1, 0);
}

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------
static std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(n - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
    return s;
}

static const char* HotkeyName(WORD hotkey)
{
    static char buf[64];
    if (!hotkey) { return "(none)"; }
    BYTE vk   = LOBYTE(LOWORD(hotkey));
    BYTE mods = HIBYTE(LOWORD(hotkey));
    buf[0] = '\0';
    if (mods & HOTKEYF_CONTROL) strcat_s(buf, "Ctrl+");
    if (mods & HOTKEYF_ALT)     strcat_s(buf, "Alt+");
    if (mods & HOTKEYF_SHIFT)   strcat_s(buf, "Shift+");
    char key[32] = {};
    GetKeyNameTextA(MapVirtualKeyA(vk, MAPVK_VK_TO_VSC) << 16, key, sizeof(key));
    strcat_s(buf, key);
    return buf;
}

void DACWindow::BuildUI()
{
    ImGuiIO& io = ImGui::GetIO();

    // Thin accent border around the borderless window
    {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        dl->AddRect(ImVec2(0.5f, 0.5f),
                    ImVec2((float)m_width - 0.5f, (float)m_height - 0.5f),
                    IM_COL32(18, 220, 122, 80), 0.0f, 0, 1.0f);
    }

    // Full-screen ImGui window — no title bar, no border
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)m_width, (float)m_height));
    ImGui::Begin("##main", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoMove     |
        ImGuiWindowFlags_NoScrollbar|
        ImGuiWindowFlags_NoSavedSettings);

    // ---- Devices panel ----
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.35f, 0.48f, 1.0f));
    ImGui::TextUnformatted("AUDIO DEVICES");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    auto& devices = m_devicesManager->GetAudioDevices();

    // Sync check state vector size
    if (m_deviceChecked.size() != devices.size())
        m_deviceChecked.resize(devices.size(), false);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.11f, 0.16f, 1.0f));
    float listH = (float)devices.size() * 22.0f + 8.0f;
    if (listH < 44.0f)  listH = 44.0f;
    if (listH > 130.0f) listH = 130.0f;

    ImGui::BeginChild("##devices", ImVec2(0, listH), true);

    // Find default device for highlight
    AudioDevice defaultDev;
    bool hasDefault = m_devicesManager->GetDefaultDevice(defaultDev);

    for (size_t i = 0; i < devices.size(); i++)
    {
        std::string name = WideToUtf8(devices[i].deviceName);
        bool isDefault = hasDefault && (devices[i].deviceId == defaultDev.deviceId);

        if (isDefault)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.80f, 0.45f, 1.0f));

        bool checked = m_deviceChecked[i];
        std::string label = (isDefault ? "* " : "  ") + name;
        if (ImGui::Checkbox(("##chk" + std::to_string(i)).c_str(), &checked))
        {
            m_deviceChecked[i] = checked;
            SaveDeviceCheckState(devices[i].deviceId, checked);
        }
        ImGui::SameLine();
        ImGui::Text("%s", label.c_str());

        if (isDefault)
            ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    // ---- Action buttons ----
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button,        kGreen);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kGreenHov);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  kGreenAct);
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.05f, 0.05f, 0.08f, 1.0f));
    if (ImGui::Button("Switch", ImVec2(90, 24)))
        DoSwitch();
    ImGui::PopStyleColor(4);

    ImGui::SameLine();
    if (ImGui::Button("Reload", ImVec2(90, 24)))
        DoReload();

    // ---- Settings section ----
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.35f, 0.48f, 1.0f));
    ImGui::TextUnformatted("SETTINGS");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Hotkey row
    bool hotkeyEnabled = m_hotkeyEnabled;
    if (ImGui::Checkbox("Global hotkey", &hotkeyEnabled))
    {
        m_hotkeyEnabled = hotkeyEnabled;
        if (!hotkeyEnabled)
        {
            UnregisterCurrentHotkey();
            RegDeleteKeyValue(m_appSettingsKey, nullptr, L"HotKey");
            m_hotkey = 0;
            m_capturingKey = false;
        }
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!m_hotkeyEnabled);

    if (m_capturingKey)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, kCoral);
        ImGui::PushStyleColor(ImGuiCol_Text,   ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Button("Press a key...", ImVec2(140, 0));
        ImGui::PopStyleColor(2);

        // Capture any key press
        for (int vk = 8; vk < 256; vk++)
        {
            if (vk == VK_ESCAPE) {
                if (ImGui::IsKeyPressed((ImGuiKey)(ImGuiKey_Escape)))
                {
                    m_capturingKey = false;
                    break;
                }
                continue;
            }
            if (GetAsyncKeyState(vk) & 0x8000)
            {
                BYTE mods = 0;
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000) mods |= HOTKEYF_CONTROL;
                if (GetAsyncKeyState(VK_MENU)    & 0x8000) mods |= HOTKEYF_ALT;
                if (GetAsyncKeyState(VK_SHIFT)   & 0x8000) mods |= HOTKEYF_SHIFT;

                // Skip if it's only a modifier key
                if (vk != VK_CONTROL && vk != VK_MENU && vk != VK_SHIFT &&
                    vk != VK_LCONTROL && vk != VK_RCONTROL &&
                    vk != VK_LMENU    && vk != VK_RMENU &&
                    vk != VK_LSHIFT   && vk != VK_RSHIFT)
                {
                    WORD newHotkey = MAKEWORD(vk, mods);
                    ApplyHotkey(newHotkey);
                    m_capturingKey = false;
                    break;
                }
            }
        }
    }
    else
    {
        std::string hkLabel = std::string(HotkeyName(m_hotkey)) + "  [change]";
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.13f, 0.30f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.10f, 0.40f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.07f, 0.22f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text,          kGreen);
        if (ImGui::Button(hkLabel.c_str(), ImVec2(140, 0)))
            m_capturingKey = true;
        ImGui::PopStyleColor(4);
    }

    ImGui::EndDisabled();

    ImGui::Spacing();

    bool startupEnabled = m_startupEnabled;
    if (ImGui::Checkbox("Start with Windows", &startupEnabled))
    {
        m_startupEnabled = startupEnabled;
        HKEY currentUser;
        if (RegOpenCurrentUser(KEY_ALL_ACCESS, &currentUser) == ERROR_SUCCESS)
        {
            HKEY runKey;
            if (RegOpenKeyEx(currentUser, STARTUP_RUN_PATH, 0, KEY_ALL_ACCESS, &runKey) == ERROR_SUCCESS)
            {
                if (startupEnabled)
                {
                    TCHAR path[MAX_PATH];
                    DWORD len = GetModuleFileName(nullptr, path, MAX_PATH);
                    RegSetValueEx(runKey, STARTUP_REG_KEY, 0, REG_SZ,
                        (BYTE*)path, len * sizeof(TCHAR));
                }
                else
                {
                    RegDeleteKeyValue(runKey, nullptr, STARTUP_REG_KEY);
                }
                RegCloseKey(runKey);
            }
            RegCloseKey(currentUser);
        }
    }

    // ---- Bottom buttons ----
    float bottomY = (float)m_height - 40.0f;
    ImGui::SetCursorPosY(bottomY);
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("About"))
        m_showAbout = true;

    ImGui::SameLine();
    float rightEdge = ImGui::GetWindowWidth()
                      - 2.0f * (65.0f + 4.0f)
                      - ImGui::GetStyle().WindowPadding.x;
    ImGui::SetCursorPosX(rightEdge);

    if (ImGui::Button("Minimize", ImVec2(65, 0)))
        ::ShowWindow(m_hwnd, SW_HIDE);

    ImGui::SameLine(0, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Button,        kCoral);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kCoralHov);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  kCoralAct);
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    if (ImGui::Button("Exit", ImVec2(65, 0)))
        DoExit();
    ImGui::PopStyleColor(4);

    ImGui::End();

    // ---- About modal ----
    if (m_showAbout)
        ImGui::OpenPopup("About##modal");

    ImGui::SetNextWindowSize(ImVec2(320, 150), ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2((float)m_width / 2, (float)m_height / 2),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("About##modal", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.20f, 0.65f, 1.0f, 1.0f));
        ImGui::Text("Default Audio Changer v1.0.5");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::TextWrapped("(c) 2011 Sergiu Giurgiu\nLicensed under the GPL v3\n\nUI rebuilt with Dear ImGui + D3D11");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(80, 0)))
        {
            m_showAbout = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------
void DACWindow::DoSwitch()
{
    auto& devices = m_devicesManager->GetAudioDevices();
    std::vector<std::wstring> ids;
    for (size_t i = 0; i < devices.size(); i++)
        if (i < m_deviceChecked.size() && m_deviceChecked[i])
            ids.push_back(devices[i].deviceId);

    if (SUCCEEDED(m_devicesManager->SwitchDevices(ids)))
        UpdateApplicationIcon();
}

void DACWindow::DoReload()
{
    m_devicesManager->LoadAudioDevices();
    LoadDeviceCheckStates();
    UpdateApplicationIcon();
}

void DACWindow::DoExit()
{
    UnregisterCurrentHotkey();
    if (m_hotKeyAtom)
    {
        GlobalDeleteAtom(m_hotKeyAtom);
        m_hotKeyAtom = 0;
    }
    PostQuitMessage(0);
}

// ---------------------------------------------------------------------------
// State loading
// ---------------------------------------------------------------------------
void DACWindow::LoadAllState()
{
    m_hotKeyAtom = GlobalAddAtom(L"DefaultAudioChangerImGui");
    LoadDeviceCheckStates();
    LoadHotkeyState();
    LoadStartupState();
}

void DACWindow::LoadDeviceCheckStates()
{
    auto& devices = m_devicesManager->GetAudioDevices();
    m_deviceChecked.assign(devices.size(), false);

    for (size_t i = 0; i < devices.size(); i++)
    {
        DWORD type;
        if (RegQueryValueEx(m_deviceSettingsKey,
                devices[i].deviceId.c_str(), nullptr, &type, nullptr, nullptr) == ERROR_SUCCESS)
            m_deviceChecked[i] = true;
    }
}

void DACWindow::LoadHotkeyState()
{
    DWORD hotKey = 0;
    DWORD sz = sizeof(DWORD);
    if (RegQueryValueEx(m_appSettingsKey, L"HotKey", nullptr, nullptr,
            (LPBYTE)&hotKey, &sz) == ERROR_SUCCESS && hotKey)
    {
        m_hotkey = (WORD)hotKey;
        m_hotkeyEnabled = true;
        ApplyHotkey(m_hotkey);
    }
}

void DACWindow::LoadStartupState()
{
    HKEY currentUser;
    if (RegOpenCurrentUser(KEY_ALL_ACCESS, &currentUser) != ERROR_SUCCESS)
        return;
    HKEY runKey;
    if (RegOpenKeyEx(currentUser, STARTUP_RUN_PATH, 0, KEY_READ, &runKey) == ERROR_SUCCESS)
    {
        DWORD type;
        m_startupEnabled = (RegQueryValueEx(runKey, STARTUP_REG_KEY,
            nullptr, &type, nullptr, nullptr) == ERROR_SUCCESS);
        RegCloseKey(runKey);
    }
    RegCloseKey(currentUser);
}

void DACWindow::SaveDeviceCheckState(const std::wstring& deviceId, bool checked)
{
    if (checked)
    {
        RegSetValueEx(m_deviceSettingsKey, deviceId.c_str(), 0, REG_SZ,
            (const BYTE*)deviceId.c_str(),
            (DWORD)(deviceId.size() * sizeof(wchar_t)));
    }
    else
    {
        RegDeleteKeyValue(m_deviceSettingsKey, nullptr, deviceId.c_str());
    }
}

void DACWindow::ApplyHotkey(WORD hotkey)
{
    UnregisterCurrentHotkey();
    m_hotkey = hotkey;
    BYTE vk   = LOBYTE(LOWORD(hotkey));
    BYTE mods = HIBYTE(LOWORD(hotkey));
    if (vk && RegisterHotKey(m_hwnd, m_hotKeyAtom, hkf2modf(mods), vk))
    {
        DWORD val = hotkey;
        RegSetValueEx(m_appSettingsKey, L"HotKey", 0, REG_BINARY,
            (BYTE*)&val, sizeof(val));
    }
}

void DACWindow::UnregisterCurrentHotkey()
{
    if (m_hotKeyAtom)
        UnregisterHotKey(m_hwnd, m_hotKeyAtom);
}

// ---------------------------------------------------------------------------
// Tray icon
// ---------------------------------------------------------------------------
bool DACWindow::ShowTrayIcon()
{
    AudioDevice dev;
    if (!m_devicesManager->GetDefaultDevice(dev))
        return false;

    if (!m_nid.cbSize)
    {
        m_nid.cbSize           = sizeof(m_nid);
        m_nid.hWnd             = m_hwnd;
        m_nid.uID              = TRAY_ICON_ID;
        m_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        m_nid.uCallbackMessage = WM_TRAYICON_MSG;
        m_nid.hIcon            = dev.largeIcon ? dev.largeIcon->GetIcon() : nullptr;
        _tcscpy_s(m_nid.szTip, dev.deviceName.c_str());
        return Shell_NotifyIcon(NIM_ADD, &m_nid) != FALSE;
    }
    else
    {
        m_nid.hIcon = dev.largeIcon ? dev.largeIcon->GetIcon() : nullptr;
        _tcscpy_s(m_nid.szTip, dev.deviceName.c_str());
        return Shell_NotifyIcon(NIM_MODIFY, &m_nid) != FALSE;
    }
}

void DACWindow::UpdateApplicationIcon()
{
    AudioDevice dev;
    if (!m_devicesManager->GetDefaultDevice(dev)) return;

    HICON large = dev.largeIcon ? dev.largeIcon->GetIcon() : nullptr;
    HICON small_ = dev.smallIcon ? dev.smallIcon->GetIcon() : nullptr;
    SendMessage(m_hwnd, WM_SETICON, ICON_BIG,   (LPARAM)large);
    SendMessage(m_hwnd, WM_SETICON, ICON_SMALL,  (LPARAM)small_);
    ShowTrayIcon();
}

void DACWindow::OnTrayIcon(WPARAM wp, LPARAM lp)
{
    switch (lp)
    {
    case WM_LBUTTONDBLCLK:
        ::ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
        break;
    case WM_RBUTTONUP:
        ShowContextMenu();
        break;
    }
}

void DACWindow::ShowContextMenu()
{
    SetForegroundWindow(m_hwnd);
    POINT pt;
    GetCursorPos(&pt);
    int cmd = TrackPopupMenu(m_hMenu,
        TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD,
        pt.x, pt.y, 0, m_hwnd, nullptr);

    switch (cmd)
    {
    case IDM_OPTIONS:
        ::ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
        break;
    case IDM_SWITCH:
        DoSwitch();
        break;
    case IDM_EXIT:
        DoExit();
        break;
    }
}

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------
void DACWindow::SetDevicesManager(CDevicesManager* dm)   { m_devicesManager = dm; }
void DACWindow::SetDeviceSettingsKey(HKEY key)           { m_deviceSettingsKey = key; }
void DACWindow::SetAppSettingsKey(HKEY key)              { m_appSettingsKey = key; }

// ---------------------------------------------------------------------------
// Hotkey helpers
// ---------------------------------------------------------------------------
BYTE DACWindow::hkf2modf(BYTE hkf)
{
    BYTE m = 0;
    if (hkf & HOTKEYF_ALT)     m |= MOD_ALT;
    if (hkf & HOTKEYF_SHIFT)   m |= MOD_SHIFT;
    if (hkf & HOTKEYF_CONTROL) m |= MOD_CONTROL;
    if (hkf & HOTKEYF_EXT)     m |= MOD_WIN;
    return m;
}

BYTE DACWindow::modf2hkf(BYTE modf)
{
    BYTE h = 0;
    if (modf & MOD_ALT)     h |= HOTKEYF_ALT;
    if (modf & MOD_SHIFT)   h |= HOTKEYF_SHIFT;
    if (modf & MOD_CONTROL) h |= HOTKEYF_CONTROL;
    if (modf & MOD_WIN)     h |= HOTKEYF_EXT;
    return h;
}

// ---------------------------------------------------------------------------
// WndProc
// ---------------------------------------------------------------------------
LRESULT CALLBACK DACWindow::WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }
    auto* self = reinterpret_cast<DACWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self)
        return self->WndProc(hwnd, msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT DACWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return TRUE;

    switch (msg)
    {
    case WM_TIMER:
        if (wParam == TIMER_RENDER && IsWindowVisible(m_hwnd))
            Render();
        return 0;

    case WM_SIZE:
        if (m_pSwapChain && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            m_pSwapChain->ResizeBuffers(0,
                LOWORD(lParam), HIWORD(lParam),
                DXGI_FORMAT_UNKNOWN, 0);
            m_width  = LOWORD(lParam);
            m_height = HIWORD(lParam);
            CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE)
        {
            ::ShowWindow(m_hwnd, SW_HIDE);
            return 0;
        }
        break;

    case WM_HOTKEY:
        if ((ATOM)wParam == m_hotKeyAtom)
            DoSwitch();
        return 0;

    case WM_TRAYICON_MSG:
        OnTrayIcon(wParam, lParam);
        return 0;

    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProcW(hwnd, msg, wParam, lParam);
        if (hit == HTCLIENT)
        {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            if (pt.y < 20)          // top 20px — drag strip
                return HTCAPTION;
        }
        return hit;
    }

    case WM_CLOSE:
        ::ShowWindow(hwnd, SW_HIDE);  // Alt+F4 → свернуть в трей
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_RENDER);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

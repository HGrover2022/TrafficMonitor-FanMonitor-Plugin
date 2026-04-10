#define NOMINMAX
#include "FANmoniter.h"
#include "resource.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <map>

// 全局实例
CFANmoniter g_instance;

// 对话框回调处理
INT_PTR CALLBACK OptionsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        CFANmoniter* pPlugin = (CFANmoniter*)lParam;
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pPlugin);
        SetDlgItemInt(hDlg, IDC_CPU_MAX_RPM, pPlugin->m_cpuMaxRpm, FALSE);
        SetDlgItemInt(hDlg, IDC_GPU_MAX_RPM, pPlugin->m_gpuMaxRpm, FALSE);
        CheckDlgButton(hDlg, IDC_ENABLE_COLOR, pPlugin->m_enableColor ? BST_CHECKED : BST_UNCHECKED);
        
        SetDlgItemTextW(hDlg, IDC_CPU_FAN_RPM_LABEL, pPlugin->m_cpuFanRpm.GetItemLableText());
        SetDlgItemTextW(hDlg, IDC_CPU_FAN_PCT_LABEL, pPlugin->m_cpuFanPct.GetItemLableText());
        SetDlgItemTextW(hDlg, IDC_GPU_FAN_RPM_LABEL, pPlugin->m_gpuFanRpm.GetItemLableText());
        SetDlgItemTextW(hDlg, IDC_GPU_FAN_PCT_LABEL, pPlugin->m_gpuFanPct.GetItemLableText());

        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            CFANmoniter* pPlugin = (CFANmoniter*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            pPlugin->m_cpuMaxRpm = GetDlgItemInt(hDlg, IDC_CPU_MAX_RPM, NULL, FALSE);
            pPlugin->m_gpuMaxRpm = GetDlgItemInt(hDlg, IDC_GPU_MAX_RPM, NULL, FALSE);
            pPlugin->m_enableColor = (IsDlgButtonChecked(hDlg, IDC_ENABLE_COLOR) == BST_CHECKED);
            pPlugin->m_cpuFanRpm.SetEnableColor(pPlugin->m_enableColor);
            pPlugin->m_cpuFanPct.SetEnableColor(pPlugin->m_enableColor);
            pPlugin->m_gpuFanRpm.SetEnableColor(pPlugin->m_enableColor);
            pPlugin->m_gpuFanPct.SetEnableColor(pPlugin->m_enableColor);

            if (pPlugin->m_cpuMaxRpm <= 0) pPlugin->m_cpuMaxRpm = 7055;
            if (pPlugin->m_gpuMaxRpm <= 0) pPlugin->m_gpuMaxRpm = 7400;

            wchar_t buf[256];
            GetDlgItemTextW(hDlg, IDC_CPU_FAN_RPM_LABEL, buf, 256); pPlugin->m_cpuFanRpm.SetCustomLabel(buf);
            GetDlgItemTextW(hDlg, IDC_CPU_FAN_PCT_LABEL, buf, 256); pPlugin->m_cpuFanPct.SetCustomLabel(buf);
            GetDlgItemTextW(hDlg, IDC_GPU_FAN_RPM_LABEL, buf, 256); pPlugin->m_gpuFanRpm.SetCustomLabel(buf);
            GetDlgItemTextW(hDlg, IDC_GPU_FAN_PCT_LABEL, buf, 256); pPlugin->m_gpuFanPct.SetCustomLabel(buf);

            pPlugin->SaveConfig();
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDC_RESTORE_DEFAULTS)
        {
            SetDlgItemInt(hDlg, IDC_CPU_MAX_RPM, 7055, FALSE);
            SetDlgItemInt(hDlg, IDC_GPU_MAX_RPM, 7400, FALSE);
            CheckDlgButton(hDlg, IDC_ENABLE_COLOR, BST_CHECKED);
            SetDlgItemTextW(hDlg, IDC_CPU_FAN_RPM_LABEL, L"CPU Fan");
            SetDlgItemTextW(hDlg, IDC_CPU_FAN_PCT_LABEL, L"CPU Fan");
            SetDlgItemTextW(hDlg, IDC_GPU_FAN_RPM_LABEL, L"GPU Fan");
            SetDlgItemTextW(hDlg, IDC_GPU_FAN_PCT_LABEL, L"GPU Fan");
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// CFanItem 实现
CFanItem::CFanItem(const wchar_t* name, const wchar_t* id, const wchar_t* labelPrefix, const wchar_t* defaultFormat)
    : m_itemName(name), m_itemId(id), m_labelPrefix(labelPrefix), m_format(defaultFormat), m_rpm(0), m_percentage(0.0),
      m_labelColor(RGB(255, 255, 255)), m_valueColor(RGB(255, 255, 255)), m_enableColor(true)
{}

const wchar_t* CFanItem::GetItemName() const { return m_itemName.c_str(); }
const wchar_t* CFanItem::GetItemId() const { return m_itemId.c_str(); }
const wchar_t* CFanItem::GetItemLableText() const 
{ 
    return m_customLabel.empty() ? m_labelPrefix.c_str() : m_customLabel.c_str(); 
}

const wchar_t* CFanItem::GetItemValueText() const 
{
    if (!m_errorMsg.empty()) return m_errorMsg.c_str();

    static std::map<std::wstring, std::wstring> textCache;
    std::wstring& formatted = textCache[m_itemId]; 
    
    formatted = m_format;
    
    size_t pos = 0;
    std::wstring rpmStr = std::to_wstring(m_rpm);
    while ((pos = formatted.find(L"X", pos)) != std::wstring::npos)
    {
        formatted.replace(pos, 1, rpmStr);
        pos += rpmStr.length();
    }

    pos = 0;
    std::wstringstream wss;
    wss << std::fixed << std::setprecision(1) << m_percentage;
    std::wstring pctStr = wss.str();
    while ((pos = formatted.find(L"Y", pos)) != std::wstring::npos)
    {
        formatted.replace(pos, 1, pctStr);
        pos += pctStr.length();
    }

    return formatted.c_str();
}

const wchar_t* CFanItem::GetItemValueSampleText() const 
{ 
    if (m_itemId.find(L"rpm") != std::wstring::npos) return L"2500 RPM";
    return L"35.0%";
}

int CFanItem::GetItemWidthEx(void* hDC) const
{
    HDC hdc = (HDC)hDC;
    std::wstring label = GetItemLableText();
    std::wstring text = label + GetItemValueText();
    SIZE size;
    GetTextExtentPoint32W(hdc, text.c_str(), (int)text.length(), &size);
    return size.cx + 12; // 增加一些内边距防止截断
}

COLORREF GetSmoothColor(int rpm)
{
    auto Lerp = [](BYTE a, BYTE b, double t) -> BYTE {
        return (BYTE)(a + (b - a) * t);
    };

    auto LerpColor = [&](COLORREF c1, COLORREF c2, double t) -> COLORREF {
        return RGB(
            Lerp(GetRValue(c1), GetRValue(c2), t),
            Lerp(GetGValue(c1), GetGValue(c2), t),
            Lerp(GetBValue(c1), GetBValue(c2), t)
        );
    };

    if (rpm <= 0) return RGB(0, 30, 255);
    
    if (rpm < 1110)
    {
        double t = (double)rpm / 1110.0;
        return LerpColor(RGB(0, 30, 255), RGB(0, 255, 115), t);
    }
    else if (rpm < 2220)
    {
        double t = (double)(rpm - 1110) / 1110.0;
        return LerpColor(RGB(0, 255, 115), RGB(106, 255, 0), t);
    }
    else if (rpm < 3330)
    {
        double t = (double)(rpm - 2220) / 1110.0;
        return LerpColor(RGB(106, 255, 0), RGB(255, 162, 0), t);
    }
    else if (rpm < 4440)
    {
        double t = (double)(rpm - 3330) / 1110.0;
        return LerpColor(RGB(255, 162, 0), RGB(255, 0, 0), t);
    }
    else if (rpm < 5550)
    {
        double t = (double)(rpm - 4440) / 1110.0;
        return LerpColor(RGB(255, 0, 0), RGB(253, 45, 215), t);
    }
    else
    {
        return RGB(253, 45, 215);
    }
}

void CFanItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC hdc = (HDC)hDC;
    SetBkMode(hdc, TRANSPARENT);

    // 获取并绘制标签文本
    std::wstring label = GetItemLableText();
    
    SIZE labelSize = { 0, 0 };
    if (!label.empty())
        GetTextExtentPoint32W(hdc, label.c_str(), (int)label.length(), &labelSize);
    
    SetTextColor(hdc, m_labelColor);
    RECT labelRect = { x, y, x + labelSize.cx, y + h };
    DrawTextW(hdc, label.c_str(), -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);

    // 绘制数值文本
    std::wstring value = GetItemValueText();
    COLORREF color;
    if (m_enableColor)
    {
        color = GetSmoothColor(m_rpm);
    }
    else
    {
        color = m_valueColor;
    }
    SetTextColor(hdc, color);
    
    RECT valueRect = { x + labelSize.cx, y, x + w + 5, y + h };
    DrawTextW(hdc, value.c_str(), -1, &valueRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
}

void CFanItem::SetFanData(int rpm, double percentage)
{
    m_rpm = rpm;
    m_percentage = percentage;
    m_errorMsg.clear();
}

void CFanItem::SetError(const wchar_t* reason) 
{ 
    m_errorMsg = reason; 
    m_rpm = 0; 
    m_percentage = 0.0; 
}

// CFANmoniter 实现
CFANmoniter::CFANmoniter() 
    : m_hInpOutDll(nullptr), m_dllLoaded(false), m_inited(false),
      m_cpuMaxRpm(7055), m_gpuMaxRpm(7400), m_enableColor(true),
      m_cpuFanRpm(L"CPU Fan RPM", L"cpu_fan_rpm", L"CPU Fan", L"X RPM"),
      m_cpuFanPct(L"CPU Fan %", L"cpu_fan_pct", L"CPU Fan", L"Y%"),
      m_gpuFanRpm(L"GPU Fan RPM", L"gpu_fan_rpm", L"GPU Fan", L"X RPM"),
      m_gpuFanPct(L"GPU Fan %", L"gpu_fan_pct", L"GPU Fan", L"Y%")
{}

IPluginItem* CFANmoniter::GetItem(int index)
{
    switch (index)
    {
    case 0: return &m_cpuFanRpm;
    case 1: return &m_cpuFanPct;
    case 2: return &m_gpuFanRpm;
    case 3: return &m_gpuFanPct;
    default: return nullptr;
    }
}

bool read_ec(unsigned char addr, unsigned char* pValue, lpfnDlPortReadPortUchar pfnRead, lpfnDlPortWritePortUchar pfnWrite)
{
    if (!pfnRead || !pfnWrite) return false;
    auto wait_ec = [&](unsigned char port, unsigned char flag, bool set) {
        for (int i = 0; i < 1000; i++) {
            if (((pfnRead(port) & flag) != 0) == set) return true;
        }
        return false;
    };
    if (!wait_ec(0x66, 0x02, false)) return false;
    pfnWrite(0x66, 0x80);
    if (!wait_ec(0x66, 0x02, false)) return false;
    pfnWrite(0x62, addr);
    if (!wait_ec(0x66, 0x01, true)) return false;
    *pValue = pfnRead(0x62);
    return true;
}

void CFANmoniter::DataRequired()
{
    if (!m_inited) Init();
    if (m_dllLoaded) {
        unsigned char ch, cl, gh, gl;
        if (read_ec(0x64, &ch, m_pfnDlPortReadPortUchar, m_pfnDlPortWritePortUchar) &&
            read_ec(0x65, &cl, m_pfnDlPortReadPortUchar, m_pfnDlPortWritePortUchar)) {
            int r = (ch << 8) | cl;
            m_cpuFanRpm.SetFanData(r, (r * 100.0) / m_cpuMaxRpm);
            m_cpuFanPct.SetFanData(r, (r * 100.0) / m_cpuMaxRpm);
        }
        if (read_ec(0x6C, &gh, m_pfnDlPortReadPortUchar, m_pfnDlPortWritePortUchar) &&
            read_ec(0x6D, &gl, m_pfnDlPortReadPortUchar, m_pfnDlPortWritePortUchar)) {
            int r = (gh << 8) | gl;
            m_gpuFanRpm.SetFanData(r, (r * 100.0) / m_gpuMaxRpm);
            m_gpuFanPct.SetFanData(r, (r * 100.0) / m_gpuMaxRpm);
        }
    }
}

ITMPlugin::OptionReturn CFANmoniter::ShowOptionsDialog(void* hParent) {
    HMODULE h = NULL; GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)TMPluginGetInstance, &h);
    if (DialogBoxParamW(h, MAKEINTRESOURCEW(IDD_OPTIONS_DIALOG), (HWND)hParent, OptionsDlgProc, (LPARAM)this) == IDOK) return OR_OPTION_CHANGED;
    return OR_OPTION_UNCHANGED;
}

const wchar_t* CFANmoniter::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME: return L"FANmoniter";
    case TMI_DESCRIPTION: return L"Monitor CPU and GPU fan speed via EC.";
    case TMI_AUTHOR: return L"HGrover";
    case TMI_VERSION: return L"114514.1919-8964";
    case TMI_COPYRIGHT: return L"Copyright (C) 2026 HGrover";
    case TMI_URL: return L"https://github.com/HGrover2022";
    default: return L"";
    }
}

const wchar_t* CFANmoniter::GetTooltipInfo() { return L"Fan Monitor Active"; }

void CFANmoniter::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) 
{ 
    if (index == EI_CONFIG_DIR) LoadConfig(data); 
    else if (index == EI_LABEL_TEXT_COLOR)
    {
        COLORREF color = _wtoi(data);
        m_cpuFanRpm.SetLabelColor(color); m_cpuFanPct.SetLabelColor(color);
        m_gpuFanRpm.SetLabelColor(color); m_gpuFanPct.SetLabelColor(color);
    }
    else if (index == EI_VALUE_TEXT_COLOR)
    {
        COLORREF color = _wtoi(data);
        m_cpuFanRpm.SetValueColor(color); m_cpuFanPct.SetValueColor(color);
        m_gpuFanRpm.SetValueColor(color); m_gpuFanPct.SetValueColor(color);
    }
}

void CFANmoniter::OnInitialize(ITrafficMonitor* pApp) { m_pApp = pApp; }

void CFANmoniter::LoadConfig(const std::wstring& configDir)
{
    m_configPath = configDir + L"FANmoniter.ini";
    m_cpuMaxRpm = GetPrivateProfileIntW(L"Settings", L"CpuMaxRpm", 7055, m_configPath.c_str());
    m_gpuMaxRpm = GetPrivateProfileIntW(L"Settings", L"GpuMaxRpm", 7400, m_configPath.c_str());
    m_enableColor = GetPrivateProfileIntW(L"Settings", L"EnableColor", 1, m_configPath.c_str()) != 0;

    auto updateItems = [&]() {
        m_cpuFanRpm.SetEnableColor(m_enableColor); m_cpuFanPct.SetEnableColor(m_enableColor);
        m_gpuFanRpm.SetEnableColor(m_enableColor); m_gpuFanPct.SetEnableColor(m_enableColor);
    };
    updateItems();

    auto loadItem = [&](CFanItem& item, const wchar_t* key) {
        wchar_t buf[256];
        if (GetPrivateProfileStringW(L"Formats", key, item.GetFormat(), buf, 256, m_configPath.c_str()))
            item.SetFormat(buf);
        if (GetPrivateProfileStringW(L"Labels", key, L"", buf, 256, m_configPath.c_str()))
        {
            std::wstring label = buf;
            if (label.length() >= 2 && label.front() == L'\"' && label.back() == L'\"')
                label = label.substr(1, label.length() - 2);
            item.SetCustomLabel(label.c_str());
        }
    };
    loadItem(m_cpuFanRpm, L"cpu_rpm"); loadItem(m_cpuFanPct, L"cpu_pct");
    loadItem(m_gpuFanRpm, L"gpu_rpm"); loadItem(m_gpuFanPct, L"gpu_pct");
}

void CFANmoniter::SaveConfig()
{
    WritePrivateProfileStringW(L"Settings", L"CpuMaxRpm", std::to_wstring(m_cpuMaxRpm).c_str(), m_configPath.c_str());
    WritePrivateProfileStringW(L"Settings", L"GpuMaxRpm", std::to_wstring(m_gpuMaxRpm).c_str(), m_configPath.c_str());
    WritePrivateProfileStringW(L"Settings", L"EnableColor", m_enableColor ? L"1" : L"0", m_configPath.c_str());
    
    auto saveItem = [&](CFanItem& item, const wchar_t* key) {
        WritePrivateProfileStringW(L"Formats", key, item.GetFormat(), m_configPath.c_str());
        std::wstring label = L"\"" + std::wstring(item.GetItemLableText()) + L"\"";
        WritePrivateProfileStringW(L"Labels", key, label.c_str(), m_configPath.c_str());
    };
    saveItem(m_cpuFanRpm, L"cpu_rpm"); saveItem(m_cpuFanPct, L"cpu_pct");
    saveItem(m_gpuFanRpm, L"gpu_rpm"); saveItem(m_gpuFanPct, L"gpu_pct");
}

void CFANmoniter::RestoreDefaults()
{
    m_cpuMaxRpm = 7055; m_gpuMaxRpm = 7400; m_enableColor = true;
    m_cpuFanRpm.SetEnableColor(true); m_cpuFanPct.SetEnableColor(true);
    m_gpuFanRpm.SetEnableColor(true); m_gpuFanPct.SetEnableColor(true);
    m_cpuFanRpm.SetFormat(L"X RPM"); m_cpuFanPct.SetFormat(L"Y%");
    m_gpuFanRpm.SetFormat(L"X RPM"); m_gpuFanPct.SetFormat(L"Y%");
    m_cpuFanRpm.SetCustomLabel(L""); m_cpuFanPct.SetCustomLabel(L"");
    m_gpuFanRpm.SetCustomLabel(L""); m_gpuFanPct.SetCustomLabel(L"");
    SaveConfig();
}

void CFANmoniter::Init()
{
    if (m_inited) return;
    m_inited = true;
    m_hInpOutDll = LoadLibraryW(L"InpOutx64.dll");
    if (m_hInpOutDll) {
        m_pfnDlPortReadPortUchar = (lpfnDlPortReadPortUchar)GetProcAddress(m_hInpOutDll, "DlPortReadPortUchar");
        m_pfnDlPortWritePortUchar = (lpfnDlPortWritePortUchar)GetProcAddress(m_hInpOutDll, "DlPortWritePortUchar");
        if (m_pfnDlPortReadPortUchar && m_pfnDlPortWritePortUchar) m_dllLoaded = true;
    }
    if (!m_dllLoaded) {
        const wchar_t* err = L"DLL Error";
        m_cpuFanRpm.SetError(err); m_cpuFanPct.SetError(err);
        m_gpuFanRpm.SetError(err); m_gpuFanPct.SetError(err);
    }
}

#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance() 
    { 
        return &g_instance; 
    }
#ifdef __cplusplus
}
#endif

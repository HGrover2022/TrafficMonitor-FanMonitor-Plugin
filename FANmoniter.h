#pragma once

#include <windows.h>
#include "PluginInterface.h"
#include <string>

// 定义 InpOutx64.dll 中的函数指针类型
typedef unsigned char (*lpfnDlPortReadPortUchar)(unsigned short Port);
typedef void (*lpfnDlPortWritePortUchar)(unsigned short Port, unsigned char Value);

// Fan Item
class CFanItem : public IPluginItem
{
public:
    CFanItem(const wchar_t* name, const wchar_t* id, const wchar_t* labelPrefix, const wchar_t* defaultFormat);

    // IPluginItem 接口实现 - 必须严格保持顺序
    virtual const wchar_t* GetItemName() const override;
    virtual const wchar_t* GetItemId() const override;
    virtual const wchar_t* GetItemLableText() const override;
    virtual const wchar_t* GetItemValueText() const override;
    virtual const wchar_t* GetItemValueSampleText() const override;
    virtual bool IsCustomDraw() const override { return true; }
    virtual int GetItemWidth() const override { return 0; }
    virtual int GetItemWidthEx(void* hDC) const override;
    virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;
    virtual int OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag) override { return 0; }
    virtual int OnKeboardEvent(int key, bool ctrl, bool shift, bool alt, void* hWnd, int flag) override { return 0; }
    virtual void* OnItemInfo(ItemInfoType type, void* para1, void* para2) override { return 0; }
    virtual int IsDrawResourceUsageGraph() const override { return 1; }
    virtual float GetResourceUsageGraphValue() const override { return (float)(m_percentage / 100.0); }

    void SetFanData(int rpm, double percentage);
    void SetError(const wchar_t* reason = L"Error");
    void SetFormat(const wchar_t* fmt) { m_format = fmt; }
    const wchar_t* GetFormat() const { return m_format.c_str(); }

    void SetLabelColor(COLORREF color) { m_labelColor = color; }
    void SetValueColor(COLORREF color) { m_valueColor = color; }
    void SetEnableColor(bool enable) { m_enableColor = enable; }
    void SetCustomLabel(const wchar_t* label) { m_customLabel = label; }

private:
    std::wstring m_itemName;
    std::wstring m_itemId;
    std::wstring m_labelPrefix;
    std::wstring m_customLabel;
    std::wstring m_format;
    std::wstring m_errorMsg;
    int m_rpm;
    double m_percentage;
    COLORREF m_labelColor;
    COLORREF m_valueColor;
    bool m_enableColor;
};

// 插件主类
class CFANmoniter : public ITMPlugin
{
public:
    CFANmoniter();

    // ITMPlugin 接口实现 - 必须严格保持顺序
    virtual int GetAPIVersion() const override { return 7; }
    virtual IPluginItem* GetItem(int index) override;
    virtual void DataRequired() override;
    virtual OptionReturn ShowOptionsDialog(void* hParent) override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
    virtual void OnMonitorInfo(const MonitorInfo& monitor_info) override {}
    virtual const wchar_t* GetTooltipInfo() override;
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override;
    virtual void* GetPluginIcon() override { return nullptr; }
    virtual int GetCommandCount() override { return 0; }
    virtual const wchar_t* GetCommandName(int command_index) override { return nullptr; }
    virtual void* GetCommandIcon(int command_index) override { return nullptr; }
    virtual void OnPluginCommand(int command_index, void* hWnd, void* para) override {}
    virtual int IsCommandChecked(int command_index) override { return false; }
    virtual void OnInitialize(ITrafficMonitor* pApp) override;

public:
    void Init();
    void LoadConfig(const std::wstring& configDir);
    void SaveConfig();
    void RestoreDefaults();

    // 4 Items
    CFanItem m_cpuFanRpm;
    CFanItem m_cpuFanPct;
    CFanItem m_gpuFanRpm;
    CFanItem m_gpuFanPct;

    // 配置参数
    int m_cpuMaxRpm;
    int m_gpuMaxRpm;
    bool m_enableColor;
    std::wstring m_configPath;

    // InpOutx64.dll 相关
    HMODULE m_hInpOutDll;
    bool m_dllLoaded;
    bool m_inited;

    lpfnDlPortReadPortUchar m_pfnDlPortReadPortUchar;
    lpfnDlPortWritePortUchar m_pfnDlPortWritePortUchar;

    ITrafficMonitor* m_pApp;
};

#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance();
#ifdef __cplusplus
}
#endif

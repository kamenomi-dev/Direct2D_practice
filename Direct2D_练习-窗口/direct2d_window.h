#pragma once
#include <string>
#include <unordered_map>
#include <d2d1.h>
#include <windows.h>

#include "direct2d_base.h"

namespace Direct2D_UI {
class Window {
  public:
    inline static std::wstring WindowClassName = L"Direct2D_window";

  private:
    inline static HINSTANCE                         _Instance = nullptr;
    inline static std::unordered_map<HWND, Window*> _InstanceMap{};

  public:
    Window() = default;
    ~Window() {
        DiscardDeviceResources();
        DiscardDeviceIndependentResources();

        if (!_InstanceMap.empty()) {
            return;
        }

        UnregisterClassW(WindowClassName.c_str(), _Instance);
    }

    void Initialize(
        HINSTANCE instance, std::wstring title = L"", D2D1_POINT_2L position = {0, 0}, D2D1_SIZE_U size = {800, 600}
    ) {
        if (title.empty()) _windowTitle = title;
        _windowPosition = position;
        _windowSize     = size;

        static ATOM classAtom = NULL;
        if (classAtom != NULL) {
            return;
        }

        _Instance = instance;

        WNDCLASSEXW classInfo{};
        classInfo.cbSize        = sizeof WNDCLASSEXW;
        classInfo.cbClsExtra    = 0;
        classInfo.cbWndExtra    = 0;
        classInfo.style         = CS_HREDRAW | CS_VREDRAW;
        classInfo.hInstance     = instance;
        classInfo.lpszMenuName  = L"";
        classInfo.lpszClassName = WindowClassName.c_str();
        classInfo.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        classInfo.lpfnWndProc   = Window::MainMessageProcedure;
        classInfo.hIcon         = nullptr;
        classInfo.hIconSm       = nullptr;
        classInfo.hCursor       = LoadCursor(nullptr, IDC_ARROW);

        classAtom = RegisterClassExW(&classInfo);
    }

    void Create() {
        _window = CreateWindowExW(
            NULL,
            WindowClassName.c_str(),
            _windowTitle.c_str(),
            WS_VISIBLE | WS_OVERLAPPEDWINDOW,
            _windowPosition.x,
            _windowPosition.y,
            _windowSize.width,
            _windowSize.height,
            nullptr,
            nullptr,
            _Instance,
            (void*)this
        );
    }

    auto& GetSize() const { return _windowSize; }
    auto& GetPosition() const { return _windowPosition; }

    bool SetSize(
        D2D_SIZE_U size
    ) {
        if (SetWindowPos(_window, nullptr, 0, 0, size.width, size.height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOZORDER)) {
            _windowSize = size;
            return true;
        }

        return false;
    }
    bool SetPosition(
        D2D_POINT_2L position
    ) {
        if (SetWindowPos(_window, nullptr, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOZORDER)) {
            _windowPosition = position;
            return true;
        }

        return false;
    }

    auto& GetMousePosition() const { return _mousePosition; }

  public:
    virtual void CreateDeviceResources(Graphics& graphics) {};
    virtual void DiscardDeviceResources() {};

    virtual void CreateDeviceIndependentResources(Graphics& graphics) {};
    virtual void DiscardDeviceIndependentResources() {};

    virtual bool OnRender(
        Graphics& graphics
    ) {
        return false;
    }

  private:
    bool CALLBACK HittestProcesdure(
        UINT message, WPARAM wParam, LPARAM lParam, _Out_ LRESULT& result
    ) {
        result = S_OK;
        return false;
    }

    bool CALLBACK MessageProcedure(
        UINT message, WPARAM wParam, LPARAM lParam, _Out_ LRESULT& result
    ) {
        result = S_OK;

        if (message == WM_CREATE) {
            Direct2D_UI::GetDirect2DFactory().AttachWindow(_window, _graphics);
            CreateDeviceIndependentResources(_graphics);
            CreateDeviceResources(_graphics);

            return true;
        }

        if (message == WM_DESTROY) {
            DiscardDeviceResources();
            DiscardDeviceIndependentResources();
            return true;
        }

        if (message == WM_SIZE) {
            D2D_SIZE_U size = {LOWORD(lParam), HIWORD(lParam)};
            _graphics.SetGraphicsSize(size);
            _windowSize = size;
            return true;
        }

        if (message == WM_MOVE) {
            _windowPosition = {LOWORD(lParam), HIWORD(lParam)};
            return true;
        }

        if (message == WM_PAINT) {
            _graphics.BeginDraw();
            OnRender(_graphics);
            _graphics.EndDraw();
            return true;
        }

        if (LRESULT procedureResult = NULL; HittestProcesdure(message, wParam, lParam, procedureResult)) {
            return procedureResult;
        }

        if (message == WM_MOUSEMOVE) {
            // float dpiX, dpiY;
            // _graphics.GetPointer()->GetDpi(&dpiX, &dpiY);
            //  _mousePosition = {.x = LOWORD(lParam) * 96.f / dpiX, .y = HIWORD(lParam) * 96.f / dpiY};
            _mousePosition = {.x = LOWORD(lParam), .y = HIWORD(lParam)};

            return true;
        }
    }

    static bool CALLBACK WindowMarginProcesdure(
        HWND window, UINT message, WPARAM wParam, LPARAM lParam, _Out_ LRESULT& result
    ) {
        result = NULL;

        if (message == WM_ACTIVATE) {
            MARGINS margins = {0};
            DwmExtendFrameIntoClientArea(window, &margins);

            return true;
        }

        if (message == WM_NCCALCSIZE) {
            int EdgeWi = GetSystemMetrics(SM_CXSIZEFRAME);
            int EdgeHe = GetSystemMetrics(SM_CYSIZEFRAME);

            if (wParam == FALSE) {
                auto* ClientRect    = (RECT*)lParam;
                ClientRect->top    += 0;
                ClientRect->left   += EdgeWi;
                ClientRect->right  -= EdgeHe;
                ClientRect->bottom -= EdgeHe;
                return true;
            }

            auto* param            = (NCCALCSIZE_PARAMS*)lParam;
            param->rgrc[0].top    += 0;
            param->rgrc[0].left   += EdgeWi;
            param->rgrc[0].right  -= EdgeHe;
            param->rgrc[0].bottom -= EdgeHe;
            return true;
        }

        return false;
    }

    static LRESULT CALLBACK MainMessageProcedure(
        HWND window, UINT message, WPARAM wParam, LPARAM lParam
    ) {
        Window* currentInstance = nullptr;
        {
            auto result = _InstanceMap.find(window);
            if (result != _InstanceMap.end()) {
                currentInstance = result->second;
            }
        }

        LRESULT procedureResult = NULL;
        if (WindowMarginProcesdure(window, message, wParam, lParam, procedureResult)) {
            return procedureResult;
        }

        if (message == WM_CREATE) {
            const auto* createStruct = (CREATESTRUCTW*)lParam;
            currentInstance          = (Window*)createStruct->lpCreateParams;

            if (currentInstance) {
                currentInstance->_window = window;
                _InstanceMap[window]     = currentInstance;
            }
        }

        if (message == WM_DESTROY) {
            _InstanceMap.erase(window);

            if (_InstanceMap.empty()) {
                PostQuitMessage(NULL);
            }

            return NULL;
        }

        if (currentInstance == nullptr) {
            return DefWindowProcW(window, message, wParam, lParam);
        }

        if (currentInstance->MessageProcedure(message, wParam, lParam, procedureResult)) {
            return procedureResult;
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

  private:
    HWND _window = nullptr;

    std::wstring          _windowTitle    = L"窗口 1";
    D2D1_POINT_2L         _windowPosition = {0, 0};
    D2D1_SIZE_U           _windowSize     = {0, 0};
    Direct2D_UI::Graphics _graphics       = {};

    D2D1_POINT_2L _mousePosition = {0, 0};
};

inline auto DoMessageLoop() {
    MSG msg;

    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
} // namespace Direct2D_UI
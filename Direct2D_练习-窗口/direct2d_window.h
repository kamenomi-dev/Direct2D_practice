#pragma once
#include <bitset>
#include <string>
#include <unordered_map>
#include <d2d1.h>
#include <windows.h>

#include "direct2d_base.h"

namespace Direct2D_UI {
struct MouseStatus {
    enum class Types : unsigned char {
        Left = 0,
        Right,
        Middle
    };

    D2D1_POINT_2L position = {0, 0};
    Types         mouseType;
    bool          isUp          = false;
    bool          isDown        = false;
    bool          isDoubleClick = false;
    bool          isMove        = false;
    bool          isNonClient   = false;
    unsigned char hittestResult = HTNOWHERE;
};

struct WindowProperties {
    D2D1::ColorF CaptionColor    = {0x252527, 1.f};
    D2D1::ColorF BackgroundColor = {0x28282B, 1.f};
};

class Window {
  public:
    inline static std::wstring WindowClassName = L"Direct2D_window";

  private:
    inline static HINSTANCE                         _Instance = nullptr;
    inline static std::unordered_map<HWND, Window*> _InstanceMap{};

  public:
    Window() = default;
    virtual ~Window() {
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
            WS_EX_APPWINDOW,
            WindowClassName.c_str(),
            _windowTitle.c_str(),
            WS_VISIBLE | WS_OVERLAPPEDWINDOW & ~WS_SYSMENU,
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

    auto& GetMousePosition() const { return _mouse.position; }

    void SetBackgroundColor(
        const D2D1::ColorF& color
    ) {
        _properties.BackgroundColor = color;
        _nativeBrush.SetColor(color);
    }

  public:
    virtual void CreateDeviceResources(
        Graphics& graphics
    ) {
        graphics.CreateSolidColorBrush(_nativeBrush);
    };
    virtual void DiscardDeviceResources() { _nativeBrush.Discard(); };

    virtual void CreateDeviceIndependentResources() {};
    virtual void DiscardDeviceIndependentResources() {};

    virtual bool OnRender(
        Graphics& graphics
    ) {
        return false;
    }

  private:
    void NativeRender(
        Graphics& graphics
    ) {
        graphics.GetPointer()->Clear();

        D2D1_RECT_F rect = {0, 0, GetSize().width * 1.f, GetSize().height * 1.f};
        _nativeBrush.SetColor(_properties.BackgroundColor);
        graphics.FillRectangle(rect, _nativeBrush);

        // Caption
        {
            const auto static height = (float)GetSystemMetrics(SM_CYCAPTION) + (float)GetSystemMetrics(SM_CYSIZEFRAME) + (float)GetSystemMetrics(SM_CXPADDEDBORDER);

            rect.bottom = height;
            _nativeBrush.SetColor(_properties.CaptionColor);
            graphics.FillRectangle(rect, _nativeBrush);

            const auto static buttonWidth = GetSystemMetrics(SM_CXSIZE);

            if (_mouse.hittestResult == HTCLOSE || _mouse.hittestResult == HTZOOM || _mouse.hittestResult == HTREDUCE) {
                if (_mouse.hittestResult == HTCLOSE) {
                    rect.left  = (float)GetSize().width - (float)buttonWidth * 1;
                    rect.right = (float)GetSize().width - (float)buttonWidth * 0;
                }

                if (_mouse.hittestResult == HTZOOM) {
                    rect.left  = (float)GetSize().width - (float)buttonWidth * 2;
                    rect.right = (float)GetSize().width - (float)buttonWidth * 1;
                }

                if (_mouse.hittestResult == HTREDUCE) {
                    rect.left  = (float)GetSize().width - (float)buttonWidth * 3;
                    rect.right = (float)GetSize().width - (float)buttonWidth * 2;
                }

                _nativeBrush.SetColor(D2D1::ColorF::Red);

                if (_mouse.isDown) {
                    _nativeBrush.SetColor(D2D1::ColorF::DarkRed);
                }

                graphics.FillRectangle(rect, _nativeBrush);
            };
        }
    }

    bool CALLBACK HittestProcesdure(
        UINT message, WPARAM, LPARAM lParam, _Out_ LRESULT& result
    ) {
        result = S_OK;

        static auto borderSize = GetSystemMetrics(SM_CYSIZEFRAME);

        if (message != WM_NCHITTEST) {
            return false;
        }

        POINT mousePosition{LOWORD(lParam), HIWORD(lParam)};
        ScreenToClient(_window, &mousePosition);

        if (mousePosition.x <= borderSize || mousePosition.x >= (int)_windowSize.width - borderSize) {
            return false;
        }

        if (mousePosition.y <= borderSize) {
            result = HTTOP;
            return true;
        }

        if (static auto captionHight = GetSystemMetrics(SM_CYCAPTION); mousePosition.y <= borderSize + captionHight) {
            static auto buttonWidth = GetSystemMetrics(SM_CXSIZE);
            auto        relatedLeft = (int)_windowSize.width - 3 * buttonWidth;
            if (relatedLeft + 2 * buttonWidth <= mousePosition.x) {
                result = HTCLOSE;
                return true;
            }

            if (relatedLeft + 1 * buttonWidth <= mousePosition.x) {
                result = HTMAXBUTTON;
                return true;
            }

            if (relatedLeft + 0 * buttonWidth <= mousePosition.x) {
                result = HTMINBUTTON;
                return true;
            }

            result = HTCAPTION;
            return true;
        }

        return false;
    }

    bool CALLBACK MessageProcedure(
        UINT message, WPARAM wParam, LPARAM lParam, _Out_ LRESULT& result
    ) {
        result = S_OK;

        static auto            isReged = false;
        static TRACKMOUSEEVENT trackEvent{.cbSize = sizeof TRACKMOUSEEVENT, .dwFlags = TME_LEAVE | TME_NONCLIENT, .dwHoverTime = HOVER_DEFAULT};
        trackEvent.hwndTrack = _window;

        if (message == WM_CREATE) {
            Direct2D_UI::GetDirect2DFactory().AttachWindow(_window, _graphics);
            CreateDeviceIndependentResources();
            CreateDeviceResources(_graphics);
            return true;
        }

        if (message == WM_MOUSELEAVE) {
            isReged = false;
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
            PAINTSTRUCT ps{};
            BeginPaint(_window, &ps);

            {
                _graphics.BeginDraw();
                NativeRender(_graphics);
                OnRender(_graphics);
                if (auto status = _graphics.EndDraw(); FAILED(status) || status == D2DERR_RECREATE_TARGET) {
                    DiscardDeviceResources();
                    _graphics.Discard();

                    Direct2D_UI::GetDirect2DFactory().AttachWindow(_window, _graphics);
                    CreateDeviceResources(_graphics);
                }

                // _mouse = MouseStatus{.position = _mouse.position};
            }

            EndPaint(_window, &ps);
            return true;
        }

        if (HittestProcesdure(message, wParam, lParam, result)) {
            return true;
        }

        if ((message >= WM_NCMOUSEMOVE && message <= WM_NCMBUTTONDBLCLK) || (message >= WM_MOUSEFIRST && message <= WM_MBUTTONDBLCLK)
            || (message == WM_MOUSELEAVE || message == WM_NCMOUSELEAVE)) {
            auto isFromNativeMessage = message >= WM_NCMOUSEMOVE && message <= WM_NCMBUTTONDBLCLK;

            _mouse.hittestResult = HTCLIENT;
            if (isFromNativeMessage) {
                _mouse.hittestResult = (unsigned char)wParam;
            }

            if (message == WM_MOUSEMOVE || message == WM_NCMOUSEMOVE) {
                _mouse.position = {.x = LOWORD(lParam), .y = HIWORD(lParam)};

                if (isFromNativeMessage) {
                    ScreenToClient(_window, &_mouse.position);
                }
            }

            auto idx = isFromNativeMessage ? (message - WM_NCMOUSEMOVE) : (message - WM_MOUSEMOVE);

            _mouse.isNonClient = !isFromNativeMessage;
            _mouse.isMove      = idx;
            if (_mouse.isMove == 0) {
                TrackMouseEvent(&trackEvent);
                InvalidateRect(_window, nullptr, FALSE);
                return true;
            }

            _mouse.mouseType     = (MouseStatus::Types)((idx - 1) / 3);
            auto action          = (unsigned char)((idx - 1) % 3);
            _mouse.isDown        = (action == 0);
            _mouse.isUp          = (action == 1);
            _mouse.isDoubleClick = (action == 2);

            if (isFromNativeMessage && wParam == HTCAPTION) {
                InvalidateRect(_window, nullptr, FALSE);
                return false;
            }

            if (message == WM_NCLBUTTONUP) {
                if (wParam == HTREDUCE) ShowWindow(_window, SW_MINIMIZE);
                if (wParam == HTZOOM) ShowWindow(_window, SW_MAXIMIZE);
                if (wParam == HTCLOSE) DestroyWindow(_window);
            }

            InvalidateRect(_window, nullptr, FALSE);
            return true;
        }

        return false;
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

        if (message == WM_ERASEBKGND) {
            return NULL;
        }

        if (currentInstance == nullptr) {
            return DefWindowProcW(window, message, wParam, lParam);
        }

        if (message == WM_DESTROY) {
            _InstanceMap.erase(window);

            if (_InstanceMap.empty()) {
                PostQuitMessage(NULL);
            }

            return NULL;
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

  private:
    MouseStatus                  _mouse;
    WindowProperties             _properties;
    Direct2D_UI::SolidColorBrush _nativeBrush;
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
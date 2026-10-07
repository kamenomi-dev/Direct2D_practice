#pragma once
#include "direct2d_window_const_scale.h"
#include "direct2d_base.h"
#include "direct2d_dwrite.h"
#include "direct2d_render.h"
#include "direct2d_resource_manager.h"

#include <bitset>
#include <string>
#include <unordered_map>
#include <d2d1.h>
#include <windows.h>

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
    char          hittestResult = HTNOWHERE;
    D2D1_POINT_2L dragDelta;
};

struct WindowProperties {
    D2D1::ColorF TextColor       = {0xFFFFFF, 1.f};
    D2D1::ColorF CaptionColor    = {0x252527, 1.f};
    D2D1::ColorF BackgroundColor = {0x28282B, 1.f};
};

class BaseWindow {
  public:
    inline static std::wstring WindowClassName = L"Direct2D_window";

  private:
    inline static HINSTANCE                             _Instance = nullptr;
    inline static std::unordered_map<HWND, BaseWindow*> _InstanceMap{};

  public:
    BaseWindow() = default;
    virtual ~BaseWindow() {
        DiscardDeviceResources();
        DiscardDeviceIndependentResources();

        if (!_InstanceMap.empty()) {
            return;
        }

        UnregisterClassW(WindowClassName.c_str(), _Instance);
    }

    void Initialize(
        HINSTANCE instance, std::wstring title = L"Untitled Window 1", D2D1_POINT_2L position = {0, 0}, D2D1_SIZE_U size = {800, 600}
    ) {
        _windowTitle    = title;
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
        classInfo.lpfnWndProc   = BaseWindow::MainMessageProcedure;
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

    virtual void CreateDeviceIndependentResources() {
        const auto captionHeight = (float)(Direct2D_UI::Window::ConstScale::WINDOW_CAPTION_HEIGHT * 96.0 / _currentDpi);
        GetDWriteFactory().CreateTextFormat(L"Segoe UI", DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, captionHeight * 0.5f, _nativeTextFormat);
        _nativeTextFormat.SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    };
    virtual void DiscardDeviceIndependentResources() { _nativeTextFormat.Discard(); };

    virtual bool OnRender(
        Graphics& graphics
    ) {
        return false;
    }

  private:
    void FillRectangleOnce(
        _In_ Graphics& graphics, _In_ const D2D1_RECT_F& rect, _In_ const D2D1::ColorF& color
    ) {
        _nativeBrush.SetColor(color);
        graphics.FillRectangle(rect, _nativeBrush);
    }

    void NativeRenderWindowFrame(
        Graphics& graphics
    ) {
        auto rect = D2D1::RectF(0, 0, (float)GetSize().width, (float)GetSize().height);
        FillRectangleOnce(graphics, rect, _properties.BackgroundColor);

        rect.bottom = (float)(Direct2D_UI::Window::ConstScale::WINDOW_CAPTION_HEIGHT / 96.0 * _currentDpi);
        FillRectangleOnce(graphics, rect, _properties.CaptionColor);

        _nativeBrush.SetColor(_properties.TextColor);
        _nativeTextFormat.SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        _nativeTextFormat.SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        graphics.GetPointer()->DrawTextW(_windowTitle.c_str(), (uint32_t)_windowTitle.length(), _nativeTextFormat.GetPointer(), rect, _nativeBrush.GetPointer());

        rect.right -= 3 * (float)(Direct2D_UI::Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_WIDTH / 96.0 * _currentDpi);

        graphics.DrawTextLayout(
            {.x = rect.right, .y = 0.f}, _maximized ? Resource::ResourceManager::MaximumControlPanelLayout : Resource::ResourceManager::NormalControlPanelLayout, _nativeBrush
        );
    }

    void NativeRender(
        Graphics& graphics
    ) {
        graphics.GetPointer()->Clear();
        NativeRenderWindowFrame(graphics);

        // Caption
        {
            const auto  width  = (float)(Direct2D_UI::Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_WIDTH / 96.0 * _currentDpi);
            const auto  height = (float)(Direct2D_UI::Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_HEIGHT / 96.0 * _currentDpi);
            D2D1_RECT_F rect   = {0, 0, GetSize().width * 1.f, height};

            if (_mouse.hittestResult == HTCLOSE || _mouse.hittestResult == HTZOOM || _mouse.hittestResult == HTREDUCE) {
                if (_mouse.hittestResult == HTCLOSE) {
                    rect.left  = (float)GetSize().width - width * 1;
                    rect.right = (float)GetSize().width - width * 0;
                }

                if (_mouse.hittestResult == HTZOOM) {
                    rect.left  = (float)GetSize().width - width * 2;
                    rect.right = (float)GetSize().width - width * 1;
                }

                if (_mouse.hittestResult == HTREDUCE) {
                    rect.left  = (float)GetSize().width - width * 3;
                    rect.right = (float)GetSize().width - width * 2;
                }

                _nativeBrush.SetColor(D2D1::ColorF{D2D1::ColorF::GhostWhite, 0.5});

                if (_mouse.isDown) {
                    _nativeBrush.SetColor(D2D1::ColorF{D2D1::ColorF::GhostWhite, 0.25});
                }

                graphics.FillRectangle(rect, _nativeBrush);
            }
        }
    }

    bool NativeCaptionHitTest(
        const POINT& position, LRESULT& result
    ) {
        const auto borderSize = GetSystemMetrics(SM_CYSIZEFRAME);

        if (position.x <= borderSize || position.x >= (int)_windowSize.width - borderSize) {
            return false;
        }

        if (position.y <= borderSize) {
            result = HTTOP;
            return true;
        }

        if (const auto captionHight = Direct2D_UI::Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_HEIGHT / 96.0 * _currentDpi; position.y <= borderSize + captionHight) {
            const auto buttonWidth = Direct2D_UI::Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_WIDTH / 96.0 * _currentDpi;
            const auto relatedLeft = _windowSize.width - 3 * buttonWidth;
            if (relatedLeft + 2 * buttonWidth <= position.x) {
                result = HTCLOSE;
                return true;
            }

            if (relatedLeft + 1 * buttonWidth <= position.x) {
                result = HTMAXBUTTON;
                return true;
            }

            if (relatedLeft + 0 * buttonWidth <= position.x) {
                result = HTMINBUTTON;
                return true;
            }

            result = HTCAPTION;
            return true;
        }

        return false;
    }

    bool CALLBACK HitTestMessageProcesdure(
        UINT message, WPARAM, LPARAM lParam, _Out_ LRESULT& result
    ) {
        result = NULL;

        if (message != WM_NCHITTEST) {
            return false;
        }

        POINT mousePosition{LOWORD(lParam), HIWORD(lParam)};
        ScreenToClient(_window, &mousePosition);

        return NativeCaptionHitTest(mousePosition, result);
    }

    bool CALLBACK MouseMessageProcedure(
        UINT message, WPARAM wParam, LPARAM lParam, _Out_ LRESULT& result
    ) {
        result = NULL;

        static bool     isTracked = false;
        TRACKMOUSEEVENT trackEvent{.cbSize = sizeof TRACKMOUSEEVENT, .dwFlags = TME_LEAVE | TME_NONCLIENT, .hwndTrack = _window};

        if (message == WM_NCMOUSELEAVE) {
            isTracked = false;
            _mouse    = MouseStatus{.position = _mouse.position};
            InvalidateRect(_window, nullptr, FALSE);
            return true;
        };

        const bool isNCMouseMessage     = message >= WM_NCMOUSEMOVE && message <= WM_NCMBUTTONDBLCLK;
        const bool isClientMouseMessage = message >= WM_MOUSEFIRST && message <= WM_MBUTTONDBLCLK;
        const bool isMouseMoveMessage   = message == WM_MOUSEMOVE || message == WM_NCMOUSEMOVE;

        if (!isNCMouseMessage && !isClientMouseMessage) {
            return false;
        }

        if (_mouse.isDown) {
            _mouse.dragDelta = {.x = LOWORD(lParam) - _mouse.position.x, .y = HIWORD(lParam) - _mouse.position.y};
        } else {
            _mouse.position = {.x = LOWORD(lParam), .y = HIWORD(lParam)};

            if (isNCMouseMessage) {
                ScreenToClient(_window, &_mouse.position);
            }
        }

        _mouse.hittestResult = HTCLIENT;
        LRESULT actualHitTest{};
        if (NativeCaptionHitTest(_mouse.position, actualHitTest)) {
            _mouse.hittestResult = (char)actualHitTest;
        }

        _mouse.isMove      = isMouseMoveMessage;
        _mouse.isNonClient = isNCMouseMessage;

        if (_mouse.isMove) {
            if (!isTracked && _mouse.isNonClient) {
                isTracked = true;
                TrackMouseEvent(&trackEvent);
            }

            InvalidateRect(_window, nullptr, FALSE);
            return true;
        }

        const auto index  = isNCMouseMessage ? (message - WM_NCMOUSEMOVE) : (message - WM_MOUSEMOVE);
        const auto action = (index - 1) % 3;

        _mouse.mouseType     = (MouseStatus::Types)((index - 1) / 3);
        _mouse.isDown        = (action == 0);
        _mouse.isUp          = (action == 1);
        _mouse.isDoubleClick = (action == 2);

        if (_mouse.isUp) {
            _mouse.dragDelta = {0, 0};
        }

        // Preserve that system process HTCAPTION hit test.
        if (isNCMouseMessage && wParam == HTCAPTION) {
            InvalidateRect(_window, nullptr, FALSE);
            return false;
        }

        InvalidateRect(_window, nullptr, FALSE);

        if (message == WM_NCLBUTTONUP) {
            if (wParam == HTREDUCE) ShowWindow(_window, SW_MINIMIZE);
            if (wParam == HTZOOM) ShowWindow(_window, _maximized ? SW_RESTORE : SW_MAXIMIZE);
            if (wParam == HTCLOSE) DestroyWindow(_window);
        }

        if (message == WM_NCLBUTTONDOWN) {
            return wParam == HTREDUCE || wParam == HTZOOM || wParam == HTCLOSE;
        }

        return true;
    }

    bool CALLBACK MessageProcedure(
        UINT message, WPARAM wParam, LPARAM lParam, _Out_ LRESULT& result
    ) {
        result = NULL;

        if (message == WM_CREATE) {
            _currentDpi = GetDpiForWindow(_window);

            Direct2D_UI::GetDirect2DFactory().AttachWindow(_window, _graphics);
            CreateDeviceIndependentResources();
            CreateDeviceResources(_graphics);
            return true;
        }

        if (message == WM_DESTROY) {
            DiscardDeviceResources();
            DiscardDeviceIndependentResources();
            return true;
        }

        if (message == WM_SIZE) {
            const auto size = D2D1::SizeU(LOWORD(lParam), HIWORD(lParam));
            _graphics.SetGraphicsSize(size);
            _maximized  = wParam == SIZE_MAXIMIZED;
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

            _graphics.BeginDraw();
            NativeRender(_graphics);
            OnRender(_graphics);

            if (auto status = _graphics.EndDraw(); FAILED(status) || status == D2DERR_RECREATE_TARGET) {
                DiscardDeviceResources();
                _graphics.Discard();

                Direct2D_UI::GetDirect2DFactory().AttachWindow(_window, _graphics);
                CreateDeviceResources(_graphics);
            }

            EndPaint(_window, &ps);
            return true;
        }

        if (HitTestMessageProcesdure(message, wParam, lParam, result)) {
            return true;
        }

        if (MouseMessageProcedure(message, wParam, lParam, result)) {
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
            int edgeWidth = GetSystemMetrics(SM_CXSIZEFRAME);
            int edgeHight = GetSystemMetrics(SM_CYSIZEFRAME);

            if (wParam) {
                auto* param            = (NCCALCSIZE_PARAMS*)lParam;
                param->rgrc[0].top    += 0;
                param->rgrc[0].left   += edgeWidth;
                param->rgrc[0].right  -= edgeHight;
                param->rgrc[0].bottom -= edgeHight;
                return true;
            }

            auto* ClientRect    = (RECT*)lParam;
            ClientRect->top    += 0;
            ClientRect->left   += edgeWidth;
            ClientRect->right  -= edgeHight;
            ClientRect->bottom -= edgeHight;
            return true;
        }

        return false;
    }

    static LRESULT CALLBACK MainMessageProcedure(
        HWND window, UINT message, WPARAM wParam, LPARAM lParam
    ) {
        BaseWindow* currentInstance = nullptr;
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
            currentInstance          = (BaseWindow*)createStruct->lpCreateParams;

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

    uint32_t              _currentDpi = 96;
    std::wstring          _windowTitle;
    D2D1_POINT_2L         _windowPosition = {0, 0};
    D2D1_SIZE_U           _windowSize     = {0, 0};
    bool                  _maximized;
    Direct2D_UI::Graphics _graphics;

  private:
    MouseStatus                  _mouse;
    WindowProperties             _properties;
    Direct2D_UI::SolidColorBrush _nativeBrush;
    Direct2D_UI::TextFormat      _nativeTextFormat;
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
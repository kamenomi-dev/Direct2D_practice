#pragma once
#include "window_const_scale.h"
#include "direct2d/interface.h"
#include "direct2d/dwrite.h"
#include "direct2d/render.h"
#include "direct2d/resource_manager.h"
#include "components/window.h"

#include <bitset>
#include <chrono>
#include <string>
#include <unordered_map>
#include <d2d1.h>
#include <Windows.h>

namespace Direct2D_UI {
class BaseWindow {
    using MouseStatus = Components::MouseStatus;

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

    auto& GetWindowTitle() const { return _componentRoot._windowProperties.Title; }

    auto& GetSize() const { return _componentRoot._windowProperties.WindowSize; }
    auto& GetPosition() const { return _componentRoot._windowProperties.WindowPosition; }

    bool SetSize(
        D2D_SIZE_U size
    ) {
        if (SetWindowPos(_window, nullptr, 0, 0, size.width, size.height, SWP_NOMOVE | SWP_NOZORDER)) {
            _componentRoot._windowProperties.WindowSize = size;
            return true;
        }

        return false;
    }
    bool SetPosition(
        D2D_POINT_2L position
    ) {
        if (SetWindowPos(_window, nullptr, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER)) {
            _componentRoot._windowProperties.WindowPosition = position;
            return true;
        }

        return false;
    }

    void SetBackgroundColor(
        const D2D1::ColorF& color
    ) {
        _nativeBrush.SetColor(_componentRoot._windowProperties.BackgroundColor = color);
    }

    auto& GetMousePosition() const { return _mouse.position; }

    auto& GetRenderableWindow() { return _componentRoot; }

    void Initialize(
        HINSTANCE instance, std::wstring title = L"Untitled Window 1", D2D1_POINT_2L position = {0, 0}, D2D1_SIZE_U size = {800, 600}
    ) {
        _componentRoot._windowProperties = {.Title = title, .WindowPosition = position, .WindowSize = size};

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
            GetWindowTitle().c_str(),
            WS_VISIBLE | WS_OVERLAPPEDWINDOW & ~WS_SYSMENU,
            GetPosition().x,
            GetPosition().y,
            GetSize().width,
            GetSize().height,
            nullptr,
            nullptr,
            _Instance,
            (void*)this
        );
    }

  public:
    virtual void CreateDeviceResources(
        Direct2D::Graphics& graphics
    ) {
        graphics.CreateSolidColorBrush(_nativeBrush);
    };
    virtual void DiscardDeviceResources() { _nativeBrush.Discard(); };

    virtual void CreateDeviceIndependentResources() {
        // Comment: The main resources are conquered by ResourceManager class.
    };
    virtual void DiscardDeviceIndependentResources() {
        // Comment: The main resources are conquered by ResourceManager class.
    };

    virtual void InitializeComponents() = 0;

    // [[deprecated("后期窗口或可能不支持直接这样绘制")]]
    virtual bool OnRender(
        Direct2D::Graphics& graphics
    ) {
        return false;
    }

  private:
    void UpdateWindowDpi(
        uint32_t dpi
    ) {
        _currentDpi = dpi;
        _componentRoot.UpdateRenderDpi((float)dpi);
    }

    inline void FillRectangleOnce(
        _In_ Direct2D::Graphics& graphics, _In_ const D2D1_RECT_F& rect, _In_ const D2D1::ColorF& color
    ) {
        _nativeBrush.SetColor(color);
        graphics.FillRectangle(rect, _nativeBrush);
    }

    bool NativeHitTest(
        const POINT& position, LRESULT& result
    ) {
        const auto borderSize = GetSystemMetricsForDpi(SM_CYSIZEFRAME, _currentDpi);

        if (position.x <= borderSize || position.x >= (int)GetSize().width - borderSize) {
            return false;
        }

        if (position.y <= borderSize) {
            result = HTTOP;
            return true;
        }

        if (const auto captionHight = Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_HEIGHT / 96.0 * _currentDpi; position.y <= borderSize + captionHight) {
            const auto buttonWidth = Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_WIDTH / 96.0 * _currentDpi;
            const auto relatedLeft = GetSize().width - 3 * buttonWidth;
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

        return NativeHitTest(mousePosition, result);
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
            _componentRoot.HandleMouseStatus(_mouse);
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
        if (NativeHitTest(_mouse.position, actualHitTest)) {
            _mouse.hittestResult = (char)actualHitTest;
        }

        _mouse.isMove      = isMouseMoveMessage;
        _mouse.isNonClient = isNCMouseMessage;

        if (_mouse.isMove) {
            if (!isTracked && _mouse.isNonClient) {
                isTracked = true;
                TrackMouseEvent(&trackEvent);
            }

            _componentRoot.HandleMouseStatus(_mouse);
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
            _componentRoot.HandleMouseStatus(_mouse);
            InvalidateRect(_window, nullptr, FALSE);
            return false;
        }

        _componentRoot.HandleMouseStatus(_mouse);
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
            UpdateWindowDpi(GetDpiForWindow(_window));
            Direct2D::Resource::ResourceManager::GetInstance().UpdateResources(_currentDpi);

            Direct2D::D2DFactory::GetInstance().AttachWindow(_window, _graphics);
            CreateDeviceIndependentResources();
            CreateDeviceResources(_graphics);
            InitializeComponents();

            _componentRoot.UpdateRenderTarget(_graphics);
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
            _componentRoot._windowProperties.WindowFrameState = (uint32_t)wParam;
            _componentRoot._windowProperties.WindowSize       = size;
            return true;
        }

        if (message == WM_MOVE) {
            _componentRoot._windowProperties.WindowPosition = {LOWORD(lParam), HIWORD(lParam)};
            return true;
        }

        if (message == WM_DPICHANGED) {
            _currentDpi = HIWORD(wParam);

            const auto* suggested = (RECT*)lParam;
            SetWindowPos(_window, nullptr, suggested->left, suggested->top, suggested->right - suggested->left, suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);

            Direct2D::Resource::ResourceManager::GetInstance().UpdateResources(_currentDpi);
            InvalidateRect(_window, nullptr, FALSE);
            return true;
        }

        if (message == WM_PAINT) {
            PAINTSTRUCT ps{};
            BeginPaint(_window, &ps);

            _graphics.BeginDraw();
            _graphics.GetPointer()->Clear();
            OnRender(_graphics);

            Components::ComponentRenderEvent event{};
            _componentRoot.MessageProcedure(&event);

            if (auto status = _graphics.EndDraw(); FAILED(status) || status == D2DERR_RECREATE_TARGET) {
                DiscardDeviceResources();
                _graphics.Discard();

                Direct2D::D2DFactory::GetInstance().AttachWindow(_window, _graphics);
                CreateDeviceResources(_graphics);
                _componentRoot.UpdateRenderTarget(_graphics);
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

        if (message != WM_NCCALCSIZE) {
            return false;
        }

        const auto dpi       = GetDpiForWindow(window);
        const int  edgeWidth = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi);
        const int  edgeHight = GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi);

        if (wParam) {
            auto* param            = (NCCALCSIZE_PARAMS*)lParam;
            param->rgrc[0].top    += 0;
            param->rgrc[0].left   += edgeWidth;
            param->rgrc[0].right  -= edgeWidth;
            param->rgrc[0].bottom -= edgeHight;
            return true;
        }

        auto* clientRect    = (RECT*)lParam;
        clientRect->top    += 0;
        clientRect->left   += edgeWidth;
        clientRect->right  -= edgeWidth;
        clientRect->bottom -= edgeHight;
        return true;
    }

    static LRESULT CALLBACK MainMessageProcedure(
        HWND window, UINT message, WPARAM wParam, LPARAM lParam
    ) {
        BaseWindow* instance = nullptr;
        {
            auto result = _InstanceMap.find(window);
            if (result != _InstanceMap.end()) {
                instance = result->second;
            }
        }

        if (message == WM_CREATE) {
            const auto* createStruct = (CREATESTRUCTW*)lParam;
            instance                 = (BaseWindow*)createStruct->lpCreateParams;

            if (instance) {
                instance->_window    = window;
                _InstanceMap[window] = instance;
            }
        }

        if (message == WM_ERASEBKGND) {
            return NULL;
        }

        LRESULT procedureResult = NULL;
        if (WindowMarginProcesdure(window, message, wParam, lParam, procedureResult)) {
            return procedureResult;
        }

        if (instance == nullptr) {
            return DefWindowProcW(window, message, wParam, lParam);
        }

        if (message == WM_DESTROY) {
            _InstanceMap.erase(window);

            if (_InstanceMap.empty()) {
                PostQuitMessage(NULL);
            }

            return NULL;
        }

        if (instance->MessageProcedure(message, wParam, lParam, procedureResult)) {
            return procedureResult;
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

  private:
    HWND _window = nullptr;

    uint32_t _currentDpi = 96;
    bool     _maximized  = false;

    Direct2D::Graphics _graphics;
    Components::Window _componentRoot;

  private:
    MouseStatus               _mouse;
    Direct2D::SolidColorBrush _nativeBrush;
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
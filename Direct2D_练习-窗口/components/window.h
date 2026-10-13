#pragma once
#include "component.h"

#include <string>

namespace Direct2D_UI::Components {
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

struct SharedWindowProperties {
    std::wstring  Title;
    D2D1_POINT_2L WindowPosition   = {0, 0};
    D2D1_SIZE_U   WindowSize       = {0, 0};
    UINT          WindowFrameState = SIZE_RESTORED; // using as SIZE_

    D2D1::ColorF TextColor       = {0xFFFFFF, 1.f};
    D2D1::ColorF CaptionColor    = {0x252527, 1.f};
    D2D1::ColorF BackgroundColor = {0x28282B, 1.f};

    MouseStatus MouseStatus;
};

class Window final : public Component {
  public:
    Window() = default;

    bool IsMaximized() const { return _windowProperties.WindowFrameState == SIZE_MAXIMIZED; }

    D2D1_POINT_2F GetPosition() const override { return Direct2D::PointLToF(_windowProperties.WindowPosition); };
    D2D1_SIZE_F   GetSize() const override { return Direct2D::SizeUToF(_windowProperties.WindowSize); };

    D2D1::ColorF& GetTextColor() { return _windowProperties.TextColor; }
    D2D1::ColorF& GetCaptionColor() { return _windowProperties.CaptionColor; }
    D2D1::ColorF& GetBackgroundColor() { return _windowProperties.BackgroundColor; }

    MouseStatus& GetMouseStatus() { return _windowProperties.MouseStatus; }

    void UpdateRenderDpi(
        float dpi
    ) {
        _renderDpi = dpi;
    }

    void CreateResources() { _graphicsRef.CreateSolidColorBrush(_nativeBrush); }
    void DiscardResources() { _nativeBrush.Discard(); }

    inline void FillRectangleOnce(
        _In_ Direct2D::Graphics& graphics, _In_ const D2D1_RECT_F& rect, _In_ const D2D1::ColorF& color
    ) {
        _nativeBrush.SetColor(color);
        graphics.FillRectangle(rect, _nativeBrush);
    }

    bool Render() {
        auto rect = D2D1::RectF(0, 0, (float)GetSize().width, (float)GetSize().height);
        FillRectangleOnce(_graphicsRef, rect, GetBackgroundColor());

        // Caption

        rect.bottom = (float)(Direct2D_UI::Window::ConstScale::WINDOW_CAPTION_HEIGHT / 96.0 * _renderDpi);
        FillRectangleOnce(_graphicsRef, rect, GetCaptionColor());

        _nativeBrush.SetColor(GetTextColor());
        Direct2D::Resource::ResourceManager::GeneralFont_Normal.SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        Direct2D::Resource::ResourceManager::GeneralFont_Normal.SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        _graphicsRef.GetPointer()->DrawTextW(
            _windowProperties.Title.c_str(),
            (uint32_t)_windowProperties.Title.length(),
            Direct2D::Resource::ResourceManager::GeneralFont_Normal.GetPointer(),
            rect,
            _nativeBrush.GetPointer()
        );

        rect.right -= 3 * Direct2D_UI::Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_WIDTH / 96.f * _renderDpi;

        _graphicsRef.DrawTextLayout(
            {.x = rect.right, .y = 0.f},
            IsMaximized() ? Direct2D::Resource::ResourceManager::MaximumControlPanelLayout : Direct2D::Resource::ResourceManager::NormalControlPanelLayout,
            _nativeBrush
        );

        const auto width  = Direct2D_UI::Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_WIDTH / 96.f * _renderDpi;
        const auto height = Direct2D_UI::Window::ConstScale::WINDOW_CONTROL_PANEL_BUTTON_HEIGHT / 96.f * _renderDpi;
        rect.right        = (float)GetSize().width;
        rect.bottom       = height;

        const auto& mouse = GetMouseStatus();
        if (mouse.hittestResult == HTCLOSE || mouse.hittestResult == HTZOOM || mouse.hittestResult == HTREDUCE) {
            if (mouse.hittestResult == HTCLOSE) {
                rect.left  = (float)GetSize().width - width * 1;
                rect.right = (float)GetSize().width - width * 0;
            }

            if (mouse.hittestResult == HTZOOM) {
                rect.left  = (float)GetSize().width - width * 2;
                rect.right = (float)GetSize().width - width * 1;
            }

            if (mouse.hittestResult == HTREDUCE) {
                rect.left  = (float)GetSize().width - width * 3;
                rect.right = (float)GetSize().width - width * 2;
            }

            _nativeBrush.SetColor(D2D1::ColorF{D2D1::ColorF::GhostWhite, 0.5});

            if (mouse.isDown) {
                _nativeBrush.SetColor(D2D1::ColorF{D2D1::ColorF::GhostWhite, 0.25});
            }

            _graphicsRef.FillRectangle(rect, _nativeBrush);
        }
        return true;
    }

  private:
    friend class Direct2D_UI::BaseWindow;
    void HandleMouseStatus(
        const MouseStatus& status
    ) {
        _windowProperties.MouseStatus = status;
    }

  private:
    friend class Direct2D_UI::BaseWindow;
    SharedWindowProperties _windowProperties;

    Direct2D::SolidColorBrush _nativeBrush;
};
} // namespace Direct2D_UI::Components
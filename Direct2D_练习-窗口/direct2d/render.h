#pragma once
#include "interface.h"
#include "dwrite.h"

#include <type_traits>
#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

namespace Direct2D_UI::Direct2D {
template <class T>
    requires std::is_base_of_v<ID2D1Resource, T>
class D2DResource : public D2DInterface<T> {
  protected:
    using D2DInterface<T>::SetLastResult;

  public:
    D2DResource() = default;
    explicit D2DResource(
        ID2D1HwndRenderTarget* target
    ) {
        if (target == nullptr) {
            SetLastResult(E_INVALIDARG);
        }

        _renderTarget = target;
        SetLastResult(S_OK);
    }

    void Move(
        D2DResource& destination
    ) {
        __super::Move(destination);
        destination._renderTarget = _renderTarget;

        _renderTarget = nullptr;
    }

  protected:
    auto* GetRenderTarget() { return _renderTarget; }

  private:
    ID2D1HwndRenderTarget* _renderTarget = nullptr;
};

class SolidColorBrush : public D2DResource<ID2D1SolidColorBrush> {
  public:
    SolidColorBrush() = default;

  private:
    friend class Graphics;
    SolidColorBrush(
        ID2D1HwndRenderTarget* graphics
    )
    : D2DResource(graphics) {
        if (!SUCCEEDED(GetLastResult())) {
            return;
        }

        SetLastResult(GetRenderTarget()->CreateSolidColorBrush(D2D1::ColorF(0, 0.f), &GetPointerRef()));
    };
    SolidColorBrush(
        ID2D1HwndRenderTarget* graphics, const D2D1::ColorF& color
    )
    : D2DResource(graphics) {
        if (!SUCCEEDED(GetLastResult())) {
            return;
        }

        SetLastResult(GetRenderTarget()->CreateSolidColorBrush(color, &GetPointerRef()));
    };

  public:
    void SetColor(
        const D2D1::ColorF& color
    ) {
        if (IsValid()) GetPointer()->SetColor(color);
    }
};

class Graphics : public D2DInterface<ID2D1HwndRenderTarget> {
  public:
    Graphics() = default;

    void AddRef(
        _Out_ Graphics& out
    ) {
        __super::AddRef(out);
        out.SetPointer(GetPointer());
        out.SetLastResult(S_OK);
    }

    void    BeginDraw() { GetPointer()->BeginDraw(); }
    HRESULT EndDraw() { return GetPointer()->EndDraw(); }

    HRESULT CreateSolidColorBrush(
        SolidColorBrush& out
    ) {
        Direct2D::SolidColorBrush{GetPointer()}.Move(out);
        return GetLastResult();
    };

    HRESULT CreateSolidColorBrush(
        const D2D1::ColorF& color, SolidColorBrush& out
    ) {
        Direct2D::SolidColorBrush{GetPointer(), color}.Move(out);
        return GetLastResult();
    };

    _Success_(return == true)
    bool SetGraphicsSize(
        const D2D_SIZE_U& size
    ) {
        if (!IsValid()) {
            return false;
        }

        GetPointer()->Resize(size);
        return true;
    }

    _Success_(return == true)
    bool SetTransform(const D2D1_MATRIX_3X2_F& matrix) {
        if (!IsValid()) {
            return false;
        }

        GetPointer()->SetTransform(matrix);
        return true;
    }

    _Success_(return == true)
    bool PushAxisAlignedClip(
        const D2D1_RECT_F& rect
    ) {
        if (!IsValid()) {
            return false;
        }

        GetPointer()->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_ALIASED);
        return true;
    }

    _Success_(
        return == true
    ) bool PopAxisAlignedClip() {
        if (!IsValid()) {
            return false;
        }

        GetPointer()->PopAxisAlignedClip();
        return true;
    }

    _Success_(return == true)
    bool FillRectangle(
        const D2D1_RECT_F& rect, _In_ SolidColorBrush& brush
    ) {
        if (!IsValid() || !brush.IsValid()) {
            return false;
        }

        GetPointer()->FillRectangle(rect, brush.GetPointer());
        return true;
    }

    _Success_(return == true)
    bool DrawText(
        _In_ const std::wstring& text, _In_ const D2D1_RECT_F& rect, _In_ TextFormat& format, _In_ SolidColorBrush& brush
    ) {
        if (!format.IsValid() || !brush.IsValid()) {
            return false;
        }

        GetPointer()->DrawTextW(text.c_str(), (uint32_t)text.length(), format.GetPointer(), rect, brush.GetPointer());
        return true;
    }

    _Success_(return == true)
    bool DrawTextLayout(
        const D2D1_POINT_2F& position, _In_ TextLayout& layout, _In_ SolidColorBrush& brush
    ) {
        if (!IsValid() || !layout.IsValid() || !brush.IsValid()) {
            return false;
        }

        GetPointer()->DrawTextLayout(position, layout.GetPointer(), brush.GetPointer());
        return true;
    }

  private:
    friend class D2DFactory;
    Graphics(
        ID2D1Factory* factory, HWND window
    ) {
        RECT rect;
        GetWindowRect(window, &rect);

        // 默认 dpi 为 0 时 Direct2D 取的是主显示器 dpi，不随窗口所在显示器变化；
        // 固定为 96 使 1 DIP == 1 像素，尺寸统一由窗口按 _currentDpi / 96 缩放
        const auto properties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(), 96.f, 96.f);

        SetLastResult(
            factory->CreateHwndRenderTarget(properties, D2D1::HwndRenderTargetProperties(window, D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top)), &GetPointerRef())
        );
    };
};

class D2DFactory final : public D2DInterface<ID2D1Factory> {
  public:
    static auto& GetInstance() {
        static Direct2D::D2DFactory factory{};
        return factory;
    }

    void AttachWindow(
        HWND window, _Out_ Graphics& graphics
    ) {
        Graphics{GetPointer(), window}.Move(graphics);
    }

  private:
    D2DFactory() { SetLastResult(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &GetPointerRef())); }
    ~D2DFactory() = default;
};
} // namespace Direct2D_UI::Direct2D
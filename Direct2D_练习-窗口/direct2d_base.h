#pragma once
#include <type_traits>
#include <windows.h>
#include <d2d1.h>
#include <dwmapi.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwmapi.lib")

namespace Direct2D_UI {
// Warning: it exists a unpredictable issue that the discard function may cause ignoring last result and the pointer's validity
template <class T>
    requires std::is_base_of_v<IUnknown, T>
class D2DInterface {
  public:
    D2DInterface() = default;
    ~D2DInterface() {
        if (GetPointer()) {
            _pointer->Release();
            _pointer = nullptr;
        }
    }

    bool IsValid() { return _pointer != nullptr && _lastResult == S_OK; }

    auto*   GetPointer() { return _pointer; }
    auto*&  GetPointerRef() { return _pointer; }
    HRESULT GetLastResult() const { return _lastResult; }

    virtual void Move(
        D2DInterface<T>& destination
    ) {
        destination.SetPointer(GetPointer());
        destination.SetLastResult(GetLastResult());

        SetPointer(nullptr);
        SetLastResult(S_OK);
    }

    void Discard() {
        if (GetPointer()) {
            _pointer->Release();
            _pointer = nullptr;
        }
    }

  protected:
    void SetPointer(
        T* pointer
    ) {
        _pointer = pointer;
    }

    void SetLastResult(
        HRESULT result
    ) {
        _lastResult = result;
    }

  private:
    T*      _pointer    = nullptr;
    HRESULT _lastResult = S_OK;
};

template <class T>
    requires std::is_base_of_v<ID2D1Resource, T>
class D2DResource : public D2DInterface<T> {
  protected:
    using D2DInterface<T>::SetLastResult;

  public:
    D2DResource() = default;
    D2DResource(
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

    void    BeginDraw() { GetPointer()->BeginDraw(); }
    HRESULT EndDraw() { return GetPointer()->EndDraw(); }

    HRESULT CreateSolidColorBrush(
        SolidColorBrush& out
    ) {
        Direct2D_UI::SolidColorBrush{GetPointer()}.Move(out);
        return GetLastResult();
    };

    HRESULT CreateSolidColorBrush(
        const D2D1::ColorF& color, SolidColorBrush& out
    ) {
        Direct2D_UI::SolidColorBrush{GetPointer(), color}.Move(out);
        return GetLastResult();
    };

  public:
    void SetGraphicsSize(
        const D2D_SIZE_U& size
    ) {
        GetPointer()->Resize(size);
    }

    template <class T>
        requires std::is_base_of_v<T, SolidColorBrush>
    bool FillRectangle(
        D2D1_RECT_F rect, T& brush
    ) {
        if (!IsValid() || !brush.IsValid()) {
            return false;
        }

        GetPointer()->FillRectangle(rect, brush.GetPointer());
        return true;
    }

  private:
    friend class Factory;
    Graphics(
        ID2D1Factory* factory, HWND window
    ) {
        RECT rect;
        GetWindowRect(window, &rect);

        SetLastResult(factory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(window, D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top)), &GetPointerRef()
        ));
    };
};

class Factory : public D2DInterface<ID2D1Factory> {
  public:
    Factory() { SetLastResult(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &GetPointerRef())); }

    void AttachWindow(
        HWND window, _Out_ Graphics& graphics
    ) {
        Graphics{GetPointer(), window}.Move(graphics);
    }
};

inline auto& GetDirect2DFactory() {
    static Direct2D_UI::Factory factory{};
    return factory;
}
} // namespace Direct2D_UI
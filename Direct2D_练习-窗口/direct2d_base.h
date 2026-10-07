#pragma once
#include <type_traits>
#include <d2d1.h>
#pragma comment(lib, "d2d1.lib")

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

    template <class U>
        requires std::is_base_of_v<T, U>
    _Success_(
        return
    ) bool As(_Out_ D2DInterface<U>& out) {
        if (GetPointer() == nullptr) {
            return false;
        }

        D2DInterface<U> transferredOut;
        if (FAILED(GetPointer()->QueryInterface(IID_PPV_ARGS(&transferredOut.GetPointerRef())))) {
            return false;
        }

        transferredOut.Move(out);
        return true;
    }

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
} // namespace Direct2D_UI
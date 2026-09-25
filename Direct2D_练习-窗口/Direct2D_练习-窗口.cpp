// Direct2D_练习-窗口.cpp : 定义应用程序的入口点。
//

#include "Direct2D_练习-窗口.h"

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow
) {
    class MainWindow : public Direct2D_UI::Window {
      public:
        explicit MainWindow(
            HINSTANCE instance
        )
        : Direct2D_UI::Window() {
            Initialize(instance);
            Create();
        }

        ~MainWindow() = default;

        void CreateDeviceResources(
            Direct2D_UI::Graphics& graphics
        ) final {
            Direct2D_UI::SolidColorBrush{graphics, D2D1::ColorF::Green}.Move(brush);
        }

        void DiscardDeviceResources() final { brush.Discard(); }

        bool OnRender(
            Direct2D_UI::Graphics& graphics
        ) final {
            graphics.GetPointer()->Clear();
            D2D1_ROUNDED_RECT rect{0};
            rect.rect.left   = GetMousePosition().x - 20.f;
            rect.rect.top    = GetMousePosition().y - 20.f;
            rect.rect.right  = GetMousePosition().x + 20.f;
            rect.rect.bottom = GetMousePosition().y + 20.f;
            rect.radiusX = rect.radiusY = 5.f;
            graphics.GetPointer()->FillRoundedRectangle(rect, brush.GetPointer());

            return true;
        }

        Direct2D_UI::SolidColorBrush brush;
    };

    MainWindow window{hInstance};

    return Direct2D_UI::DoMessageLoop();
}
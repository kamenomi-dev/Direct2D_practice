// Direct2D_练习-窗口.cpp : 定义应用程序的入口点。
//

#include "Direct2D_练习-窗口.h"

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow
) {
    Direct2D_UI::Direct2D::Resource::ResourceManager::GetInstance();

    class MainWindow final : public Direct2D_UI::BaseWindow {
      public:
        explicit MainWindow(
            HINSTANCE instance
        )
        : Direct2D_UI::BaseWindow() {
            Initialize(instance);
            Create();
        }

        ~MainWindow() = default;

        void CreateDeviceResources(
            Direct2D_UI::Direct2D::Graphics& graphics
        ) override {
            __super::CreateDeviceResources(graphics);
            graphics.CreateSolidColorBrush(D2D1::ColorF(.5, .5, .5, .5), brush);
        }

        void DiscardDeviceResources() override {
            brush.Discard();
            __super::DiscardDeviceResources();
        }

        void InitializeComponents() override {
            label = std::make_shared<Direct2D_UI::Components::DurableLabel>();
            label->SetContent(L"Hello world!");
            GetRenderableWindow().AppendChild(label);
        }

        bool OnRender(
            Direct2D_UI::Direct2D::Graphics& graphics
        ) override {
            D2D1_ROUNDED_RECT rect{0};
            rect.rect.left   = GetMousePosition().x - 8.f;
            rect.rect.top    = GetMousePosition().y - 8.f;
            rect.rect.right  = GetMousePosition().x + 8.f;
            rect.rect.bottom = GetMousePosition().y + 8.f;
            rect.radiusX = rect.radiusY = 5.f;
            graphics.GetPointer()->FillRoundedRectangle(rect, brush.GetPointer());

            return true;
        }

        Direct2D_UI::Direct2D::SolidColorBrush                 brush;
        std::shared_ptr<Direct2D_UI::Components::DurableLabel> label;
    };

    MainWindow window{hInstance};

    return Direct2D_UI::DoMessageLoop();
}
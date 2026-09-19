#include "ui/ui_context.hpp"
#include "ui/widgets/panel.hpp"
#include "ui/widgets/button.hpp"
#include "ui/widgets/label.hpp"
#include "ui/widgets/slider.hpp"
#include "ui/widgets/text_input.hpp"
#include "ui/layout/layout.hpp"
#include "backend/window.hpp"
#include "backend/device.hpp"
#include "backend/swap_chain.hpp"
#include <iostream>
#include <stdexcept>
#include <memory>

using namespace xel::backend;

int main()
{
    try
    {
        Window window(800, 600, "Cosine GUI Engine");
        Device device(window);
        auto swapChain = std::make_unique<SwapChain>(device, window.get_extent());

        xel::ui::UIContext uiContext(window, device, *swapChain);

        auto mainPanel = std::make_shared<xel::ui::Panel>(glm::vec2{20, 20}, glm::vec2{760, 560});
        uiContext.addWidget(mainPanel);

        auto titleLabel = std::make_shared<xel::ui::Label>("Cosine GUI Engine Demo", glm::vec2{40, 40}, 20.f);
        uiContext.addWidget(titleLabel);

        auto nameInput = std::make_shared<xel::ui::TextInput>(glm::vec2{40, 80}, "Enter your name...");
        uiContext.addWidget(nameInput);

        auto slider1 = std::make_shared<xel::ui::Slider>(glm::vec2{40, 130}, 0.f, 100.f, 60.f);
        uiContext.addWidget(slider1);

        auto slider2 = std::make_shared<xel::ui::Slider>(glm::vec2{40, 170}, 0.f, 1.f, 0.3f);
        uiContext.addWidget(slider2);

        auto slider3 = std::make_shared<xel::ui::Slider>(glm::vec2{40, 210}, -50.f, 50.f, 0.f);
        uiContext.addWidget(slider3);

        auto btnOk = std::make_shared<xel::ui::Button>("OK", xel::ui::Rect{40, 270, 120, 40});
        btnOk->setCallback([]() { std::cerr << "OK clicked!" << std::endl; });
        uiContext.addWidget(btnOk);

        auto btnCancel = std::make_shared<xel::ui::Button>("Cancel", xel::ui::Rect{180, 270, 120, 40});
        btnCancel->setCallback([]() { std::cerr << "Cancel clicked!" << std::endl; });
        uiContext.addWidget(btnCancel);

        auto btnReset = std::make_shared<xel::ui::Button>("Reset", xel::ui::Rect{320, 270, 120, 40});
        btnReset->setCallback([slider1, slider2, slider3, nameInput]()
        {
            slider1->setValue(60.f);
            slider2->setValue(0.3f);
            slider3->setValue(0.f);
            nameInput->setText("");
            std::cerr << "Reset!" << std::endl;
        });
        uiContext.addWidget(btnReset);

        auto statusLabel = std::make_shared<xel::ui::Label>("Status: Ready", glm::vec2{40, 340}, 16.f);
        uiContext.addWidget(statusLabel);

        uiContext.run([](float dt)
        {
            (void)dt;
        });
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

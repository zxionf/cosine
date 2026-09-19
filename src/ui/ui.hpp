#pragma once

#include "backend/device.hpp"
#include "backend/swap_chain.hpp"
#include "res_manager.hpp"

namespace xel::ui
{
    class Ui
    {
    public:
        Ui(backend::Device& device, backend::SwapChain& swap_chain);
        ~Ui();

    private:
        backend::Device& device_;
        backend::SwapChain& swap_chain_;
        ResourceManager resourcec_manager_{device_, swap_chain_.get_render_pass()};
    };
}
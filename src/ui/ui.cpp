#include "ui.hpp"

namespace xel::ui
{
    Ui::Ui(backend::Device& device, backend::SwapChain& swap_chain)
    : device_{device}, swap_chain_{swap_chain}
    {
    }
}
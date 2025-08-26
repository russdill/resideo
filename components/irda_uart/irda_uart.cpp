// irda_uart.cpp
#include "irda_uart.h"
#include "driver/uart.h"
#include "soc/uart_reg.h"

namespace esphome {
namespace irda_uart {

void IrdaUart::setup() {
  uart_port_t port = (uart_port_t) this->uart_->get_hw_serial_number();

  ESP_ERROR_CHECK(uart_set_mode(port, UART_MODE_IRDA));
  if (tx_en_) {
    if (invert_) {
      ESP_ERROR_CHECK(uart_set_line_inverse(port, UART_SIGNAL_IRDA_TX_INV));
    }
    REG_SET_BIT(UART_CONF0_REG(port), UART_IRDA_TX_EN);   // TX-only
  } else {
    if (invert_)
      ESP_ERROR_CHECK(uart_set_line_inverse(port, UART_SIGNAL_IRDA_RX_INV));
    REG_CLR_BIT(UART_CONF0_REG(port), UART_IRDA_TX_EN);   // RX-only
  }
}

}  // namespace irda_uart
}  // namespace esphome

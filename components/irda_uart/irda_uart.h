// irda_uart.h
#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/uart/uart_component_esp_idf.h"

namespace esphome {
namespace irda_uart {

class IrdaUart : public Component {
 public:
  void set_uart(uart::IDFUARTComponent *u) { uart_ = u; }
  void set_tx_en(bool en) { tx_en_ = en; }
  void set_invert(bool inv) { invert_ = inv; }
  void setup() override;

 private:
  uart::IDFUARTComponent *uart_{nullptr};
  bool tx_en_{false};
  bool invert_{false};
};

}  // namespace irda_uart
}  // namespace esphome

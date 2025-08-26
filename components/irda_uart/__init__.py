import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

ns = cg.esphome_ns.namespace("irda_uart")
IrdaUart = ns.class_("IrdaUart", cg.Component)

CONF_UART_ID = "uart_id"
CONF_TX_EN = "tx_en"
CONF_INVERT = "invert"

CONFIG_SCHEMA = cv.All(
    cv.ensure_list(
        cv.Schema({
            cv.GenerateID(): cv.declare_id(IrdaUart),
            cv.Required(CONF_UART_ID): cv.use_id(uart.IDFUARTComponent),
            cv.Optional(CONF_TX_EN, default=False): cv.boolean,
            cv.Optional(CONF_INVERT, default=False): cv.boolean,
        }).extend(cv.COMPONENT_SCHEMA)
    )
)

async def to_code(config):
    for conf in config:
        var = cg.new_Pvariable(conf[CONF_ID])
        uart_var = await cg.get_variable(conf[CONF_UART_ID])
        cg.add(var.set_uart(uart_var))
        cg.add(var.set_tx_en(conf[CONF_TX_EN]))
        cg.add(var.set_invert(conf[CONF_INVERT]))
        await cg.register_component(var, conf)


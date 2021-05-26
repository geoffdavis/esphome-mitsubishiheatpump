import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.components.logger import HARDWARE_UART_TO_SERIAL
from esphome.const import CONF_ID, CONF_HARDWARE_UART, CONF_UPDATE_INTERVAL
from esphome.core import CORE, coroutine

AUTO_LOAD = ["climate"]

MitsubishiHeatPump = cg.global_ns.class_("MitsubishiHeatPump", climate.Climate, cg.PollingComponent)


def valid_uart(uart):
    if CORE.is_esp8266:
        uarts = ["UART0"] # UART1 is tx-only
    elif CORE.is_esp32:
        uarts = ["UART0", "UART1", "UART2"]
    else:
        raise NotImplementedError

    return cv.one_of(*uarts, upper=True)(uart)


CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(MitsubishiHeatPump),
        cv.Optional(CONF_HARDWARE_UART, default="UART0"): valid_uart,

        # If polling interval is greater than 9 seconds, the HeatPump library
        # reconnects, but doesn't then follow up with our data request.
        cv.Optional(CONF_UPDATE_INTERVAL, default="500ms"): cv.All(
            cv.update_interval,
            cv.Range(max=cv.TimePeriod(milliseconds=9000))
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


@coroutine
def to_code(config):
    serial = HARDWARE_UART_TO_SERIAL[config[CONF_HARDWARE_UART]]
    var = cg.new_Pvariable(config[CONF_ID], cg.RawExpression(f'&{serial}'))
    yield cg.register_component(var, config)
    yield climate.register_climate(var, config)
    cg.add_library("https://github.com/SwiCago/HeatPump", None)


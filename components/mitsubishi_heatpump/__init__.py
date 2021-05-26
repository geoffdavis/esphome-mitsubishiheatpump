import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.components.logger import HARDWARE_UART_TO_SERIAL
from esphome.const import (
    CONF_ID,
    CONF_HARDWARE_UART,
    CONF_UPDATE_INTERVAL,
    CONF_MODE,
    CONF_FAN_MODE,
    CONF_SWING_MODE,
)
from esphome.core import CORE, coroutine

AUTO_LOAD = ["climate"]

CONF_SUPPORTS = "supports"
DEFAULT_CLIMATE_MODES = ['AUTO', 'COOL', 'HEAT', 'DRY', 'FAN_ONLY']
DEFAULT_FAN_MODES = ['AUTO', 'DIFFUSE', 'LOW', 'MEDIUM', 'MIDDLE', 'HIGH']
DEFAULT_SWING_MODES = ['OFF', 'VERTICAL']

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

        # Optionally override the supported ClimateTraits.
        cv.Optional(CONF_SUPPORTS, default={}): cv.Schema(
            {
                cv.Optional(CONF_MODE, default=DEFAULT_CLIMATE_MODES): 
                    cv.ensure_list(climate.validate_climate_mode),
                cv.Optional(CONF_FAN_MODE, default=DEFAULT_FAN_MODES): 
                    cv.ensure_list(climate.validate_climate_fan_mode),
                cv.Optional(CONF_SWING_MODE, default=DEFAULT_SWING_MODES): 
                    cv.ensure_list(climate.validate_climate_swing_mode),
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


@coroutine
def to_code(config):
    serial = HARDWARE_UART_TO_SERIAL[config[CONF_HARDWARE_UART]]
    var = cg.new_Pvariable(config[CONF_ID], cg.RawExpression(f'&{serial}'))

    traits = []
    for mode in config[CONF_SUPPORTS][CONF_MODE]:
        if mode == 'OFF':
            continue
        traits.append(f'set_supports_{mode.lower()}_mode')
    for mode in config[CONF_SUPPORTS][CONF_FAN_MODE]:
        traits.append(f'set_supports_fan_mode_{mode.lower()}')
    for mode in config[CONF_SUPPORTS][CONF_SWING_MODE]:
        traits.append(f'set_supports_swing_mode_{mode.lower()}')
    for trait in traits:
        cg.add(getattr(var.config_traits(), trait)(True))

    yield cg.register_component(var, config)
    yield climate.register_climate(var, config)
    cg.add_library("https://github.com/SwiCago/HeatPump", None)


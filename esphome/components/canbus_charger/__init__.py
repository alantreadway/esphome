import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.canbus import (
    # CONF_CANBUS_ID,
    CanbusComponent,
    CANBUS_DEVICE_SCHEMA,
)
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_DEBUG,
)

CODEOWNERS = ["@clydebarrow"]
DEPENDENCIES = ["canbus", "canbus_bms"]
AUTO_LOAD = ["text_sensor", "binary_sensor"]
MULTI_CONF = True

CONF_BMS_ID = "bms_id"
CONF_SCALE = "scale"
CONF_MSG_ID = "msg_id"
CONF_BIT_NO = "bit_no"

charger = cg.esphome_ns.namespace("canbus_charger")
ChargerComponent = charger.class_(
    "CanbusChargerComponent", cg.PollingComponent, CanbusComponent
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ChargerComponent),
            cv.Optional(CONF_DEBUG, default=False): cv.boolean,
            cv.Optional(CONF_NAME, default="CanbusCharger"): cv.string,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(CANBUS_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    # canbus = await cg.get_variable(config[CONF_CANBUS_ID])
    cg.add(var.set_name(config[CONF_NAME]))
    cg.add(var.set_debug(config[CONF_DEBUG]))

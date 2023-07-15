import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import canbus
from esphome.const import CONF_ID, CONF_TRIGGER_ID, CONF_NAME, CONF_DEBUG, CONF_THROTTLE
from canbus import CONF_CANBUS_ID, CANBUS_DEVICE_SCHEMA

CODEOWNERS = ["@clydebarrow"]
DEPENDENCIES = ["canbus"]
AUTO_LOAD = ["sensor", "binary_sensor"]
MULTI_CONF = True

CONF_BMS_ID = "bms_id"
CONF_OFFSET = "offset"
CONF_LENGTH = "length"
CONF_SCALE = "scale"
CONF_MSG_ID = "msg_id"
CONF_BIT_NO = "bit_no"


bms = cg.esphome_ns.namespace("canbus_bms")
BmsComponent = bms.class_(
    "CanbusBmsComponent", cg.PollingComponent, canbus.CanbusComponent
)
BmsTrigger = bms.class_("BmsTrigger", canbus.CanbusTrigger)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BmsComponent),
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BmsTrigger),
            cv.Optional(CONF_DEBUG, default=False): cv.boolean,
            cv.Optional(CONF_THROTTLE, default="15s"): cv.positive_time_period,
            cv.Optional(CONF_NAME, default="CanbusBMS"): cv.string,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(CANBUS_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    canbus = await cg.get_variable(config[CONF_CANBUS_ID])
    cg.add(var.set_name(config[CONF_NAME]))
    cg.add(var.set_debug(config[CONF_DEBUG]))
    cg.add(var.set_throttle(config[CONF_THROTTLE].total_milliseconds))
    trigger = cg.new_Pvariable(config[CONF_TRIGGER_ID], var, canbus)
    await cg.register_component(trigger, config)

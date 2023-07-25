import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.canbus import (
    CONF_CANBUS_ID,
    CanbusComponent,
)
from esphome.components.canbus_bms import (
    CONF_BMS_ID,
    BmsComponent,
)

from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_DEBUG,
    CONF_INTERVAL,
)

CODEOWNERS = ["@clydebarrow"]
DEPENDENCIES = ["canbus", "canbus_bms"]
AUTO_LOAD = ["text_sensor", "binary_sensor"]
MULTI_CONF = True

CONF_SCALE = "scale"
CONF_BATTERIES = "batteries"
CONF_MSG_ID = "msg_id"
CONF_BIT_NO = "bit_no"
CONF_BMS_HEARTBEAT = "heartbeat"

BMS_NAMESPACE = "bms_charger"
charger = cg.esphome_ns.namespace(BMS_NAMESPACE)
ChargerComponent = charger.class_("BmsChargerComponent", cg.PollingComponent)
BatteryDesc = charger.class_("BatteryDesc")

entry_battery_parameters = {
    cv.Required(CONF_BMS_ID): cv.use_id(BmsComponent),
    cv.Optional(CONF_BMS_HEARTBEAT, default=0x308): cv.hex_int_range(0x00, 0x3FF),
}
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ChargerComponent),
        cv.Optional(CONF_DEBUG, default=False): cv.boolean,
        cv.Optional(CONF_NAME, default="BmsCharger"): cv.string,
        cv.Optional(CONF_INTERVAL, default="1s"): cv.time_period,
        cv.Required(CONF_CANBUS_ID): cv.use_id(CanbusComponent),
        cv.Required(CONF_BATTERIES): cv.All(
            cv.ensure_list(entry_battery_parameters), cv.Length(min=1, max=4)
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    canbus = await cg.get_variable(config[CONF_CANBUS_ID])
    charger = cg.new_Pvariable(
        config[CONF_ID],
        config[CONF_NAME],
        canbus,
        config[CONF_DEBUG],
        config[CONF_INTERVAL].total_milliseconds,
    )
    await cg.register_component(charger, config)
    for conf in config[CONF_BATTERIES]:
        bms_id = conf[CONF_BMS_ID]
        battery = await cg.get_variable(bms_id)
        cg.add(charger.add_battery(BatteryDesc.new(battery, conf[CONF_BMS_HEARTBEAT])))

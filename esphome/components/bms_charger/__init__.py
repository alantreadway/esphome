import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.components.canbus import (
    CONF_CANBUS_ID,
    CanbusComponent,
)
from esphome.components.canbus_bms import (
    CONF_BMS_ID,
    BmsComponent,
    BmsTrigger,
)
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_TRIGGER_ID,
    CONF_DEBUG,
    CONF_INTERVAL,
    CONF_TIMEOUT,
    CONF_PROTOCOL,
)

CODEOWNERS = ["@clydebarrow"]
DEPENDENCIES = ["canbus", "canbus_bms"]
AUTO_LOAD = ["text_sensor", "binary_sensor", "switch"]
MULTI_CONF = True

CONF_SCALE = "scale"
CONF_BATTERIES = "batteries"
CONF_CHARGER_ID = "charger_id"
CONF_MSG_ID = "msg_id"
CONF_BIT_NO = "bit_no"
CONF_BMS_HEARTBEAT_ID = "heartbeat_id"
CONF_BMS_HEARTBEAT_TEXT = "heartbeat_text"

BMS_NAMESPACE = "bms_charger"
charger = cg.esphome_ns.namespace(BMS_NAMESPACE)
CurrentNumber = charger.class_("CurrentNumber", number.Number, cg.Component)
ChargerComponent = charger.class_("BmsChargerComponent", cg.PollingComponent)
BatteryDesc = charger.class_("BatteryDesc")
InverterProtocol = charger.enum("InverterProtocol")
PROTOCOLS = {
    "pylon": InverterProtocol.PROTOCOL_PYLON,
    "sma": InverterProtocol.PROTOCOL_SMA,
}


def string_1_8(value):
    value = cv.alphanumeric(value)
    if len(value) > 8 or len(value) < 1:
        raise cv.Invalid("Heartbeat text length must be 1-8")
    return value


entry_battery_parameters = {
    cv.Required(CONF_BMS_ID): cv.use_id(BmsComponent),
    cv.Optional(CONF_BMS_HEARTBEAT_ID, default=0x308): cv.hex_int_range(0x00, 0x3FF),
    cv.Optional(CONF_BMS_HEARTBEAT_TEXT, default="ESPHome"): string_1_8,
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ChargerComponent),
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BmsTrigger),
        cv.Optional(CONF_DEBUG, default=False): cv.boolean,
        cv.Required(CONF_PROTOCOL): cv.enum(PROTOCOLS, lower=True),
        cv.Optional(CONF_NAME, default="BmsCharger"): cv.string,
        cv.Optional(CONF_INTERVAL, default="1s"): cv.time_period,
        cv.Optional(CONF_TIMEOUT, default="10s"): cv.time_period,
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
        config[CONF_TIMEOUT].total_milliseconds,
        canbus,
        config[CONF_DEBUG],
        config[CONF_INTERVAL].total_milliseconds,
        config[CONF_PROTOCOL],
    )
    await cg.register_component(charger, config)
    trigger = cg.new_Pvariable(config[CONF_TRIGGER_ID], charger, canbus)
    await cg.register_component(trigger, config)
    for conf in config[CONF_BATTERIES]:
        bms_id = conf[CONF_BMS_ID]
        battery = await cg.get_variable(bms_id)
        cg.add(
            charger.add_battery(
                BatteryDesc.new(
                    battery,
                    conf[CONF_BMS_HEARTBEAT_ID],
                    conf[CONF_BMS_HEARTBEAT_TEXT],
                )
            )
        )

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from . import BmsComponent, CONF_BMS_ID, CONF_MSG_ID, CONF_OFFSET, CONF_BIT_NO

CONF_OVER_CURRENT = "over_current"


def desc(msg_id, offset, bitno):
    return {
        CONF_MSG_ID: msg_id,
        CONF_OFFSET: offset,
        CONF_BIT_NO: bitno,
    }


# The sensor map from conf id to message and data decoding information
TYPES = {
    CONF_OVER_CURRENT: desc(0x356, 0, 2),
}

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_BMS_ID): cv.use_id(BmsComponent),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_BMS_ID])
    for key, desc in TYPES.items():
        if key in config:
            conf = config[key]
            sens = await binary_sensor.new_binary_sensor(conf)
            cg.add(
                hub.add_binary_sensor(
                    sens,
                    desc[CONF_MSG_ID],
                    desc[CONF_OFFSET],
                    desc[CONF_BIT_NO],
                )
            )

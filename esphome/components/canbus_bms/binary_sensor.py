import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import BmsComponent, CONF_BMS_ID, CONF_MSG_ID, CONF_OFFSET, CONF_BIT_NO


# define an alarm or warning bit, found as a bit in a byte at an offset in a message
def desc(msg_id, offset, bitno):
    return {
        CONF_MSG_ID: msg_id,
        CONF_OFFSET: offset,
        CONF_BIT_NO: bitno,
    }


# The sensor map from conf id to message and data decoding information
TYPES = {
    "general_alarm": desc(0x35A, 0, 0),
    "high_voltage": desc(0x35A, 0, 2),
    "low_voltage": desc(0x35A, 0, 4),
    "high_temperature": desc(0x35A, 0, 6),
    "low_temperature": desc(0x35A, 1, 0),
    "high_temperature_charge": desc(0x35A, 1, 2),
    "low_temperature_charge": desc(0x35A, 1, 4),
    "high_current": desc(0x35A, 1, 6),
    "high_current_charge": desc(0x35A, 2, 0),
    "contactor_error": desc(0x35A, 2, 2),
    "short_circuit": desc(0x35A, 2, 4),
    "bms_internal_error": desc(0x35A, 2, 6),
    "cell_imbalance": desc(0x35A, 3, 0),
}


def alarm_schema(name):
    return (
        cv.Optional(name),
        binary_sensor.binary_sensor_schema(
            device_class=DEVICE_CLASS_PROBLEM,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    )


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_BMS_ID): cv.use_id(BmsComponent),
        }
    )
    .extend(dict(map(alarm_schema, TYPES)))
    .extend(cv.COMPONENT_SCHEMA)
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
                    key,
                    desc[CONF_MSG_ID],
                    desc[CONF_OFFSET],
                    desc[CONF_BIT_NO],
                )
            )

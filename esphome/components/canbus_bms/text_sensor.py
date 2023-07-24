import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import (
    BmsComponent,
    CONF_BMS_ID,
    CONF_MSG_ID,
    CONF_WARNINGS,
    CONF_ALARMS,
    CONF_OFFSET,
    CONF_BIT_NO,
)

ICON_CAR_BATTERY = "mdi:car-battery"
SMA_MSG_ID = 0x35A
PYLON_MSG_ID = 0x359
CONF_WARN_OFFSET = "warn_offset"
CONF_WARN_BIT_NO = "warn_bit_no"


def text_desc(msg_id=-1):
    return {
        CONF_MSG_ID: msg_id,
    }


# define an alarm or warning bit, found as a bit in a byte at an offset in a message
def bit_desc(msg_id, offset, bitno, warn_offset, warn_bitno):
    return {
        CONF_MSG_ID: msg_id,
        CONF_OFFSET: offset,
        CONF_BIT_NO: bitno,
        CONF_WARN_OFFSET: warn_offset,
        CONF_WARN_BIT_NO: warn_bitno,
    }


TEXT_SENSORS = {
    "bms_name": text_desc(0x35E),  # maps directly to a CAN message
    CONF_ALARMS: text_desc(),  # Synthesised from alarm bits
    CONF_WARNINGS: text_desc(),
}

ALARMS = {
    "general_alarm": (bit_desc(SMA_MSG_ID, 0, 0, 4, 0),),
    "high_voltage": (
        bit_desc(SMA_MSG_ID, 0, 2, 4, 2),
        bit_desc(PYLON_MSG_ID, 2, 1, 0, 1),
    ),
    "low_voltage": (
        bit_desc(SMA_MSG_ID, 0, 4, 4, 4),
        bit_desc(PYLON_MSG_ID, 2, 2, 0, 2),
    ),
    "high_temperature": (
        bit_desc(SMA_MSG_ID, 0, 6, 4, 6),
        bit_desc(PYLON_MSG_ID, 2, 3, 0, 3),
    ),
    "low_temperature": (
        bit_desc(SMA_MSG_ID, 1, 0, 5, 0),
        bit_desc(PYLON_MSG_ID, 2, 4, 0, 4),
    ),
    "high_temperature_charge": (bit_desc(SMA_MSG_ID, 1, 2, 5, 2),),
    "low_temperature_charge": (bit_desc(SMA_MSG_ID, 1, 4, 5, 4),),
    "high_current": (
        bit_desc(SMA_MSG_ID, 1, 6, 5, 6),
        bit_desc(PYLON_MSG_ID, 2, 7, 0, 7),
    ),
    "high_current_charge": (
        bit_desc(SMA_MSG_ID, 2, 0, 6, 0),
        bit_desc(PYLON_MSG_ID, 3, 0, 1, 0),
    ),
    "contactor_erorr": (bit_desc(SMA_MSG_ID, 2, 2, 6, 2),),
    "short_circuit": (bit_desc(SMA_MSG_ID, 2, 4, 6, 4),),
    "bms_internal_erorr": (
        bit_desc(SMA_MSG_ID, 2, 6, 6, 6),
        bit_desc(PYLON_MSG_ID, 3, 3, 1, 6),
    ),
    "cell_imbalance": (bit_desc(SMA_MSG_ID, 3, 0, 7, 0),),
}


def text_schema(name):
    return (
        cv.Optional(name),
        text_sensor.text_sensor_schema(
            icon=ICON_CAR_BATTERY,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    )


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_BMS_ID): cv.use_id(BmsComponent),
        }
    )
    .extend(dict(map(lambda name: text_schema(name), TEXT_SENSORS)))
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_BMS_ID])
    for key, entries in ALARMS.items():
        for entry in entries:
            cg.add(
                hub.add_flag(
                    key,
                    key.title().replace("_", " "),  # convert key to title case phrase.
                    entry[CONF_MSG_ID],
                    entry[CONF_OFFSET],
                    entry[CONF_BIT_NO],
                    entry[CONF_WARN_OFFSET],
                    entry[CONF_WARN_BIT_NO],
                )
            )
    for key, entry in TEXT_SENSORS.items():
        if key in config:
            conf = config[key]
            sensor = await text_sensor.new_text_sensor(conf)
            cg.add(
                hub.add_text_sensor(
                    sensor,
                    key,
                    entry[CONF_MSG_ID],
                )
            )

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    DEVICE_CLASS_PROBLEM,
    DEVICE_CLASS_BATTERY_CHARGING,
    ENTITY_CATEGORY_DIAGNOSTIC,
    CONF_OFFSET,
)

from . import (
    BmsComponent,
    CONF_BMS_ID,
    CONF_MSG_ID,
    CONF_BIT_NO,
    CONF_WARNINGS,
    CONF_ALARMS,
)


# define an alarm or warning bit, found as a bit in a byte at an offset in a message
def bms_bit_desc(msg_id=-1, offset=-1, bitno=-1):
    return {
        CONF_MSG_ID: msg_id,
        CONF_OFFSET: offset,
        CONF_BIT_NO: bitno,
    }


PYLON_REQUEST_MSG_ID = 0x35C

# The sensor map from conf id to message and data decoding information. Each sensor may have multiple
# implementations corresponding to different BMS protocols.
REQUESTS = {
    "charge_enable": (bms_bit_desc(PYLON_REQUEST_MSG_ID, 0, 7),),
    "discharge_enable": (bms_bit_desc(PYLON_REQUEST_MSG_ID, 0, 6),),
    "force_charge_1": (bms_bit_desc(PYLON_REQUEST_MSG_ID, 0, 5),),
    "force_charge_2": (bms_bit_desc(PYLON_REQUEST_MSG_ID, 0, 4),),
    "request_full_charge": (bms_bit_desc(PYLON_REQUEST_MSG_ID, 0, 3),),
}

FLAGS = {
    CONF_ALARMS,
    CONF_WARNINGS,
}


def binary_schema(name, device_class):
    return (
        cv.Optional(name),
        binary_sensor.binary_sensor_schema(
            device_class=device_class,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    )


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_BMS_ID): cv.use_id(BmsComponent),
        }
    )
    .extend(
        dict(
            map(
                lambda name: binary_schema(name, DEVICE_CLASS_BATTERY_CHARGING),
                REQUESTS,
            )
        )
    )
    .extend(
        dict(
            map(
                lambda name: binary_schema(name, DEVICE_CLASS_PROBLEM),
                FLAGS,
            )
        )
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_BMS_ID])
    for key, entries in REQUESTS.items():
        if key in config:
            conf = config[key]
            sensor = await binary_sensor.new_binary_sensor(conf)
            for entry in entries:
                cg.add(
                    hub.add_binary_sensor(
                        sensor,
                        key,
                        entry[CONF_MSG_ID],
                        entry[CONF_OFFSET],
                        entry[CONF_BIT_NO],
                    )
                )
    for key in FLAGS:
        if key in config:
            conf = config[key]
            sensor = await binary_sensor.new_binary_sensor(conf)
            cg.add(
                hub.add_binary_sensor(
                    sensor,
                    key,
                    -1,
                    -1,
                    -1,
                )
            )

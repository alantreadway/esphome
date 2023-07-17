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
)

ICON_CAR_BATTERY = "mdi:car-battery"


def desc(msg_id):
    return {
        CONF_MSG_ID: msg_id,
    }


SENSORS = {
    "bms_name": desc(0x35E),
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
    .extend(dict(map(lambda name: text_schema(name), SENSORS)))
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_BMS_ID])
    for key, entries in SENSORS.items():
        if key in config:
            conf = config[key]
            sensor = await text_sensor.new_text_sensor(conf)
            for entry in entries:
                cg.add(
                    hub.add_text_sensor(
                        sensor,
                        key,
                        entry[CONF_MSG_ID],
                    )
                )

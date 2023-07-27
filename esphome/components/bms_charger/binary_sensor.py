import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    DEVICE_CLASS_CONNECTIVITY,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import (
    ChargerComponent,
    CONF_CHARGER_ID,
)

CONF_CONNECTED = "connected"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_CHARGER_ID): cv.use_id(ChargerComponent),
            cv.Optional(CONF_CONNECTED): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_CONNECTIVITY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    component = await cg.get_variable(config[CONF_CHARGER_ID])
    if CONF_CONNECTED in config:
        conf = config[CONF_CONNECTED]
        sensor = await binary_sensor.new_binary_sensor(conf)
        cg.add(component.add_connectivity_sensor(sensor))

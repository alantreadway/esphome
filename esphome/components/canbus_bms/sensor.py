import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_VOLTAGE,
    CONF_FILTERS,
    CONF_CURRENT,
    CONF_TEMPERATURE,
    UNIT_VOLT,
    UNIT_CELSIUS,
    UNIT_AMPERE,
    UNIT_PERCENT,
    ICON_FLASH,
    ICON_THERMOMETER,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_BATTERY,
    STATE_CLASS_MEASUREMENT,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from . import (
    BmsComponent,
    CONF_BMS_ID,
    CONF_MSG_ID,
    CONF_SCALE,
    CONF_LENGTH,
    CONF_OFFSET,
)

ICON_CURRENT_DC = "mdi:current-dc"
ICON_BATTERY_OUTLINE = "mdi:battery-high"
ICON_HEALTH = "mdi:bottle-tonic-plus-outline"
ICON_FLASH_ALERT = "mdi:flash-alert"
ICON_NUMERIC = "mdi:numeric"

CONF_HEALTH = "health"
CONF_CHARGE = "charge"
CONF_MAX_CHARGE_VOLTAGE = "max_charge_voltage"
CONF_MAX_CHARGE_CURRENT = "max_charge_current"
CONF_MAX_DISCHARGE_CURRENT = "max_discharge_current"
CONF_MIN_DISCHARGE_VOLTAGE = "min_discharge_voltage"


def desc(msg_id, offset, length, scale):
    return {
        CONF_MSG_ID: msg_id,
        CONF_SCALE: scale,
        CONF_LENGTH: length,
        CONF_OFFSET: offset,
    }


# The sensor map from conf id to message and data decoding information
TYPES = {
    CONF_VOLTAGE: desc(0x356, 0, 2, 0.01),
    CONF_CURRENT: desc(0x356, 2, 2, 0.1),
    CONF_TEMPERATURE: desc(0x356, 4, 2, 0.1),
    CONF_CHARGE: desc(0x355, 0, 2, 1),
    CONF_HEALTH: desc(0x355, 2, 2, 1),
    CONF_MAX_CHARGE_VOLTAGE: desc(0x351, 0, 2, 0.1),
    CONF_MAX_CHARGE_CURRENT: desc(0x351, 2, 2, 0.1),
    CONF_MAX_DISCHARGE_CURRENT: desc(0x351, 4, 2, 0.1),
    CONF_MIN_DISCHARGE_VOLTAGE: desc(0x351, 6, 2, 0.1),
}

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_BMS_ID): cv.use_id(BmsComponent),
            cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                icon=ICON_FLASH,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                icon=ICON_CURRENT_DC,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CHARGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                icon=ICON_BATTERY_OUTLINE,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_BATTERY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_HEALTH): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                icon=ICON_HEALTH,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_BATTERY,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_MAX_CHARGE_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                icon=ICON_FLASH_ALERT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_MAX_CHARGE_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                icon=ICON_FLASH_ALERT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_MAX_DISCHARGE_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                icon=ICON_FLASH_ALERT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_MIN_DISCHARGE_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                icon=ICON_FLASH_ALERT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_BMS_ID])
    for key, desc in TYPES.items():
        if key in config:
            conf = config[key]
            sens = await sensor.new_sensor(conf)
            cg.add(
                hub.add_sensor(
                    sens,
                    key,
                    desc[CONF_MSG_ID],
                    desc[CONF_OFFSET],
                    desc[CONF_LENGTH],
                    desc[CONF_SCALE],
                    CONF_FILTERS in conf,
                )
            )

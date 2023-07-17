import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    DEVICE_CLASS_PROBLEM,
    DEVICE_CLASS_SAFETY,
    DEVICE_CLASS_BATTERY_CHARGING,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import (
    BmsComponent,
    CONF_BMS_ID,
    CONF_MSG_ID,
    CONF_OFFSET,
    CONF_BIT_NO,
)


# define an alarm or warning bit, found as a bit in a byte at an offset in a message
def desc(msg_id, offset, bitno):
    return {
        CONF_MSG_ID: msg_id,
        CONF_OFFSET: offset,
        CONF_BIT_NO: bitno,
    }


SMA_ALARM_MSG_ID = 0x35A
PYLON_ALARM_MSG_ID = 0x359
PYLON_REQUEST_MSG_ID = 0x35C

# The sensor map from conf id to message and data decoding information. Each sensor may have multiple
# implementations corresponding to different BMS protocols.
ALARMS = {
    "general_alarm": (desc(SMA_ALARM_MSG_ID, 0, 0),),
    "high_voltage_alarm": (
        desc(SMA_ALARM_MSG_ID, 0, 2),
        desc(PYLON_ALARM_MSG_ID, 2, 1),
    ),
    "low_voltage_alarm": (
        desc(SMA_ALARM_MSG_ID, 0, 4),
        desc(PYLON_ALARM_MSG_ID, 2, 2),
    ),
    "high_temperature_alarm": (
        desc(SMA_ALARM_MSG_ID, 0, 6),
        desc(PYLON_ALARM_MSG_ID, 2, 3),
    ),
    "low_temperature_alarm": (
        desc(SMA_ALARM_MSG_ID, 1, 0),
        desc(PYLON_ALARM_MSG_ID, 2, 4),
    ),
    "high_temperature_charge_alarm": (desc(SMA_ALARM_MSG_ID, 1, 2),),
    "low_temperature_charge_alarm": (desc(SMA_ALARM_MSG_ID, 1, 4),),
    "high_current_alarm": (
        desc(SMA_ALARM_MSG_ID, 1, 6),
        desc(PYLON_ALARM_MSG_ID, 2, 7),
    ),
    "high_current_charge_alarm": (
        desc(SMA_ALARM_MSG_ID, 2, 0),
        desc(PYLON_ALARM_MSG_ID, 3, 0),
    ),
    "contactor_alarm": (desc(SMA_ALARM_MSG_ID, 2, 2),),
    "short_circuit_alarm": (desc(SMA_ALARM_MSG_ID, 2, 4),),
    "bms_internal_alarm": (
        desc(SMA_ALARM_MSG_ID, 2, 6),
        desc(PYLON_ALARM_MSG_ID, 3, 3),
    ),
    "cell_imbalance_alarm": (desc(SMA_ALARM_MSG_ID, 3, 0),),
}

WARNINGS = {
    "general_warning": (desc(SMA_ALARM_MSG_ID, 4, 0),),
    "high_voltage_warning": (
        desc(SMA_ALARM_MSG_ID, 4, 2),
        desc(PYLON_ALARM_MSG_ID, 0, 1),
    ),
    "low_voltage_warning": (
        desc(SMA_ALARM_MSG_ID, 4, 4),
        desc(PYLON_ALARM_MSG_ID, 0, 2),
    ),
    "high_temperature_warning": (
        desc(SMA_ALARM_MSG_ID, 4, 6),
        desc(PYLON_ALARM_MSG_ID, 0, 3),
    ),
    "low_temperature_warning": (
        desc(SMA_ALARM_MSG_ID, 5, 0),
        desc(PYLON_ALARM_MSG_ID, 0, 4),
    ),
    "high_temperature_charge_warning": (desc(SMA_ALARM_MSG_ID, 5, 2),),
    "low_temperature_charge_warning": (desc(SMA_ALARM_MSG_ID, 5, 4),),
    "high_current_warning": (
        desc(SMA_ALARM_MSG_ID, 5, 6),
        desc(PYLON_ALARM_MSG_ID, 0, 7),
    ),
    "high_current_charge_warning": (
        desc(SMA_ALARM_MSG_ID, 6, 0),
        desc(PYLON_ALARM_MSG_ID, 1, 0),
    ),
    "contactor_warning": (desc(SMA_ALARM_MSG_ID, 6, 2),),
    "short_circuit_warning": (desc(SMA_ALARM_MSG_ID, 6, 4),),
    "bms_internal_warning": (
        desc(SMA_ALARM_MSG_ID, 6, 6),
        desc(PYLON_ALARM_MSG_ID, 1, 3),
    ),
    "cell_imbalance_warning": (desc(SMA_ALARM_MSG_ID, 7, 0),),
}

REQUESTS = {
    "charge_enable": (desc(PYLON_REQUEST_MSG_ID, 0, 7),),
    "discharge_enable": (desc(PYLON_REQUEST_MSG_ID, 0, 6),),
    "force_charge_1": (desc(PYLON_REQUEST_MSG_ID, 0, 5),),
    "force_charge_2": (desc(PYLON_REQUEST_MSG_ID, 0, 4),),
    "request_full_charge": (desc(PYLON_REQUEST_MSG_ID, 0, 3),),
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
    .extend(dict(map(lambda name: binary_schema(name, DEVICE_CLASS_SAFETY), ALARMS)))
    .extend(
        dict(
            map(
                lambda name: binary_schema(name, DEVICE_CLASS_BATTERY_CHARGING),
                REQUESTS,
            )
        )
    )
    .extend(dict(map(lambda name: binary_schema(name, DEVICE_CLASS_PROBLEM), WARNINGS)))
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_BMS_ID])
    combined = ALARMS.copy()
    combined.update(WARNINGS)
    combined.update(REQUESTS)
    for key, entries in combined.items():
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

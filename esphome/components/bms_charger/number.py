import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_CURRENT,
    ENTITY_CATEGORY_CONFIG,
    ICON_CURRENT_DC,
    UNIT_AMPERE,
)
from . import CurrentNumber, BmsComponent, CONF_BMS_ID

CONF_MAX_CHARGE_CURRENT = "max_charge_current"
CONF_MAX_DISCHARGE_CURRENT = "max_discharge_current"


NUMBERS = (CONF_MAX_DISCHARGE_CURRENT, CONF_MAX_CHARGE_CURRENT)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BMS_ID): cv.use_id(BmsComponent),
        cv.Optional(CONF_MAX_CHARGE_CURRENT): number.number_schema(
            CurrentNumber,
            unit_of_measurement=UNIT_AMPERE,
            device_class=DEVICE_CLASS_CURRENT,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_CURRENT_DC,
        ),
        cv.Optional(CONF_MAX_DISCHARGE_CURRENT): number.number_schema(
            CurrentNumber,
            unit_of_measurement=UNIT_AMPERE,
            device_class=DEVICE_CLASS_CURRENT,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_CURRENT_DC,
        ),
    }
)


async def to_code(config):
    bms_id = config[CONF_BMS_ID]
    print(config)
    bms_component = await cg.get_variable(bms_id)
    for conf_key in NUMBERS:
        if entry := config.get(conf_key):
            n = await conf_key.new_number(entry, min_value=0, max_value=100, step=1)
            await cg.register_parented(n, config[CONF_BMS_ID])
            cg.add(cg.RawExpression(f"{bms_component}.set_{conf_key}_number({n}))"))

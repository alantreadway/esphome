//
// Created by Clyde Stubbs on 25/7/2023.

// Define an interface for bms devices.
//

#pragma once

namespace esphome {
namespace bms {
static const char *const CONF_HEALTH = "health";
static const char *const CONF_CHARGE = "charge";
static const char *const CONF_CURRENT = "current";
static const char *const CONF_VOLTAGE = "voltage";
static const char *const CONF_TEMPERATURE = "temperature";
static const char *const CONF_MAX_CHARGE_VOLTAGE = "max_charge_voltage";
static const char *const CONF_MAX_CHARGE_CURRENT = "max_charge_current";
static const char *const CONF_MAX_DISCHARGE_CURRENT = "max_discharge_current";
static const char *const CONF_MIN_DISCHARGE_VOLTAGE = "min_discharge_voltage";

class Bms {
 public:
  virtual float getVoltage() = 0;
  virtual float getCurrent() = 0;
  virtual float getCharge() = 0;
  virtual float getTemperature() = 0;
  virtual float getHealth() = 0;
  virtual float getMaxVoltage() = 0;
  virtual float getMinVoltage() = 0;
  virtual float getMaxChargeCurrent() = 0;
  virtual float getMaxDischargeCurrent() = 0;
};
}
}

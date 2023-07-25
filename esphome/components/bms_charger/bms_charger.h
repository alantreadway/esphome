#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/canbus/canbus.h"
#include "esphome/components/canbus_bms/canbus_bms.h"
#include <set>
#include <vector>
#include <map>

namespace esphome {
namespace bms_charger {

class BatteryDesc {
  friend class BmsChargerComponent;

 public:
  BatteryDesc(canbus_bms::CanbusBmsComponent *battery, uint32_t heartbeat)
      : battery_{battery},
        heartbeat_{heartbeat} {}

 protected:
  canbus_bms::CanbusBmsComponent *battery_;
  const uint32_t heartbeat_;
};

class BmsChargerComponent : public PollingComponent {
 public:
  BmsChargerComponent(const char *name, canbus::Canbus *canbus, bool debug, uint32_t interval)
      : PollingComponent(interval),
        name_{name},
        canbus_{canbus},
        debug_{debug} {}

  void update() override;

  void add_battery(BatteryDesc *battery) {
    this->batteries_.push_back(battery);
  }

 protected:
  const char *name_;
  canbus::Canbus *canbus_;
  const bool debug_;
  std::vector<BatteryDesc *> batteries_;
};

}
}

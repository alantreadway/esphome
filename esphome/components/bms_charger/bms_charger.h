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
  BatteryDesc(canbus_bms::CanbusBmsComponent *battery, uint32_t heartbeat_id, const char * heartbeat_text)
      : battery_{battery},
        heartbeat_id_{heartbeat_id},
        heartbeat_text_{heartbeat_text} {}

 protected:
  canbus_bms::CanbusBmsComponent *battery_;
  const uint32_t heartbeat_id_;
  const char * heartbeat_text_;
};

class BmsChargerComponent : public PollingComponent, public Action<std::vector<uint8_t>, uint32_t, bool> {
 public:
  BmsChargerComponent(const char *name, uint32_t timeout, canbus::Canbus *canbus, bool debug, uint32_t interval)
      : PollingComponent(interval),
        name_{name},
        timeout_{timeout},
        canbus_{canbus},
        debug_{debug} {}

  // called when a CAN Bus message is received
  void play(std::vector<uint8_t> data, uint32_t can_id, bool remote_transmission_request) override;

  void update() override;

  void add_connectivity_sensor(binary_sensor::BinarySensor *binarySensor) {
    this->connectivity_sensor_ = binarySensor;
  }

  void add_battery(BatteryDesc *battery) {
    this->batteries_.push_back(battery);
  }

 protected:
  const char *name_;
  uint32_t timeout_;
  canbus::Canbus *canbus_;
  const bool debug_;
  uint32_t last_rx_ = 0;
  size_t counter_ = 0;
  std::vector<BatteryDesc *> batteries_;
  binary_sensor::BinarySensor *connectivity_sensor_{};

  boolean isConnected_();
};

}
}

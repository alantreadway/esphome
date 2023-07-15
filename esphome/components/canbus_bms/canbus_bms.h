#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/canbus/canbus.h"
#include <set>
#include <vector>
#include <map>

namespace esphome {
namespace canbus_bms {

class SensorDesc {
  friend class CanbusBmsComponent;
  SensorDesc(sensor::Sensor *sensor, int msg_id, int offset, int length, float scale) {
    this->sensor_ = sensor;
    this->length_ = length;
    this->offset_ = offset;
    this->scale_ = scale;
    this->msg_id_ = msg_id;
  }

 protected:
    sensor::Sensor *sensor_;
    int msg_id_;
    int offset_;
    int length_;
    float scale_;
};

/**
  * This class captures state from a CANbus connected BMS, and reports sensor values.
  * It implements Action so that it can be connected to an Automation.
  */

class CanbusBmsComponent : public Component, public Action<std::vector<uint8_t>, uint32_t, bool> {
 public:
  CanbusBmsComponent() = default;

  void setup() override;
  void dump_config() override;

  void set_name(const char * name) { name_ = name; }
  void set_debug(bool debug) { debug_ = debug; }
  void set_throttle(uint32_t throttle) { throttle_ = throttle; }

  // called when a canbus message is received
  void play(std::vector<uint8_t> data, uint32_t can_id, bool remote_transmission_request) override;

  float get_setup_priority() const override;

  void add_sensor(sensor::Sensor *sensor, int msg_id, int offset, int length, float scale) {
    sensors_.push_back(new SensorDesc(sensor, msg_id, offset, length, scale));
  }

 protected:


 private:
  // our name
  const char * name_ = "CanbusBMS";
  bool debug_ = false;
  uint32_t throttle_ = 0;
  // track received ids TODO - make this work. received_ids.insert(can_id) won't compile!!
  std::set<int> received_ids_;
  std::vector<binary_sensor::BinarySensor *> binary_sensors_{};
  std::vector<SensorDesc *> sensors_{};
  std::map<int, std::vector<SensorDesc *>*> sensor_map_;
};

/**
 * Class used to link the canbus receiver to our component. Creation of an instance automatically
 * sets up the necessary chain of events:
 *       canbus->trigger->automation->bmscomponent
 * so that a received canbus message results in the CanbusBmsComponent::play() function being called.
 */
class BmsTrigger: public canbus::CanbusTrigger {
 public:
  BmsTrigger(CanbusBmsComponent * bms_component, canbus::Canbus *parent): canbus::CanbusTrigger(parent, 0, 0, false){
    // it would have been nice to have been able to simply add a listener to the Canbus object, but
    // no, we have to also create both a Trigger and an Automation to make them play together.
    // add_actions() is used even though we add a single Action, because using add_action() doesn't work
    // for some obscure reason.
    (new Automation<std::vector<uint8_t>, uint32_t, bool>(this))->add_actions({bms_component});
  }
};

}  // namespace canbus_bms
}  // namespace esphome

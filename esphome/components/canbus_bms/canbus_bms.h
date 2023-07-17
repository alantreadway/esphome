#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/canbus/canbus.h"
#include "esphome/core/automation.h"
#include <set>
#include <vector>
#include <map>

namespace esphome {
namespace canbus_bms {

class TextSensorDesc {
  friend class CanbusBmsComponent;
  TextSensorDesc(text_sensor::TextSensor *sensor, int msg_id):
    sensor_{sensor}, msg_id_{msg_id} {}

 protected:
    binary_sensor::BinarySensor *sensor_;
    const int msg_id_;
};

class BinarySensorDesc {
  friend class CanbusBmsComponent;
  BinarySensorDesc(binary_sensor::BinarySensor *sensor, int msg_id, int offset, int bit_no):
    sensor_{sensor}, msg_id_{msg_id}, offset_{offset}, bit_no_{bit_no} {}

 protected:
    binary_sensor::BinarySensor *sensor_;
    const int msg_id_;
    const int offset_;
    const int bit_no_;
};

class SensorDesc {
  friend class CanbusBmsComponent;
  SensorDesc(sensor::Sensor *sensor, int msg_id, int offset, int length, float scale):
    sensor_{sensor}, msg_id_{msg_id}, offset_{offset}, length_{length}, scale_{scale} {}

 protected:
    sensor::Sensor *sensor_;
    const int msg_id_;
    const int offset_;
    const int length_;
    const float scale_;
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
  void set_timeout(uint32_t timeout) { timeout_ = timeout; }

  // called when a canbus message is received
  void play(std::vector<uint8_t> data, uint32_t can_id, bool remote_transmission_request) override;

  float get_setup_priority() const override;

  void add_sensor(sensor::Sensor *sensor, const char * sensor_id, int msg_id, int offset, int length, float scale) {
    sensors_.push_back(new SensorDesc(sensor, msg_id, offset, length, scale));
    sensor_index_[sensor_id] = sensor;
  }
  void add_binary_sensor(binary_sensor::BinarySensor *sensor, const char * sensor_id, int msg_id, int offset, int bit_no) {
    binary_sensors_.push_back(new BinarySensorDesc(sensor, msg_id, offset, bit_no));
    binary_sensor_index_[sensor_id] = sensor;
  }

  void add_text_sensor(text_sensor::TextSensor *sensor, const char * sensor_id, int msg_id) {
    text_sensors_.push_back(new TextSensorDesc(sensor, msg_id));
    text_sensor_index_[sensor_id] = sensor;
  }

 protected:
  // our name
  const char * name_ = "CanbusBMS";
  bool debug_ = false;
  // min and max intervals between publish
  uint32_t throttle_ = 0;
  uint32_t timeout_ = 0;
  // log received canbus message IDs
  std::set<int> received_ids_;
  // all the sensors we are handling
  std::vector<const BinarySensorDesc *> binary_sensors_{};
  std::vector<const SensorDesc *> sensors_{};
  std::vector<const TextSensorDesc *> text_sensors_{};

  // construct maps of the above for efficient message processing
  std::map<int, std::vector<const BinarySensorDesc *>*> binary_sensor_map_;
  std::map<int, std::vector<const SensorDesc *>*> sensor_map_;
  std::map<int, std::vector<const TextSensorDesc *>*> text_sensor_map_;

  std::map<const char *, const sensor::Sensor *> sensor_index_;
  std::map<const char *, const binary_sensor::BinarySensor *> binary_sensor_index_;
  std::map<const char *, const text_sensor::TextSensor *> text_sensor_index_;
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

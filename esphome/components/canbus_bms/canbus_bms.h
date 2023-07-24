#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/select/select.h"
#include "esphome/components/canbus/canbus.h"
#include "esphome/core/automation.h"
#include <set>
#include <vector>
#include <map>

namespace esphome {
namespace canbus_bms {

class TextSensorDesc {
  friend class CanbusBmsComponent;

 public:
  TextSensorDesc(text_sensor::TextSensor *sensor, int msg_id) : sensor_{sensor}, msg_id_{msg_id} {}

 protected:
  text_sensor::TextSensor *sensor_;
  const int msg_id_;
};

class FlagDesc {
  friend class CanbusBmsComponent;

 public:
  FlagDesc(const char *key, const char *message, int msg_id, int offset, int bit_no, int warn_offset, int warn_bit_no)
      : key_{key},
        message_{message},
        msg_id_{msg_id},
        offset_{offset},
        bit_no_{bit_no},
        warn_offset_{warn_offset},
        warn_bit_no_{warn_bit_no} {}

 protected:
  const char *key_;
  const char *message_;
  const int msg_id_;
  const int offset_;
  const int bit_no_;
  const int warn_offset_;
  const int warn_bit_no_;
  bool warned_ = false;
  bool alarmed_ = false;
};

class BinarySensorDesc {
  friend class CanbusBmsComponent;

 public:
  BinarySensorDesc(binary_sensor::BinarySensor *sensor, int msg_id, int offset, int bit_no)
      : sensor_{sensor}, msg_id_{msg_id}, offset_{offset}, bit_no_{bit_no} {}

 protected:
  binary_sensor::BinarySensor *sensor_;
  const int msg_id_;
  const int offset_;
  const int bit_no_;
};

class SensorDesc {
  friend class CanbusBmsComponent;

 public:
  SensorDesc(sensor::Sensor *sensor, int msg_id, int offset, int length, float scale, bool filtered,
             uint32_t throttle, uint32_t timeout)
      : sensor_{sensor}, msg_id_{msg_id}, offset_{offset}, length_{length}, scale_{scale}, filtered_{filtered},
        timeout_filter_{std::make_shared<sensor::TimeoutFilter>(timeout)},
        throttle_filter_{std::make_shared<sensor::ThrottleFilter>(throttle)} {}

 protected:
  sensor::Sensor *sensor_;
  const int msg_id_;
  const int offset_;
  const int length_;
  const float scale_;
  const bool filtered_;
  std::shared_ptr<sensor::TimeoutFilter> timeout_filter_;
  std::shared_ptr<sensor::ThrottleFilter> throttle_filter_;
};

/**
 * This class captures state from a CANBus connected BMS, and reports sensor values.
 * It implements Action so that it can be connected to an Automation.
 */

class CanbusBmsComponent : public Action<std::vector<uint8_t>, uint32_t, bool>, public Component {
 public:
  CanbusBmsComponent() = default;
  void setup() override;
  void dump_config() override;
  void set_name(const char *name) { name_ = name; }
  void set_debug(bool debug) { debug_ = debug; }
  void set_throttle(uint32_t throttle) { throttle_ = throttle; }
  void set_timeout(uint32_t timeout) { timeout_ = timeout; }
  // called when a CAN Bus message is received
  void play(std::vector<uint8_t> data, uint32_t can_id, bool remote_transmission_request) override;
  float get_setup_priority() const override;

  void add_sensor(sensor::Sensor *sensor, const char *sensor_id, int msg_id, int offset, int length, float scale,
                  bool filtered) {
    this->sensors_.push_back(std::make_shared<SensorDesc>(sensor, msg_id, offset, length,
                             scale, filtered, this->throttle_, this->timeout_));
    this->sensor_index_[sensor_id] = sensor;
  }

  void add_binary_sensor(binary_sensor::BinarySensor *sensor, const char *sensor_id, int msg_id, int offset,
                         int bit_no) {
    this->binary_sensors_.push_back(std::make_shared<BinarySensorDesc>(sensor, msg_id, offset, bit_no));
    this->binary_sensor_index_[sensor_id] = sensor;
  }

  void add_text_sensor(text_sensor::TextSensor *sensor, const char *sensor_id, int msg_id) {
    this->text_sensors_.push_back(std::make_shared<TextSensorDesc>(sensor, msg_id));
    this->text_sensor_index_[sensor_id] = sensor;
  }

  // add flags for warnings and alarms
  void add_flag(const char *key, const char *message, int msg_id, int offset, int bit_no, int warn_offset,
                int warn_bit_no) {
    this->flags_.push_back(std::make_shared<FlagDesc>(key, message, msg_id, offset, bit_no, warn_offset, warn_bit_no));
  }

 protected:
  // our name
  const char *name_ = "CanbusBMS";
  bool debug_ = false;
  // min and max intervals between publish
  uint32_t throttle_ = 0;
  uint32_t timeout_ = 0;
  // log received canbus message IDs
  std::set<int> received_ids_;
  // all the sensors we are handling
  std::vector<std::shared_ptr<const BinarySensorDesc>> binary_sensors_{};
  std::vector<std::shared_ptr<const SensorDesc>> sensors_{};
  std::vector<std::shared_ptr<const TextSensorDesc>> text_sensors_{};
  std::vector<std::shared_ptr<FlagDesc>> flags_{};

  // construct maps of the above for efficient message processing
  std::map<int, std::shared_ptr<std::vector<std::shared_ptr<const BinarySensorDesc>>>> binary_sensor_map_;
  std::map<int, std::shared_ptr<std::vector<std::shared_ptr<const SensorDesc>>>> sensor_map_;
  std::map<int, std::shared_ptr<std::vector<std::shared_ptr<const TextSensorDesc>>>> text_sensor_map_;
  std::map<int, std::shared_ptr<std::vector<std::shared_ptr<FlagDesc>>>> flag_map_;

  std::map<const char *, sensor::Sensor *> sensor_index_;
  std::map<const char *, binary_sensor::BinarySensor *> binary_sensor_index_;
  std::map<const char *, text_sensor::TextSensor *> text_sensor_index_;

  text_sensor::TextSensor *alarm_text_sensor_;
  text_sensor::TextSensor *warning_text_sensor_;
  binary_sensor::BinarySensor *alarm_binary_sensor_;
  binary_sensor::BinarySensor *warning_binary_sensor_;

  void update_alarms_();
};

/**
 * Class used to link the canbus receiver to our component. Creation of an instance automatically
 * sets up the necessary chain of events:
 *       canbus->trigger->automation->bmscomponent
 * so that a received canbus message results in the CanbusBmsComponent::play() function being called.
 */
class BmsTrigger : public canbus::CanbusTrigger {
 public:
  BmsTrigger(CanbusBmsComponent *bms_component, canbus::Canbus *parent) : canbus::CanbusTrigger(parent, 0, 0, false) {
    (new Automation<std::vector<uint8_t>, uint32_t, bool>(this))->add_actions({bms_component});
  }
};

}  // namespace canbus_bms
}  // namespace esphome

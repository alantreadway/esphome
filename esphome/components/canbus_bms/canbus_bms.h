#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/canbus/canbus.h"
#include "esphome/core/automation.h"
#include "bms.h"
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
  uint32_t last_time_ = 0;  // records last time a value was sent
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
  uint32_t last_time_ = 0;  // records last time a value was sent
};

class SensorDesc {
  friend class CanbusBmsComponent;

 public:
  SensorDesc(const char * key, sensor::Sensor *sensor, int msg_id, int offset, int length, float scale, bool filtered)
      : key_{key}, sensor_{sensor}, msg_id_{msg_id}, offset_{offset},
        length_{length}, scale_{scale}, filtered_{filtered} {}

 protected:
  const char * key_;
  sensor::Sensor *sensor_;
  const int msg_id_;        // message id for this sensor
  const int offset_;        // byte position in message
  const int length_;        // length in bytes
  const float scale_;       // scale factor
  const bool filtered_;     // if sensor has its own filter chain
  uint32_t last_time_ = 0;  // records last time a value was sent
  float last_value_ = NAN;

  void publish(float value) {
    this->last_value_ = value;
    if (!this->filtered_)
      this->last_time_ = millis();
    if (this->sensor_ != NULL)
      this->sensor_->publish_state(value);
  }
};

/**
 * This class captures state from a CANBus connected BMS, and reports sensor values.
 * It implements Action so that it can be connected to an Automation.
 */

 class CanbusBmsComponent : public Action<std::vector<uint8_t>, uint32_t, bool>,
     public PollingComponent, public bms::Bms {
 public:
  CanbusBmsComponent(uint32_t throttle, uint32_t timeout, const char *name, bool debug)
      : PollingComponent(std::min(throttle, 15000U)),
        name_{name},
        debug_{debug},
        throttle_{throttle},
        timeout_{timeout} {}

  void setup() override;
  void update() override;
  void dump_config() override;
  float getVoltage() override;
  float getCurrent() override;
  float getCharge() override;
  float getTemperature() override;
  float getHealth() override;
  float getMaxVoltage() override;
  float getMinVoltage() override;
  float getMaxChargeCurrent() override;
  float getMaxDischargeCurrent() override;

  void set_canbus(canbus::Canbus * canbus) {
    this->canbus_ = canbus;
  }
// called when a CAN Bus message is received
  void play(std::vector<uint8_t> data, uint32_t can_id, bool remote_transmission_request) override;
  float get_setup_priority() const override;

  // add a list of sensors that are encoded in a given message.
  void add_sensor_list(uint32_t msg_id, std::vector<SensorDesc*> *sensors) {
    this->sensor_map_[msg_id] = sensors;
    for (auto * sensor: *sensors) {
      this->sensor_index_[sensor->key_] = sensor;
      this->sensors_.push_back(sensor);
    }
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

  // get the last known value of a value with given key
  float getValue(const char *key) {
    if (this->sensor_index_.count(key) != 0)
      return this->sensor_index_[key]->last_value_;
    return NAN;
  }

  // send data to the underlying canbus
  void send_data(uint32_t can_id, bool use_extended_id, bool remote_transmission_request,
                       const std::vector<uint8_t> &data) {
    if(this->canbus_)
      this->canbus_->send_data(can_id, use_extended_id, remote_transmission_request, data);
  }

 protected:
  // our name
  canbus::Canbus *canbus_;
  const char *name_;
  const bool debug_;
  // min and max intervals between publish
  const uint32_t throttle_;
  const uint32_t timeout_;
  // log received canbus message IDs
  std::set<int> received_ids_;
  // all the sensors we are handling
  std::vector<std::shared_ptr<BinarySensorDesc>> binary_sensors_{};
  std::vector<SensorDesc*> sensors_{};
  std::vector<std::shared_ptr<TextSensorDesc>> text_sensors_{};
  std::vector<std::shared_ptr<FlagDesc>> flags_{};

  // construct maps of the above for efficient message processing
  std::map<int, std::shared_ptr<std::vector<std::shared_ptr<BinarySensorDesc>>>> binary_sensor_map_;
  std::map<int, std::vector<SensorDesc*>*> sensor_map_;
  std::map<int, std::shared_ptr<std::vector<std::shared_ptr<TextSensorDesc>>>> text_sensor_map_;
  std::map<int, std::shared_ptr<std::vector<std::shared_ptr<FlagDesc>>>> flag_map_;

  std::map<const char *, SensorDesc*> sensor_index_;
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

  BmsTrigger(Action<std::vector<uint8_t>, uint32_t, bool> *bms_component, canbus::Canbus *parent)
    : canbus::CanbusTrigger(parent, 0, 0, false) {
      (new Automation<std::vector<uint8_t>, uint32_t, bool>(this))->add_actions({bms_component});
  }
};

}  // namespace canbus_bms
}  // namespace esphome

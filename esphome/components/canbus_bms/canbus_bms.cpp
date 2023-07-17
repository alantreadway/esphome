#include "canbus_bms.h"
#include "esphome/core/log.h"
#include "esphome/components/sensor/filter.h"
#include "esphome/components/binary_sensor/filter.h"


namespace esphome {
namespace canbus_bms {

static const char *const TAG = "canbus_bms";

/**
    BMS messages we know about:

    Common messages
    0x351,8,uint16 max voltage * 10, max charge * 10,max discharge * 10,discharge voltage * 10
    0x355,4,uint16=charge%,uint16=health%
    0x356,6,uint16 Batt voltage * 100,Battery Current * 10,,uint16 temperature x 10.
    0x35E,3 Battery name (BYD)

    BYD messages
    0x308,8,Inverter name (GW5048ES) - Sent by inverter as heartbeat once per second
    0x35F,8, Cell chemistry (Li), hardware version (0E04), capacity *10 (10.4),sw version (0C11)
    0xx35A,8 - status flags

    Pylon messages
    0x359,8,2 bytes protection flags, 2 bytes alarm flags, one byte = number of modules, 'PN', one unused byte
    0x35C,2,one byte charge/discharge enable and force flags, one unused byte
    0x305,8, Sent by inverter once per second, all zero data bytes

    SMA messages
    0x305,8,Inverter heartbeat: battery voltage, current, temperature, SOC (16 bits each)
    0x306,8,Inverter heartbeat: SOH, charging procedure, state, errors, charge voltage setpoint
    0xx35A,8 - status flags - same as BYD


 */

/*
 * Decode a value from a sequence of bytes. Little endian data of length bytes is extracted from data[offset]
 * All data is treated as signed - some things aren't, but will
 * never be big enough to overflow.
 */

static int decode_value(std::vector<uint8_t> data, int offset, int length) {
    int value = 0;
    for(int i = 0 ; i != length ; i++) {
        value += data[i+offset] << (i*8);
    }
    // sign extend if required.
    if(value & (1 << (length*8-1)) != 0)
      value |= ~0 << length * 8;
    return value;
}

void CanbusBmsComponent::setup() {
  for(auto sensor : this->sensors_) {
    if(sensor_map_.count(sensor->msg_id_) == 0)
      sensor_map_[sensor->msg_id_] = new std::vector<const SensorDesc *>();
    sensor_map_[sensor->msg_id_]->push_back(sensor);
    if(this->throttle_ != 0)
      sensor->sensor_->add_filter(new sensor::ThrottleFilter(this->throttle_));
    if(this->timeout_ != 0)
      sensor->sensor_->add_filter(new sensor::TimeoutFilter(this->timeout_));
  }
  for(auto sensor : this->binary_sensors_) {
    if(binary_sensor_map_.count(sensor->msg_id_) == 0)
      binary_sensor_map_[sensor->msg_id_] = new std::vector<const BinarySensorDesc *>();
    binary_sensor_map_[sensor->msg_id_]->push_back(sensor);
  }
}

void CanbusBmsComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "CANBus BMS:");
  ESP_LOGCONFIG(TAG, "  Name: %s", this->name_);
  ESP_LOGCONFIG(TAG, "  Throttle: %dms", this->throttle_);
  ESP_LOGCONFIG(TAG, "  Timeout: %dms", this->timeout_);
  ESP_LOGCONFIG(TAG, "  Sensors: %d", this->sensors_.size());
  ESP_LOGCONFIG(TAG, "  Binary Sensors: %d", this->binary_sensors_.size());
  ESP_LOGCONFIG(TAG, "  Text Sensors: %d", this->text_sensors_.size());
}

float CanbusBmsComponent::get_setup_priority() const { return setup_priority::DATA; }

void CanbusBmsComponent::play(std::vector<uint8_t> data, uint32_t can_id, bool remote_transmission_request) {
  // ideally these switch cases would be split out into separate functions, but in C++ that's just too much work.
  bool handled = false;
  if(this->debug_)
    ESP_LOGI(TAG, "%s: Received id 0x%02X, len %d", this->name_, can_id, data.size());
  if(this->sensor_map_.count(can_id) != 0) {
    for(auto sensor : *this->sensor_map_[can_id]) {
      if(data.size() >= sensor->offset_ + sensor->length_) {
        int16_t value = decode_value(data, sensor->offset_, sensor->length_);
        sensor->sensor_->publish_state((float)value * sensor->scale_);
        handled = true;
      }
    }
  }
  if(this->binary_sensor_map_.count(can_id) != 0) {
    for(auto sensor : *this->binary_sensor_map_[can_id]) {
      if(data.size() >= sensor->offset_) {
        bool value = data[sensor->offset_] & (1 << sensor->bit_no_) != 0;
        sensor->sensor_->publish_state(value);
        handled = true;
      }
    }
  }
  if(this->text_sensor_map_.count(can_id) != 0) {
    for(auto sensor : *this->text_sensor_map_[can_id]) {
      if(data.size() != 0) {
        char str[CAN_MAX_DATA_LENGTH+1];
        memcpy(&data[0], str, std::max(CAN_MAX_DATA_LENGTH, data.size()));
        str[data.size()] = 0;
        sensor->publish_state(str);
        handled = true;
    }
  }
  if(!handled && (this->debug_ || this->received_ids_.count((int)can_id) == 0)) {
    ESP_LOGW(TAG, "%s: Received unhandled id 0x%02X, len %d", this->name_, can_id, data.size());
    this->received_ids_.insert((int)can_id);
  }
}

}  // namespace canbus_bms
}  // namespace esphome

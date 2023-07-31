#include "bms_charger.h"
#include <cmath>

namespace esphome {
namespace bms_charger {

static const char *const TAG = "BmsCharger";
static const uint8_t BMS_NAME[] = {'E', 'S', 'P', 'H', 'o', 'm', 'e'};

// intervals for various messages. Could be made config values if required.

static const size_t NAME_INTERVAL = 30;
static const size_t LIMITS_INTERVAL = 3;
static const size_t CHARGE_INTERVAL = 3;
static const size_t STATUS_INTERVAL = 3;
//static const size_t ALARMS_INTERVAL = 10;
//static const size_t INFO_INTERVAL = 10;

// message types

static const uint32_t NAME_MSG = 0x35E;
static const uint32_t LIMITS_MSG = 0x351;
//static const uint32_t ALARMS_MSG = 0x35A;
static const uint32_t CHARGE_MSG = 0x355;
static const uint32_t STATUS_MSG = 0x356;
//static const uint32_t INFO_MSG = 0x35F;     // TODO

static void update_list(std::vector<float> &list, float value) {
  if (!std::isnan(value))
    list.push_back(value);
}

static void put_int16(float value, std::vector<uint8_t> &buffer, const float scale) {
  if (std::isnan(value))
    value = 0.0;
  int16_t int_value = (int16_t)(value / scale);
  buffer.push_back((uint8_t)(int_value & 0xFF));
  buffer.push_back((uint8_t)((int_value >> 8) & 0xFF));
}

static void log_msg(const char * text, uint32_t id, std::vector<uint8_t> data) {
  size_t len = std::min(data.size(), 8U);
  char buffer[32];
  for(size_t i = 0 ; i != len ; i++) {
    sprintf(buffer + i * 3 , "%02X ", data[i]);
  }
  ESP_LOGI(TAG, "%s 0x%X: %s", text, id, buffer);
}


void BmsChargerComponent::play(std::vector<uint8_t> data, uint32_t can_id, bool remote_transmission_request) {
  this->last_rx_ = millis();
  if(this->debug_)
    log_msg("Received from inverter", can_id, data);
}
// called at a typically 1 second interval
void BmsChargerComponent::update() {

  this->counter_++;

  std::vector<float> voltages;
  std::vector<float> currents;
  std::vector<float> charges;
  std::vector<float> temperatures;
  std::vector<float> healths;
  std::vector<float> max_voltages;
  std::vector<float> min_voltages;
  std::vector<float> max_charge_currents;
  std::vector<float> max_discharge_currents;

  boolean nowConnected = this->last_rx_ + this->timeout_ > millis();
  if(this->connectivity_sensor_)
    this->connectivity_sensor_->publish_state(nowConnected);

  // this loop could be broken up and only the parts necessary done on each update() call,
  // but the time used here is not that significant, unlike the CAN send_message() calls.
  for (auto battery: this->batteries_) {
    // calculate average battery voltage
    auto bms = battery->battery_;
    update_list(voltages, bms->getVoltage());
    update_list(currents, bms->getCurrent());
    update_list(charges, bms->getCharge());
    update_list(temperatures, bms->getTemperature());
    update_list(healths, bms->getHealth());
    update_list(max_voltages, bms->getMaxVoltage());
    update_list(min_voltages, bms->getMinVoltage());
    update_list(max_charge_currents, bms->getMaxChargeCurrent());
    update_list(max_discharge_currents, bms->getMaxDischargeCurrent());
  }
  if (this->debug_)
    ESP_LOGI(TAG, "MaxVoltages.size() = %d", max_voltages.size());

  std::vector <uint8_t> data;
  float acc = 0.0;
  if(this->counter_ % STATUS_INTERVAL == 0 && !voltages.empty()) {
    // average measured voltages, resolution .01V
    for (auto value: voltages)
      acc += value;
    float voltage = acc / voltages.size();
    put_int16(voltage, data, 0.01);

    // sum currents, resolution 0.1
    acc = 0.0;
    for (auto value: currents)
      acc += value;
    float current = acc;
    put_int16(current, data, 0.1);

    // average temperatures, resolution 0.1
    acc = 0.0;
    for (auto value: temperatures)
      acc += value;
    float temperature = acc / temperatures.size();
    put_int16(temperature, data, 0.1);

    this->canbus_->send_data(STATUS_MSG, false, false, data);
    if (this->debug_) {
      ESP_LOGI(TAG, "Voltage=%.1f, current=%.1f, temperature=%.1f", voltage, current, temperature);
      log_msg("Status", STATUS_MSG, data);
    }
  }
  data.clear();

  if (this->counter_ % CHARGE_INTERVAL == 1 && !charges.empty())  {
    // average charge
    acc = 0.0;
    for (auto value: charges)
      acc += value;
    float charge = acc / charges.size();
    put_int16(charge, data, 1.0);

    // average health
    acc = 0.0;
    for (auto value: healths)
      acc += value;
    float health = acc / healths.size();
    put_int16(health, data, 1.0);
    this->canbus_->send_data(CHARGE_MSG, false, false, data);
    if (this->debug_) {
      ESP_LOGI(TAG, "Charge=%.0f%%, health=%.0f%%", charge, health);
      log_msg("Charge", CHARGE_MSG, data);
    }
  }

  // send charge/discharge limits

  if (this->counter_ % LIMITS_INTERVAL == 2 && !max_voltages.empty())  {
    // max voltage is the highest reported. TODO is this the best choice? Using the lowest may compromise balancing.
    data.clear();
    acc = 0.0;
    for (auto value: max_voltages)
      acc = std::max(acc, value);
    float max_voltage = acc;
    put_int16(acc, data, 0.1);

    // max charge current is the lowest value times the number of batteries.
    // or the average value, whichever is greater
    // TODO - dynamically adjust this to keep all batteries within their limits.
    acc = 10000.0;
    float avg = 0.0;
    for (auto value: max_charge_currents) {
      acc = std::min(acc, value);
      avg += value;
    }
    float max_charge = std::max(acc * max_charge_currents.size(), avg / max_charge_currents.size());
    put_int16(max_charge, data, 0.1);

    // similarly with discharge currents
    acc = 10000.0;
    for (auto value: max_discharge_currents)
      acc = std::min(acc, value);
    float max_discharge = acc * max_discharge_currents.size();
    put_int16(max_discharge, data, 0.1);

    acc = 0.0;
    // use the highest minimum voltage - this is a conservative choice. Should have little downside.
    for (auto value: min_voltages)
      acc = std::max(acc, value);
    float min_voltage = acc;
    put_int16(min_voltage, data, 0.1);

    this->canbus_->send_data(LIMITS_MSG, false, false, data);
    if (this->debug_) {
      ESP_LOGI(TAG, "Max volts=%.1f, max charge=%.1f, max discharge=%.1f, min volts=%.1f",
         max_voltage, max_charge, max_discharge, min_voltage);
      log_msg("Limits", LIMITS_MSG, data);
    }
  }

  // send name
  data.clear();
  data.insert(data.end(), &BMS_NAME[0], &BMS_NAME[sizeof(BMS_NAME)]);
  if (this->counter_ % NAME_INTERVAL == 0)  {
    this->canbus_->send_data(NAME_MSG, false, false, data);
    if (this->debug_)
      log_msg("Name", NAME_MSG, data);
  }

  // send heartbeats to the batteries every time, if we are connected to a charger or inverter.
  if(nowConnected) {
    for (auto desc: batteries_) {
      data.clear();
      data.insert(data.end(), (uint8_t * ) & desc->heartbeat_text_[0],
                  (uint8_t * ) & desc->heartbeat_text_[strlen(desc->heartbeat_text_)]);
      desc->battery_->send_data(desc->heartbeat_id_, false, false, data);
      if (this->debug_)
        log_msg("Heartbeat", desc->heartbeat_id_, data);
    }
  }
}

}
}

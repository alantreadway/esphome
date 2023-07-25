#include "bms_charger.h"
#include <cmath>

namespace esphome {
namespace bms_charger {


static const uint8_t BMS_NAME[] = {'E', 'S', 'P', 'H', 'o', 'm', 'e', ' '};

static void update_list(std::vector<float> &list, float value) {
  if (!std::isnan(value))
    list.push_back(value);
}

static void put_int16(float value, std::vector <uint8_t> &buffer, const float scale) {
  if (std::isnan(value))
    value = 0.0;
  int16_t int_value = (int16_t)(value / scale);
  buffer.push_back((uint8_t)(int_value & 0xFF));
  buffer.push_back((uint8_t)((int_value >> 8) & 0xFF));
}

// called at a typically 1 second interval
void BmsChargerComponent::update() {

  std::vector<float> voltages;
  std::vector<float> currents;
  std::vector<float> charges;
  std::vector<float> temperatures;
  std::vector<float> healths;
  std::vector<float> max_voltages;
  std::vector<float> min_voltages;
  std::vector<float> max_charge_currents;
  std::vector<float> max_discharge_currents;


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

  std::vector <uint8_t> data;
  float acc = 0.0;
  // average measured voltages, resolution .01V
  for (auto value: voltages)
    acc += value;
  put_int16(acc / voltages.size(), data, 0.01);

  // sum currents, resolution 0.1
  acc = 0.0;
  for (auto value: currents)
    acc += value;
  put_int16(acc, data, 0.1);

  // average temperatures, resolution 0.1
  acc = 0.0;
  for (auto value: temperatures)
    acc += value;
  put_int16(acc / temperatures.size(), data, 0.1);
  this->canbus_->send_data(0x356, false, false, data);

  data.clear();

  // average charge
  acc = 0.0;
  for (auto value: charges)
    acc += value;
  put_int16(acc / charges.size(), data, 1.0);

  // average health
  acc = 0.0;
  for (auto value: healths)
    acc += value;
  put_int16(acc / healths.size(), data, 1.0);
  this->canbus_->send_data(0x355, false, false, data);

  data.clear();
  acc = 0.0;
  for (auto value: max_voltages)
    acc = std::min(acc, value);
  put_int16(acc, data, 0.1);

  acc = 0.0;
  for (auto value: max_charge_currents)
    acc += value;
  put_int16(acc, data, 0.1);

  acc = 0.0;
  for (auto value: max_discharge_currents)
    acc += value;
  put_int16(acc, data, 0.1);

  acc = 0.0;
  for (auto value: min_voltages)
    acc = std::max(acc, value);
  put_int16(acc, data, 0.1);

  this->canbus_->send_data(0x351, false, false, data);

  // send name
  data.clear();
  data.insert(data.end(), &BMS_NAME[0], &BMS_NAME[sizeof(BMS_NAME)]);
  this->canbus_->send_data(0x35E, false, false, data);
  // send heartbeats to the batteries
  for (auto desc: batteries_) {
    desc->battery_->send_data(desc->heartbeat_, false, false, data);
  }
}

}
}

#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include <vector>

namespace esphome {
namespace mr24fdb1 {

// Frame: 0x55, LEN_L, LEN_H, FUNC, A1, A2, DATA..., CRC_L, CRC_H
static const uint8_t MSG_HEAD     = 0x55;
static const uint8_t FUNC_READ    = 0x01;
static const uint8_t FUNC_WRITE   = 0x02;
static const uint8_t FUNC_PASSIVE = 0x03;
static const uint8_t FUNC_ACTIVE  = 0x04;
// część firmware raportuje FALL pod FUNC=0x06

class MR24FDB1 : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Settery encji
  void set_presence_sensor(binary_sensor::BinarySensor *s) { presence_sensor_ = s; }
  void set_fall_sensor(binary_sensor::BinarySensor *s) { fall_sensor_ = s; }
  void set_state_text_sensor(text_sensor::TextSensor *s) { state_text_ = s; }

  void set_env_triplet_text_sensor(text_sensor::TextSensor *s) { env_triplet_text_ = s; }
  void set_env_label_text_sensor(text_sensor::TextSensor *s) { env_label_text_ = s; }
  void set_approach_text_sensor(text_sensor::TextSensor *s) { approach_text_ = s; }
  void set_fall_state_text_sensor(text_sensor::TextSensor *s) { fall_state_text_ = s; }
  void set_sign_parameter_sensor(sensor::Sensor *s) { sign_parameter_ = s; }

 protected:
  std::vector<uint8_t> buf_;
  uint32_t last_byte_ms_{0};

  binary_sensor::BinarySensor *presence_sensor_{nullptr};
  binary_sensor::BinarySensor *fall_sensor_{nullptr};

  text_sensor::TextSensor *state_text_{nullptr};
  text_sensor::TextSensor *env_triplet_text_{nullptr};
  text_sensor::TextSensor *env_label_text_{nullptr};
  text_sensor::TextSensor *approach_text_{nullptr};
  text_sensor::TextSensor *fall_state_text_{nullptr};

  sensor::Sensor *sign_parameter_{nullptr}; // 0..100 (aktywność / body sign)

  void handle_frame_(const std::vector<uint8_t> &frame);
  bool check_crc16_(const uint8_t *data, size_t total_len);
  uint16_t crc16_modbus_(const uint8_t *data, size_t len);

  void handle_env_status_(const uint8_t *data, size_t len);
  void handle_approach_(const uint8_t *data, size_t len);
  void handle_fall_report_(const uint8_t *data, size_t len);
  void handle_sign_parameter_(const uint8_t *data, size_t len);
};

}  // namespace mr24fdb1
}  // namespace esphome

#include "mr24fdb1.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mr24fdb1 {

static const char *const TAG = "mr24fdb1";

void MR24FDB1::setup() {
  if (this->presence_sensor_) this->presence_sensor_->publish_state(false);
  if (this->fall_sensor_)     this->fall_sensor_->publish_state(false);
}

void MR24FDB1::dump_config() {
  ESP_LOGCONFIG(TAG, "MR24FDB1 (UART 9600 8N1)");
  if (this->presence_sensor_) ESP_LOGCONFIG(TAG, "  presence: configured");
  if (this->fall_sensor_)     ESP_LOGCONFIG(TAG, "  fall: configured");
  if (this->state_text_)      ESP_LOGCONFIG(TAG, "  state text: configured");
  if (this->env_triplet_text_) ESP_LOGCONFIG(TAG, "  env_triplet: configured");
  if (this->env_label_text_)   ESP_LOGCONFIG(TAG, "  env_label: configured");
  if (this->approach_text_)    ESP_LOGCONFIG(TAG, "  approach_state: configured");
  if (this->fall_state_text_)  ESP_LOGCONFIG(TAG, "  fall_state: configured");
  if (this->sign_parameter_)   ESP_LOGCONFIG(TAG, "  sign_parameter: configured");
}

void MR24FDB1::loop() {
  const uint32_t now = millis();
  if (!this->buf_.empty() && (now - this->last_byte_ms_ > 150)) {
    this->buf_.clear();
  }
  while (this->available()) {
    uint8_t b;
    if (!this->read_byte(&b)) break;
    this->last_byte_ms_ = millis();

    if (this->buf_.empty()) {
      if (b != MSG_HEAD) continue;
    }
    this->buf_.push_back(b);

    if (this->buf_.size() >= 3) {
      const uint16_t len16 = (uint16_t)this->buf_[1] | ((uint16_t)this->buf_[2] << 8);
      const size_t expected_total = (size_t)(1 + len16); // 55 + LEN

      if (this->buf_.size() == expected_total) {
        std::vector<uint8_t> frame = this->buf_;
        this->buf_.clear();

        if (!this->check_crc16_(frame.data(), frame.size())) {
          ESP_LOGW(TAG, "CRC mismatch (size=%u)", (unsigned)frame.size());
          continue;
        }
        this->handle_frame_(frame);

      } else if (this->buf_.size() > expected_total) {
        this->buf_.clear();
      }
    }
  }
}

bool MR24FDB1::check_crc16_(const uint8_t *data, size_t total_len) {
  if (total_len < 8) return false;
  const uint16_t got = (uint16_t)data[total_len - 2] | ((uint16_t)data[total_len - 1] << 8);
  const uint16_t calc = this->crc16_modbus_(data, total_len - 2);
  return calc == got;
}

uint16_t MR24FDB1::crc16_modbus_(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) { crc >>= 1; crc ^= 0xA001; }
      else { crc >>= 1; }
    }
  }
  return crc;
}

void MR24FDB1::handle_frame_(const std::vector<uint8_t> &frame) {
  if (frame.size() < 8) return;

  const uint16_t len16 = (uint16_t)frame[1] | ((uint16_t)frame[2] << 8);
  const uint8_t  func  = frame[3];
  const uint8_t  a1    = frame[4];
  const uint8_t  a2    = frame[5];
  int data_len = (int)len16 - 5; // func(1)+a1(1)+a2(1)+crc(2)
  if (data_len < 0) data_len = 0;

  const uint8_t *data = (data_len > 0) ? &frame[6] : nullptr;

  if (this->state_text_) {
    char buf[64];
    snprintf(buf, sizeof(buf), "func=0x%02X a1=0x%02X a2=0x%02X dlen=%d",
             func, a1, a2, data_len);
    this->state_text_->publish_state(buf);
  }

  // aktywne/pasywne raporty z dokumentacji
  if (func == FUNC_ACTIVE || func == FUNC_PASSIVE) {
    if (a1 == 0x03 && a2 == 0x05 && data && data_len >= 3) { // Environment status
      this->handle_env_status_(data, (size_t)data_len);
      return;
    }
    if (a1 == 0x03 && a2 == 0x07 && data && data_len >= 1) { // Approach/Away
      this->handle_approach_(data, (size_t)data_len);
      return;
    }
    if (a1 == 0x06 && data && data_len >= 4 && this->sign_parameter_) {
      // „Signs parameters” jako 4-bajtowy float (aktywność/mikro-ruch 0..100) – wg GitHub/Wiki
      this->handle_sign_parameter_(data, (size_t)data_len);
      return;
    }
    if (a1 == 0x05) {
      // Heartbeat/Other info – ignorujemy lub można dodać diagnostykę
      return;
    }
  }

  // Fall reporting – część FW: FUNC=0x06, A1=0x01 (0x00 suspected, 0x01 real, 0x02 none)
  if (func == 0x06 && a1 == 0x01 && data && data_len >= 1) {
    this->handle_fall_report_(data, (size_t)data_len);
    return;
  }
}

void MR24FDB1::handle_env_status_(const uint8_t *d, size_t len) {
  if (len < 3) return;
  const uint8_t b0 = d[0], b1 = d[1], b2 = d[2];

  if (this->env_triplet_text_) {
    char trip[12];
    snprintf(trip, sizeof(trip), "%02X %02X %02X", b0, b1, b2);
    this->env_triplet_text_->publish_state(trip);
  }

  std::string label = "Occupied (other)";
  bool occ = true;

  if (b0 == 0x00 && b1 == 0xFF && b2 == 0xFF) {
    label = "Unoccupied";
    occ = false;
  } else if (b0 == 0x01 && b1 == 0x00 && b2 == 0xFF) {
    label = "Someone stationary";
    occ = true;
  } else if (b0 == 0x01 && b1 == 0x01 && b2 == 0x01) {
    label = "Someone moving";
    occ = true;
  }
  if (this->env_label_text_)  this->env_label_text_->publish_state(label);
  if (this->presence_sensor_) this->presence_sensor_->publish_state(occ);
}

void MR24FDB1::handle_approach_(const uint8_t *d, size_t len) {
  if (!this->approach_text_) return;
  std::string ap = "None";
  if (len >= 1) {
    if (d[0] == 0x02) ap = "Close";
    else if (d[0] == 0x03) ap = "Away";
  }
  this->approach_text_->publish_state(ap);
}

void MR24FDB1::handle_fall_report_(const uint8_t *d, size_t len) {
  std::string fs = "None";
  bool real = false;
  if (len >= 1) {
    if (d[0] == 0x00) fs = "Suspected";
    else if (d[0] == 0x01) { fs = "Real"; real = true; }
    else fs = "None";
  }
  if (this->fall_state_text_) this->fall_state_text_->publish_state(fs);
  if (this->fall_sensor_)     this->fall_sensor_->publish_state(real);
}

void MR24FDB1::handle_sign_parameter_(const uint8_t *d, size_t len) {
  if (!this->sign_parameter_ || len < 4) return;
  // 4-bajtowy float w DATA (wg przykładowych implementacji), ale wiele FW podaje 1-bajtową skalę 0..100.
  // Najpierw spróbuj float (LE); jeśli absurdalny, spróbuj 1 bajt.
  float v;
  memcpy(&v, d, 4);
  if (!std::isfinite(v) || v < 0.0f || v > 1000.0f) {
    // fallback: jeśli radar wysyła 1 bajt 0..100
    v = (float)d[0];
  }
  this->sign_parameter_->publish_state(v);
}

}  // namespace mr24fdb1
}  // namespace esphome

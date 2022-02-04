#include "mcp4728_output.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mcp4728 {

static const char *const TAG = "mcp4728";

void MCP4728Output::setup() {
  ESP_LOGCONFIG(TAG, "Setting up MCP4728OutputComponent...");
}

void MCP4728Output::dump_config() {
  ESP_LOGCONFIG(TAG, "MCP4728:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with MCP4728 failed!");
  }
}

void MCP4728Output::loop() {
  if (this->update_) {
    this->update_ = false;
    if (this->eeprom_)
      this->seq_write_();
    else
      this->multi_write_();
  }
}

void MCP4728Output::set_channel_value_(MCP4728ChannelIdx channel, uint16_t value) {
  uint8_t cn = 0;
  if (channel == MCP4728_CHANNEL_A)
    cn = 'A';
  else if (channel == MCP4728_CHANNEL_B)
    cn = 'B';
  else if (channel == MCP4728_CHANNEL_C)
    cn = 'C';
  else
    cn = 'D';
  ESP_LOGD(TAG, "Setting MCP4728 channel %c to %d!", cn, value);
  reg_[channel].data = value;
  this->update_ = true;
}

uint8_t MCP4728Output::multi_write_() {
  for (uint8_t i = 0; i < 4; ++i) {
    uint8_t wd[3];
    wd[0] = ((uint8_t)CMD::MULTI_WRITE | (i << 1)) & 0xFE;
    wd[1] = ((uint8_t)reg_[i].vref << 7) | ((uint8_t)reg_[i].pd << 5) |
            ((uint8_t)reg_[i].gain << 4) | (reg_[i].data >> 8);
    wd[2] = reg_[i].data & 0xFF;
    this->write(wd, sizeof(wd));
  }
  return 0;
}

uint8_t MCP4728Output::seq_write_() {
  uint8_t wd[9];
  wd[0] = (uint8_t)CMD::SEQ_WRITE;
  for (uint8_t i = 0; i < 4; i++) {
    wd[i * 2 + 1] = ((uint8_t)reg_[i].vref << 7) | ((uint8_t)reg_[i].pd << 5) |
                    ((uint8_t)reg_[i].gain << 4) | (reg_[i].data >> 8);
    wd[i * 2 + 2] = reg_[i].data & 0xFF;
  }
  this->write(wd, sizeof(wd));
  return 0;
}

void MCP4728Output::select_vref_(MCP4728ChannelIdx channel, MCP4728Vref vref) {
  reg_[channel].vref = vref;

  this->update_ = true;
}

void MCP4728Output::select_power_down_(MCP4728ChannelIdx channel, MCP4728PwrDown pd) {
  reg_[channel].pd = pd;

  this->update_ = true;
}

void MCP4728Output::select_gain_(MCP4728ChannelIdx channel, MCP4728Gain gain) {
  reg_[channel].gain = gain;

  this->update_ = true;
}

void MCP4728Channel::write_state(float state) {
  const uint16_t max_duty = 4095;
  const float duty_rounded = roundf(state * max_duty);
  auto duty = static_cast<uint16_t>(duty_rounded);
  this->parent_->set_channel_value_(this->channel_, duty);
}

}  // namespace mcp4728
}  // namespace esphome
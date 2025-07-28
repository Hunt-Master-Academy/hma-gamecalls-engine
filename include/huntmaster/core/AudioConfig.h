/**
 * @file AudioConfig.h
 * @brief Audio Configuration Interface
 */

#pragma once

namespace huntmaster {
namespace core {

/**
 * @brief Simple audio configuration structure
 */
struct AudioConfig {
    float sample_rate = 44100.0f;
    size_t buffer_size = 1024;
    size_t channel_count = 1;
};

}  // namespace core
}  // namespace huntmaster

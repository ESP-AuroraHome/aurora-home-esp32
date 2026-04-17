#pragma once

/**
 * @file ISensor.h
 * @brief Common base interface for every AuroraHome sensor.
 *
 * Each concrete sensor implements begin()/name() and exposes its own
 * typed read(...) methods returning the measurements it supports.
 */
class ISensor {
 public:
    virtual ~ISensor() = default;

    /**
     * @brief Initialize the sensor (I2C probe, config registers, calibration).
     * @return true on success, false otherwise.
     */
    virtual bool begin() = 0;

    /**
     * @brief Human-readable sensor name for logs.
     */
    virtual const char* name() const = 0;
};

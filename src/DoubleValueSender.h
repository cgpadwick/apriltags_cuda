#ifndef DOUBLEVALUESENDER_H
#define DOUBLEVALUESENDER_H

#include "networktables/NetworkTable.h"
#include "networktables/NetworkTableInstance.h"
#include "networktables/DoubleTopic.h"

#include <string>

class DoubleValueSender {
  private:
    nt::NetworkTableInstance inst_;
    nt::DoublePublisher publisher_;
  public:
    // Constructor declaration
    DoubleValueSender(std::string key);

    // Method declarations
    void sendValue(double value);
    void setDefaultValue(double value);
};

#endif
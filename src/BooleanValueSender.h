#ifndef BOOLEANVALUESENDER_H
#define BOOLEANVALUESENDER_H

#include "networktables/NetworkTable.h"
#include "networktables/NetworkTableInstance.h"
#include "networktables/BooleanTopic.h"

#include <string>

class BooleanValueSender {
  private:
    nt::NetworkTableInstance inst_;
    nt::BooleanPublisher publisher_;
  public:
    // Constructor declaration
    BooleanValueSender(std::string key);

    // Method declarations
    void sendValue(bool value);
    void setDefaultValue(bool value);
};

#endif
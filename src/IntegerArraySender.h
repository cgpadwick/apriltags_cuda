#ifndef INTEGERARRAYSENDER_H
#define INTEGERARRAYSENDER_H

#include "networktables/NetworkTable.h"
#include "networktables/NetworkTableInstance.h"
#include "networktables/IntegerArrayTopic.h"

#include <string>
#include <vector>
#include <span>

class IntegerArraySender {
  private:
    nt::NetworkTableInstance inst_;
    nt::IntegerArrayPublisher publisher_;
  
  public:
    // Constructor declaration
    IntegerArraySender(std::string key);

    // Method declarations
    void sendValue(std::span<const int64_t> value);
    void setDefaultValue(std::span<const int64_t>value);
};

#endif
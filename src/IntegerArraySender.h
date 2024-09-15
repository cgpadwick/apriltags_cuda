#ifndef INTEGERARRAYSENDER_H
#define INTEGERARRAYSENDER_H

#include "networktables/NetworkTable.h"
#include "networktables/NetworkTableInstance.h"
#include "networktables/IntegerArrayTopic.h"

#include <string>
#include <vector>

class IntegerArraySender {
  private:
    nt::NetworkTableInstance inst_;
    nt::IntegerArrayPublisher publisher_;
  
  public:
    // Constructor declaration
    IntegerArraySender(std::string key);

    // Method declarations
    void sendValue(std::vector<int> value);
    void setDefaultValue(std::vector<int> value);
};

#endif
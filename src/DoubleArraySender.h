#ifndef DOUBLEARRAYSENDER_H
#define DOUBLEARRAYSENDER_H

#include "networktables/NetworkTable.h"
#include "networktables/NetworkTableInstance.h"
#include "networktables/DoubleArrayTopic.h"

#include <string>
#include <vector>

class DoubleArraySender {
  private:
    nt::NetworkTableInstance inst_;
    nt::DoubleArrayPublisher publisher_;
  
  public:
    // Constructor declaration
    DoubleArraySender(std::string key);

    // Method declarations
    void sendValue(std::vector<double> value);
    void setDefaultValue(std::vector<double> value);
};

#endif
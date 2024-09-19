#ifndef TWODIMENSIONALDOUBLEARRAYSENDER_H
#define TWODIMENSIONALDOUBLEARRAYSENDER_H

#include "networktables/NetworkTable.h"
#include "networktables/NetworkTableInstance.h"
#include "networktables/DoubleArrayTopic.h"

#include <string>
#include <vector>

class TwoDimensionalDoubleArraySender {
  private:
    nt::NetworkTableInstance inst_;
    nt::DoubleArrayPublisher publisher_;
  
  public:
    // Constructor declaration
    TwoDimensionalDoubleArraySender(std::string key);

    // Method declarations
    void sendValue(std::vector<std::vector<double>> value);
};

#endif
#ifndef NETWORKTABLESUTIL_H
#define NETWORKTABLESUTIL_H


#include "networktables/NetworkTable.h"
#include "networktables/NetworkTableInstance.h"
#include "wpi/timestamp.h"

class NetworkTablesUtil {
 private:
  nt::NetworkTableInstance inst_;

 public:
  // Constructor declaration
  NetworkTablesUtil();

  // Method declarations
  double getTime();
};

#endif
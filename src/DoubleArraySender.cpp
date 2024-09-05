#include "networktables/NetworkTable.h"
#include "networktables/NetworkTableInstance.h"
#include "networktables/DoubleArrayTopic.h"

#include <string>
#include <iostream>
#include <vector>

/*
Right now, the table is /SmartDashboard so we can visualize the values sent from the Orin.
Later, this should be changed to a new table, such as /Orin or /Vision, to avoid populating the SmartDashboard with unnecessary values
*/

class DoubleArraySender{
  private:
    nt::NetworkTableInstance inst;
    nt::DoubleArrayPublisher publisher;
  public:
    DoubleArraySender(std::string key){
      inst = nt::NetworkTableInstance::GetDefault();
      inst.SetServer("10.7.66.2");
      inst.StartClient4("10.7.66.2");
      auto table = inst.GetTable("/SmartDashboard");
      nt::DoubleArrayTopic topic = table->GetDoubleArrayTopic(key);
      publisher = topic.Publish();
    }
    void sendValue(std::vector<double> value){
      publisher.Set(value);
    }
    void setDefaultValue(std::vector<double> value){
      publisher.SetDefault(value);
    }
};

// Class use example

// int main(){
//   DoubleArraySender sender("NVIDIA ORIN ARRAY TEST");
//   while(2>1){
//     std::vector<double> values = {1.0, 2.0, 3.0};
//     sender.sendValue(values);
//     std::cout << "Sent value" << std::endl;
//   }
//   return 0;
// }
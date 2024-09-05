#include "networktables/NetworkTable.h"
#include "networktables/DoubleTopic.h"
#include "networktables/NetworkTableInstance.h"
#include "networktables/DoubleArrayTopic.h"

#include <string>
#include <iostream>

/*
Right now, the table is /SmartDashboard so we can visualize the values sent from the Orin.
Later, this should be changed to a new table, such as /Orin or /Vision, to avoid populating the SmartDashboard with unnecessary values
*/

// class DoubleValueSender{
//   private:
//     nt::NetworkTableInstance inst;
//     nt::DoublePublisher publisher;
//   public:
//     DoubleValueSender(std::string key){
//       inst = nt::NetworkTableInstance::GetDefault();
//       inst.SetServer("10.7.66.2");
//       inst.StartClient4("10.7.66.2");
//       auto table = inst.GetTable("/SmartDashboard");
//       nt::DoubleTopic topic = table->GetDoubleTopic(key);
      
//       publisher = topic.Publish();
//     }

//     void sendValue(double value){
//       publisher.Set(value);
//     }

//     void setDefaultValue(double value){
//       publisher.SetDefault(value);
//     }
  
// };

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

/*
*
* Example on how to use each class
*
*/

// int main(){
//   DoubleValueSender sender("NVIDIA ORIN TEST");
//   while(2>1){
//     sender.sendValue(1.0);
//     std::cout << "Sent value" << std::endl;
//   }
//   return 0;
// }

int main(){
  DoubleArraySender sender("NVIDIA ORIN ARRAY TEST");
  while(2>1){
    std::vector<double> values = {1.0, 2.0, 3.0};
    sender.sendValue(values);
    std::cout << "Sent value" << std::endl;
  }
  return 0;
}
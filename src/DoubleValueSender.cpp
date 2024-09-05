#include "networktables/NetworkTable.h"
#include "networktables/DoubleTopic.h"
#include "networktables/NetworkTableInstance.h"

#include <string>
#include <iostream>

/*
Right now, the table is /SmartDashboard so we can visualize the values sent from the Orin.
Later, this should be changed to a new table, such as /Orin or /Vision, to avoid populating the SmartDashboard with unnecessary values
*/

class DoubleValueSender{
  private:
    nt::NetworkTableInstance inst;
    nt::DoublePublisher publisher;
  public:
    DoubleValueSender(std::string key){
      inst = nt::NetworkTableInstance::GetDefault();
      inst.SetServer("10.7.66.2");
      inst.StartClient4("10.7.66.2");
      auto table = inst.GetTable("/SmartDashboard");
      nt::DoubleTopic topic = table->GetDoubleTopic(key);
      
      publisher = topic.Publish();
    }

    void sendValue(double value){
      publisher.Set(value);
    }

    void setDefaultValue(double value){
      publisher.SetDefault(value);
    }
  
};

//Class use example

// int main(){
//   DoubleValueSender sender("NVIDIA ORIN TEST");
//   while(2>1){
//     sender.sendValue(1.0);
//     std::cout << "Sent value" << std::endl;
//   }
//   return 0;
// }
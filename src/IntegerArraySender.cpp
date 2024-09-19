#include "IntegerArraySender.h"

/*
Right now, the table is /SmartDashboard so we can visualize the values sent from the Orin.
Later, this should be changed to a new table, such as /Orin or /Vision, to avoid populating the SmartDashboard with unnecessary values
*/


IntegerArraySender::IntegerArraySender(std::string key){
  inst_ = nt::NetworkTableInstance::GetDefault();
  inst_.SetServer("10.7.66.2");
  inst_.StartClient4("10.7.66.2");
  auto table = inst_.GetTable("/SmartDashboard");
  nt::IntegerArrayTopic topic = table->GetIntegerArrayTopic(key);
  publisher_ = topic.Publish();
}
void IntegerArraySender::sendValue(std::span<const int64_t> value){
  publisher_.Set(value);
}
void IntegerArraySender::setDefaultValue(std::span<const int64_t> value){
  publisher_.SetDefault(value);
}

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
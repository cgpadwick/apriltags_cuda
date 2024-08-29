#include "networktables/NetworkTable.h"
#include "networktables/DoubleTopic.h"
#include "networktables/NetworkTableInstance.h"

#include <string>

class ValueSender{
  private:
    nt::NetworkTableInstance inst;
  public:
    ValueSender(){
      inst = nt::NetworkTableInstance::GetDefault();
      inst.SetServerTeam(766);
    }

    void sendValue(std::string key, double value){
      auto table = inst.GetTable("SmartDashboard");
      table->PutNumber(key, value);
    }
  
};

int main(){
  ValueSender sender;
  while(2>1){
    sender.sendValue("test", 1.0);
    cout << "Sent value" << endl;
  }
  return 0;
}
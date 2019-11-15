

#include "timer.hpp"

namespace timer {
  
// timer functions definitions
// 1. calculate the expiration that will occur in 7 days
/*void starttimer(eosio::name usern){
  require_auth(usern);
  print("start the timer, ", name{usern})
    //user _user(_self, usern.value);    
    auto it = _user.find(usern.value);
    
    if(it == _user.end()){
      it = _user.emplace(usern, [&](auto& modified_user){
        modified_user.usern = usern;
        modified_user.timer = current_time_point();
        modified_user.expiration = current_time_point();
    });
  }
}*/

/*void expiration(eosio::name usern){
  require_auth(usern);
  static const uint32_t now = current_time_point().sec_since_epoch();
  static const uint32_t r = now % hours(24).to_seconds();
  static const time_point_sec expire_date = (time_point_sec)(now - r + (7 * hours(24).to_seconds()));

  // start the clock
  auto it = _user.find(usern.value);
     _user.modify(it, get_self(), [&]( auto& modified_user){
        modified_user.expiration = expire_date;
  });
}*/


/*void elapsedtime(eosio::name usern){
    require_auth(usern);
  
 
  auto itr = _user.find(usern.value);
  
  if(itr->expiration > current_time_point()){
    print("the deadline has passed");
    // reset the clock
   
  }else{
    print("the deadline has not yet passed");
    // missed payments are accummulated
  }
}*/


  
  // fetch the time of the clock
  eosio::time_point_sec feeclock::get_time(){
    return clockstart;
  }
  
  // set the time of the clock
  void feeclock::set_time(eosio::time_point _time){
    uint32_t now = _time;  // _time == current_time_point().sec_since_epoch();
    clockstart = (time_point_sec)(_time);
  }
  
  // function that gets the start time of the late-pay peroid
  eosio::time_point_sec startclock(feeclock const& clock_){
    return clock.get_time();
  }
  
  // function that gets the expiry date of the late-pay time
  eosio::time_point_sec expirydate(feeclock const& clock_){
    //static const uint32_t now = current_time_point().sec_since_epoch();
    static const uint32_t now = _clock.get_time();
    static const uint32_t r = now % hours(24).to_seconds();
    static const time_point_sec expire_date = (time_point_sec)(now - r + (7 * hours(24).to_seconds()));
    return expiry_date;
  }
  
  eosio::time_point_sec elapsedtime(feeclock const& clock_){}
  
  void resetclock(feeclock const& clock_, eosio::time_point_sec default_time){
    clock_.set_time(default_time);
  }
  
  
}



























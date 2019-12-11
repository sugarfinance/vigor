#pragma once

using namespace eosio;

namespace timer {

  // default value for the clock
  eosio::time_point_sec DEFAULT_TIME = (eosio::time_point_sec)(eosio::current_time_point().sec_since_epoch() - eosio::current_time_point().sec_since_epoch());

  
  
  struct feeclock{
    
    public:
    //default ctor 
    feeclock(){}
    
    // ctor with initialiser list
    feeclock(eosio::time_point_sec timemeasure, eosio::time_point_sec expiry): 
     startclock(timemeasure)
    ,clockexpiry(expiry){}
    
    
    // getter
    eosio::time_point_sec get_time();
    // setter
    //void set_time(eosio::time_point_sec _time);
    void set_time();
    
  private:
    // data members to be used for the timer methods
    uint8_t timerstate;
    eosio::time_point_sec startclock;        
    eosio::time_point_sec clockexpiry;
    
    EOSLIB_SERIALIZE(feeclock, (timerstate)(startclock)(clockexpiry));
    
  };
  
  // timer functions definitions - using the interface princple
  // can use templates
  eosio::time_point_sec startclock(feeclock const& clock_);
  eosio::time_point_sec expirydate(feeclock const& clock_);
  eosio::time_point_sec elapsedtime(feeclock const& clock_);
  eosio::time_point_sec resetclock(feeclock const& clock_);

 
};


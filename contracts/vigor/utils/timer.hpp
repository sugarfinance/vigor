#pragma once

//using namespace eosio;

namespace timer {

  // default value for the clock
  eosio::time_point_sec DEFAULT_TIME = (eosio::time_point_sec)(eosio::current_time_point().sec_since_epoch() - eosio::current_time_point().sec_since_epoch());

 
};


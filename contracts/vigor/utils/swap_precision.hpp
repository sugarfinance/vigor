/*
*   IK Nwoke 
*   This is a function that swaps the asset precision from 10 decimal points to 4 decimals or vice-versa
*
*/


#pragma once


namespace swap_precision {

  // default value for the clock
  eosio::time_point_sec DEFAULT_TIME = (eosio::time_point_sec)(eosio::current_time_point().sec_since_epoch() - eosio::current_time_point().sec_since_epoch());
    
  
  // the swap function
  auto swapprecision(eosio::asset asset_){
    // getting the symbol of the assetin token
    auto _sym = asset_.symbol.code();

    eosio::asset amt;
    
    // find out the precision of asset amount
    if(asset_.symbol.precision() < 10)
    {
        int n = asset_.symbol.precision();
        
        // --- create an asset with a precision of 10
        eosio::asset amt10 = eosio::asset(0, eosio::symbol(_sym, 10));
        
        // converting the asset of precision n to precision 10
        // assiging the value of the asset amount to the internal asset pf precision 10
        amt10.amount = asset_.amount;
        
        // calculate the difference between 10 and n
        amt10.amount = amt10.amount * std::pow(10.0, (10 - n));
        
        amt = amt10;
    
    }else if(asset_.symbol.precision() == 10){

        int n = 4;

        // --- create an asset with a precision of 4
        eosio::asset amt4 = eosio::asset(0, eosio::symbol(_sym, 4));
        
        // converting the asset of precision 10 to precision 4
        // assiging the value of the asset amount to the internal asset pf precision 10
        amt4.amount = asset_.amount;
        
        // calculate the difference between 10 and n
        amt4.amount = amt4.amount / std::pow(10.0, (10 - n));
        
        amt = amt4;
    }
    return amt;
  }
  
};


/*
*   IK Nwoke 
*   This is a function that swaps the asset precision from 10 decimal points to 4 decimals or vice-versa
*
*/


#pragma once


namespace swap_precision {
  
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
        // --- create an asset with a precision of 4
        eosio::asset amt4 = eosio::asset(0, eosio::symbol(_sym, 4));
        
        // --- assign the asset amt4 the value ot truncated amt10
        // --- this truncates asset10 to asset4
        amt4.amount = int64_t(std::round(asset_.amount));
        
        amt = amt4;
    }
    return amt;
  }
  
};


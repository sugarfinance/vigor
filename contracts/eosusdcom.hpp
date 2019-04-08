#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

using namespace std;
using namespace eosio;

namespace eosiosystem {
   class system_contract;
}

CONTRACT eosusdcom : public eosio::contract {

private:

    TABLE user_s{
      name usern;
      vector<asset> collateral;
      vector<asset> insurance;
      double valueofcol = 0.0;
      double valueofins = 0.0;
      asset debt;
      double tesprice = 0.0;
      
      /* measured by how much VIG was paid in the past
       * relative to number of late payments and collections
      */ uint64_t creditscore = 500; //out of 800
      
      double feespaid = 0.0;
      uint64_t recaps = 0;
      uint64_t latepayments = 0;
      
      /* Own Funds = amount of crypto collateral 
       * pledged by insurers minus our best estimate
       * normal market value of the TES contracts.
      */

      auto primary_key() const { return usern.value; }
    };

   typedef eosio::multi_index<name("user"), user_s> user_t;
   user_t _user;

   TABLE swap_s {
      name buyer;
      name seller;
      double protection; // the cost to bail-out an under-collateralized loan
      bool triggered; // value of collateral falling below the value of debt
      
      time nextPayment; // buyer makes a periodic premium payment at this timestamp
      time created; 
   } typedef eosio::multi_index<name("swap"), swap_s> swap_t;
   swap_t _swap;   
   
   TABLE eosusd {
    uint64_t id;
    name owner; 
    uint64_t value;
    uint64_t average;
    uint64_t timestamp;
    
    uint64_t primary_key() const {return id;}
    uint64_t by_timestamp() const {return timestamp;}
    uint64_t by_value() const {return value;}

    EOSLIB_SERIALIZE( eosusd, (id)(owner)(value)(average)(timestamp))
  };

    typedef eosio::multi_index<name("eosusd"), eosusd,
      indexed_by<name("value"), const_mem_fun<eosusd, uint64_t, &eosusd::by_value>>, 
      indexed_by<name("timestamp"), const_mem_fun<eosusd, uint64_t, &eosusd::by_timestamp>>> usdtable;
    usdtable _eosusd;

    double scale = 1.0;

    TABLE account {
       asset    balance;
       uint64_t primary_key()const { return balance.symbol.code().raw(); }
    };

    TABLE currency_stats {
       asset    supply;
       asset    max_supply;
       name     issuer;
       uint64_t solvency;
       uint64_t volatility; // stdev, scale factor for price discovery
       /*
       * Coefficients of correlation between this asset and all other
       * assets tracked by the contract
       */ map <symbol, double> correlation_matrix;

       uint64_t primary_key()const { return supply.symbol.code().raw(); }
    };

    typedef eosio::multi_index< "accounts"_n, account > accounts;
    typedef eosio::multi_index< "stat"_n, currency_stats > stats;

    void sub_balance( name owner, asset value );
    void add_balance( name owner, asset value, name ram_payer );

   map <symbol, double> dummy_corr {
   {symbol("SYS",4), 0.42},
   {symbol("VIG",4), 0.42},
   {symbol("IQ",4), 0.42},
   {symbol("UTG",4), 0.42},
   {symbol("PTI",4), 0.42},
   {symbol("OWN",4),	0.42},
   {symbol("EOS",4),	0,42}
   };

    map <symbol, name> issueracct {
    {symbol("SYS",4),	    name("eosio.token")},
    {symbol("VIG",4),	    name("vig111111111")},
    {symbol("IQ",4),	       name("dummytokens1")},
    {symbol("UTG",4),	    name("dummytokens1")},
    {symbol("PTI",4),	    name("dummytokens1")},
    {symbol("OWN",4),	    name("dummytokens1")},
    {symbol("EOS",4),	    name("eosio.token")}
    };

    map <symbol, uint64_t> fxrate {
    {symbol("SYS",4),	    54000},
    {symbol("VIG",4),	    200},
    {symbol("IQ",4),	       39},
    {symbol("UTG",4),	    2},
    {symbol("PTI",4),	    63},
    {symbol("OWN",4),	    198},
    {symbol("EOS",4),	    54000}
    };

public:

    using contract::contract;

    eosusdcom(name receiver, name code, datastream<const char*> ds):contract(receiver, code, ds),_user(receiver, receiver.value),_eosusd(receiver, receiver.value){}
    float dollar_conversion = 3.51; // from oracle
   
    //ACTION deleteuser(name user);
    ACTION assetin(  name       from,
                      name       to,
                      asset      assetin,
                      string     memo);
    ACTION assetout(name usern, asset assetout, string memo);
    //ACTION cleardebt(name usern, asset debt);
    ACTION borrow(name usern, asset debt);
  //  ACTION recap(name user);
 //   void payfee(name user, double tesprice);
    void update(name usern);

    //double pricingmodel(double scale, double collateral, asset debt, double stdev, uint64_t creditscore);
    double pricingmodel(name usern);

    ACTION doupdate();

    ACTION create( name   issuer,
                 asset  maximum_supply);
    
    ACTION issue( name to, asset quantity, string memo );

    ACTION retire( asset quantity, string memo );
    
    ACTION transfer( name    from,
                     name    to,
                     asset   quantity,
                     string  memo );
    
    ACTION open( name owner, const symbol& symbol, name ram_payer );
    
    ACTION close( name owner, const symbol& symbol );

    ACTION setsupply( name issuer, asset maximum_supply );
    
    static asset get_supply( name token_contract_account, symbol_code sym_code )
    {
       stats statstable( token_contract_account, sym_code.raw() );
       const auto& st = statstable.get( sym_code.raw() );
       return st.supply;
    }
    
    static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
    {
       accounts accountstable( token_contract_account, owner.value );
       const auto& ac = accountstable.get( sym_code.raw() );
       return ac.balance;
    }
};
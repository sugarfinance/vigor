#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

using namespace std;
using namespace eosio;

namespace eosiosystem {
   class system_contract;
}

CONTRACT vigor : public eosio::contract {

private:

    TABLE user_s{
      name usern;
      vector<asset> collateral;
      vector<asset> insurance;
      double valueofcol = 0.0;
      double valueofins = 0.0;
      asset debt;
      double tesprice = 0.0;
      uint64_t creditscore = 500; //out of 800
      double feespaid = 0.0;
      uint64_t recaps = 0;
      uint64_t latepayments = 0;
      
      auto primary_key() const { return usern.value; }
    };

    typedef eosio::multi_index<name("user"), user_s> user_t;
    user_t _user;

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
       uint64_t primary_key()const { return supply.symbol.code().raw(); }
    };

    typedef eosio::multi_index< "accounts"_n, account > accounts;
    typedef eosio::multi_index< "stat"_n, currency_stats > stats;

    void sub_balance( name owner, asset value );
    void add_balance( name owner, asset value, name ram_payer );

    map <symbol, name> issueracct {
    {symbol("SYS",4),	    name("eosio.token")},
    {symbol("EOS",4),	    name("eosio.token")},
    {symbol("BLACK",4),	    name("eosblackteam")},
    {symbol("IQ",3),	    name("everipediaiq")},
    {symbol("DICE",4),	    name("betdicetoken")},
    {symbol("PGL",4),	    name("prospectorsg")},
    {symbol("TPT",4),	    name("eosiotptoken")},
    {symbol("PTI",4),	    name("ptitokenhome")},
    {symbol("CET",4),	    name("eosiochaince")},
    {symbol("SEED",4),	    name("parslseed123")},
    {symbol("OCT",4),	    name("octtothemoon")},
    {symbol("MEETONE",4),	name("eosiomeetone")},
    {symbol("EOSDAC",4),	name("eosdactokens")},
    {symbol("HORUS",4),	    name("horustokenio")},
    {symbol("KARMA",4),	    name("therealkarma")},
    {symbol("INF",4),	    name("infinicoinio")},
    {symbol("TRYBE",4),	    name("trybenetwork")},
    {symbol("CAN",4),	    name("eoscancancan")},
    {symbol("POKER",4),	    name("eospokercoin")},
    {symbol("PSI",4),	    name("psidicetoken")},
    {symbol("ARN",8),	    name("aeronaerozzz")},
    {symbol("EGT",4),	    name("eosiotokener")},
    {symbol("EPRA",4),	    name("epraofficial")},
    {symbol("ECTT",4),	    name("ectchaincoin")},
    {symbol("LKT",4),	    name("chyyshayysha")},
    {symbol("EATCOIN",4),	name("eatscience14")},
    {symbol("LLG",4),	    name("llgonebtotal")},
    {symbol("GT",4),	    name("eosgetgtoken")},
    {symbol("ADD",4),	    name("eosadddddddd")}
    };

    map <symbol, uint64_t> fxrate {
    {symbol("SYS",4),	    54000},
    {symbol("EOS",4),	    54000},
    {symbol("BLACK",4),	  1},
    {symbol("IQ",3),	    1},
    {symbol("DICE",4),	    1},
    {symbol("PGL",4),	    1},
    {symbol("TPT",4),	    1},
    {symbol("PTI",4),	    1},
    {symbol("CET",4),	    1},
    {symbol("SEED",4),	    1},
    {symbol("OCT",4),	    1},
    {symbol("MEETONE",4),	1},
    {symbol("EOSDAC",4),	1},
    {symbol("HORUS",4),	    1},
    {symbol("KARMA",4),	    1},
    {symbol("INF",4),	    1},
    {symbol("TRYBE",4),	    1},
    {symbol("CAN",4),	    1},
    {symbol("POKER",4),	    1},
    {symbol("PSI",4),	    1},
    {symbol("ARN",8),	    1},
    {symbol("EGT",4),	    1},
    {symbol("EPRA",4),	    1},
    {symbol("ECTT",4),	    1},
    {symbol("LKT",4),	    1},
    {symbol("EATCOIN",4),	1},
    {symbol("LLG",4),	    1},
    {symbol("GT",4),	    1},
    {symbol("ADD",4),	    1}
    };

public:

    using contract::contract;

    vigor(name receiver, name code, datastream<const char*> ds):contract(receiver, code, ds),_user(receiver, receiver.value),_eosusd(receiver, receiver.value){}
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

    double pricingmodel(double scale, double collateral, asset debt, double stdev, uint64_t creditscore);

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
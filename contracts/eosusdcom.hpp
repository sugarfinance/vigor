#pragma once

#include <eosiolib/transaction.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>
#include <cmath>

using namespace std;
using namespace eosio;

namespace eosiosystem {
   class system_contract;
}

CONTRACT eosusdcom : public eosio::contract {

   private:
      TABLE user_s {
         name usern;
         asset debt;

         vector<asset> collateral;
         vector<asset> support;

         double valueofcol = 0.0;
         double valueofins = 0.0;

         double tesvalue = 0.0;
         double tesprice = 0.0;
         double iportVaR = 0.0; //  (1 - portVaR_COL ) * COL
         double feespaid = 0.0;
      /* measured by how much VIG was paid in the past
       * relative to number of late payments and collections
      */ uint64_t creditscore = 500; //out of 800
         uint64_t lastupdate = 0;
         uint64_t latepays = 0;
         uint64_t recaps = 0;
         
         /* Own Funds = amount of crypto collateral 
         * pledged by supporters minus our best estimate
         * normal market value of the TES contracts.
         */
         auto primary_key() const { return usern.value; }

         //EOSLIB_SERIALIZE(user_s, (usern)(debt)(collateral)(support)(valueofcol)(valueofins)(tesvalue)(tesprice)(iportVaR)(feespaid)(creditscore)(lastupdate)(latepays)(recaps))
      }; typedef eosio::multi_index<name("user"), user_s> user_t;
                                                          user_t _user;
      TABLE eosusd {
         uint64_t id;
         name owner; 
         uint64_t value;
         uint64_t average;
         uint64_t timestamp;
      
         uint64_t primary_key() const { return id; }
         uint64_t by_timestamp() const { return timestamp; }
         uint64_t by_value() const { return value; }

         EOSLIB_SERIALIZE( eosusd, (id)(owner)(value)(average)(timestamp))
      }; typedef eosio::multi_index<name("eosusd"), eosusd,
                        indexed_by<name("value"), const_mem_fun<eosusd, uint64_t, &eosusd::by_value>>, 
                        indexed_by<name("timestamp"), const_mem_fun<eosusd, uint64_t, &eosusd::by_timestamp>>> usdtable;
                                                                                                               usdtable _eosusd;
      TABLE globalstats {
         double solvency = 0.0;
         double valueofcol = 0.0;
         double valueofins = 0.0;

         double scale = 1.0;
         double iportVaRcol = 0.0; // SUM_i [ (1 - portVaR_COLi ) * COLi ]
         double iportVaRins = 0.0; // [ (1 - portVaR_INS ) * INS ]

         map <symbol, uint64_t> fxrate = { 
            { symbol("SYS", 4), 54000 },
            { symbol("EOS", 4), 54000 },
            { symbol("VIG", 4), 200 },
            { symbol("OWN", 4), 198 },
            { symbol("PTI", 4), 63 },
            { symbol("IQ", 4), 39 },
            { symbol("UTG", 4), 2 }
         };
         uint64_t inreserve = 0; // vig
         uint64_t totaldebt = 0; // uzd
         
         vector<asset> support;
         vector<asset> collateral;
   
         //EOSLIB_SERIALIZE(globalstats, (solvency)(valueofcol)(valueofins)(scale)(iportVaRcol)(iportVaRins)(fxrate)(inreserve)(totaldebt)(support)(collateral))
      }; typedef eosio::singleton<"globals"_n, globalstats> globals;
         typedef eosio::multi_index<"globals"_n, globalstats> globalsm;
                                                              globals globalstab;
                                                              globalsm globalstabm;
      void calcStats();
      void update(name usern); 
      void payfee(name usern);
      void bailout(name usern);    
      void pricingmodel(name usern);

      map <symbol, name> issueracct {
         {symbol("SYS",4),	    name("eosio.token")},
         {symbol("VIG",4),	    name("vig111111111")},
         {symbol("IQ",4),	    name("dummytokens1")},
         {symbol("UTG",4),	    name("dummytokens1")},
         {symbol("PTI",4),	    name("dummytokens1")},
         {symbol("OWN",4),	    name("dummytokens1")},
         {symbol("EOS",4),	    name("eosio.token")}
      };

      TABLE account {
         asset    balance;
         uint64_t primary_key()const { return balance.symbol.code().raw(); }
      }; typedef eosio::multi_index< "accounts"_n, account > accounts;

      TABLE currency_stats {
         asset    supply;
         asset    max_supply;
         name     issuer;
         /*
         * Coefficients of correlation between this asset and all other
         * assets tracked by the contract
         */
         map <symbol, double> correlation_matrix {
            {symbol("SYS",4), 0.42},
            {symbol("VIG",4), 0.42},
            {symbol("IQ",4), 0.42},
            {symbol("UTG",4), 0.42},
            {symbol("PTI",4), 0.42},
            {symbol("OWN",4),	0.42},
            {symbol("EOS",4),	0.42}
         };
         double volatility = 0.1; // stdev, scale factor for price discovery

         uint64_t primary_key()const { return supply.symbol.code().raw(); }

         //EOSLIB_SERIALIZE(currency_stats, (supply)(max_supply)(issuer)(correlation_matrix)(volatility))
      }; typedef eosio::multi_index< "stat"_n, currency_stats > stats;
                                                                stats _stats;

      void sub_balance( name owner, asset value );
      void add_balance( name owner, asset value, name ram_payer );

   public:
      using contract::contract;
      eosusdcom(name receiver, name code, datastream<const char*> ds):contract(receiver, code, ds), 
      _user(receiver, receiver.value), _stats(receiver, receiver.value), _eosusd(receiver, receiver.value),
      globalstab(receiver, receiver.value), globalstabm(receiver, receiver.value) {}

      //ACTION deleteuser(name user);
      ACTION assetin( name   from,
                     name   to,
                     asset  assetin,
                     string memo);
      ACTION assetout(name usern, asset assetout, string memo);

      ACTION doupdate(bool up);

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
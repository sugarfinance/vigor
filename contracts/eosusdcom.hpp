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

         double valueofcol = 0.0; // dollar value of user collateral portfolio
         double valueofins = 0.0; // dollar value of user support portfolio

         double tesprice = 0.0; // annualized rate borrowers pay in periodic premiums to insure their collateral
         double tesvalue = 0.0;  // dollar value for borrowers to insure their collateral
         double volcol = 1.0; // volatility of the user collateral portfolio
         double stresscol = 0.0; // model suggested percentage loss that the user collateral portfolio would experience in a stress event.
         double istresscol = 0.0; // market determined implied percentage loss that the user collateral portfolio would experience in a stress event.
         double svalueofcol = 0.0; // model suggested dollar value of the user collateral portfolio in a stress event.
         double svalueofcole = 0.0; // model suggested dollar amount of insufficient collateral of a user loan in a stressed market.   min((1 - svalueofcol ) * valueofcol - debt,0) 
         double feespaid = 0.0;
      /* measured by how much VIG was paid in the past
       * relative to number of late payments and collections
      */ uint64_t creditscore = 500; //out of 800
         uint32_t lastupdate = 0;
         uint32_t latepays = 0;
         uint32_t recaps = 0;
         
         /* Own Funds = amount of crypto collateral 
         * pledged by supporters minus our best estimate
         * normal market value of the TES contracts.
         */
         auto primary_key() const { return usern.value; }

         EOSLIB_SERIALIZE(user_s, (usern)(debt)(collateral)(support)(valueofcol)(valueofins)(tesvalue)(tesprice)(volcol)(stresscol)(istresscol)(svalueofcol)(svalueofcole)(feespaid)(creditscore)(lastupdate)(latepays)(recaps))
      }; typedef eosio::multi_index<name("user"), user_s> user_t;
                                                          user_t _user;

  //Holds the last datapoints_count datapoints from qualified oracles
      struct [[eosio::table]] datapoints {
         uint64_t id;
         name owner; 
         uint64_t value;
         uint64_t median;
         uint64_t timestamp;
   
         uint64_t primary_key() const {return id;}
         uint64_t by_timestamp() const {return timestamp;}
         uint64_t by_value() const {return value;}

         EOSLIB_SERIALIZE( datapoints, (id)(owner)(value)(median)(timestamp))
      };

      typedef eosio::multi_index<name("datapoints"), datapoints,
         indexed_by<name("value"), const_mem_fun<datapoints, uint64_t, &datapoints::by_value>>, 
         indexed_by<name("timestamp"), const_mem_fun<datapoints, uint64_t, &datapoints::by_timestamp>>> datapointstable;
                                                                                                        datapointstable _datapointstable;

      TABLE globalstats {
         double solvency = 1.0;
         double valueofcol = 0.0; // dollar value of total collateral portfolio
         double valueofins = 0.0; // dollar value of total support portfolio

         double scale = 1.0; // TES pricing model parameters are scaled to drive risk (solvency) to a target set by custodians.
         double tesvalue = 0.0; // dollar value for borrowers to insure their collateral
         double svalueofcole = 0.0; // model suggested dollar value of the sum of all insufficient collateral in a stressed market SUM_i [ min((1 - svalueofcoli ) * valueofcoli - debti,0) ]
         double svalueofins = 0.0; // model suggested dollar value of the total insurance asset portfolio in a stress event. [ (1 - stressins ) * INS ]
         double stressins = 0.0; // model suggested percentage loss that the total insurance asset portfolio would experience in a stress event.

  /*        map <symbol, uint64_t> fxrate = { 
            { symbol("EOS", 4), 61000 },
            { symbol("VIG", 4), 200 },
            { symbol("IQ", 3), 42 },
            { symbol("PEOS", 4), 661 },
            { symbol("DICE", 4), 12 },
            { symbol("TPT", 4), 30 }
         };
         */
         uint64_t inreserve = 0; // vig
         asset totaldebt = asset( 0, symbol("VIGOR", 4) ); // VIGOR
         
         vector<asset> support;
         vector<asset> collateral;

   
         EOSLIB_SERIALIZE(globalstats, (solvency)(valueofcol)(valueofins)(scale)(tesvalue)(svalueofcole)(svalueofins)(stressins)(inreserve)(totaldebt)(support)(collateral))
      }; typedef eosio::multi_index<name("globals"), globalstats> globalsm; 
         typedef eosio::singleton<name("globals"), globalstats> globals;
                                                            globals _globals;
      void risk();
      double riskx(name usern);
      void stresscol(name usern);
      void stressins();
      double stressinsx(name usern);
      double portVarianceCol(name usern);
      void update(name usern);
      void payfee(name usern);
      void bailout(name usern);
      void pricing(name usern);
      double pcts(name usern, double RM);
      double RM();

      map <symbol, name> issueracct {
         {symbol("EOS",4),	    name("eosio.token")},
         {symbol("VIG",4),	    name("vig111111111")},
         {symbol("IQ",3),	    name("dummytokens1")},
         {symbol("PEOS",4),	    name("dummytokens1")},
         {symbol("DICE",4),	    name("dummytokens1")},
         {symbol("TPT",4),	    name("dummytokens1")},
      };

      map <symbol, name> issuerfeed {
         {symbol("EOS",4),	    name("eosusd")},
         {symbol("VIG",4),	    name("vigeos")},
         {symbol("IQ",3),	    name("iqeos")},
         {symbol("PEOS",4),	    name("peoseos")},
         {symbol("DICE",4),	    name("diceeos")},
         {symbol("TPT",4),	    name("tpteos")},
      };

      const uint64_t one_minute = 1000000.0 * 60.0; 
      const uint64_t five_minute = 1000000.0 * 60.0 * 5.0;
      const uint64_t fifteen_minute = 1000000.0 * 60.0 * 15.0;
      const uint64_t one_hour = 1000000.0 * 60.0 * 60.0;
      const uint64_t four_hour = 1000000.0 * 60.0 * 60.0 * 4.0; 
      const uint64_t one_day = 1000000.0 * 60.0 * 60.0 * 24.0;
      const double volPrecision = 1000000;
      const double corrPrecision = 1000000;
      const double pricePrecision = 1000000;
      double stressQuantile = 1.65;
      double solvencyTarget = 1.0;


      TABLE account {
         asset    balance;
         uint64_t primary_key()const { return balance.symbol.code().raw(); }
      }; typedef eosio::multi_index< name("accounts"), account > accounts;

      TABLE currency_stats {
         asset    supply;
         asset    max_supply;
         name     issuer;
         /*
         * Coefficients of correlation between this asset and all other
         * assets tracked by the contract
         */
         map <symbol, double> correlation_matrix {
            {symbol("EOS",4),0.42},
            {symbol("VIG",4), 0.42},
            {symbol("IQ",3), 0.42},
            {symbol("PEOS",4), 0.42},
            {symbol("DICE",4), 0.42},
            {symbol("TPT",4), 0.42},
        };
         double volatility = 0.1; // stdev, scale factor for price discovery

         uint64_t primary_key()const { return supply.symbol.code().raw(); }

         EOSLIB_SERIALIZE(currency_stats, (supply)(max_supply)(issuer)(correlation_matrix)(volatility))
      }; typedef eosio::multi_index< name("stat"), currency_stats > stats;
                                                                stats _stats;

      //From datepreproc, holds the time series of prices, returns, volatility and correlation
      TABLE statspre {
         uint64_t freq;
         uint64_t timestamp;
         std::deque<uint64_t> price;
         std::deque<int64_t> returns;
         std::map <symbol, int64_t> correlation_matrix;
         std::uint64_t vol = 1000;

         uint64_t primary_key() const {return freq;}

      };

      typedef eosio::multi_index<name("stats"), statspre> statstable;
                                                         statstable _statstable;

      void sub_balance( name owner, asset value );
      void add_balance( name owner, asset value, name ram_payer );

   public:
      using contract::contract;
      eosusdcom(name receiver, name code, datastream<const char*> ds):contract(receiver, code, ds), 
      _user(receiver, receiver.value), _datapointstable(receiver, receiver.value),
      _stats(receiver, receiver.value), _globals(receiver, receiver.value),
      _statstable(receiver, receiver.value)  {}
     
      //ACTION deleteuser(name user);
      ACTION assetin( name   from,
                     name   to,
                     asset  assetin,
                     string memo);
      ACTION assetout(name usern, asset assetout, string memo);

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
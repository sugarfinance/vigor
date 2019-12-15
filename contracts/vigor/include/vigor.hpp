#include <eosio/transaction.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include <string>
#include <cmath>
#include <utility>  // using std::pair

#include "../utils/rng.hpp"
#include "../utils/swap_precision.hpp"
#include "../utils/timer.hpp"
#include "../utils/payfeefsm.hpp"


using namespace std;
using namespace timer;
using namespace eosio;

namespace eosiosystem {
   class system_contract;
}

      const uint32_t one_minute = 60; 
      const uint32_t five_minute = 60 * 5;
      const uint32_t fifteen_minute = 60 * 15;
      const uint32_t one_hour = 60 * 60;
      const uint32_t four_hour = 60 * 60 * 4; 
      const uint32_t one_day = 60 * 60 * 24; 
      const double volPrecision = 1000000;
      const double corrPrecision = 1000000;
      const double pricePrecision = 1000000;
      const uint64_t defaultVol = 600000;
      
      inline uint128_t usernticker(name usern, symbol_code ticker) {
      return (((uint128_t)usern.value)<<64) | ticker.raw();
      }

CONTRACT vigor : public eosio::contract {

   private:

        TABLE collateral_s {
          uint64_t         id;
          name             usern;
          symbol_code      ticker;
          int64_t         amount;
          uint64_t  primary_key()const { return id; }
          uint128_t  get_usernticker() const { return usernticker(usern,ticker); }
         
         EOSLIB_SERIALIZE(collateral_s, (id)(usern)(ticker)(amount))
        };
         typedef eosio::multi_index<name("collateral"), collateral_s,
          indexed_by<name("usernticker"), const_mem_fun<collateral_s, uint128_t, &collateral_s::get_usernticker>>
          > collateral_t;
            collateral_t _collateral;


      TABLE user_s {
         name usern;
         asset debt = asset(0, symbol("VIGOR", 4));

         vector<asset> collateral;
         vector<asset> insurance;

         double valueofcol = 0.0; // dollar value of user portfolio of collateral crypto assets
         double valueofins = 0.0; // dollar value of user portfolio of insurance crypto assets

         double tesprice = 0.0; // annualized rate borrowers pay in periodic premiums to insure their collateral
         double earnrate = 0.0; // annualized rate of return on user portfolio of insurance crypto assets
         double pcts = 0.0; // percent contribution to solvency (weighted marginal contribution to risk (solvency) rescaled by sum of that
         double volcol = 1.0; // volatility of the user collateral portfolio
         double stresscol = 0.0; // model suggested percentage loss that the user collateral portfolio would experience in a stress event.
         double istresscol = 0.0; // market determined implied percentage loss that the user collateral portfolio would experience in a stress event.
         double svalueofcol = 0.0; // model suggested dollar value of the user collateral portfolio in a stress event.
         double svalueofcole = 0.0; // model suggested dollar amount of insufficient collateral of a user loan in a stressed market.   min((1 - svalueofcol ) * valueofcol - debt,0) 
         double svalueofcoleavg = 0.0; // model suggested dollar amount of insufficient collateral of a user loan on average in down markets, expected loss
         double premiums = 0.0; // dollar amount of premiums borrowers would pay in one year to insure their collateral
         asset feespaid = asset(0, symbol("VIG", 10)); //VIG
         asset totallatepay = asset(0, symbol("VIG", 10)); // VIG with precision 10
         uint64_t creditscore = 500; //out of 800
         time_point lastupdate = time_point(microseconds(0));;
         uint32_t latepays = 0;
         uint32_t recaps = 0;

         asset l_debt = asset( 0, symbol("VIGOR", 4) ); // VIGOR stablecoin locked as collateral

         vector<asset> l_collateral; //EOSIO native tokens borrowed
         vector<asset> l_lrtoken;
         vector<asset> l_lrpayment;
         vector<name> l_lrname;

         double l_valueofcol = 0.0; // dollar value of user portfolio of collateral crypto assets

         double l_tesprice = 0.0; // annualized rate borrowers pay in periodic premiums to insure their collateral
         double l_earnrate; // annualized rate of return on user portfolio of insurance crypto assets
         double l_pcts = 0.0; // percent contribution to solvency (weighted marginal contribution to risk (solvency) rescaled by sum of that
         double l_volcol = 1.0; // volatility of the user collateral portfolio
         double l_stresscol = 0.0; // model suggested percentage loss that the user collateral portfolio would experience in a stress event.
         double l_istresscol = 0.0; // market determined implied percentage loss that the user collateral portfolio would experience in a stress event.
         double l_svalueofcol = 0.0; // model suggested dollar value of the user collateral portfolio in a stress event.
         double l_svalueofcole = 0.0; // model suggested dollar amount of insufficient collateral of a user loan in a stressed market.   min((1 - svalueofcol ) * valueofcol - debt,0) 
         double l_svalueofcoleavg = 0.0; // model suggested dollar amount of insufficient collateral of a user loan on average in down markets, expected loss
         double l_premiums = 0.0; // dollar amount of premiums borrowers would pay in one year to insure their collateral

         uint32_t l_latepays = 0;
         uint32_t l_recaps = 0;
         
         // data members to be used for the timer methods
         eosio::time_point_sec starttime;
         eosio::time_point_sec expiry_time;
         
         // a variable that counts the number of days that payments have been missed
         int elapsed_days = 0;
         
         // late pays gets accumulated here
         //asset accumulatepays = asset(0, symbol("VIG", 10));
        

         // nomenclature note: 
         // this contract has two major features that are mirror images of each other:
         // 1 token repo: (borrow VIGOR stablecoin against collateral (EOS tokens)
         // 2 token lend: (borrow EOS tokens against collateral (VIGOR tablecoin)
         // 'collateral' or 'debt' to one user could mean stablecoin, but to another user could mean EOS tokens
         // so prefix l_ is introduced to implement feature 2 by re-rusing the same code written initially for feature 1
         // any variable with prefix l_ is for 2 token lend. prefix l_ indicates 'reverse' (swap the word debt <-> collateral) in your mind
         // examples:
         // consider a token repo: when user locks EOS to borrow VIGOR, the VIGOR borrow is stored in debt and EOS is collateral
         // consider a token lend: when user locks VIGOR to borrow EOS, the VIGOR is stored in l_debt and EOS borrow is l_collateral
         // the variable 'debt' means user is borrowing VIGOR in a token repo
         // the variable "l_debt" means users is locking VIGOR as collateral in a token lend
         // the variable 'collateral' means user is locking EOS as collateral in a token repo
         // the vriable "l_collateral" means user is borrowing EOS in a token lend
         
         auto primary_key() const { return usern.value; }

         EOSLIB_SERIALIZE(user_s, (usern)(debt)(collateral)(insurance)(valueofcol)(valueofins)(tesprice)(earnrate)(pcts)(volcol)(stresscol)(istresscol)(svalueofcol)(svalueofcole)(svalueofcoleavg)(premiums)(feespaid)(totallatepay)(creditscore)(lastupdate)(latepays)(recaps)(l_debt)(l_collateral)(l_lrtoken)(l_lrpayment)(l_lrname)(l_valueofcol)(l_tesprice)(l_earnrate)(l_pcts)(l_volcol)(l_stresscol)(l_istresscol)(l_svalueofcol)(l_svalueofcole)(l_svalueofcoleavg)(l_premiums)(l_latepays)(l_recaps)(starttime)(expiry_time)(elapsed_days))
      }; typedef eosio::multi_index<name("user"), user_s> user_t;
                                                          user_t _user;

      TABLE globalstats {
         double solvency = 1.0; // solvency, represents capital adequacy to back the stablecoin
         double valueofcol = 0.0; // dollar value of total portfolio of borrowers crypto collateral assets
         double valueofins = 0.0; // dollar value of total portfolio of insurance crypto assets
         double scale = 1.0; // TES pricing model parameters are scaled to drive risk (solvency) to a target set by custodians.
         double svalueofcole = 0.0; // model suggested dollar value of the sum of all insufficient collateral in a stressed market SUM_i [ min((1 - svalueofcoli ) * valueofcoli - debti,0) ]
         double svalueofins = 0.0; // model suggested dollar value of the total insurance asset portfolio in a stress event. [ (1 - stressins ) * INS ]
         double stressins = 0.0; // model suggested percentage loss that the total insurance asset portfolio would experience in a stress event.
         double svalueofcoleavg = 0.0; // model suggested dollar value of the sum of all insufficient collateral on average in down markets, expected loss
         double svalueofinsavg = 0.0; // model suggested dollar value of the total insurance asset portfolio on average in down market
         double raroc = 0.0; // RAROC risk adjusted return on capital. expected return on capital employed. (Revenues - Expected Losses)/ Economic Capital
         double premiums = 0.0; // total dollar amount of premiums all borrowers would pay in one year to insure their collateral
         double scr = 0.0; // solvency capial requirement is the dollar amount of insurance assets required to survive a sress event
         double earnrate = 0.0; // annualized rate of return on total portfolio of insurance crypto assets
         time_point lastupdate = time_point(microseconds(0));;
         
         asset totaldebt = asset(0, symbol("VIGOR", 4)); // VIGOR
         
         vector<asset> insurance;
         vector<asset> collateral;

         double l_solvency = 1.0; // solvency, represents capital adequacy to back the stablecoin
         double l_valueofcol = 0.0; // dollar value of total portfolio of borrowers crypto collateral assets
         double l_scale = 1.0; // TES pricing model parameters are scaled to drive risk (solvency) to a target set by custodians.
         double l_svalueofcole = 0.0; // model suggested dollar value of the sum of all insufficient collateral in a stressed market SUM_i [ min((1 - svalueofcoli ) * valueofcoli - debti,0) ]
         double l_svalueofins = 0.0; // model suggested dollar value of the total insurance asset portfolio in a stress event. [ (1 - stressins ) * INS ]
         double l_svalueofcoleavg = 0.0; // model suggested dollar value of the sum of all insufficient collateral on average in down markets, expected loss
         double l_svalueofinsavg = 0.0; // model suggested dollar value of the total insurance asset portfolio on average in down market
         double l_raroc = 0.0; // RAROC risk adjusted return on capital. expected return on capital employed. (Revenues - Expected Losses)/ Economic Capital
         double l_premiums = 0.0; // total dollar amount of premiums all borrowers would pay in one year to insure their collateral
         double l_scr = 0.0; // solvency capial requirement is the dollar amount of insurance assets required to survive a sress event
         double l_earnrate = 0.0; // annualized rate of return on total portfolio of insurance crypto assets

         asset l_totaldebt = asset( 0, symbol("VIGOR", 4) ); // VIGOR stablecoin locked as collateral

         vector<asset> l_collateral; //EOSIO native tokens borrowed
             
         EOSLIB_SERIALIZE(globalstats, (solvency)(valueofcol)(valueofins)(scale)(svalueofcole)(svalueofins)(stressins)(svalueofcoleavg)(svalueofinsavg)(raroc)(premiums)(scr)(earnrate)(lastupdate)(totaldebt)(insurance)(collateral)(l_solvency)(l_valueofcol)(l_scale)(l_svalueofcole)(l_svalueofins)(l_svalueofcoleavg)(l_svalueofinsavg)(l_raroc)(l_premiums)(l_scr)(l_earnrate)(l_totaldebt)(l_collateral))
      }; typedef eosio::multi_index<name("globals"), globalstats> globalsm;
         typedef eosio::singleton<name("globals"), globalstats> globals;
                                                            globals _globals;
                                                            
      void doupdate();                                                      
      void risk();                                                      
      void l_risk();
      double riskx(name usern);
      double l_riskx(name usern);
      void stresscol(name usern);
      void l_stresscol(name usern);
      void stressins();
      double stressinsx(name usern);
      double portVarianceCol(name usern);
      double l_portVarianceCol(name usern);
      double portVarianceIns(name usern, double valueofins);
      double portVarianceIns();
      void update(name usern);
      void updateglobal();
      void payfee(name usern);
      void bailout(name usern);
      void bailoutup(name usern);
      void pricing(name usern);
      void l_pricing(name usern);
      void performance(name usern);
      void l_performance(name usern);
      void performanceglobal();
      void l_performanceglobal();
      void pcts(name usern, double RM);
      void l_pcts(name usern, double RM);
      double RM();
      double l_RM();
      void reserve();
      void payback_borrowed_token(name from, asset  assetin);
      
      
      // timer functions definitions
      eosio::time_point_sec expirydate();
      //void starttimer(name usern);
      //void expiration(name usern);
      //void elapsedtime(name usern);
      //void resttimer(name usern);

      // this function determines and drives state transisiton
      void statedriver(
         eosio::name usern,
         eosio::time_point_sec default_time,
         Array2d<fsmpayfee::aistate>& obj,
         std::pair<int, int>& duoval,
         eosio::asset p_amta_,
         eosio::time_point_sec& st,  // start time
         eosio::time_point_sec& et,  // expiry time
         const double tespay_,
         bool late_,
         bool updateglobal_
      ); 

      map <symbol, name> issueracct {
         {symbol("EOS",4),	    name("eosio.token")},
         {symbol("VIG",4),	    name("vig111111111")},
         {symbol("VIG",10),	    name("vig111111111")},
         {symbol("IQ",3),	    name("dummytokensx")},
         {symbol("PEOS",4),	    name("dummytokensx")},
         {symbol("DICE",4),	    name("dummytokensx")},
         {symbol("TPT",4),	    name("dummytokensx")},
         {symbol("VIGOR",4),	    name("vigor1111112")}
      };

      map <symbol, name> issuerfeed {
         {symbol("EOS",4),	    name("eosusd")},
         {symbol("VIG",10),	    name("vigeos")},
         {symbol("IQ",3),	    name("iqeos")},
         {symbol("PEOS",4),	    name("peoseos")},
         {symbol("DICE",4),	    name("diceeos")},
         {symbol("TPT",4),	    name("tpteos")},
         {symbol("VIGOR",4),	    name("vigorusd")}
      };

      double alphatest = 0.90;
      double solvencyTarget = 1.0;
      double l_solvencyTarget = 2.5;
      double maxtesprice = 0.5;
      double mintesprice = 0.005;
      double calibrate = 1.0;
      double maxtesscale = 2.0;
      double mintesscale = 0.1;
      double reservecut = 0.25;
      double maxlends = 0.5; // max percentage of insurance tokens allowable to lend out
      
            
      TABLE account {
         asset    balance;
         uint64_t primary_key()const { return balance.symbol.code().raw(); }
      }; typedef eosio::multi_index< name("accounts"), account > accounts;

      TABLE currency_stats {
         asset    supply;
         asset    max_supply;
         name     issuer;

         uint64_t primary_key()const { return supply.symbol.code().raw(); }

         EOSLIB_SERIALIZE(currency_stats, (supply)(max_supply)(issuer))
      }; typedef eosio::multi_index< name("stat"), currency_stats > stats;
                                                                stats _coinstats;

      //From datepreproc, holds the time series of prices, returns, volatility and correlation
      TABLE statspre {
         uint32_t freq;
         time_point timestamp;
         std::deque<uint64_t> price;
         std::deque<int64_t> returns;
         std::map <symbol, int64_t> correlation_matrix;
         std::uint64_t vol = defaultVol;

         uint32_t primary_key() const {return freq;}

      };

      typedef eosio::multi_index<name("tseries"), statspre> t_series;
                                                         //statstable _statstable;
                                                         t_series _statstable;

      void sub_balance( name owner, asset value );
      void add_balance( name owner, asset value, name ram_payer );
      
   public:
      using contract::contract;
      vigor(name receiver, name code, datastream<const char*> ds):contract(receiver, code, ds), 
      _user(receiver, receiver.value),
      _coinstats(receiver, receiver.value), _globals(receiver, receiver.value),
      _statstable(receiver, receiver.value), _collateral(receiver, receiver.value)  {}
     
      ACTION assetin( name   from,
                     name   to,
                     asset  assetin,
                     string memo);
                     
      ACTION assetout(name usern, asset assetout, string memo);

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
         stats cstatstable( token_contract_account, sym_code.raw() );
         const auto& st = cstatstable.get( sym_code.raw() );
         return st.supply;
      }
      
      static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
      {
         accounts accountstable( token_contract_account, owner.value );
         const auto& ac = accountstable.get( sym_code.raw() );
         return ac.balance;
      }
};
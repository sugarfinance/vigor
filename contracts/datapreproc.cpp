#include <eosio.system/eosio.system.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/chain.h>
#include <eosiolib/time.hpp>
#include <eosiolib/eosio.hpp>
#include <math.h>

using namespace eosio;

const uint64_t one_minute = 1000000.0 * 60.0; 
const uint64_t five_minute = 1000000.0 * 60.0 * 5.0;
const uint64_t fifteen_minute = 1000000.0 * 60.0 * 15.0;
const uint64_t one_hour = 1000000.0 * 60.0 * 60.0;
const uint64_t four_hour = 1000000.0 * 60.0 * 60.0 * 4.0; 
const uint64_t one_day = 1000000.0 * 60.0 * 60.0 * 24.0; 
const uint64_t cronlag = 5000000; //give extra time for cron jobs
const uint64_t dequesize = 30;

CONTRACT datapreproc : public eosio::contract {
 public:
  datapreproc(name receiver, name code, datastream<const char*> ds) : eosio::contract(receiver, code, ds) {}

 //Holds the latest datapoints from qualified oracles
  TABLE datapoints {
    uint64_t id;
    name owner; 
    uint64_t value;
    uint64_t median;
    uint64_t timestamp;

    uint64_t primary_key() const {return id;}
    uint64_t by_timestamp() const {return timestamp;}
    uint64_t by_value() const {return value;}

  };

    typedef eosio::multi_index<name("datapoints"), datapoints,
      indexed_by<name("value"), const_mem_fun<datapoints, uint64_t, &datapoints::by_value>>, 
      indexed_by<name("timestamp"), const_mem_fun<datapoints, uint64_t, &datapoints::by_timestamp>>> datapointstable;

  //Holds the list of pairs available in the oracle
  TABLE pairs {
    uint64_t id;
    name aname;

    uint64_t primary_key() const {return id;}
    uint64_t by_name() const {return aname.value;}

  };
    typedef eosio::multi_index<name("pairs"), pairs, 
      indexed_by<name("aname"), const_mem_fun<pairs, uint64_t, &pairs::by_name>>> pairstable;

  //Holds the list of pairs to process
  TABLE pairtoproc {
    uint64_t id;
    name aname;

    uint64_t primary_key() const {return id;}
    uint64_t by_name() const {return aname.value;}

  };

    typedef eosio::multi_index<name("pairtoproc"), pairtoproc, 
      indexed_by<name("aname"), const_mem_fun<pairtoproc, uint64_t, &pairtoproc::by_name>>> pairtoproctb;


  //Holds the time series of prices, returns, vol and correlation
  TABLE stats {
    uint64_t freq; 
    uint64_t timestamp;
    extended_asset base;
    extended_asset quote;
    std::deque<uint64_t> price;
    std::deque<int64_t> returns;
    std::deque<extended_asset> cov;
    std::deque<extended_asset> cor;

    uint64_t primary_key() const {return freq;}

  };

  typedef eosio::multi_index<name("stats"), stats> statstable;

//add to the list of pairs to process
ACTION addpair(name newpair) {
    
    require_auth(_self);

    pairstable pairs(name("oracle111111"), name("oracle111111").value);
    auto pairsname = pairs.get_index<name("aname")>();
    auto itr = pairsname.find(newpair.value);
    if ( itr != pairsname.end() ) { //pair must exist in the oracle
        pairtoproctb pairtoproc(_self,_self.value);
        auto pairtoprocn = pairtoproc.get_index<name("aname")>();
        auto it = pairtoprocn.find(newpair.value);
        if ( it == pairtoprocn.end() ) { //add pair if hasn't already been added
            pairtoproc.emplace(_self, [&](auto& o) {
                o.id = pairtoproc.available_primary_key();
                o.aname = newpair;
            });
        };
   };
}

  //Clear the list of pairs to process
  ACTION clear() {

    require_auth(_self);
   
    pairtoproctb pairtoproc(_self,_self.value);

    while (pairtoproc.begin() != pairtoproc.end()) {
        auto itr = pairtoproc.end();
        itr--;
        pairtoproc.erase(itr);
    }

    pairstable pairs(name("oracle111111"), name("oracle111111").value);
    auto pairsname = pairs.get_index<name("aname")>();
    for ( auto it = pairsname.begin(); it != pairsname.end(); it++ ) {
      statstable store(_self, it->aname.value);

      while (store.begin() != store.end()) {
          auto itr = store.end();
          itr--;
          store.erase(itr);
      };
    };

  }

  uint64_t get_last_price(name pair){

    uint64_t eosusd = 0;
    datapointstable dstoreos(name("oracle111111"), name("eosusd").value);
    auto newesteos = dstoreos.begin();
    if (newesteos != dstoreos.end())
      eosusd = newesteos->median;

    datapointstable dstore(name("oracle111111"), pair.value);
    auto newest = dstore.begin();
    if (newest != dstore.end()) {
        if (pair==name("eosusd"))
          return newest->median;
        else
          return (uint64_t)(1000000.0*((newest->median/1000000.0) * (eosusd/1000000.0)));
    } else
        return 0;
  }

//  get median price and store in vector as a historical time series
  ACTION update(){
    require_auth(_self);

    getprices();
    calcstats();

  }

//  get median price and store in deque as a historical time series
  void getprices(){
    require_auth(_self);

    pairtoproctb pairtoproc(_self,_self.value);
    pairstable pairs(name("oracle111111"), name("oracle111111").value);
    auto pairsname = pairs.get_index<name("aname")>();
    for ( auto it = pairtoproc.begin(); it != pairtoproc.end(); it++ ) {
        auto itr = pairsname.find(it->aname.value);
        if ( itr != pairsname.end() ) { //pair must exist in the oracle
        uint64_t lastprice = get_last_price(it->aname);
        //eosio::print("pair to process: ", eosio::name{it->aname}, "\n");
        get_last_price(it->aname, one_minute, lastprice);
        get_last_price(it->aname, five_minute, lastprice);
        get_last_price(it->aname, fifteen_minute, lastprice);
        get_last_price(it->aname, one_hour, lastprice);
        get_last_price(it->aname, four_hour, lastprice);
        get_last_price(it->aname, one_day, lastprice);
        };
    }
  }

  //  calculate statistics covariance matrix and correlation matrix
  void calcstats(){

    require_auth(_self);

    pairtoproctb pairtoproc(_self,_self.value);
    for ( auto it = pairtoproc.begin(); it != pairtoproc.end(); it++ ) {
          statstable store(_self, it.aname.value);
          auto itr = store.find(one_minute);
          if (itr != store.end()) {
            covariance(itr->aname, one_minute, itr->returns);
          }
     //   get_last_price(it->aname, five_minute, lastprice);
     //   get_last_price(it->aname, fifteen_minute, lastprice);
      //  get_last_price(it->aname, one_hour, lastprice);
      //  get_last_price(it->aname, four_hour, lastprice);
     //   get_last_price(it->aname, one_day, lastprice);
        };
    }
  }

  //  calculate statistics covariance matrix and correlation matrix
  void covariance(name aname, uint64_t freq, deque<int64_t> returns){
    require_auth(_self);

    pairtoproctb pairtoproc(_self,_self.value);
    pairstable pairs(name("oracle111111"), name("oracle111111").value);
    auto pairsname = pairs.get_index<name("aname")>();
    for ( auto it = pairtoproc.begin(); it != pairtoproc.end(); it++ ) {
        auto itr = pairsname.find(it->aname.value);
        if ( itr != pairsname.end() ) { //pair must exist in the oracle

          statstable store(_self, aname.value);
          auto itr = store.find(freq);
          if (itr != store.end()) {
            itr.returns

          };
     //   get_last_price(it->aname, five_minute, lastprice);
     //   get_last_price(it->aname, fifteen_minute, lastprice);
      //  get_last_price(it->aname, one_hour, lastprice);
      //  get_last_price(it->aname, four_hour, lastprice);
     //   get_last_price(it->aname, one_day, lastprice);
        };
    }
  }

   //get last price from the oracle and prepend to time series
  void get_last_price(const name pair, const uint64_t freq, const uint64_t lastprice){

    statstable store(_self, pair.value);

    auto itr = store.find(freq);
    uint64_t ctime = current_time();
    if (itr != store.end()) {
      auto last = store.get(freq);
      if (last.timestamp + freq - cronlag <= ctime){

        if (size(last.price)==dequesize){ // append to time series, remove oldest
          store.modify( itr, _self, [&]( auto& s ) {
            s.timestamp = ctime;
            uint64_t prevprice = s.price.back();
            s.price.push_back(lastprice);
            s.returns.push_back((int64_t)(10000.0*(((double)lastprice/(double)prevprice)-1.0)));
            s.price.pop_front();
            s.returns.pop_front();
          });
        } else { // append to time series, don't remove oldest
          store.modify( itr, _self, [&]( auto& s ) {
            s.timestamp = ctime;
            uint64_t prevprice = s.price.back();
            s.price.push_back(lastprice);
            s.returns.push_back((int64_t)(10000.0*(((double)lastprice/(double)prevprice)-1.0)));
          });
        }

      } else { // too early so just overwrite latest point
          store.modify( itr, _self, [&]( auto& s ) {
            s.price.back() = lastprice;
            if (s.price.size() > 1)
              s.returns.back() = (int64_t)(10000.0*(((double)lastprice/(double)s.price[s.price.size()-2])-1.0));
          });
      };

    } else { // first data point

          store.emplace(_self, [&](auto& s) {
            s.freq=freq;
            s.timestamp = ctime;
            s.price.push_front(lastprice);
          });
    
    }

  }

};

extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        if(code==receiver)
        {
            switch(action)
            {
                EOSIO_DISPATCH_HELPER(datapreproc, (update)(addpair)(clear))
            }
        }
       // else if(code=="eosio.token"_n.value && action=="transfer"_n.value) {
       //     execute_action( name(receiver), name(code), &datapreproc::transfer);
      //  }
    }
}
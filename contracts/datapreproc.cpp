#include <eosio.system/eosio.system.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/chain.h>
#include <eosiolib/time.hpp>
#include <eosiolib/eosio.hpp>
#include <cmath>

using namespace eosio;

const uint64_t one_minute = 1000000.0 * 60.0; 
const uint64_t five_minute = 1000000.0 * 60.0 * 5.0;
const uint64_t fifteen_minute = 1000000.0 * 60.0 * 15.0;
const uint64_t one_hour = 1000000.0 * 60.0 * 60.0;
const uint64_t four_hour = 1000000.0 * 60.0 * 60.0 * 4.0; 
const uint64_t one_day = 1000000.0 * 60.0 * 60.0 * 24.0; 
const uint64_t cronlag = 5000000; //give extra time for cron jobs
const uint64_t dequesize = 30;
const uint64_t volPrecision = 1000000.0;


CONTRACT datapreproc : public eosio::contract {
 public:
  datapreproc(name receiver, name code, datastream<const char*> ds) : eosio::contract(receiver, code, ds) {}

//Types
  enum asset_type: uint16_t {
      fiat=1,
      cryptocurrency=2, 
      erc20_token=3, 
      eosio_token=4, 
      equity=5, 
      derivative=6, 
      other=7
  };

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
    
    bool active = false;
    bool bounty_awarded = false;
    bool bounty_edited_by_custodians = false;

    name proposer;
    name aname;

    asset bounty_amount = asset(0, symbol("EOS",4));

    std::vector<name> approving_custodians;
    std::vector<name> approving_oracles;

    symbol base_symbol;
    uint64_t base_type;
    name base_contract;

    symbol quote_symbol;
    uint64_t quote_type;
    name quote_contract;
    
    uint64_t quoted_precision;

    uint64_t primary_key() const {return aname.value;}

  };

    typedef eosio::multi_index<name("pairs"), pairs> pairstable;

  //Holds the list of pairs to process
  TABLE pairtoproc {
    uint64_t id;
    name aname;

    symbol base_symbol;
    uint64_t base_type;
    name base_contract;

    symbol quote_symbol;
    uint64_t quote_type;
    name quote_contract;
    
    uint64_t quoted_precision;

    uint64_t primary_key() const {return id;}
    uint64_t by_name() const {return aname.value;}

  };

    typedef eosio::multi_index<name("pairtoproc"), pairtoproc, 
      indexed_by<name("aname"), const_mem_fun<pairtoproc, uint64_t, &pairtoproc::by_name>>> pairtoproctb;


  //Holds the time series of prices, returns, volatility and correlation
  TABLE stats {
    uint64_t freq;
    uint64_t timestamp;
    std::deque<uint64_t> price;
    std::deque<int64_t> returns;
    std::map <symbol, int64_t> correlation_matrix;
    std::uint64_t vol = 1000;

    uint64_t primary_key() const {return freq;}

  };

  typedef eosio::multi_index<name("stats"), stats> statstable;

//add to the list of pairs to process
ACTION addpair(name newpair) {
    
    //require_auth(_self);

    pairstable pairsname(name("oracle111111"), name("oracle111111").value);
    auto itr = pairsname.find(newpair.value);
    if ( itr != pairsname.end() ) { //pair must exist in the oracle
        pairtoproctb pairtoproc(_self,_self.value);
        auto pairtoprocn = pairtoproc.get_index<name("aname")>();
        auto it = pairtoprocn.find(newpair.value);
        if ( it == pairtoprocn.end() ) { //add pair if hasn't already been added
            pairtoproc.emplace(_self, [&](auto& o) {
            o.id = pairtoproc.available_primary_key();
            o.aname = newpair;
            o.base_symbol = itr->base_symbol;
            o.base_type = itr->base_type;
            o.base_contract = itr->base_contract;
            o.quote_symbol = itr->quote_symbol;
            o.quote_type = itr->quote_type;
            o.quote_contract = itr->quote_contract;
            o.quoted_precision = itr->quoted_precision;
            });
        };
   };
}

  //Clear the list of pairs to process
  ACTION clear() {

    //require_auth(_self);
   
    pairtoproctb pairtoproc(_self,_self.value);

    while (pairtoproc.begin() != pairtoproc.end()) {
        auto itr = pairtoproc.end();
        itr--;
        pairtoproc.erase(itr);
    }

    pairstable pairsname(name("oracle111111"), name("oracle111111").value);
    for ( auto it = pairsname.begin(); it != pairsname.end(); it++ ) {
      statstable store(_self, it->aname.value);

      while (store.begin() != store.end()) {
          auto itr = store.end();
          itr--;
          store.erase(itr);
      };
    };

  }

  uint64_t get_last_price(name pair, uint64_t quoted_precision){

    uint64_t eosusd = 1;
    uint64_t eos_precision = 1;
    datapointstable dstoreos(name("oracle111111"), name("eosusd").value);
    auto newesteos = dstoreos.begin();
    if (newesteos != dstoreos.end()){
      pairtoproctb pairtoproc(_self,_self.value);
      auto pairtoprocn = pairtoproc.get_index<name("aname")>();
      auto eospair = pairtoprocn.get(name("eosusd").value);
      eos_precision = eospair.quoted_precision;
      eosusd = newesteos->median;
    }
    datapointstable dstore(name("oracle111111"), pair.value);
    auto newest = dstore.begin();
    if (newest != dstore.end()) {
        if (pair==name("eosusd"))
          return newest->median;
        else
          return (uint64_t)(std::pow(10.0,quoted_precision)*((newest->median/std::pow(10.0,quoted_precision)) * (eosusd/std::pow(10.0,eos_precision))));
    } else
        return 0;
  }

//  get median price and store in vector as a historical time series
  ACTION update(){
    
    getprices();
  }

//  get median price and store in deque as a historical time series
  void getprices(){

    pairtoproctb pairtoproc(_self,_self.value);
    
    pairstable pairsname(name("oracle111111"), name("oracle111111").value);
    
    for ( auto it = pairtoproc.begin(); it != pairtoproc.end(); it++ ) {
        auto itr = pairsname.find(it->aname.value);
        if ( itr != pairsname.end() ) { //pair must exist in the oracle
        uint64_t lastprice = get_last_price(it->aname, it->quoted_precision);
        eosio::print("pair to process: ", eosio::name{it->aname}, "\n");
        store_last_price(it->aname, one_minute, lastprice);
        store_last_price(it->aname, five_minute, lastprice);
        store_last_price(it->aname, fifteen_minute, lastprice);
        store_last_price(it->aname, one_hour, lastprice);
        store_last_price(it->aname, four_hour, lastprice);
        store_last_price(it->aname, one_day, lastprice);
        };
    }
  }

  //  calculate vol and correlation matrix
  void calcstats(uint64_t freq){
    pairstable pairsname(name("oracle111111"), name("oracle111111").value);
    pairtoproctb pairtoproc(_self,_self.value);
    for ( auto it = pairtoproc.begin(); it != pairtoproc.end(); it++ ) {
        auto ito = pairsname.find(it->aname.value);
        if ( ito != pairsname.end() ) { //pair must exist in the oracle
          statstable store(_self, it->aname.value);
          auto itr = store.find(freq);
          if (itr != store.end()) {
            auto last = store.get(freq);
              int64_t vol = 1000;
                if (size(last.price)>5){
                  int64_t vol = volCalc(last.returns, sizeof(last.returns));
                    store.modify( itr, _self, [&]( auto& s ) {
                      s.vol = vol;
                    });
                }
            for ( auto jt = pairtoproc.begin(); jt != pairtoproc.end(); jt++ ) {
                auto jto = pairsname.find(jt->aname.value);
                if ( jto != pairsname.end() ) { //pair must exist in the oracle

                  statstable storej(_self, jt->aname.value);
                  auto jtr = storej.find(freq);
                  if (jtr != storej.end()) {
                    auto lastj = storej.get(freq);
       
                      int64_t corr = 5000;
                      if (size(lastj.price)==size(last.price) && size(last.price)>5){
                        corr = corrCalc(last.returns, lastj.returns, sizeof(last.returns));
                        store.modify( itr, _self, [&]( auto& s ) {
                          s.correlation_matrix[jt->base_symbol] = corr;
                        });
                      }
                  }
                }
            }
          }
        }
    }
  }
  
// correlation coefficient
int64_t corrCalc(std::deque<int64_t> X, std::deque<int64_t> Y, uint64_t n) 
{ 
  
    double sum_X = 0.0, sum_Y = 0.0, sum_XY = 0.0, x = 0.0, y = 0.0;
    double squareSum_X = 0.0, squareSum_Y = 0.0; 
  
    for (uint64_t i = 0; i < n; i++) 
    { 
        x = X[i]/10000.0;
        y = Y[i]/10000.0;
        sum_X = sum_X + x; 
        sum_Y = sum_Y + y; 
        sum_XY = sum_XY + x * y; 
        squareSum_X = squareSum_X + x * x; 
        squareSum_Y = squareSum_Y + y * y; 
    } 
    int64_t corr = (int64_t)(10000.0)*(n * sum_XY - sum_X * sum_Y)  
                  / sqrt((n * squareSum_X - sum_X * sum_X)  
                      * (n * squareSum_Y - sum_Y * sum_Y)); 
    return corr; 
} 

uint64_t volCalc(std::deque<int64_t> returns, uint64_t n) {

     double variance = 0.0;
     double t = returns[0]/10000.0;
     for (int i = 1; i < n; i++)
     {
          t += returns[i]/10000.0;
          double diff = ((i + 1) * returns[i]/10000.0) - t;
         
          variance += (diff * diff) / ((i + 1) *i);
     }
     eosio::print("vol: ", (uint64_t)(volPrecision*sqrt(variance / (n - 1))), "\n");
     return (uint64_t)(volPrecision*sqrt(variance / (n - 1)));
}

  
   //store last price from the oracle, append to time series
  void store_last_price(const name pair, const uint64_t freq, const uint64_t lastprice){

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
      calcstats(freq);
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
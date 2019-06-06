#include <eosio.system/eosio.system.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/chain.h>
#include <eosiolib/time.hpp>
#include <eosiolib/eosio.hpp>
#include <math.h>

using namespace eosio;

const uint64_t one_minute = 1000000 * 60 - 5000000; //give extra time for cron jobs
const uint64_t five_minute = 1000000 * 60 * 5 - 5000000;
const uint64_t fifteen_minute = 1000000 * 60 * 15 - 5000000;
const uint64_t one_hour = 1000000 * 60 * 60 - 5000000;
const uint64_t four_hour = 1000000 * 60 * 60 * 4 - 5000000; 
const uint64_t one_day = 1000000 * 60 * 60 * 24 - 5000000; 

CONTRACT datapreproc : public eosio::contract {
 public:
  datapreproc(name receiver, name code, datastream<const char*> ds) : eosio::contract(receiver, code, ds) {}

  //Holds the last datapoints_count datapoints from qualified oracles
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


  //Holds the time of last writes, vol and correlation
  TABLE stats {
    uint64_t freq; 
    uint64_t timestamp;
    uint64_t count;
    uint64_t last_claim;
    asset balance;

    uint64_t primary_key() const {return freq;}
    uint64_t by_count() const {return count;}

  };

  typedef eosio::multi_index<name("stats"), stats,
      indexed_by<name("count"), const_mem_fun<stats, uint64_t, &stats::by_count>>> statstable;

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

  }


  uint64_t get_last_price(name pair){

    datapointstable dstore(name("oracle111111"), pair.value);
    //eosio_assert(dstore.begin() != dstore.end(), "no datapoints");
    if (dstore.begin() != dstore.end()) {
        auto newest = dstore.begin();
        return newest->median;
    } else {
        eosio::print("pair has no datapoints: ", eosio::name{pair}, "\n");
        return -1;
    }

  }

  ACTION update(){
    require_auth(_self);

    pairtoproctb pairtoproc(_self,_self.value);
    pairstable pairs(name("oracle111111"), name("oracle111111").value);
    auto pairsname = pairs.get_index<name("aname")>();
    for ( auto it = pairtoproc.begin(); it != pairtoproc.end(); it++ ) {
        auto itr = pairsname.find(it->aname.value);
        if ( itr != pairsname.end() ) { //pair must exist in the oracle
        uint64_t lastprice = get_last_price(it->aname);
        eosio::print("pair to process: ", eosio::name{it->aname}, "\n");
        eosio::print("last price: ", lastprice, "\n");
 
        check_last_push(it->aname, one_minute);
        update_datapoints(lastprice, it->aname);

        };
    }
  }


   //Ensure account cannot push data more often than every 60 seconds
  void check_last_push(const name pair, const uint64_t freq){

    statstable gstore(_self,_self.value);
    statstable store(_self, pair.value);

    auto itr = store.find(owner.value);
    if (itr != store.end()) {

      uint64_t ctime = current_time();
      auto last = store.get(owner.value);

      eosio_assert(last.timestamp + one_minute <= ctime, "can only call every 60 seconds");

      store.modify( itr, _self, [&]( auto& s ) {
        s.timestamp = ctime;
        s.count++;
      });

    } else {

      store.emplace(_self, [&](auto& s) {
        s.owner = owner;
        s.timestamp = current_time();
        s.count = 1;
        s.balance = asset(0, symbol("EOS",4));
        s.last_claim = 0;
      });

    }

    auto gitr = gstore.find(owner.value);
    if (gitr != gstore.end()) {

      uint64_t ctime = current_time();

      gstore.modify( gitr, _self, [&]( auto& s ) {
        s.timestamp = ctime;
        s.count++;
      });

    } else {

      gstore.emplace(_self, [&](auto& s) {
        s.owner = owner;
        s.timestamp = current_time();
        s.count = 1;
       s.balance = asset(0, symbol("EOS",4));
       s.last_claim = 0;
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
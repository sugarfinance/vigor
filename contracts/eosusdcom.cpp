#include "eosusdcom.hpp"

void eosusdcom::doupdate()
{
   //require_auth(_self);
   usdtable eosusdtable(name("oracle111111"),name("oracle111111").value);
   auto iterator = eosusdtable.begin();
   fxrate[symbol("EOS",4)] = iterator->average;
   eosio::print( "EOS fxrate updated : ", iterator->average, "\n");

   user_t _user(_self, _self.value);
   for ( auto it = _user.begin(); it != _user.end(); it++ ) {
    update(it->usern);
    //eosio::print( "update complete for: ", eosio::name{it->usern}, "\n");
   }
   transaction txn{};
   txn.actions.emplace_back(  permission_level { _self, "active"_n },
                              _self, "doupdate"_n, make_tuple()
                           ); txn.delay_sec = 60;
   uint128_t txid = (uint128_t(_self.value) << 64) | now();
   txn.send(txid, _self); 
}

void eosusdcom::create( name   issuer,
                        asset  maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
       //s.volatility    = 0.42;
       //s.correlation_matrix = correlation_matrix;
    });
}

void eosusdcom::setsupply( name issuer, asset maximum_supply )
{
    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before setting supply" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( maximum_supply.is_valid(), "invalid maximum_supply" );
    eosio_assert( maximum_supply.amount > 0, "must issue positive maximum_supply" );

    eosio_assert( maximum_supply.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( maximum_supply.amount >= st.supply.amount, "cannot set max_supply to less than available supply");

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.max_supply = maximum_supply;
    });
}

void eosusdcom::issue( name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
      SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                          { st.issuer, to, quantity, memo }
      );
    }
}

void eosusdcom::retire( asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must retire positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

void eosusdcom::transfer(name    from,
                      name    to,
                      asset   quantity,
                      string  memo )
{
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( is_account( to ), "to account does not exist");
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    if (to == _self) {
      auto &user = _user.get(from.value,"User not found");
      eosio_assert( quantity.symbol == symbol("UZD", 4), 
                    "Debt asset type must be UZD"
                  );
      update(from);
      
      eosio_assert(user.debt.amount >= quantity.amount, "Payment too high");
      
      _user.modify(user, _self, [&]( auto& modified_user) { // Transfer stablecoin into user
        modified_user.debt -= quantity;
      });
     
      auto sym = quantity.symbol.code();
      stats statstable( _self, sym.raw() );
      const auto& st = statstable.get( sym.raw() );

      require_recipient( from );

      eosio_assert( quantity.is_valid(), "invalid quantity" );
      eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
      eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
      eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

      auto payer = has_auth( to ) ? to : from;

      sub_balance( from, quantity );
      add_balance( to, quantity, payer );

      action(permission_level{_self, name("active")}, _self, 
        name("retire"), std::make_tuple(quantity, memo)
      ).send();
    } 
    else {
      auto sym = quantity.symbol.code();
      stats statstable( _self, sym.raw() );
      const auto& st = statstable.get( sym.raw() );

      eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

      require_recipient( from );
      require_recipient( to );
      
      auto payer = has_auth( to ) ? to : from;

      sub_balance( from, quantity );
      add_balance( to, quantity, payer );
    }
}

void eosusdcom::sub_balance( name owner, asset value ) {
   accounts from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
      });
}

void eosusdcom::add_balance( name owner, asset value, name ram_payer )
{
   accounts to_acnts( _self, owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void eosusdcom::open( name owner, const symbol& symbol, name ram_payer )
{
   require_auth( ram_payer );

   auto sym_code_raw = symbol.code().raw();

   stats statstable( _self, sym_code_raw );

   const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );

   eosio_assert( st.supply.symbol == symbol, "symbol precision mismatch" );

   accounts acnts( _self, owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
      });
   }
}

void eosusdcom::close( name owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( _self, owner.value );
   auto it = acnts.find( symbol.code().raw() );
   eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

void eosusdcom::assetin( name   from,
                         name   to,
                         asset  assetin,
                         string memo ) {
  if ( from == _self )
    return;

  require_auth( from );
  eosio_assert(assetin.symbol.is_valid(), "Symbol must be valid.");
  eosio_assert(assetin.amount > 0, "Amount must be > 0.");
  eosio_assert(memo.c_str()==string("collateral") || memo.c_str()==string("support"), "memo must be composed of either word: support or collateral");
  
     // Create user, if not exist
  auto itr = _user.find(from.value);
  if ( itr == _user.end() ) {
    itr = _user.emplace(_self,  [&](auto& new_user) {
      new_user.usern = from;
      new_user.debt = asset(0,symbol("UZD", 4));
    });
    action(permission_level{_self, name("active")},
      _self, name("open"), std::make_tuple(
        from, symbol("UZD", 4), _self
      )).send();
  }
  auto &user = *itr;

  globals globalstab( _self, _self.value );
  globalstats stats;
  if (globalstab.exists())
    stats = globalstab.get();

  if (memo.c_str() == string("collateral")) {
    for ( auto it = user.collateral.begin() ; it <= user.collateral.end(); ++it ) {
      if (it == user.collateral.end())
        //User collateral type not found
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.collateral.push_back(assetin);
        });
      else if (it->symbol == assetin.symbol) {
        //User collateral type found
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.collateral[it - user.collateral.begin()].amount += assetin.amount;
        });
        break;
      }
    }
  } else if (memo.c_str() == string("support")) {
    for ( auto it = user.support.begin(); it <= user.support.end(); ++it ) {
      if ( it == user.support.end() ) // User support type not found
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.support.push_back(assetin);
        });
      else if ( it->symbol == assetin.symbol ) { // User support type found
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.support[it - user.support.begin()].amount += assetin.amount;
        });
        break;
      }
    }
    for ( auto it = stats.support.begin(); it <= stats.support.end(); ++it ) {
      if ( it == stats.support.end() )
        stats.support.push_back(assetin);
      else if ( it->symbol == assetin.symbol ) {
        stats.support[it - stats.support.begin()] += assetin;
        break;
      }
    }
    for ( auto it = stats.collateral.begin(); it <= stats.collateral.end(); ++it ) {
      if ( it == stats.collateral.end() )
        stats.collateral.push_back(assetin);
      else if ( it->symbol == assetin.symbol ) {
        stats.collateral[it - stats.collateral.begin()] += assetin;
        break;
      }
    }
  } 
  else {
    eosio_assert( assetin.symbol == user.debt.symbol, 
                  "must payoff debt with same asset as debt"
                );
    eosio_assert( user.debt.amount >= assetin.amount, 
                  "erasing more debt than available" 
                );
    _user.modify(user, _self, [&]( auto& modified_user) {
      modified_user.debt -= assetin;
    });
    stats.bel_n -= assetin.amount / 10000; //TODO
    stats.bel_n = 0;

  }
  globalstab.set(stats, _self);
  update(from);
}

void eosusdcom::assetout(name usern, asset assetout, string memo) {

  require_auth(usern);

  auto &user = _user.get( usern.value,"User not found" );
  eosio_assert( assetout.symbol.is_valid(), "Symbol must be valid." );
  eosio_assert( assetout.amount > 0, "Amount must be > 0." );
  eosio_assert( memo.c_str() == string("collateral") || 
                memo.c_str() == string("support") || 
                memo.c_str() == string("borrow"), 
                "memo must be composed of either word: support | collateral | borrow"
              );
  globals globalstab( _self, _self.value );
  globalstats stats;
  if (globalstab.exists())
    stats = globalstab.get();

  if ( memo.c_str() == string("collateral") ) {
    eosio_assert( !user.collateral.empty(), 
                  "User does not have any collateral"
                );
    for ( auto it = user.collateral.begin(); it != user.collateral.end(); ++it )
      if (it->symbol == assetout.symbol) { //User collateral type found
        eosio_assert((it->amount >= assetout.amount),"Insufficient collateral available.");
        
        double valueofasset = assetout.amount / std::pow(10.0, it->symbol.precision());
        valueofasset *= fxrate[assetout.symbol] / std::pow(10.0, 4);
        double valueofcol = user.valueofcol - valueofasset;

        eosio_assert( valueofcol >= 1.01 * ( user.debt.amount / std::pow(10.0, 4) ),
        "Dollar value of collateral would become less than dollar value of debt" );
        
        if ( it->amount - assetout.amount == 0 )
          _user.modify(user, _self, [&]( auto& modified_user) {
            modified_user.collateral.erase(it);
          });
        else
          _user.modify(user, _self, [&]( auto& modified_user) {
            modified_user.collateral[it - user.collateral.begin()] -= assetout;
          });
        action( permission_level{_self, name("active")}, 
          issueracct[assetout.symbol], name("transfer"),
          std::make_tuple( _self, usern, assetout,
            std::string("Transfer loan collateral out: ") + usern.to_string()
          )).send();
        break;
      }
  } 
  else if ( memo.c_str() == string("support") ) {
    eosio_assert( !user.support.empty(), 
                  "User does not have any support asset"
                );
    for ( auto it = user.support.begin() ; it < user.support.end(); ++it )
      if (it->symbol == assetout.symbol) { // User support type found
        eosio_assert( it->amount >= assetout.amount,
        "Insufficient support asset available." );
        
        if ( it->amount - assetout.amount == 0 )
          _user.modify(user, _self, [&]( auto& modified_user) {
            modified_user.support.erase(it);
          });
        else 
          _user.modify(user, _self, [&]( auto& modified_user) {
            modified_user.support[it - user.support.begin()] -= assetout;
          });
        action( permission_level{_self, name("active")},
          issueracct[assetout.symbol], name("transfer"),
          std::make_tuple(_self, usern, assetout,
            std::string("Transfer support assets out: ") + usern.to_string()
        )).send(); 
        break;
      }
    bool found = false;
    for ( auto it = stats.support.begin(); it < stats.support.end(); ++it ) {
      if ( it->symbol == assetout.symbol ) {
        stats.support[it - stats.support.begin()] -= assetout;
        found = true;
        break;
      }
    }
    eosio_assert(found, "support asset not found");
    found = false;
    for ( auto it = stats.collateral.begin(); it < stats.collateral.end(); ++it ) {
      if ( it->symbol == assetout.symbol ) {
        stats.collateral[it - stats.collateral.begin()] -= assetout;
        found = true;
        break;
      }
    }
    eosio_assert(found, "collateral asset not found");
  } 
  else {
    eosio_assert( assetout.symbol == symbol("UZD", 4), 
                  "Borrow asset type must be UZD" 
                );
    stats.bel_n += assetout.amount / 10000; //TODO
    asset debt = user.debt + assetout;
    eosio_assert( user.valueofcol >= 1.01 * ( debt.amount / std::pow(10.0,4) ),
    "Dollar value of collateral would become less than dollar value of debt" );
    
    _user.modify(user, _self, [&]( auto& modified_user) {
      modified_user.debt = debt;
    });
    action( permission_level{_self, name("active")},
      _self, name("issue"), std::make_tuple(
        usern, debt, std::string("UZD issued to ") + usern.to_string()
      )).send();
  }
  globalstab.set(stats, _self);
  update(usern);
}

/* Portfolio variance is a measurement of how the aggregate actual returns
 * of a set of securities making up a portfolio fluctuate over time. This
 * portfolio variance statistic is calculated using the standard deviations
 * of each security in the portfolio as well as the correlations of each 
 * security pair in the portfolio.
 E.g. for two assets Variance = 
   [(weight_asset1)^2 x (stdev_asset1)^2] +  
   [(weight_asset2)^2 x (stdev_asset2)^2] +  
   (2 x weight_asset1 x stdev_asset1 x 
        weight_asset2 x stdev_asset2 x 
        correlation between the two assets) 
*/
void eosusdcom::pricingmodel(name usern) {

const auto& user = _user.get( usern.value, "User not found" ); 
   

  globals globalstab( _self, _self.value );
  eosio_assert(globalstab.exists(), "No support yet");
  globalstats stats = globalstab.get();

double portVariance = 0.0;
if (user.collateral.size()==3){

std::vector<asset>::const_iterator it = user.collateral.begin();
double w1 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / user.valueofcol;
double sig1 = this->volatility;
it++;
double w2 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / user.valueofcol;
double sig2 = this->volatility;
it++;
double w3 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / user.valueofcol;
double sig3 = this->volatility;
double corr12 = this->correlation;
double corr13 = this->correlation;
double corr23 = this->correlation;

portVariance = std::pow(w1,2)*std::pow(sig1,2) + 
std::pow(w2,2)*std::pow(sig2,2) + 
std::pow(w3,2)*std::pow(sig3,2) + 
2.0*w1*w1*corr12*sig1*sig2 +
2.0*w1*w3*corr13*sig1*sig3 +
2.0*w2*w3*corr23*sig2*sig3;

} else if(user.collateral.size()==2){

std::vector<asset>::const_iterator it = user.collateral.begin();
double w1 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / user.valueofcol;
double sig1 = this->volatility;
it++;
double w2 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / user.valueofcol;
double sig2 = this->volatility;
double corr12 = this->correlation;

portVariance = std::pow(w1,2)*std::pow(sig1,2) + 
std::pow(w2,2)*std::pow(sig2,2) + 
2.0*w1*w1*corr12*sig1*sig2;

}else if(user.collateral.size()==1){

double sig1 = this->volatility;

portVariance = std::pow(sig1,2);

};

  eosio::print( "portVariance : ", portVariance, "\n");

  // premium payments in exchange for contingient payoff in the event that a price threshhold is breached

  double impliedvol = sqrt(portVariance) * this->scale; 

  double iportVaR = std::min(3.0*impliedvol,1.0); // value at risk

  double payoff = std::max(1.0*(user.debt.amount/std::pow(10.0,4)) - user.valueofcol*(1-iportVaR),0.0);

  uint32_t T = 1;
  
  double d = ((std::log(user.valueofcol / (user.debt.amount/std::pow(10.0,4)))) + (-std::pow(impliedvol,2)/2) * T)/ (impliedvol * std::sqrt(T));

  double tesprice = std::max((payoff * std::erfc(-d/std::sqrt(2))/2)/(user.debt.amount/std::pow(10.0,4)),0.005*this->scale);
  double tesvalue = std::max((payoff * std::erfc(-d/std::sqrt(2))/2),0.005*this->scale);
  tesprice = tesprice/(1.6*(user.creditscore/800.0)); // credit score of 500 means no discount or penalty.
  

  globalstab.set(stats, _self);
  iportVaR = std::max(user.debt.amount - ((1.0 - iportVaR) * user.valueofcol),0.0); // for solvency

  _user.modify(user, _self, [&]( auto& modified_user) { // Update value of collateral
    modified_user.iportVaR = iportVaR;
    modified_user.tesvalue = tesvalue;
    modified_user.tesprice = tesprice;
  });
}

void eosusdcom::calcStats(double delta_iportVaRcol) 
{  

    symbol sst = symbol("UZD", 4);
    auto sym_code_raw = sst.code().raw();
    stats statstable( _self, sym_code_raw );
    double totdebt = 0.0;
    auto existing = statstable.find( sym_code_raw );
    if (existing != statstable.end()){
      totdebt = existing->supply.amount/std::pow(10.0,4);
    };
    eosio::print( "totdebt : ", totdebt, "\n");

  globals globalstab( _self, _self.value );
  eosio_assert(globalstab.exists(), "No support yet");
  globalstats stats = globalstab.get();

double portVariance = 0.0;
if (stats.support.size()==3){

auto it = stats.support.begin();
double w1 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / stats.valueofins;
double sig1 = this->volatility;
it++;
double w2 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / stats.valueofins;
double sig2 = this->volatility;
it++;
double w3 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / stats.valueofins;
double sig3 = this->volatility;
double corr12 = this->correlation;
double corr13 = this->correlation;
double corr23 = this->correlation;

portVariance = std::pow(w1,2)*std::pow(sig1,2) + 
std::pow(w2,2)*std::pow(sig2,2) + 
std::pow(w3,2)*std::pow(sig3,2) + 
2.0*w1*w1*corr12*sig1*sig2 +
2.0*w1*w3*corr13*sig1*sig3 +
2.0*w2*w3*corr23*sig2*sig3;

} else if(stats.support.size()==2){

auto it = stats.support.begin();
double w1 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / stats.valueofins;
double sig1 = this->volatility;
it++;
double w2 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / stats.valueofins;
double sig2 = this->volatility;
double corr12 = this->correlation;

portVariance = std::pow(w1,2)*std::pow(sig1,2) + 
std::pow(w2,2)*std::pow(sig2,2) + 
2.0*w1*w1*corr12*sig1*sig2;

}else if(stats.support.size()==1){

double sig1 = this->volatility;

portVariance = std::pow(sig1,2);

};


  eosio::print( "portVarianceins : ", portVariance, "\n");
  double impliedvol = std::sqrt(portVariance);
  double iportVaR = std::min(3.0 * impliedvol, 1.0); // value at risk 
  stats.iportVaRins = (1.0 - iportVaR) * stats.valueofins;

  


portVariance = 0.0;
if (stats.collateral.size()==3){

auto it = stats.collateral.begin();
double w1 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / stats.valueofcol;
double sig1 = this->volatility;
it++;
double w2 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / stats.valueofcol;
double sig2 = this->volatility;
it++;
double w3 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / stats.valueofcol;
double sig3 = this->volatility;
double corr12 = this->correlation;
double corr13 = this->correlation;
double corr23 = this->correlation;

portVariance = std::pow(w1,2)*std::pow(sig1,2) + 
std::pow(w2,2)*std::pow(sig2,2) + 
std::pow(w3,2)*std::pow(sig3,2) + 
2.0*w1*w1*corr12*sig1*sig2 +
2.0*w1*w3*corr13*sig1*sig3 +
2.0*w2*w3*corr23*sig2*sig3;

} else if(stats.collateral.size()==2){

auto it = stats.collateral.begin();
double w1 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / stats.valueofcol;
double sig1 = this->volatility;
it++;
double w2 = ((it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4))) / stats.valueofcol;
double sig2 = this->volatility;
double corr12 = this->correlation;

portVariance = std::pow(w1,2)*std::pow(sig1,2) + 
std::pow(w2,2)*std::pow(sig2,2) + 
2.0*w1*w1*corr12*sig1*sig2;

}else if(stats.collateral.size()==1){

double sig1 = this->volatility;

portVariance = std::pow(sig1,2);

};


  eosio::print( "portVariancol : ", portVariance, "\n");
  impliedvol = std::sqrt(portVariance);
  iportVaR = std::min(3.0 * impliedvol, 1.0); // value at risk 
  //stats.iportVaRcol = (1.0 - iportVaR) * stats.valueofins;

  stats.iportVaRcol = std::max(totdebt - ((1.0 - iportVaR) * stats.valueofcol),0.0); // for solvency
  eosio::print( "portVariancol : ", portVariance, "\n");

  // const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
  /*
  auto existing = statstable.find( sym.code().raw() );
    if existing != statstable.end(){
      double totdebt = existing.supply.symbol.amount/std::pow(10.0,4);
    }
    eosio::print( "totdebt : ", totdebt, "\n");
*/
  //stats.iportVaRcol += delta_iportVaRcol;

  double bel_s = stats.iportVaRcol;
  double mva_n = stats.valueofins;
  double mva_s = stats.iportVaRins;
  eosio::print( "bel_s : ", bel_s, "\n");
  eosio::print( "mva_n : ", mva_n, "\n");
  eosio::print( "mva_s : ", mva_s, "\n");


  double own_n = mva_n;
  double own_s = mva_s - bel_s;
  double scr = own_n - own_s;

  eosio::print( "own_n : ", own_n, "\n");

  eosio::print( "own_s : ", own_s, "\n");

  eosio::print( "scr : ", scr, "\n");
  
  stats.solvency = own_n / scr;
  globalstab.set(stats, _self);
}

/* premium payments in exchange for contingient payoff in 
 * the event that a price threshhold is breached
*/
void eosusdcom::payfee(name usern) {
  auto &user = _user.get( usern.value, "User not found" );
  
  globals globalstab( _self, _self.value );
  eosio_assert(globalstab.exists(), "No support yet");
  globalstats stats = globalstab.get();

  uint64_t amt = 0;
  uint64_t T = 360*24*60;
  double tespay = (user.debt.amount / std::pow(10.0, 4)) * (std::pow((1 + user.tesprice), (1 / T)) - 1);

  for ( auto it = user.collateral.begin(); it != user.collateral.end(); ++it )
    if ( it->symbol == symbol("VIG",4) ) {
      amt = ( tespay * std::pow(10.0, 4) ) / 
            ( fxrate[it->symbol] / std::pow(10.0, 4) );
      if ( it->amount >= amt )
        _user.modify(user, _self, [&]( auto& modified_user) { // withdraw fee
          modified_user.feespaid += tespay;
          modified_user.collateral[it - user.collateral.begin()].amount -= amt;
        });
      else 
        amt = 0;
      break;
    }
  if (!amt)
    _user.modify(user, _self, [&]( auto& modified_user) { // withdraw fee
      modified_user.latepays += 1;
    });
  else 
    for ( auto itr = _user.begin(); itr != _user.end(); ++itr )
      if ( itr->valueofins > 0 ) {
        double weight = itr->valueofins / stats.valueofins;

        for ( auto it = itr->support.begin(); it != itr->support.end(); ++it )
          if ( it->symbol == symbol("VIG", 4) ) {
            _user.modify( itr, _self, [&]( auto& modified_user ) { // weighted fee deposit
              modified_user.support[it - itr->support.begin()].amount += amt * weight;
            });
            break;
          } 
      }
}

void eosusdcom::update(name usern) {

  auto &user = _user.get( usern.value, "User not found" );
  globals globalstab( _self, _self.value );
  globalstats stats;
  
  if (globalstab.exists())
    stats = globalstab.get();
  
  stats.valueofins -= user.valueofins;
  stats.valueofcol -= user.valueofcol;

  double valueofins = 0.0;
  double valueofcol = 0.0;
  
  for ( auto it = user.support.begin(); it != user.support.end(); ++it )
    valueofins += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( fxrate[it->symbol] / std::pow(10.0, 4) );
  for ( auto it = user.collateral.begin(); it != user.collateral.end(); ++it )
    valueofcol += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( fxrate[it->symbol] / std::pow(10.0, 4) );
  
  stats.valueofins += valueofins;
  stats.valueofcol += valueofcol;

  _user.modify(user, _self, [&]( auto& modified_user) { // Update value of collateral
    modified_user.valueofins = valueofins;
    modified_user.valueofcol = valueofcol;
  }); globalstab.set(stats, _self);

  if ( user.valueofcol > 0.0 && user.debt.amount > 0 ) { // Update tesprice    
    double iportVaRold = user.iportVaR;
    pricingmodel(usern); 
    calcStats(iportVaRold - user.iportVaR);
  //  payfee(usern);
    
    if (user.latepays > 4) {
      _user.modify(user, _self, [&]( auto& modified_user) {
        modified_user.latepays = 0; 
        modified_user.recaps += 1;
      });
      bailout(usern);
    } 
    else if ( 1.01 * ( user.debt.amount / std::pow(10.0, 4) ) > user.valueofcol ) {
      _user.modify(user, _self, [&]( auto& modified_user) {
        modified_user.recaps += 1;
      });
      bailout(usern);
    }
    
  }
}


/* illiquidity risk is offloaded to insurers who are compensated
 * to take this risk. insurers may start of with zero debt but in
 * a bailout they acquire it, along with a failed loan's remaining 
 * collateral. some of their support assets will be assigned to 
 * their collateral bucket so that it overcollateralizes their debt
 * at some default setting like 1.5
*/
void eosusdcom::bailout(name usern) {
  auto user = _user.find(usern.value);
  eosio_assert(user != _user.end(), "User not found");
  globals globalstab( _self, _self.value );
  globalstats stats = globalstab.get();

  for ( auto itr = _user.begin(); itr != _user.end(); ++itr ) {
    if (itr->valueofins > 0) {
      double weight = itr->valueofins / stats.valueofins;

      for ( auto c = user->collateral.begin(); c != user->collateral.end(); ++c ) {
        uint64_t amt = c->amount * weight;
        _user.modify(user, _self, [&]( auto& modified_user) { // weighted fee withdrawl
            modified_user.collateral[c - user->collateral.begin()].amount -= amt;
        });
        for ( auto it = itr->collateral.begin(); it != itr->collateral.end(); ++it )
          if (it->symbol == c->symbol) {
            _user.modify(itr, _self, [&]( auto& modified_user) { // weighted fee deposit
              modified_user.collateral[it - itr->collateral.begin()].amount += amt;
            });
            amt = 0;
            break;
          } 
        if (amt > 0) 
          _user.modify(itr, _self, [&]( auto& modified_user) { // weighted fee deposit
            modified_user.collateral.push_back(asset(amt, c->symbol));
          });
      }
      _user.modify(itr, _self, [&]( auto& modified_user) { // weighted fee deposit
          modified_user.debt.amount += user->debt.amount * weight;
      });
      for ( auto i = itr->support.begin(); i != itr->support.end(); ++i ) {
        uint64_t amt = i->amount * 0.25; //convert some support into collateral
        _user.modify(itr, _self, [&]( auto& modified_user) { // weighted fee withdrawl
            modified_user.support[i - user->support.begin()].amount -= amt;
        });
        for ( auto it = itr->collateral.begin(); it != itr->collateral.end(); ++it )
          if ( it->symbol == i->symbol ) {
            _user.modify(itr, _self, [&]( auto& modified_user) { // weighted fee deposit
              modified_user.collateral[it - itr->collateral.begin()].amount += amt;
            });
            amt = 0;
            break;
          } 
        if (amt > 0) 
          _user.modify(itr, _self, [&]( auto& modified_user) { // weighted fee deposit
            modified_user.collateral.push_back(asset(amt, i->symbol));
          });
      }
    }
  }
}

extern "C" {
  [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if((code==name("eosio.token").value ||
        code==name("vig111111111").value ||
        code==name("dummytokens1").value) && action==name("transfer").value) {
      eosio::execute_action(name(receiver),name(code), &eosusdcom::assetin);
    }
    if (code == receiver) {
      switch (action) { 
          EOSIO_DISPATCH_HELPER(eosusdcom, (create)(assetout)(issue)(transfer)(open)(close)(retire)(setsupply)(doupdate)) 
      }    
    }
        eosio_exit(0);
  }
}

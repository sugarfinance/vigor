#include "eosusdcom.hpp"

void eosusdcom::doupdate()
{
   require_auth(_self);
   usdtable eosusdtable(name("oracle111111"),name("oracle111111").value);
   auto iterator = eosusdtable.begin();
   fxrate[symbol("EOS",4)] = iterator->average;
   eosio::print( "EOS fxrate updated: ", iterator->average, "\n");

   user_t _user(_self, _self.value);
   for (auto it = _user.begin(); it != _user.end(); it++){
    update(it->usern);
    eosio::print( "update complete for: ", eosio::name{it->usern}, "\n");
   };

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
       s.volatility    = 0.42;
       s.correlation_matrix = correlation_matrix;
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
                         asset  assetin,
                         string memo ) {
  require_auth( from );
  eosio_assert( from == _self, "from cannot be self" );
  eosio_assert(assetin.symbol.is_valid(), "Symbol must be valid.");
  eosio_assert(assetin.amount > 0, "Amount must be > 0.");
  eosio_assert(memo.c_str()==string("collateral") || memo.c_str()==string("insurance"), "memo must be composed of either word: insurance or collateral");
  
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

  if (memo.c_str() == string("collateral")) {
    for (std::vector<asset>::const_iterator it = user.collateral.begin() ; it <= user.collateral.end(); ++it){
      if (it == user.collateral.end()){
        //User collateral type not found
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.collateral.push_back(assetin);
        });
      }
      else if (it->symbol == assetin.symbol) {
        //User collateral type found
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.collateral[it - user.collateral.begin()].amount += assetin.amount;
        });
        break;
      }
    }
  } else if (memo.c_str() == string("insurance")) {
    for (std::vector<asset>::const_iterator it = user.insurance.begin() ; it <= user.insurance.end(); ++it){
      if (it == user.insurance.end()) {
        //User insurance type not found
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.insurance.push_back(assetin);
        });
      }
      else if (it->symbol == assetin.symbol) {
        //User insurance type found
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.insurance[it - user.insurance.begin()].amount += assetin.amount;
        });
        break;
      }
    }
  } else if (memo.c_str() == string("payoffdebt")) {
    eosio_assert( assetin.symbol == user.debt.symbol, 
                  "must payoff debt with same asset as debt"
                );
    eosio_assert( user.debt.amount >= assetin.amount, 
                  "erasing more debt than available" 
                );
    _user.modify(user, _self, [&]( auto& modified_user) {
      modified_user.debt -= assetin;
    });
  }
    update(from);
}

void eosusdcom::assetout(name usern, asset assetout, string memo) {

  require_auth(usern);

  auto &user = _user.get( usern.value,"User not found" );
  eosio_assert( assetout.symbol.is_valid(), "Symbol must be valid." );
  eosio_assert( assetout.amount > 0, "Amount must be > 0." );
  eosio_assert( memo.c_str() == string("collateral") || 
                memo.c_str() == string("insurance") || 
                memo.c_str() == string("borrow"), 
                "memo must be composed of either word: insurance | collateral | borrow"
              );
  if (memo.c_str() == string("collateral")) {
    eosio_assert(!user.collateral.empty(), "User does not have any collateral");
    for (std::vector<asset>::const_iterator it = user.collateral.begin(); it != user.collateral.end(); ++it)
      if (it->symbol == assetout.symbol) { //User collateral type found
        eosio_assert((it->amount >= assetout.amount),"Insufficient collateral available.");
        
        double valueofasset = assetout.amount / std::pow(10.0,it->symbol.precision());
        valueofasset *= fxrate[assetout.symbol] / std::pow(10.0,4);
        double valueofcol = user.valueofcol - valueofasset;

        eosio_assert( valueofcol >= 1.01 * ( user.debt.amount / std::pow(10.0,4) ),
        "Dollar value of collateral would become less than dollar value of debt" );
        if (it->amount - assetout.amount == 0)
          _user.modify(user, _self, [&]( auto& modified_user) {
            modified_user.collateral.erase(it);
          });
        else
          _user.modify(user, _self, [&]( auto& modified_user) {
            modified_user.collateral[it - user.collateral.begin()].amount -= assetout.amount;
          });
        action( permission_level{_self, name("active")}, 
          issueracct[assetout.symbol], name("transfer"),
          std::make_tuple( _self, usern, assetout,
            std::string("Transfer loan collateral out: ") + usern.to_string()
          )).send();
        break;
      }
  } 
  else if (memo.c_str() == string("insurance")) {
    eosio_assert(!user.insurance.empty(), "User does not have any insurance asset");
    for (std::vector<asset>::const_iterator it = user.insurance.begin() ; it < user.insurance.end(); ++it)
      if (it->symbol == assetout.symbol) {//User insurance type found
        eosio_assert( it->amount >= assetout.amount,
        "Insufficient insurance asset available." );
        if (it->amount - assetout.amount == 0)
          _user.modify(user, _self, [&]( auto& modified_user) {
            modified_user.insurance.erase(it);
          });
        else 
          _user.modify(user, _self, [&]( auto& modified_user) {
            modified_user.insurance[it - user.insurance.begin()].amount -= assetout.amount;
          });
        action( permission_level{_self, name("active")},
          issueracct[assetout.symbol], name("transfer"),
          std::make_tuple(_self, usern, assetout,
            std::string("Transfer insurance assets out: ") + usern.to_string()
        )).send();
        break;
      }
  } 
  else {
    eosio_assert( assetout.symbol == symbol("UZD",4), "Borrow asset type must be UZD" );
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
double eosusdcom::pricingmodel(name usern) {

  const auto& user = _user.get( usern.value, "User not found" ); 

  double weightsq_x_stdevsq = 0.0; 
  double weightN_x_stdevN = 1.0;
  uint64_t n = 0;

  auto it = user.collateral.begin();
  while ( it != user.collateral.end()) 
  { n += 1;
    auto sym_code_raw = it->symbol.code().raw();
    stats statstable( _self, sym_code_raw );
    const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );

    double stdevsq = std::pow(this->scale * st.volatility, 2);
    
    double value = (it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4));
    
    double weight = value / user.valueofcol;
    double weightsq = std::pow(weight, 2);  

    weightsq_x_stdevsq += weightsq * stdevsq;
    weightN_x_stdevN *= weight * this->scale * st.volatility;
    
    auto itr = it;

    while ( ++itr != user.collateral.end() )
      weightN_x_stdevN *= st.correlation_matrix.at(itr->symbol);
  }

  double iportVariance = weightsq_x_stdevsq + n * weightN_x_stdevN;

  // premium payments in exchange for contingient payoff in the event that a price threshhold is breached
  
  double iportVaR = std::min(3.0*iportVariance,1.0); // value at risk

  double payoff = std::max(1.0*(user.debt.amount/std::pow(10.0,4)) - user.valueofcol*(1-iportVaR),0.0);

  uint32_t T = 1;
  
  double d = ((std::log(user.valueofcol / (user.debt.amount/std::pow(10.0,4)))) + (-std::pow(iportVariance,2)/2) * T)/ (iportVariance * std::sqrt(T));

  double tesprice = std::max((payoff * std::erfc(-d/std::sqrt(2))/2)/(user.debt.amount/std::pow(10.0,4)),0.01*this->scale);
  tesprice = tesprice/(1.6*(user.creditscore/800.0)); // credit score of 500 means no discount or penalty.
  return tesprice;
}

void eosusdcom::payfee(name usern, double tesprice) {
  auto &user = _user.get( usern.value, "User not found" );

  uint64_t amt = 0;
  for ( auto it = user.collateral.begin(); it != user.collateral.end(); ++it )
    if (it->symbol == symbol("VIG",4)) {
      amt = (tesprice * std::pow(10.0,4)) / (fxrate[it->symbol]/std::pow(10.0,4));
      if (it->amount >= amt)
        _user.modify(user, _self, [&]( auto& modified_user) { // withdraw fee
          modified_user.feespaid += tesprice;
          modified_user.collateral[it - user.collateral.begin()].amount -= amt;
        });
      else 
        amt = 0;
      break;
    }
  if (!amt)
    _user.modify(user, _self, [&]( auto& modified_user) { // withdraw fee
      modified_user.latepayments += 1;
    });
  else 
    for ( auto itr = _user.begin(); itr != _user.end(); ++itr )
      if (itr->valueofins > 0) {
        double weight = itr->valueofins / this->totalins;
        for ( auto it = itr->insurance.begin(); it != itr->insurance.end(); ++it )
          if (it->symbol == symbol("VIG",4)) {
            _user.modify(itr, _self, [&]( auto& modified_user) { // weighted fee deposit
              modified_user.insurance[it - itr->insurance.begin()].amount += amt * weight;
            });
            break;
          } 
      }
}

void eosusdcom::update(name usern) {

  auto &user = _user.get( usern.value, "User not found" );

  double valueofcol = 0.0;
  for ( auto it = user.collateral.begin(); it != user.collateral.end(); ++it )
    valueofcol += (it->amount)/std::pow(10.0,it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0,4));
  
  _user.modify(user, _self, [&]( auto& modified_user) { // Update value of collateral
    modified_user.valueofcol = valueofcol;
  });

  double valueofins = 0.0;
  this->totalins -= user.valueofins;
  
  for ( auto it = user.insurance.begin(); it != user.insurance.end(); ++it )
    valueofins += (it->amount)/std::pow(10.0,it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0,4));
  
  this->totalins += valueofins;
  
  _user.modify(user, _self, [&]( auto& modified_user) { // Update value of insurance
    modified_user.valueofins = valueofins;
  });

  double tesprice = 0.0;
  if ( user.valueofcol > 0.0 && user.debt.amount > 0 ) { // Update tesprice
    tesprice = pricingmodel(usern);
    _user.modify(user, _self, [&]( auto& modified_user) {
      modified_user.tesprice = tesprice;
    });
    payfee(usern, tesprice);
  }
  if (user.latepayments > 4) {
    _user.modify(user, _self, [&]( auto& modified_user) {
      modified_user.latepayments = 0; 
      modified_user.recaps += 1;
    });
    bailout(usern);
  } else if ( 1.01 * ( user.debt.amount / std::pow(10.0,4) ) > valueofcol ) {
    _user.modify(user, _self, [&]( auto& modified_user) {
      modified_user.recaps += 1;
    });
    bailout(usern);
  }
}


/* illiquidity risk is offloaded to insurers who are compensated
 * to take this risk. insurers may start of with zero debt but in
 * a bailout they acquire it, along with a failed loan's remaining 
 * collateral. some of their insurance assets will be assigned to 
 * their collateral bucket so that it overcollateralizes their debt
 * at some default setting like 1.5
*/
void eosusdcom::bailout(name usern) {
  auto user = _user.find(usern.value);
  eosio_assert(user != _user.end(), "User not found");

  for ( auto itr = _user.begin(); itr != _user.end(); ++itr ) {
    if (itr->valueofins > 0) {
      double weight = itr->valueofins / this->totalins;

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
      //TODO: loop through insurances and assign some portion to collaterals
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


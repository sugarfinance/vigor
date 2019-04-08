#include "eosusdcom.hpp"
#include <cmath>

void eosusdcom::doupdate()
{
   require_auth( _self );

   usdtable eosusdtable(name("oracle111111"),name("oracle111111").value);
   auto iterator = eosusdtable.begin();
   fxrate[symbol("EOS",4)] = iterator->average;
   eosio::print( "EOS fxrate updated: ", iterator->average, "\n");

   user_t _user(_self, _self.value);
   for (auto it = _user.begin(); it != _user.end(); it++){
    update(it->usern);
    eosio::print( "update complete for: ", eosio::name{it->usern}, "\n");
   };
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
       s.correlation_matrix = dummy_corr;
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

    if (to == _self){
      auto &user = _user.get(from.value,"User not found");
      eosio_assert(quantity.symbol == symbol("UZD",4), "Debt asset type must be UZD");
      update(from);
      eosio_assert(user.debt.amount - quantity.amount >= 0, "Payment too high");
      // Transfer stablecoin into user
      _user.modify(user, _self, [&]( auto& modified_user) {
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

      action(
        permission_level{_self, name("active")},
        _self, 
        name("retire"),
        std::make_tuple(quantity, memo)
      ).send();
    } else {

    auto sym = quantity.symbol.code();
    stats statstable( _self, sym.raw() );
    const auto& st = statstable.get( sym.raw() );

    require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

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

void eosusdcom::assetin(name    from,
                      name    to,
                      asset   assetin,
                      string  memo ) {

  if (from == _self){
    return;
  }

  require_auth(from);
  eosio_assert(assetin.symbol.is_valid(), "Symbol must be valid.");
  eosio_assert(assetin.amount > 0, "Amount must be > 0.");
  eosio_assert(memo.c_str()==string("collateral") || memo.c_str()==string("insurance"), "memo must be composed of either word: insurance or collateral");
  
     // Create user, if not exist
  auto itr = _user.find(from.value);
  if (itr == _user.end()) {
    itr = _user.emplace(_self,  [&](auto& new_user) {
      new_user.usern = from;
      new_user.debt = asset(0,symbol("UZD", 4));
    });

      action(
        permission_level{_self, name("active")},
        _self, 
        name("open"),
        std::make_tuple(from, symbol("UZD", 4), _self)
      ).send();
  }

  auto &user = _user.get(from.value,"User not found");

  if (memo.c_str() == string("collateral")){
    for (std::vector<asset>::const_iterator it = user.collateral.begin() ; it <= user.collateral.end(); ++it){
      if (it == user.collateral.end()){
        //User collateral type not found
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.collateral.push_back(assetin);
        });
        break;
      }
      if (it->symbol == assetin.symbol){
        //User collateral type found
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.collateral[it - user.collateral.begin()].amount += assetin.amount;
        });
        break;
      }
    }
  } else if (memo.c_str() == string("insurance")){
    for (std::vector<asset>::const_iterator it = user.insurance.begin() ; it <= user.insurance.end(); ++it){
      if (it == user.insurance.end()){
        //User insurance type not found
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.insurance.push_back(assetin);
        });
        break;
      }
      if (it->symbol == assetin.symbol){
        //User insurance type found
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.insurance[it - user.insurance.begin()].amount += assetin.amount;
        });
        break;
      }
    }
  }
    update(from);
}

void eosusdcom::assetout(name usern, asset assetout, string memo) {

  require_auth(usern);

  auto &user = _user.get(usern.value,"User not found");
  eosio_assert(assetout.symbol.is_valid(), "Symbol must be valid.");
  eosio_assert(assetout.amount > 0, "Amount must be > 0.");
  eosio_assert(memo.c_str()==string("collateral") || memo.c_str()==string("insurance"), "memo must be composed of either word: insurance or collateral");

  if (memo.c_str() == string("collateral")){
    eosio_assert(!user.collateral.empty(), "User does not have any collateral");
    for (std::vector<asset>::const_iterator it = user.collateral.begin() ; it < user.collateral.end(); ++it){

      if (it->symbol == assetout.symbol){
        //User collateral type found

        eosio_assert((it->amount >= assetout.amount),"Insufficient collateral available.");
        eosio_assert(((user.valueofcol) - ((assetout.amount/std::pow(10.0,it->symbol.precision())) * (fxrate[assetout.symbol]/std::pow(10.0,4)))) >= 1.01*(user.debt.amount/std::pow(10.0,4)),

        "Dollar value of collateral would become less than dollar value of debt");

        if (it->amount - assetout.amount == 0){
          _user.modify(user, _self, [&]( auto& modified_user) {
            modified_user.collateral.erase(it);
          });
        } else {
          _user.modify(user, _self, [&]( auto& modified_user) {
            modified_user.collateral[it - user.collateral.begin()].amount -= assetout.amount;
          });
        }

      action(
        permission_level{_self, name("active")},
        issueracct[assetout.symbol], 
        name("transfer"),
        std::make_tuple(_self, usern, assetout,std::string("Transfer loan collateral out: ") + usern.to_string())
      ).send();
      update(usern);
      break;
      }
    }
  } else if (memo.c_str() == string("insurance")){
    eosio_assert(!user.insurance.empty(), "User does not have any insurance asset");
    for (std::vector<asset>::const_iterator it = user.insurance.begin() ; it < user.insurance.end(); ++it){

      if (it->symbol == assetout.symbol){
        //User insurance type found

        eosio_assert((it->amount >= assetout.amount),"Insufficient insurance asset available.");

        if (it->amount - assetout.amount == 0){
          _user.modify(user, _self, [&]( auto& modified_user) {
            modified_user.insurance.erase(it);
          });
        } else {
          _user.modify(user, _self, [&]( auto& modified_user) {
            modified_user.insurance[it - user.insurance.begin()].amount -= assetout.amount;
          });
        }

      action(
        permission_level{_self, name("active")},
        issueracct[assetout.symbol], 
        name("transfer"),
        std::make_tuple(_self, usern, assetout,std::string("Transfer insurance assets out: ") + usern.to_string())
      ).send();
      update(usern);
      break;
      }
    }
  }

}

void eosusdcom::borrow(name usern, asset debt) {

// todo: make this vectorized for multicollateral
  require_auth(usern);
  update(usern);
  auto &user = _user.get(usern.value,"User not found");
  eosio_assert(debt.symbol == symbol("UZD",4), "Debt asset type must be UZD");
  eosio_assert(user.valueofcol >= 1.01*((user.debt.amount + debt.amount)/std::pow(10.0,4)),
    "Dollar value of collateral would become less than dollar value of debt");

  // Transfer stablecoin out of user
  _user.modify(user, _self, [&]( auto& modified_user) {
    modified_user.debt += debt;
  });

 // issue(usern, debt,std::string("UZD issued to ") + usern.to_string());
    action(
        permission_level{_self, name("active")},
        _self, 
        name("issue"),
        std::make_tuple(usern, debt, std::string("UZD issued to ") + usern.to_string())
    ).send();
    update(usern);
}

/* Portfolio variance is a measurement of how the aggregate actual returns
 * of a set of securities making up a portfolio fluctuate over time. This
 * portfolio variance statistic is calculated using the standard deviations
 * of each security in the portfolio as well as the correlations of each 
 * security pair in the portfolio.
*/

double eosusdcom::pricingmodel(name usern) {
//double eosusdcom::pricingmodel(double scale, double collateral, asset debt, double stdev, uint64_t creditscore) {

auto &user = _user.get(usern.value,"User not found"); 

/* E.g. for two assets Variance = 
   [(weight_asset1)^2 x (stdev_asset1)^2] +  
   [(weight_asset2)^2 x (stdev_asset2)^2] +  
   (2 x weight_asset1 x stdev_asset1 x 
        weight_asset2 x stdev_asset2 x 
   the correlation between the two assets 
*/

double weightsq_x_stdevsq = 0; 
double n_x_weightN_x_stdevN = 1;
uint64_t n = 0;

auto it = user.collateral.begin();
while ( it != user.collateral.end()) 
{
  auto sym_code_raw = it->symbol.code().raw();
  stats statstable( _self, sym_code_raw );
  const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );

  n += 1;

  double stdevsq = std::pow(st.volatility, 2);
  double value = (it->amount)/std::pow(10.0, it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0, 4));
  
  double weight = value / user.valueofcol;
  double weightsq = std::pow(weight, 2);  

  weightsq_x_stdev += weightsq * stdevsq;
  n_x_weightN_x_stdevN *= weight * st.volatility;
  
  auto itr = it;

  while(++itr != user.collateral.end())
    n_x_weightN_x_stdevN *= st.correlation_matrix[itr->symbol];
}

return weightsq_x_stdev + n_x_weightN_x_stdevN;
  

// premium payments in exchange for contingient payoff in the event that a price threshhold is breached
// todo: make this vectorized for multicollateral
double impliedvol = stdev * scale;



double valueatrisk = std::min(3.0*impliedvol,1.0);
double payoff = std::max(1.0*(debt.amount/std::pow(10.0,4)) - collateral*(1-valueatrisk),0.0);
uint32_t T = 1;
double d = ((std::log(collateral / (debt.amount/std::pow(10.0,4)))) + (-std::pow(impliedvol,2)/2) * T)/ (impliedvol * std::sqrt(T));
double tesprice = std::max((payoff * std::erfc(-d/std::sqrt(2))/2)/(debt.amount/std::pow(10.0,4)),0.01*scale);
tesprice = tesprice/(1.6*(creditscore/800.0)); // credit score of 500 means no discount or penalty.
return tesprice;

}

void eosusdcom::update(name usern){

  auto &user = _user.get(usern.value,"User not found");

  double valueofcol = 0.0;
    for (std::vector<asset>::const_iterator it = user.collateral.begin() ; it != user.collateral.end(); ++it){
      valueofcol += (it->amount)/std::pow(10.0,it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0,4));
    }
    // Update value of collateral
    _user.modify(user, _self, [&]( auto& modified_user) {
      modified_user.valueofcol = valueofcol;
      });

  double valueofins = 0.0;
    for (std::vector<asset>::const_iterator it = user.insurance.begin() ; it != user.insurance.end(); ++it){
      valueofins += (it->amount)/std::pow(10.0,it->symbol.precision()) * (fxrate[it->symbol]/std::pow(10.0,4));
    }

    // Update value of insurance
    _user.modify(user, _self, [&]( auto& modified_user) {
      modified_user.valueofins = valueofins;
      });

  double tesprice = 0.0;
  if (user.valueofcol>0.0 && user.debt.amount>0){
  double tesprice = pricingmodel(this->scale, user.valueofcol, user.debt, .1, user.creditscore);
    // Update tesprice
    _user.modify(user, _self, [&]( auto& modified_user) {
      modified_user.tesprice = tesprice;
      });
  }
 // payfee(name,tesprice);


  //BAILOUT
  //assign the debt and impaired collateral to the insurers
  
  /* insurers may start of with zero debt but in a bailout 
  * they get some. When that happens, some of their insurance
  * assets will be assigned to their collateral bucket so that
  * it overcollateralizes their debt at some default setting like 1.5.
  * insurers receive premium in exchange for agreeing to bailout loans
  * so they willfully take it, a bailout means that the insurer is assigned
  * a small piece of debt and impaired collateral. All users have three stacks
  * of tokens: collateral, insurance, debt.
  */

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
                EOSIO_DISPATCH_HELPER(eosusdcom, (create)(assetout)(borrow)(issue)(transfer)(open)(close)(retire)(setsupply)(doupdate)) 
            }    
    }
        eosio_exit(0);
  }
}


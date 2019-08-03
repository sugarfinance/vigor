#include "eosusdcom.hpp"

void eosusdcom::doupdate()
{
   //require_auth(_self);

  for ( auto it = _user.begin(); it != _user.end(); it++ ) {
    update(it->usern);
  }

  for ( auto it = _user.begin(); it != _user.end(); it++ ) {
    stresscol(it->usern);
  }

  stressins();

  risk();

  for ( auto it = _user.begin(); it != _user.end(); it++ ) {
    pricing(it->usern);
  }

  //double rm = RM();
  //for ( auto it = _user.begin(); it != _user.end(); it++ ) {
  //  pcts(it->usern,rm);
 // }

}

void eosusdcom::create( name   issuer,
                        asset  maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    auto existing = _stats.find( sym.code().raw() );
    eosio_assert( existing == _stats.end(), "token with symbol already exists" );

    _stats.emplace( _self, [&]( auto& s ) {
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

    auto existing = _stats.find( sym.code().raw() );
    eosio_assert( existing != _stats.end(), "token with symbol does not exist, create token before setting supply" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( maximum_supply.is_valid(), "invalid maximum_supply" );
    eosio_assert( maximum_supply.amount > 0, "must issue positive maximum_supply" );

    eosio_assert( maximum_supply.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( maximum_supply.amount >= st.supply.amount, "cannot set max_supply to less than available supply");

    _stats.modify( st, same_payer, [&]( auto& s ) {
       s.max_supply = maximum_supply;
    });
}

void eosusdcom::issue( name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto existing = _stats.find( sym.code().raw() );
    eosio_assert( existing != _stats.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    _stats.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
   //   SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
     //                     { st.issuer, to, quantity, memo }
     // );
    }
}

void eosusdcom::retire( asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto existing = _stats.find( sym.code().raw() );
    eosio_assert( existing != _stats.end(), "token with symbol does not exist 6" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must retire positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    _stats.modify( st, same_payer, [&]( auto& s ) {
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

    auto payer = has_auth( to ) ? to : from;

    if (to == _self && quantity.symbol == symbol("VIGOR", 4)) {
      auto &user = _user.get(from.value,"User not found");
      
      eosio_assert(user.debt.amount >= quantity.amount, "Payment too high");
      
      eosio_assert(_globals.exists(), "");
      globalstats gstats = _globals.get();

      _user.modify(user, _self, [&]( auto& modified_user) { // Transfer stablecoin into user
        modified_user.debt -= quantity;
      });
      
      gstats.totaldebt -= quantity;
      
      double totdebt = gstats.totaldebt.amount / std::pow(10.0, 4);
      eosio::print( "totdebt : ",  totdebt, "\n");

      _globals.set(gstats, _self);

      //clear the debt from circulating supply
      action(permission_level{_self, name("active")}, _self, 
        name("retire"), std::make_tuple(quantity, memo)
      ).send();
      
      doupdate();
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
   if( to == to_acnts.end() )
    to_acnts.emplace( ram_payer, [&]( auto& a ){
      a.balance = value;
    });
   else
    to_acnts.modify( to, same_payer, [&]( auto& a ) {
      a.balance += value;
    });
}

void eosusdcom::open( name owner, const symbol& symbol, name ram_payer )
{
   require_auth( ram_payer );

   auto sym_code_raw = symbol.code().raw();
   const auto& st = _stats.get( sym_code_raw, "symbol does not exist 7" );

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
  eosio_assert( memo.c_str() == string("support") ||
                memo.c_str() == string("collateral"),  
                "memo must be composed of either word: support or collateral"
              );
  auto itr = _user.find(from.value);
  if ( itr == _user.end() ) {
    itr = _user.emplace(_self, [&](auto& new_user) {
      new_user.usern = from;
      new_user.creditscore = 500;
      new_user.lastupdate = now();
      new_user.debt = asset( 0, symbol("VIGOR", 4) );
    });
    action( permission_level{ _self, name("active") },
      _self, name("open"), std::make_tuple(
        from, symbol("VIGOR", 4), _self
      )).send();
  }

  auto &user = *itr;
  bool found = false;

  globalstats gstats;
  if (_globals.exists())
    gstats = _globals.get();

  symbol sym = assetin.symbol;
  auto st = _stats.find( sym.code().raw());

  if ( st == _stats.end() )        
    _stats.emplace( _self, [&]( auto& s ) {
      s.supply.symbol = sym;
      s.max_supply.symbol = sym;
      s.issuer = get_code(); //TODO: verify against issuer map
    });

  if (memo.c_str() == string("support")) {
    auto it = user.support.begin();
    while ( !found && it++ != user.support.end() )
      found = (it-1)->symbol == assetin.symbol;//User collateral type found
    _user.modify(user, _self, [&]( auto& modified_user) {
      if (!found)
        modified_user.support.push_back(assetin);
      else
        modified_user.support[(it-1) - user.support.begin()] += assetin;
    }); found = false;
    it = gstats.support.begin();
    while ( !found && it++ != gstats.support.end() )
      found = (it-1)->symbol == assetin.symbol;
    if ( !found )
      gstats.support.push_back(assetin);
    else
      gstats.support[(it-1) - gstats.support.begin()] += assetin;
  }
  else {
    auto it = user.collateral.begin();
    while ( !found && it++ != user.collateral.end() )
      found = (it-1)->symbol == assetin.symbol; //User collateral type found
    _user.modify(user, _self, [&]( auto& modified_user) {
      if (!found)
        modified_user.collateral.push_back(assetin);
      else
        modified_user.collateral[(it-1) - user.collateral.begin()] += assetin;
    }); found = false;
    it = gstats.collateral.begin();
    while ( !found && it++ != gstats.collateral.end() )
      found = (it-1)->symbol == assetin.symbol;
    if (!found)
      gstats.collateral.push_back(assetin);
    else
      gstats.collateral[(it-1) - gstats.collateral.begin()] += assetin;
  } 
  _globals.set(gstats, _self);
  doupdate();
}

void eosusdcom::assetout(name usern, asset assetout, string memo) 
{
  require_auth(usern);

  auto &user = _user.get( usern.value,"User not found" );
  eosio_assert( assetout.symbol.is_valid(), "Symbol must be valid." );
  eosio_assert( assetout.amount > 0, "Amount must be > 0." );
  eosio_assert( memo.c_str() == string("collateral") || 
                memo.c_str() == string("support") || 
                memo.c_str() == string("borrow"), 
                "memo must be composed of either word: support | collateral | borrow"
              );
  eosio_assert(_globals.exists(), "globals don't exist");
  globalstats gstats = _globals.get();
  bool found = false;

  if ( memo.c_str() == string("borrow") ) {
    eosio_assert( assetout.symbol == symbol("VIGOR", 4), 
                  "Borrow asset type must be VIGOR" 
                );
    asset debt = user.debt + assetout;
    /*
    * if overcollateralization is C then leverage L is found as 
    * L = 1 / ( 1 - ( 1 / C ) )
    */
    eosio_assert( user.valueofcol >= 1.11 * ( debt.amount / std::pow(10.0, 4) ),
    "Dollar value of collateral would become less than dollar value of debt" );
    
    _user.modify(user, _self, [&]( auto& modified_user) {
      modified_user.debt = debt;
    });
    gstats.totaldebt += assetout;

    double totdebt = gstats.totaldebt.amount / std::pow(10.0, 4);
    eosio::print( "totdebt : ",  totdebt, "\n");

    action( permission_level{_self, name("active")},
      _self, name("issue"), std::make_tuple(
        usern, assetout, std::string("VIGOR issued to ") + usern.to_string()
      )).send();
  }
  else {
    if ( memo.c_str() == string("support") ) {
      for ( auto it = user.support.begin(); it < user.support.end(); ++it )
        if (it->symbol == assetout.symbol) { // User support type found
          eosio_assert( it->amount >= assetout.amount,
          "Insufficient support assets available." );
          if ( it->amount - assetout.amount == 0 )
            _user.modify(user, _self, [&]( auto& modified_user) {
              modified_user.support.erase(it);
            });
          else 
            _user.modify(user, _self, [&]( auto& modified_user) {
              modified_user.support[it - user.support.begin()] -= assetout;
            });
          found = true;
          break;
        }
      for ( auto it = gstats.support.begin(); it != gstats.support.end(); ++it )
        if ( it->symbol == assetout.symbol ) {
          gstats.support[it - gstats.support.begin()] -= assetout;
          break;
        }
    }
    else {
      for ( auto it = user.collateral.begin(); it != user.collateral.end(); ++it )
        if ( it->symbol == assetout.symbol ) { //User collateral type found
          eosio_assert( it->amount >= assetout.amount,
          "Insufficient collateral assets available." );
          
          double valueofasset = assetout.amount / std::pow(10.0, it->symbol.precision());

          statstable stats(name("datapreproc1"),name(issuerfeed[assetout.symbol]).value);
          auto itr = stats.find(1);
          valueofasset *= (double)itr->price[0] / pricePrecision;

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
          found = true;
          break;
        }
      for ( auto it = gstats.collateral.begin(); it != gstats.collateral.end(); ++it ) 
        if ( it->symbol == assetout.symbol ) {
          gstats.collateral[it - gstats.collateral.begin()] -= assetout;
          break;
        }  
    }
    eosio_assert(found, "asset not found in user");
    memo += std::string("assets to be transfered out for: ") + usern.to_string();
    action( permission_level{_self, name("active")},
            issueracct[assetout.symbol], name("transfer"),
            std::make_tuple(_self, usern, assetout, memo
          )).send(); 
  } 
  _globals.set(gstats, _self);
  doupdate();
}

void eosusdcom::stresscol(name usern) {

  const auto& user = _user.get( usern.value, "User not found" );  
  
  eosio_assert(_globals.exists(), "globals not found");
  globalstats gstats = _globals.get();

  double portVariance = portVarianceCol(usern);
  double stresscol = -1.0*(std::exp(-1.0*stressQuantile * std::sqrt(portVariance))-1.0);
  double istresscol = -1.0*(std::exp(-1.0*stressQuantile * std::sqrt(portVariance) * gstats.scale)-1.0);
  double svalueofcol = ((1.0 - stresscol) * user.valueofcol);
  double svalueofcole = std::max( 0.0,
    user.debt.amount / std::pow(10.0, 4) - ((1.0 - stresscol) * user.valueofcol)
  );

  gstats.svalueofcole += svalueofcole - user.svalueofcole; // model suggested dollar value of the sum of all insufficient collateral in a stressed market
  
  _globals.set(gstats, _self);

  _user.modify(user, _self, [&]( auto& modified_user) { 
    modified_user.volcol = std::sqrt(portVariance); // volatility of the user collateral portfolio
    modified_user.stresscol = stresscol; // model suggested percentage loss that the user collateral portfolio would experience in a stress event.
    modified_user.istresscol = istresscol; // market determined implied percentage loss that the user collateral portfolio would experience in a stress event.
    modified_user.svalueofcol = svalueofcol; // model suggested dollar value of the user collateral portfolio in a stress event.
    modified_user.svalueofcole = svalueofcole; // model suggested dollar amount of insufficient collateral of a user loan in a stressed market.
  });

  }

  double eosusdcom::portVarianceCol(name usern) {
  
  const auto& user = _user.get( usern.value, "User not found" );  
  
  eosio_assert(_globals.exists(), "globals not found");
  globalstats gstats = _globals.get();
  
  double portVariance = 0.0;
  for ( auto i = user.collateral.begin(); i != user.collateral.end(); ++i ) {
    auto sym_code_raw = i->symbol.code().raw();
    const auto& iV = _stats.get( sym_code_raw, "symbol does not exist" );
    
    statstable stats(name("datapreproc1"),name(issuerfeed[i->symbol]).value);
    auto itr = stats.find(1);
    double iVvol = (double)itr->vol/volPrecision;
    //eosio::print( "iVvol : ", iVvol, "\n");
    double iW = (double)itr->price[0] / pricePrecision;
    iW *= i->amount / std::pow(10.0, i->symbol.precision()); 
    iW /= user.valueofcol;

    for (auto j = i + 1; j != user.collateral.end(); ++j ) {
      double c = (double)itr->correlation_matrix.at(j->symbol)/corrPrecision;
      //double c = iV.correlation_matrix.at(j->symbol);
      sym_code_raw = j->symbol.code().raw();
      const auto& jV = _stats.get( sym_code_raw, "symbol does not exist" );

      statstable statsj(name("datapreproc1"),name(issuerfeed[j->symbol]).value);
      auto itr = statsj.find(1);
      double jVvol = (double)itr->vol/volPrecision;
      double jW = (double)itr->price[0] / pricePrecision;
      jW *= j->amount / std::pow(10.0, j->symbol.precision());
      jW /= user.valueofcol;

      portVariance += 2.0 * iW * jW * c * iVvol * jVvol;
    }
    portVariance += std::pow(iW, 2) * std::pow(iVvol, 2);
  }

  //eosio::print( "portVariance : ", portVariance, "\n");
  return portVariance;
}

void eosusdcom::stressins()
{
  eosio_assert( _globals.exists(), "No support yet" );
  globalstats gstats = _globals.get();

  double portVariance = 0.0;

  for ( auto i = gstats.support.begin(); i != gstats.support.end(); ++i ) {
    auto sym_code_raw = i->symbol.code().raw();
    const auto& iV = _stats.get( sym_code_raw, "symbol does not exist" );

    statstable stats(name("datapreproc1"),name(issuerfeed[i->symbol]).value);
    auto itr = stats.find(1);
    double iVvol = (double)itr->vol/volPrecision;
    double iW = (double)itr->price[0] / pricePrecision;
    iW *= i->amount / std::pow(10.0, i->symbol.precision());
    iW /=  gstats.valueofins;

    for (auto j = i + 1; j != gstats.support.end(); ++j ) {
      double c = (double)itr->correlation_matrix.at(j->symbol)/corrPrecision;
      //double c = iV.correlation_matrix.at(j->symbol);
      
      sym_code_raw = j->symbol.code().raw();
      const auto& jV = _stats.get( sym_code_raw, "symbol does not exist" );

      statstable stats(name("datapreproc1"),name(issuerfeed[j->symbol]).value);
      auto itr = stats.find(1);
      double jVvol = (double)itr->vol/volPrecision;
      double jW = (double)itr->price[0] / pricePrecision;
      jW *= j->amount / std::pow(10.0, j->symbol.precision());
      jW /=  gstats.valueofins; 

      portVariance += 2.0 * iW * jW * c * iVvol * jVvol;
    }
    portVariance += std::pow(iW, 2) * std::pow(iVvol, 2);
  }
  //eosio::print( "portVarianceins : ", portVariance, "\n");
  
  double stressins = -1.0*(std::exp(-1.0*stressQuantile * std::sqrt(portVariance))-1.0); // model suggested percentage loss that the total insurance asset portfolio would experience in a stress event.
  gstats.stressins = stressins;
  gstats.svalueofins = (1.0 - stressins) * gstats.valueofins; // model suggested dollar value of the total insurance asset portfolio in a stress event.

  _globals.set(gstats, _self);
}

void eosusdcom::risk()
{
  eosio_assert( _globals.exists(), "No support yet" );
  globalstats gstats = _globals.get();

  // market value of assets and liabilities from the perspective of insurers

  // normal markets
  double mva_n = gstats.valueofins; //market value of insurance assets in normal markets, collateral is not an asset of the insurers
  double mvl_n = 0; // no upfront is paid for tes, and insurers can walk away at any time, debt is not a liability of the insurers
  
  //stressed markets
  double mva_s = gstats.svalueofins;
  double mvl_s = gstats.svalueofcole;

  double own_n = mva_n - mvl_n;
  double own_s = mva_s - mvl_s;
  
  double scr = own_n - own_s;

  //eosio::print( "own_n : ", own_n, "\n");
  //eosio::print( "own_s : ", own_s, "\n");
  //eosio::print( "scr : ", scr, "\n");
  
  double solvency = own_n / scr;

  gstats.solvency = solvency;
  eosio::print( "solvency : ", solvency, "\n");

  gstats.scale = std::min(std::max(solvencyTarget/solvency,0.5),2.0);
  eosio::print( "scale : ", gstats.scale, "\n");

  _globals.set(gstats, _self);
}

void eosusdcom::pricing(name usern) {
/* premium payments in exchange for contingient payoff in 
 * the event that a price threshhold is breached
*/
  const auto& user = _user.get( usern.value, "User not found" );  
  
  eosio_assert(_globals.exists(), "globals not found");
  globalstats gstats = _globals.get();

  double ivol = user.volcol * gstats.scale; // market determined implied volaility

  double payoff = std::max(  0.0,
    1.0 * (user.debt.amount / std::pow(10.0,4)) - user.valueofcol * (1.0 - user.istresscol)
  );
  double T = 1.0;
  double d = ((std::log(user.valueofcol / (user.debt.amount/std::pow(10.0,4)))) + (-std::pow(ivol,2)/2) * T)/ (ivol * std::sqrt(T));

  double tesprice = std::max( 0.005 * gstats.scale,
    (payoff * std::erfc(-d / std::sqrt(2)) / 2) / (user.debt.amount / std::pow(10.0, 4))
  );

  tesprice /= 1.6 * (user.creditscore / 800.0); // credit score of 500 means no discount or penalty.

  _globals.set(gstats, _self);

  _user.modify(user, _self, [&]( auto& modified_user) { 
    modified_user.tesprice = tesprice; // annualized rate borrowers pay in periodic premiums to insure their collateral
    modified_user.lastupdate = now();
  });
}

double eosusdcom::stressinsx(name usern) {

  const auto& user = _user.get( usern.value, "User not found" );  

  eosio_assert( _globals.exists(), "No support yet" );
  globalstats gstats = _globals.get();

  double portVariancex = 0.0;

  for ( auto i = gstats.support.begin(); i != gstats.support.end(); ++i ) {
    auto sym_code_raw = i->symbol.code().raw();
    const auto& iV = _stats.get( sym_code_raw, "symbol does not exist" );

    statstable stats(name("datapreproc1"),name(issuerfeed[i->symbol]).value);
    auto itr = stats.find(1);
    double iVvol = (double)itr->vol/volPrecision;
    double iW = (double)itr->price[0] / pricePrecision;
    double uW = (double)itr->price[0] / pricePrecision;
    iW *= i->amount / std::pow(10.0, i->symbol.precision());
    for ( auto u = user.support.begin(); u != user.support.end(); ++u ) {
      if (u->symbol==i->symbol){
        uW *= u->amount / std::pow(10.0, i->symbol.precision());
        iW -= uW;
        break;
      }
    }
      iW /=  (gstats.valueofins - user.valueofins);

    for (auto j = i + 1; j != gstats.support.end(); ++j ) {
      double c = (double)itr->correlation_matrix.at(j->symbol)/corrPrecision;
      //double c = iV.correlation_matrix.at(j->symbol);
      
      sym_code_raw = j->symbol.code().raw();
      const auto& jV = _stats.get( sym_code_raw, "symbol does not exist" );

      statstable stats(name("datapreproc1"),name(issuerfeed[j->symbol]).value);
      auto itr = stats.find(1);
      double jVvol = (double)itr->vol/volPrecision;
      double jW = (double)itr->price[0] / pricePrecision;
      double uW = (double)itr->price[0] / pricePrecision;
      jW *= j->amount / std::pow(10.0, j->symbol.precision());
      for ( auto u = user.support.begin(); u != user.support.end(); ++u ) {
        if (u->symbol==j->symbol) {
          uW *= u->amount / std::pow(10.0, j->symbol.precision());
          jW -= uW;
          break;
        }
      }
        jW /=  (gstats.valueofins - user.valueofins);

      portVariancex += 2.0 * iW * jW * c * iVvol * jVvol;
    }
    portVariancex += std::pow(iW, 2) * std::pow(iVvol, 2);
  }
  //eosio::print( "portVarianceinsx : ", portVariancex, "\n");
  
  double stressinsx = -1.0*(std::exp(-1.0*stressQuantile * std::sqrt(portVariancex))-1.0); // model suggested percentage loss that the total insurance asset portfolio would experience in a stress event.
  double svalueofinsx = (1.0 - stressinsx) * (gstats.valueofins  - user.valueofins); // model suggested dollar value of the total insurance asset portfolio in a stress event.
  
  return svalueofinsx;
}

double eosusdcom::riskx(name usern)
{
  eosio_assert( _globals.exists(), "No support yet" );
  globalstats gstats = _globals.get();

  // market value of assets and liabilities from the perspective of insurers

  // normal markets
  double mva_n = gstats.valueofins; //market value of insurance assets in normal markets, collateral is not an asset of the insurers
  double mvl_n = 0; // no upfront is paid for tes, and insurers can walk away at any time, debt is not a liability of the insurers
  
  //stressed markets
  double mva_s = stressinsx(usern);
  double mvl_s = gstats.svalueofcole;

  double own_n = mva_n - mvl_n;
  double own_s = mva_s - mvl_s;
  
  double scr = own_n - own_s;

  //eosio::print( "own_nx : ", own_n, "\n");
  //eosio::print( "own_sx : ", own_s, "\n");
  //eosio::print( "scrx : ", scr, "\n");
  
  double solvencyx = own_n / scr;
  eosio::print( "solvencyx : ", solvencyx, "\n");

  return solvencyx;
}



double eosusdcom::RM() { // sum of weighted marginal contribution to risk (solvency)

  eosio_assert( _globals.exists(), "No support yet" );
  globalstats gstats = _globals.get();

  double smctr = 0;
  for ( auto it = _user.begin(); it != _user.end(); it++ ) {
    double solvencyx = riskx(it->usern);
    double dRMdw =  (gstats.solvency - solvencyx);
    double w =  it->valueofins / gstats.valueofins;
    smctr +=  w * dRMdw;
  }
  return smctr;
}

double eosusdcom::pcts(name usern, double RM) { // percent contribution to risk (solvency)

  const auto& user = _user.get( usern.value, "User not found" );
  eosio_assert( _globals.exists(), "No support yet" );
  globalstats gstats = _globals.get();

  double solvencyx = riskx(usern);
  double w =  user.valueofins / gstats.valueofins;
  double dRMdw =  (gstats.solvency - solvencyx);
  double pcts =  w * dRMdw / RM; // weighted marginal contribution to risk (solvency) rescaled by sum of that, aka percent contribution to risk (where our risk measure is solvency)
  //eosio::print( "user : ", usern, "\n");
  //eosio::print( "gstats.solvency: ", gstats.solvency, "\n");
  //eosio::print( "dRMdw: ", dRMdw, "\n");
  //eosio::print( "x : ", w, "\n");
  //eosio::print( "percent contribution to risk : ", pcts, "\n");
  return pcts;
}

void eosusdcom::payfee(name usern) {
  auto &user = _user.get( usern.value, "User not found" );
  eosio_assert(_globals.exists(), "No support yet");
  globalstats gstats = _globals.get();

  bool late = true;
  uint64_t amt = 0;
  symbol vig = symbol("VIG", 4);
  uint32_t dsec = now() - user.lastupdate + 1; //+1 to protect against 0
  uint32_t T = (uint32_t)(360.0 * 24.0 * 60.0 * (60.0 / (double)dsec));
  double tespay = (user.debt.amount / std::pow(10.0, 4)) * (std::pow((1 + user.tesprice), (1.0 / T)) - 1);
  for ( auto it = user.collateral.begin(); it != user.collateral.end(); ++it )
    if ( it->symbol ==  vig) {
      const auto& st = _stats.get( vig.code().raw(), "symbol doesn't exist");
      statstable stats(name("datapreproc1"),name(issuerfeed[vig]).value);
      auto itr = stats.find(1);
      amt = uint64_t(( tespay * std::pow(10.0, 4) ) / 
            ((double)itr->price[0] / pricePrecision));
      if (amt > it->amount)
        _user.modify(user, _self, [&]( auto& modified_user) { // withdraw fee
          modified_user.latepays += 1;
        });
      else if (amt > 0) {
        _user.modify(user, _self, [&]( auto& modified_user) { // withdraw fee
          modified_user.feespaid += amt / std::pow(10.0, 4);
          modified_user.collateral[it - user.collateral.begin()].amount -= amt;
          eosio::print( "number of VIG paid : ", amt / std::pow(10.0, 4), "\n");
        });
        for ( auto itr = gstats.collateral.begin(); itr != gstats.collateral.end(); ++itr )
          if ( itr->symbol == vig ) {
            gstats.support[itr - gstats.support.begin()].amount -= amt;
            break;
          }
        late = false;
      }
      break;
    }
  if (!late) {
    uint64_t res = amt * 0.25;
    gstats.inreserve += res;
    _globals.set(gstats, _self);
    
    amt *= 0.75; 
    for ( auto itr = _user.begin(); itr != _user.end(); ++itr )
      if ( itr->valueofins > 0 ) {
        //double weight = itr->valueofins / gstats.valueofins;
        double weight = pcts(itr->usern,RM());
        eosio::print( "percent contribution to risk : ", weight, "\n");
        asset viga = asset(amt * weight, vig);
        bool found = false;
        for ( auto it = itr->support.begin(); it != itr->support.end(); ++it )
          if ( it->symbol == vig ) {
            _user.modify( itr, _self, [&]( auto& modified_user ) { // weighted fee deposit
              modified_user.support[it - itr->support.begin()] += viga;
            });
            found = true;
            break;
          } 
        if (!found)
          _user.modify(itr, _self, [&]( auto& modified_user) {
            modified_user.support.push_back(viga);
          });
      }
    for ( auto it = gstats.support.begin(); it != gstats.support.end(); ++it )
      if ( it->symbol == vig ) {
        gstats.support[it - gstats.support.begin()].amount += amt;
        break;
      }
  }
}

void eosusdcom::update(name usern) 
{
  eosio_assert(_globals.exists(), "globals not found");
  auto &user = _user.get(usern.value, "User not found");
  globalstats gstats = _globals.get();

  double valueofins = 0.0;
  double valueofcol = 0.0;
  
  for ( auto it = user.support.begin(); it != user.support.end(); ++it ) {
    statstable stats(name("datapreproc1"),name(issuerfeed[it->symbol]).value);
    auto itr = stats.find(1);
    valueofins += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( (double)itr->price[0] / pricePrecision );
  }
  for ( auto it = user.collateral.begin(); it != user.collateral.end(); ++it ){
    statstable statsj(name("datapreproc1"),name(issuerfeed[it->symbol]).value);
    auto itr = statsj.find(1);
    valueofcol += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( (double)itr->price[0] / pricePrecision );
  }
  gstats.valueofins += valueofins - user.valueofins;
  gstats.valueofcol += valueofcol - user.valueofcol;
  _globals.set(gstats, _self);

  _user.modify( user, _self, [&]( auto& modified_user ) { // Update value of collateral
    modified_user.valueofins = valueofins;
    modified_user.valueofcol = valueofcol;
  });
  
  if ( user.valueofcol > 0.0 && user.debt.amount > 0 ) { // Update tesprice    
    payfee(usern);
    

    if (user.latepays > 4) {
      _user.modify(user, _self, [&]( auto& modified_user) {
        modified_user.latepays = 0; 
        modified_user.recaps += 1;
      });
      bailout(usern);
    } 
    else if (( user.debt.amount / std::pow(10.0, 4) ) > user.valueofcol ) {
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
void eosusdcom::bailout(name usern) 
{
  eosio_assert(_globals.exists(), "globals not found");
  auto &user = _user.get(usern.value, "User not found");
  globalstats gstats = _globals.get();

  for ( auto itr = _user.begin(); itr != _user.end(); ++itr ) {
    if (itr->valueofins > 0) {
      double weight = itr->valueofins / gstats.valueofins;
      asset debt = user.debt;
      debt.amount *= weight;

      for ( auto c = user.collateral.begin(); c != user.collateral.end(); ++c ) {
        asset amt = *c;
        amt.amount *= weight;
        _user.modify(user, _self, [&]( auto& modified_user) { // weighted fee withdrawl
            modified_user.collateral[c - user.collateral.begin()] -= amt;
        });
        for ( auto it = itr->collateral.begin(); it != itr->collateral.end(); ++it )
          if (it->symbol == c->symbol) {
            _user.modify(itr, _self, [&]( auto& modified_user) { // weighted fee deposit
              modified_user.collateral[it - itr->collateral.begin()] += amt;
            });
            amt.amount = 0;
            break;
          } 
        if (amt.amount > 0) 
          _user.modify(itr, _self, [&]( auto& modified_user) { // weighted fee deposit
            modified_user.collateral.push_back(amt);
          });
      }
      _user.modify(itr, _self, [&]( auto& modified_user) { // weighted fee deposit
        modified_user.debt += debt;
      });
      for ( auto i = itr->support.begin(); i != itr->support.end(); ++i ) {
        asset amt = *i;
        amt.amount *= 0.25; //convert some support into collateral
        _user.modify(itr, _self, [&]( auto& modified_user) { // weighted fee withdrawl
            modified_user.support[i - itr->support.begin()] -= amt;
        });
        for ( auto it = itr->collateral.begin(); it != itr->collateral.end(); ++it )
          if ( it->symbol == i->symbol ) {
            _user.modify(itr, _self, [&]( auto& modified_user) { // weighted fee deposit
              modified_user.collateral[it - itr->collateral.begin()] += amt;
            });
            amt.amount = 0;
            break;
          } 
        if (amt.amount > 0) 
          _user.modify(itr, _self, [&]( auto& modified_user) { // weighted fee deposit
            modified_user.collateral.push_back(amt);
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
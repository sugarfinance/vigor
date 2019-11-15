#include <vigor.hpp>

//#include <vigor.hpp>
#include <boost/math/special_functions/erf.hpp>
using boost::math::erfc_inv;

void vigor::doupdate()
{
   //require_auth(_self);

  for ( auto it = _user.begin(); it != _user.end(); it++ )
    update(it->usern);
  updateglobal();

  //for ( auto it = _user.begin(); it != _user.end(); it++ ) {
 //   if ( it->debt.amount > 0 ) 
     // payfee(it->usern);
 // }

  for ( auto it = _user.begin(); it != _user.end(); it++ ) 
    update(it->usern);
  updateglobal();

bool exitbailout;
for (int i=1; i<11; i++){
  eosio::print( "bailout loop count: ", i, "\n");
  exitbailout = true;
  for ( auto it = _user.begin(); it != _user.end(); it++ ) {
    auto &user = _user.get(it->usern.value,"User not found14");
      if (it->debt.amount > 0 && it->latepays > 4) {
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.latepays = 0; 
          modified_user.recaps += 1;
        });
        bailout(it->usern);
        exitbailout=false;
        break;
      }
      else if (( it->debt.amount / std::pow(10.0, 4) ) > it->valueofcol ) {
        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.recaps += 1;
        });
        bailout(it->usern);
        exitbailout=false;
        break;
      }
  //  for ( auto itr = _user.begin(); itr != _user.end(); itr++ )
   //   update(itr->usern);
  //  updateglobal();
  }

  for ( auto itr = _user.begin(); itr != _user.end(); itr++ )
    update(itr->usern);
  updateglobal();
  
  for ( auto it = _user.begin(); it != _user.end(); it++ )
    stresscol(it->usern);
  
  for ( auto it = _user.begin(); it != _user.end(); it++ )
    l_stresscol(it->usern);

  stressins();

  risk();
  l_risk();

  for ( auto it = _user.begin(); it != _user.end(); it++ ) 
    pricing(it->usern);

  for ( auto it = _user.begin(); it != _user.end(); it++ ) 
    l_pricing(it->usern);

  double rm = RM();
  for ( auto it = _user.begin(); it != _user.end(); it++ )
    pcts(it->usern,rm);

  double l_rm = l_RM();
  for ( auto it = _user.begin(); it != _user.end(); it++ )
    l_pcts(it->usern,l_rm);

    eosio::print( "l_pcts ", "\n"); 

  reserve();

  for ( auto it = _user.begin(); it != _user.end(); it++ ) 
    performance(it->usern);

  for ( auto it = _user.begin(); it != _user.end(); it++ ) 
    l_performance(it->usern);

  performanceglobal();
  l_performanceglobal();

  if (exitbailout)
    break;
}
}

void vigor::create( name   issuer,
                        asset  maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( maximum_supply.is_valid(), "invalid supply");
    check( maximum_supply.amount > 0, "max-supply must be positive");

    auto existing = _coinstats.find( sym.code().raw() );
    check( existing == _coinstats.end(), "token with symbol already exists" );

    _coinstats.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}

void vigor::setsupply( name issuer, asset maximum_supply )
{
    auto sym = maximum_supply.symbol;
    check( sym.is_valid(), "invalid symbol name" );

    auto existing = _coinstats.find( sym.code().raw() );
    check( existing != _coinstats.end(), "token with symbol does not exist, create token before setting supply" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( maximum_supply.is_valid(), "invalid maximum_supply" );
    check( maximum_supply.amount > 0, "must issue positive maximum_supply" );

    check( maximum_supply.symbol == st.supply.symbol, "symbol precision mismatch5" );
    check( maximum_supply.amount >= st.supply.amount, "cannot set max_supply to less than available supply");

    _coinstats.modify( st, same_payer, [&]( auto& s ) {
       s.max_supply = maximum_supply;
    });
}

void vigor::issue( name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    auto existing = _coinstats.find( sym.code().raw() );
    check( existing != _coinstats.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must issue positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch4" );
    check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    _coinstats.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
      SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                          { st.issuer, to, quantity, memo }
      );
    }
}

void vigor::retire( asset quantity, string memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    auto existing = _coinstats.find( sym.code().raw() );
    check( existing != _coinstats.end(), "token with symbol does not exist 6" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must retire positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch3" );

    _coinstats.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });
    sub_balance( st.issuer, quantity );
}

void vigor::transfer(name    from,
                      name    to,
                      asset   quantity,
                      string  memo )
{
    check( from != to, "cannot transfer to self" );
    require_auth( from );
    check( is_account( to ), "to account does not exist");
    const auto& st = _coinstats.get(quantity.symbol.code().raw(), "symbol does not exist" );

    require_recipient( from );
    require_recipient( to );

    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must transfer positive quantity" );
    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch2" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    auto payer = has_auth( to ) ? to : from;

    sub_balance( from, quantity );
    add_balance( to, quantity, payer );

    if(to == _self && quantity.symbol == symbol("VIGOR", 4)){

        if(memo.c_str() == string("collateral")){
                // Transfer stablecoin into user for use as collateral to borrow crypto

                auto itr = _user.find(from.value);
                if ( itr == _user.end() ) {
                  itr = _user.emplace(_self, [&](auto& new_user) {
                    new_user.usern = from;
                    new_user.l_debt = quantity;
                  });
                } else {
                  auto &user = *itr;
                  _user.modify(user, _self, [&]( auto& modified_user) {
                    modified_user.l_debt += quantity;
                  });
                }

                  globalstats gstats;

                  if (_globals.exists())
                    gstats = _globals.get();
                  gstats.l_totaldebt += quantity;
                  _globals.set(gstats, _self);
                  doupdate();
          
        }else if(memo.c_str() == string("insurance")){
            // Transfer stablecoin into user for use as insurance

                auto itr = _user.find(from.value);
                if ( itr == _user.end() ) {
                  itr = _user.emplace(_self, [&](auto& new_user) {
                    new_user.usern = from;
                    new_user.lastupdate = current_time_point();
                  });
                }

                auto &user = *itr;
                bool found = false;
  
                globalstats gstats;
                
                if (_globals.exists())
                  gstats = _globals.get();

                auto it = user.insurance.begin();

                while ( !found && it++ != user.insurance.end() )
                  found = (it-1)->symbol == quantity.symbol;

                _user.modify(user, _self, [&]( auto& modified_user) {
                  if (!found)
                    modified_user.insurance.push_back(quantity);
                  else
                    modified_user.insurance[(it-1) - user.insurance.begin()] += quantity;
                });
                
                found = false;
                
                it = gstats.insurance.begin();

                while ( !found && it++ != gstats.insurance.end() )
                  found = (it-1)->symbol == quantity.symbol;

                if ( !found )
                  gstats.insurance.push_back(quantity);
                else
                gstats.insurance[(it-1) - gstats.insurance.begin()] += quantity;

                _globals.set(gstats, _self);
                doupdate();
          
          
        }else if(memo.c_str() == string("payoff debt")){
            // Payoff debt: Transfer stablecoin into user and retire
            auto &user = _user.get(from.value,"User not found");

            check(user.debt.amount >= quantity.amount, "Payment too high");
            
            globalstats gstats;
            if (_globals.exists())
              gstats = _globals.get();
      
            _user.modify(user, _self, [&]( auto& modified_user) {
              modified_user.debt -= quantity;
            });
            
            gstats.totaldebt -= quantity;
            
            _globals.set(gstats, _self);
      
            //clear the debt from circulating supply
            action(permission_level{_self, name("active")}, _self, 
              name("retire"), std::make_tuple(quantity, memo)
            ).send();
            
            doupdate();
        }
    }

}

void vigor::sub_balance( name owner, asset value ) {
   accounts from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
      a.balance -= value;
   });
}

void vigor::add_balance( name owner, asset value, name ram_payer )
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

void vigor::open( name owner, const symbol& symbol, name ram_payer )
{
   require_auth( ram_payer );

   auto sym_code_raw = symbol.code().raw();
   const auto& st = _coinstats.get( sym_code_raw, "symbol does not exist" );

   check( st.supply.symbol == symbol, "symbol precision mismatch1" );

   accounts acnts( _self, owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
      });
   }
}

void vigor::close( name owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( _self, owner.value );
   auto it = acnts.find( symbol.code().raw() );
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}


// the asset comes in here with a precision of n
void vigor::assetin( name   from, // handler for notification of transfer action
                         name   to,
                         asset  assetin,
                         string memo ) {
  
  if ( from == _self || to != _self)
    return;

  // the precision of the asset is converted here
  auto amt10 = swap_precision::swapprecision(assetin);
  
  require_auth( from );
  check( from != to, "cannot transfer to self" );
  //require_auth( from );
  check(assetin.symbol.is_valid(), "Symbol must be valid.");
  check( is_account( to ), "to account does not exist");
  check(issueracct.find(assetin.symbol) != issueracct.end(),"assetin symbol precision mismatch7");
  check( assetin.is_valid(), "invalid assetin" );
  check( assetin.amount > 0, "must transfer positive assetin" );
  check( memo.size() <= 256, "memo has more than 256 bytes" );
  check(memo.c_str() == string("insurance") ||
          memo.c_str() == string("collateral") ||
          memo.c_str() == string("payback borrowed token"),
            "memo must be composed of either word: insurance or collateral or payback borrowed token"
          );

  auto itr = _user.find(from.value);
  if ( itr == _user.end() ) {
    
    itr = _user.emplace(_self, [&](auto& new_user) {
      new_user.usern = from;
      new_user.lastupdate = current_time_point();
    });
    action( permission_level{ _self, name("active") },
      _self,
      name("open"), 
      std::make_tuple(
        from,
        symbol("VIGOR", 4), // VIGOR is created internally with precision 4
        _self
      )).send();
  }

  auto &user = *itr;
  bool found = false;

  globalstats gstats;

  if (_globals.exists())
    gstats = _globals.get();

  if (memo.c_str() == string("insurance")) {
 
    auto it = user.insurance.begin();

    while ( !found && it++ != user.insurance.end() )
        found = (it-1)->symbol == amt10.symbol; 

    _user.modify(user, _self, [&]( auto& modified_user) {
      if (!found)
        modified_user.insurance.push_back(amt10);
      else
        modified_user.insurance[(it-1) - user.insurance.begin()] += amt10;
    }); found = false;
    it = gstats.insurance.begin();
    while ( !found && it++ != gstats.insurance.end() )
      found = (it-1)->symbol == amt10.symbol;
    if ( !found )
      gstats.insurance.push_back(amt10);
    else
      gstats.insurance[(it-1) - gstats.insurance.begin()] += amt10;
  }
  else if (memo.c_str() == string("collateral")) {
    auto it = user.collateral.begin();
    while ( !found && it++ != user.collateral.end() )
      found = (it-1)->symbol == amt10.symbol; 

    _user.modify(user, _self, [&]( auto& modified_user) {
      if (!found)
        modified_user.collateral.push_back(amt10);
      else
        modified_user.collateral[(it-1) - user.collateral.begin()] += amt10;
    }); found = false;
    it = gstats.collateral.begin();
    while ( !found && it++ != gstats.collateral.end() )
      found = (it-1)->symbol == amt10.symbol;
    if (!found)
      gstats.collateral.push_back(amt10);
    else
      gstats.collateral[(it-1) - gstats.collateral.begin()] += amt10;
  } 

  else if (memo.c_str() == string("payback borrowed token")) {
      //payback borrowed token into user l_collateral (not stablecoin)
      auto itc = user.l_collateral.begin();
      
      while ( !found && itc++ != user.l_collateral.end() )
        found = (itc-1)->symbol == amt10.symbol; // here the asset has precision of 10

      if (!found) 
        check(false,"Can't payback that asset; not found in user borrows");
      else 
        check(user.l_collateral[(itc-1) - user.l_collateral.begin()].amount >= amt10.amount,"Payback amount too high.");  
      found = false;

      asset locatesremaining = amt10;

      asset paymentasset = asset(0, symbol("VIGOR", 10)); // within assetin VIGOR is set to precision 10
  
      asset amt = amt10; 
      double pct;
      
      for ( auto itr = _user.begin(); itr != _user.end(); ++itr ) {
        if (locatesremaining.amount==0) 
            break;
        if (itr->usern.value == name("finalreserve").value || itr->usern.value == name("reinvestment").value)
          continue;
        eosio::print( "itr->usern ", itr->usern, "\n");  
      
      
      //  eosio::print( "itr->l_lrtoken.size() ", itr->l_lrtoken.size(), "\n"); 

        if (!itr->l_lrtoken.empty()) { 
            for ( auto it = itr->l_lrtoken.begin(); it != itr->l_lrtoken.end(); ++it ) {
              if (locatesremaining.amount==0) 
                break;
                eosio::print( "it->symbol ", it->symbol, "\n"); 
                eosio::print("assetin.symbol ", amt10.symbol, "\n");
                eosio::print( "itr->l_lrname[it - itr->l_lrtoken.begin()] ", itr->l_lrname[it - itr->l_lrtoken.begin()], "\n"); 
                eosio::print( "user.usern ", user.usern, "\n"); 

              if(it->symbol == amt10.symbol && itr->l_lrname[it - itr->l_lrtoken.begin()].value == user.usern.value){
            //    eosio::print( "test","\n");
                amt.amount = std::min(itr->l_lrtoken[it - itr->l_lrtoken.begin()].amount, locatesremaining.amount);
                pct = (double)amt.amount/(double)itr->l_lrtoken[it - itr->l_lrtoken.begin()].amount;
                eosio::print( "pct ", pct,"\n");
                locatesremaining -= amt;
                eosio::print( "locatesremaining ", locatesremaining,"\n");
                _user.modify(itr, _self, [&]( auto& modified_user) { 
                  if (modified_user.l_lrtoken[it - itr->l_lrtoken.begin()].amount - amt.amount == 0.0){
                      paymentasset = modified_user.l_lrpayment[it - itr->l_lrtoken.begin()];   
                      modified_user.l_lrtoken[it - itr->l_lrtoken.begin()].amount = 0;
                      modified_user.l_lrpayment[it - itr->l_lrtoken.begin()].amount = 0;
                      modified_user.l_lrname[it - itr->l_lrtoken.begin()] = name("delete");
                      eosio::print( "erase l_lrtoken", amt,"\n"); 
                      eosio::print( "erase paymentasset ", paymentasset,"\n"); 
                  } else {
                      modified_user.l_lrtoken[it - itr->l_lrtoken.begin()] -= amt;
                      paymentasset.amount = pct*modified_user.l_lrpayment[it - itr->l_lrtoken.begin()].amount;
                      modified_user.l_lrpayment[it - itr->l_lrtoken.begin()] -= paymentasset; 
                      eosio::print( "paymentasset ", paymentasset,"\n");
                      eosio::print( "pct ", pct,"\n");
                      eosio::print( "lender l_lrpayment ", modified_user.l_lrpayment[it - itr->l_lrtoken.begin()]  ," amt ", paymentasset,"\n");
                      eosio::print( "lender l_lrtoken ", modified_user.l_lrtoken[it - itr->l_lrtoken.begin()] ," amt ", amt,"\n");
                  }
                });

                // add located asset to lender insurance
                found = false;
                auto iti = itr->insurance.begin();
                while ( !found && iti++ != itr->insurance.end() )
                  found = (iti-1)->symbol == amt.symbol;
                _user.modify(itr, _self, [&]( auto& modified_user) {
                  if (!found)
                    modified_user.insurance.push_back(amt);
                  else
                    modified_user.insurance[(iti-1) - itr->insurance.begin()] += amt;
                }); found = false;


                //add located asset to global insurance
                found = false;
                auto itg = gstats.insurance.begin();
                while ( !found && itg++ != gstats.insurance.end() )
                  found = (itg-1)->symbol == amt.symbol;
                eosio::print( "found", found,"\n");
                if ( !found ) {
                  gstats.insurance.push_back(amt);
                  eosio::print( "push_back", amt,"\n");
                }
                else {
                  gstats.insurance[(itg-1) - gstats.insurance.begin()] += amt;
                  eosio::print( "gstats.insurance[(itg-1) - gstats.insurance.begin()]", gstats.insurance[(itg-1) - gstats.insurance.begin()],"\n");
                }

              // subtract located asset from borrower l_collateral
                _user.modify(user, _self, [&]( auto& modified_user) {
                if (modified_user.l_collateral[(itc-1) - user.l_collateral.begin()].amount - amt.amount == 0)
                  modified_user.l_collateral.erase(itc-1);
                else
                  modified_user.l_collateral[(itc-1) - user.l_collateral.begin()] -= amt;
                });

                // move located asset out of global l_collateral
                found = false;
                itg = gstats.l_collateral.begin();
                while ( !found && itg++ != gstats.l_collateral.end() )
                  found = (itg-1)->symbol == amt.symbol;
                eosio::print( "found", found,"\n");
                if ( !found )
                  check(false,"payment asset not found in global l_collateral");
                else {
                  if (gstats.l_collateral[(itg-1) - gstats.l_collateral.begin()].amount - amt.amount == 0){
                    eosio::print( "gstats.l_collateral[(itg-1) - gstats.l_collateral.begin()] erased", gstats.l_collateral[(itg-1) - gstats.l_collateral.begin()],"\n");
                    gstats.l_collateral.erase(itg-1);
                    } 
                  else {
                    gstats.l_collateral[(itg-1) - gstats.l_collateral.begin()] -= amt;
                    eosio::print( "gstats.l_collateral[(itg-1) - gstats.l_collateral.begin()]", gstats.l_collateral[(itg-1) - gstats.l_collateral.begin()],"\n");
                  }
                }

                // move paymentasset out of reinvestment insurance
                found = false;
                auto &reinvestment = _user.get(name("reinvestment").value, "reinvestment not found");
                auto itre = reinvestment.insurance.begin();
                while ( !found && itre++ != reinvestment.insurance.end() )
                  found = (itre-1)->symbol == paymentasset.symbol;  
                if (found) {
                  _user.modify(reinvestment, _self, [&]( auto& modified_user) {
                  if (modified_user.insurance[(itre-1) - reinvestment.insurance.begin()].amount - paymentasset.amount == 0)
                    modified_user.insurance.erase(itre-1);
                  else 
                    modified_user.insurance[(itre-1) - reinvestment.insurance.begin()] -= paymentasset;
                  });
                }
                else
                  check(false,"payment asset not found in reinvestment account");
                // move paymentasset to borrower l_debt
                //_user.modify(user, _self, [&]( auto& modified_user) {
                //  modified_user.l_debt += paymentasset;
                //});


                // move paymentasset out of global insurance
                found = false;
                itg = gstats.insurance.begin();
                while ( !found && itg++ != gstats.insurance.end() )
                  found = (itg-1)->symbol == paymentasset.symbol;
                eosio::print( "found", found,"\n");
                if ( !found )
                  check(false,"payment asset not found in global insurance");
                else {
                  if (gstats.insurance[(itg-1) - gstats.insurance.begin()].amount - paymentasset.amount == 0){
                    eosio::print( "gstats.insurance[(itg-1) - gstats.insurance.begin()] erased", gstats.insurance[(itg-1) - gstats.insurance.begin()],"\n");
                    gstats.insurance.erase(itg-1);
                    } 
                  else {
                    gstats.insurance[(itg-1) - gstats.insurance.begin()] -= paymentasset;
                    eosio::print( "gstats.insurance[(itg-1) - gstats.insurance.begin()]", gstats.insurance[(itg-1) - gstats.insurance.begin()],"\n");
                  }
                }

                _globals.set(gstats, _self);
            }
            }
                _user.modify(itr, _self, [&]( auto& modified_user) { //removed vector elements with zero amount
                modified_user.l_lrtoken.erase(
                    std::remove_if(modified_user.l_lrtoken.begin(), modified_user.l_lrtoken.end(),
                          [](const asset & o) { return o.amount==0; }),
                    modified_user.l_lrtoken.end());
                });
                _user.modify(itr, _self, [&]( auto& modified_user) { //removed vector elements with zero amount
                modified_user.l_lrpayment.erase(
                    std::remove_if(modified_user.l_lrpayment.begin(), modified_user.l_lrpayment.end(),
                          [](const asset & o) { return o.amount==0; }),
                    modified_user.l_lrpayment.end());
                });
                _user.modify(itr, _self, [&]( auto& modified_user) { //removed vector elements with zero amount
                modified_user.l_lrname.erase(
                    std::remove_if(modified_user.l_lrname.begin(), modified_user.l_lrname.end(),
                          [](const name & o) { return o.value==name("delete").value; }),
                    modified_user.l_lrname.end());
                });
        }
      }
      eosio::print( "locatesremaining.amount ", locatesremaining.amount,"\n");
      check(locatesremaining.amount==0,"Not enough locates receipts found");
    // found = true;
    // check(false,"success");
  }
  _globals.set(gstats, _self);
  doupdate();
}


// the asset comes in with precision 10
// the asset leaves here with a precision of 4
// excpet for VIGOR
void vigor::assetout(name usern, asset assetout, string memo)
{
  require_auth( usern );
  check( usern != _self, "cannot transfer to self" );
  check(assetout.symbol.is_valid(), "Symbol must be valid.");
  check( is_account( usern ), "to account does not exist");
  check(issueracct.find(assetout.symbol) != issueracct.end(),"assetout symbol or precision invalid");
  check( assetout.is_valid(), "invalid assetout" );
  check( assetout.amount > 0, "must transfer positive assetout" );
  check( memo.size() <= 256, "memo has more than 256 bytes" );
  check( memo.c_str() == string("collateral") || 
              memo.c_str() == string("insurance") || 
              memo.c_str() == string("borrow"), 
              "memo must be composed of either word: insurance | collateral | borrow"
            );
  check(_globals.exists(), "globals don't exist");
  globalstats gstats = _globals.get();
  bool found = false;

  // the precision of the asset is converted here
  auto amt4 = swap_precision::swapprecision(assetout);

  auto &user = _user.get( usern.value,"User not found16" );


  if ( memo.c_str() == string("borrow") && assetout.symbol == symbol("VIGOR", 4) ) {
    // borrow stablecoins against crypto as collateral
    asset debt = user.debt + assetout;

    // if overcollateralization is C then leverage L = 1 / ( 1 - ( 1 / C ) )
    check( user.valueofcol >= 1.11 * ( debt.amount / std::pow(10.0, 4) ),
    "Collateral must exceed borrowings by 1.11" );
    
    _user.modify(user, _self, [&]( auto& modified_user) {
      modified_user.debt = debt;
    });
    gstats.totaldebt += assetout;

    action( permission_level{_self, name("active")},
      _self, name("issue"), std::make_tuple(
        usern, assetout, std::string("VIGOR issued to ") + usern.to_string()
      )).send();
    
    _globals.set(gstats, _self);
  } 
  else {
                if ( memo.c_str() == string("insurance")) { 
                    //withdraw tokens from insurance pool (can also be stablecoin)
                    for ( auto it = user.insurance.begin(); it < user.insurance.end(); ++it )
                        if (it->symbol == amt4.symbol) { 
                        check( it->amount >= amt4.amount,
                        "Insufficient insurance assets available." );
                        if ( it->amount - amt4.amount == 0 )
                            _user.modify(user, _self, [&]( auto& modified_user) {
                            modified_user.insurance.erase(it);
                            });
                        else 
                            _user.modify(user, _self, [&]( auto& modified_user) {
                            modified_user.insurance[it - user.insurance.begin()] -= amt4;
                            });
                        found = true;
                        break;
                        }
                    if (found)
                    for ( auto it = gstats.insurance.begin(); it != gstats.insurance.end(); ++it )
                        if ( it->symbol == amt4.symbol ) {
                        if ( it->amount - amt4.amount == 0 )
                            gstats.insurance.erase(it);
                        else
                            gstats.insurance[it - gstats.insurance.begin()] -= amt4;
                        break;
                        }
                    _globals.set(gstats, _self);
                } else if ( memo.c_str() == string("collateral") && assetout.symbol != symbol("VIGOR", 4) ) {
                    // withdraw tokens from collateral
                    for ( auto it = user.collateral.begin(); it != user.collateral.end(); ++it )
                        if ( it->symbol == amt4.symbol ) { 
                        check( it->amount >= amt4.amount,
                        "Insufficient collateral assets available." );
                        
                        double valueofasset = amt4.amount / std::pow(10.0, it->symbol.precision());

                        t_series stats(name("datapreprocx"),name(issuerfeed[amt4.symbol]).value);
                        auto itr = stats.find(1);
                        check(itr != stats.end(),"asset not found in the datapreprocessor, or precision invalid");
                        valueofasset *= (double)itr->price[0] / pricePrecision;

                        double valueofcol = user.valueofcol - valueofasset;

                        check( valueofcol >= 1.11 * ( user.debt.amount / std::pow(10.0, 4) ),
                        "Collateral must exceed borrowings by 1.11"   );
                        
                        if ( it->amount - amt4.amount == 0 )
                            _user.modify(user, _self, [&]( auto& modified_user) {
                            modified_user.collateral.erase(it);
                            });
                        else
                            _user.modify(user, _self, [&]( auto& modified_user) {
                            modified_user.collateral[it - user.collateral.begin()] -= amt4;
                            });
                        found = true;
                        break;
                        }
                    if (found)
                    for ( auto it = gstats.collateral.begin(); it != gstats.collateral.end(); ++it )
                        if ( it->symbol == amt4.symbol ) {
                        if ( it->amount - amt4.amount == 0 )
                            gstats.collateral.erase(it);
                        else
                            gstats.collateral[it - gstats.collateral.begin()] -= amt4;
                        break;
                        }
                    _globals.set(gstats, _self);
                } else if ( memo.c_str() == string("borrow") && assetout.symbol != symbol("VIGOR", 4) ) {
                        // borrow tokens against stablecoin as collateral
                        
                        auto &user = _user.get(usern.value, "User not found");
                        globalstats gstats = _globals.get();

                        t_series stats(name("datapreprocx"),name(issuerfeed[amt4.symbol]).value);
                        auto itrp = stats.find(1);
                        check(itrp != stats.end(),"asset not found in the datapreprocessor, or precision invalid");
                        double valueofassetout = (amt4.amount) / std::pow(10.0, amt4.symbol.precision()) * 
                                    ( (double)itrp->price[0] / pricePrecision );

                        eosio::print( "valueofassetout : ", valueofassetout, "\n");
                        eosio::print( "user.l_valueofcol : ", user.l_valueofcol, "\n");
                        eosio::print( "1.11 * ( user.l_debt.amount / std::pow(10.0, 4) ) : ", ( user.l_debt.amount / std::pow(10.0, 4) ) / 1.11, "\n");

                        check( user.l_valueofcol + valueofassetout <= ( user.l_debt.amount / std::pow(10.0, 4) ) / 1.11,
                            "Collateral must exceed borrowings by 1.111" );

                        // can't borrow from the finalreserve for new borrows, so remove it from gstats.insurance
                        auto &finalreserve = _user.get(name("finalreserve").value, "finalreserve not found");
                        auto it = finalreserve.insurance.begin();
                        asset lr = amt4;
                        while ( !found && it++ != finalreserve.insurance.end() )
                        found = (it-1)->symbol == amt4.symbol;
                        if (found)
                        lr.amount = (it-1)->amount;
                        else
                        lr.amount = 0;

                        // can't borrow from the reinvestment for new borrows, so remove it from gstats.insurance
                        found = false;
                        auto &reinvestment = _user.get(name("reinvestment").value, "reinvestment not found");
                        it = reinvestment.insurance.begin();
                        //asset ri = assetout;
                        asset ri = amt4;
                        while ( !found && it++ != reinvestment.insurance.end() )
                        found = (it-1)->symbol == amt4.symbol;
                        if (found)
                        ri.amount = (it-1)->amount;
                        else
                        ri.amount = 0;

                        found = false;
                        bool locatesavailable = false;
                        it = gstats.insurance.begin();
                        asset amt = amt4;
                        while ( !found && it++ != gstats.insurance.end() )
                        found = (it-1)->symbol == amt4.symbol;
                        if (found)
                        if (gstats.insurance[(it-1) - gstats.insurance.begin()].amount - lr.amount -ri.amount >= amt4.amount)
                            locatesavailable = true;
                        check(locatesavailable,"Can't locate enough to borrow");

                        // locate assetout by searching all users that have assets in their insurance
                        asset locatesremaining = amt4;
                        //asset paymentasset = asset( 0, symbol("VIGOR", 4) );
                        asset paymentasset = asset( 0, symbol("VIGOR", 10) );
                        for ( auto itr = _user.begin(); itr != _user.end(); ++itr ) {
                        if (locatesremaining.amount==0)
                            break;
                        if (itr->usern.value == name("finalreserve").value || itr->usern.value == name("reinvestment").value)
                            continue;
                        if (itr->valueofins>0.0) {
                            auto it = itr->insurance.begin();
                            found = false;
                            while ( !found && it++ != itr->insurance.end() )
                            found = (it-1)->symbol == amt4.symbol; 
                            eosio::print( "itr->usern.value : ", itr->usern, "\n");
                            eosio::print( "itr->insurance[(it-1) - itr->insurance.begin()]", itr->insurance[(it-1) - itr->insurance.begin()], "\n");
                            if (found) {
                            // move located asset from lender to borrower
                            // subtract located asset from lender insurance
                            amt.amount = std::min(itr->insurance[(it-1) - itr->insurance.begin()].amount, locatesremaining.amount);
                            locatesremaining -= amt;
                            _user.modify(itr, _self, [&]( auto& modified_user) {
                                modified_user.insurance[(it-1) - itr->insurance.begin()] -= amt;
                                eosio::print( "lender insurance ", modified_user.insurance[(it-1) - itr->insurance.begin()] ," amt ", amt,"\n");
                                paymentasset.amount = std::pow(10.0, 4)*((amt.amount) / std::pow(10.0, amt.symbol.precision()) * ( (double)itrp->price[0] / pricePrecision ));
                                // give locate receipt to lender
                                modified_user.l_lrtoken.push_back(amt);
                                modified_user.l_lrpayment.push_back(paymentasset);
                                modified_user.l_lrname.push_back(usern);
                            });
                            // subtract located asset from global insurance
                            for ( auto itg = gstats.insurance.begin(); itg != gstats.insurance.end(); ++itg )
                                if ( itg->symbol == amt4.symbol ) {
                                if ( itg->amount - amt.amount == 0 )
                                    gstats.insurance.erase(itg);
                                else
                                    gstats.insurance[itg - gstats.insurance.begin()] -= amt;
                                }          
                            //add located asset to global l_collateral
                            found = false;
                            auto itg = gstats.l_collateral.begin();
                            while ( !found && itg++ != gstats.l_collateral.end() )
                                found = (itg-1)->symbol == amt.symbol;
                            eosio::print( "found", found,"\n");
                            if ( !found ) {
                                gstats.l_collateral.push_back(amt);
                                eosio::print( "push_back", amt,"\n");
                            }
                            else {
                                gstats.l_collateral[(itg-1) - gstats.l_collateral.begin()] += amt;
                                eosio::print( "gstats.l_collateral[(itg-1) - gstats.l_collateral.begin()]", gstats.l_collateral[(itg-1) - gstats.l_collateral.begin()],"\n");
                            }
                            // add located asset to borrower l_collateral, and subtract payment asset from borrower
                            for ( auto it = user.l_collateral.begin(); it != user.l_collateral.end(); ++it ) {
                                if (it->symbol == amt4.symbol) {
                                _user.modify(user, _self, [&]( auto& modified_user) {
                                    modified_user.l_collateral[it - user.l_collateral.begin()] += amt; //previous borrowings of type assetout exist already, so add to it
                                    eosio::print( "borrower l_collateral incremented", modified_user.l_collateral[it - user.l_collateral.begin()]," amt ", amt,"\n");
                                    //modified_user.l_debt -= paymentasset;
                                });
                                amt.amount = 0;
                                break;
                                }
                            }
                            if (amt.amount > 0) 
                                _user.modify(user, _self, [&]( auto& modified_user) {
                                eosio::print( "borrower l_collateral push_back ", amt,"\n");
                                modified_user.l_collateral.push_back(amt); //previous borrowings of type assetout do not exist, so create one
                                //modified_user.l_debt -= paymentasset;
                                });
                            // add payment asset to reinvestment account as an insurance asset to earn VIG
                            eosio::print( "add payment asset to reinvestment account paymentasset ", paymentasset,"\n");
                            found = false;
                            auto it = reinvestment.insurance.begin();
                            while ( !found && it++ != reinvestment.insurance.end() )
                                found = (it-1)->symbol == paymentasset.symbol;
                            _user.modify(reinvestment, _self, [&]( auto& modified_user) {
                                if (!found)
                                modified_user.insurance.push_back(paymentasset);
                                else
                                modified_user.insurance[(it-1) - reinvestment.insurance.begin()] += paymentasset;
                            }); found = false;
                            it = gstats.insurance.begin();
                            while ( !found && it++ != gstats.insurance.end() )
                                found = (it-1)->symbol == paymentasset.symbol;
                            eosio::print( "found", found,"\n");
                            if ( !found ) {
                                gstats.insurance.push_back(paymentasset);
                                eosio::print( "push_back", paymentasset,"\n");
                            }
                            else {
                                gstats.insurance[(it-1) - gstats.insurance.begin()] += paymentasset;
                                eosio::print( "gstats.insurance[(it-1) - gstats.insurance.begin()]", gstats.insurance[(it-1) - gstats.insurance.begin()],"\n");
                            }
                            _globals.set(gstats, _self);

                            _user.modify(itr, _self, [&]( auto& modified_user) { //removed vector elements with zero amount
                                modified_user.insurance.erase(
                                    std::remove_if(modified_user.insurance.begin(), modified_user.insurance.end(),
                                        [](const asset & o) { return o.amount==0; }),
                                    modified_user.insurance.end());
                            });
                            }
                        }
                        }

                        check(locatesremaining.amount==0,"Not enough locates to borrow");
                        found = true;
                        
                } else if ( memo.c_str() == string("collateral") && assetout.symbol == symbol("VIGOR", 4) ) {
                    // withdraw stablecoins from collateral 

                    asset l_debt = user.l_debt - assetout;

                    // if overcollateralization is C then leverage L = 1 / ( 1 - ( 1 / C ) )
                    check( user.l_valueofcol * 1.11 <= ( l_debt.amount / std::pow(10.0, 4) ),
                    "Collateral must exceed borrowings by 1.11"  );
                    found = true;
                    
                    _user.modify(user, _self, [&]( auto& modified_user) {
                    modified_user.l_debt = l_debt;
                    });
                    gstats.l_totaldebt -= assetout;

                    _globals.set(gstats, _self);

                }
                
                check(found, "asset not found in user");
                eosio::print( "transfer borrowed tokens to user ", "\n");

               
                // select the precision of the asset to send in the inline action
                if(assetout.symbol.precision() < 10)
                {
                    action( permission_level{_self, name("active")},
                        issueracct[assetout.symbol], name("transfer"),
                        std::make_tuple(_self, usern, assetout, memo
                    )).send();
                }else if(assetout.symbol.precision() == 10){
                    action( permission_level{_self, name("active")},
                        issueracct[assetout.symbol], name("transfer"),
                        std::make_tuple(_self, usern, amt4, memo
                    )).send();
                }
                
  }

    doupdate();
}

void vigor::stresscol(name usern) {

  const auto& user = _user.get( usern.value, "User not found17" );  
  
  check(_globals.exists(), "globals not found");
  globalstats gstats = _globals.get();

  double portVariance = portVarianceCol(usern);

  double stresscol = std::log(1.0 + (((std::exp(-1.0*(std::pow(-1.0*std::sqrt(2.0)*erfc_inv(2.0*alphatest),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-alphatest)) * std::sqrt(portVariance)));
                         
  double svalueofcol = ((1.0 - stresscol) * user.valueofcol);
  double svalueofcole = std::max( 0.0,
    user.debt.amount / std::pow(10.0, 4) - ((1.0 - stresscol) * user.valueofcol)
  );
  gstats.svalueofcole += svalueofcole - user.svalueofcole; // model suggested dollar value of the sum of all insufficient collateral in a stressed market

  double stresscolavg = std::log(1.0 + (((std::exp(-1.0*(std::pow(-1.0*std::sqrt(2.0)*erfc_inv(2.0*0.5),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-0.5)) * std::sqrt(portVariance)));
  double svalueofcoleavg = std::max( 0.0,
    user.debt.amount / std::pow(10.0, 4) - ((1.0 - stresscolavg) * user.valueofcol)
  );

  gstats.svalueofcoleavg += svalueofcoleavg - user.svalueofcoleavg; // model suggested dollar value of the sum of all insufficient collateral on average in stressed markets, expected loss
  
  _globals.set(gstats, _self);

  _user.modify(user, _self, [&]( auto& modified_user) { 
    modified_user.volcol = std::sqrt(portVariance); // volatility of the user collateral portfolio
    modified_user.stresscol = stresscol; // model suggested percentage loss that the user collateral portfolio would experience in a stress event.
    modified_user.svalueofcol = svalueofcol; // model suggested dollar value of the user collateral portfolio in a stress event.
    modified_user.svalueofcole = svalueofcole; // model suggested dollar amount of insufficient collateral of a user loan in a stressed market.
    modified_user.svalueofcoleavg = svalueofcoleavg; // model suggested dollar amount of insufficient collateral of a user loan on average in stressed markets, expected loss
  });

}

void vigor::l_stresscol(name usern) {

  const auto& user = _user.get( usern.value, "User not foundl_17" );
  
  check(_globals.exists(), "globals not found");
  globalstats gstats = _globals.get();

  double l_portVariance = l_portVarianceCol(usern);
 
  double l_stresscol = std::log(1.0 + (((std::exp(-1.0*(std::pow(-1.0*std::sqrt(2.0)*erfc_inv(2.0*alphatest),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-alphatest)) * std::sqrt(l_portVariance)));
  double l_svalueofcol = ((1.0 + l_stresscol) * user.l_valueofcol);
  double l_svalueofcole = std::max( 0.0,
    ((1.0 + l_stresscol) * user.l_valueofcol) - user.l_debt.amount / std::pow(10.0, 4)
  );
  gstats.l_svalueofcole += l_svalueofcole - user.l_svalueofcole; // model suggested dollar value of the sum of all insufficient collateral in a stressed market

  double l_stresscolavg = std::log(1.0 + (((std::exp(-1.0*(std::pow(-1.0*std::sqrt(2.0)*erfc_inv(2.0*0.5),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-0.5)) * std::sqrt(l_portVariance)));
  double l_svalueofcoleavg = std::max( 0.0,
    ((1.0 + l_stresscolavg) * user.l_valueofcol) - user.l_debt.amount / std::pow(10.0, 4)
  );

  gstats.l_svalueofcoleavg += l_svalueofcoleavg - user.l_svalueofcoleavg; // model suggested dollar value of the sum of all insufficient collateral on average in stressed markets, expected loss
  
  _globals.set(gstats, _self);

  _user.modify(user, _self, [&]( auto& modified_user) { 
    modified_user.l_volcol = std::sqrt(l_portVariance); // volatility of the user collateral portfolio
    modified_user.l_stresscol = l_stresscol; // model suggested percentage loss that the user collateral portfolio would experience in a stress event.
    modified_user.l_svalueofcol = l_svalueofcol; // model suggested dollar value of the user collateral portfolio in a stress event.
    modified_user.l_svalueofcole = l_svalueofcole; // model suggested dollar amount of insufficient collateral of a user loan in a stressed market.
    modified_user.l_svalueofcoleavg = l_svalueofcoleavg; // model suggested dollar amount of insufficient collateral of a user loan on average in stressed markets, expected loss
  });

}

double vigor::portVarianceCol(name usern)
{

  const auto& user = _user.get( usern.value, "User not found18" );  

  double portVariance = 0.0;
  for ( auto i = user.collateral.begin(); i != user.collateral.end(); ++i ) {
    
    t_series stats(name("datapreprocx"),name(issuerfeed[i->symbol]).value);
    auto itr = stats.find(1);
    double iVvol = (double)itr->vol/volPrecision;
    double iW = (double)itr->price[0] / pricePrecision;
    iW *= i->amount / std::pow(10.0, i->symbol.precision()); 
    iW /= user.valueofcol;

  for (auto j = i + 1; j != user.collateral.end(); ++j ) {
    double c = (double)itr->correlation_matrix.at(j->symbol)/corrPrecision;

    t_series statsj(name("datapreprocx"),name(issuerfeed[j->symbol]).value);
    auto itr = statsj.find(1);
    double jVvol = (double)itr->vol/volPrecision;
    double jW = (double)itr->price[0] / pricePrecision;
    jW *= j->amount / std::pow(10.0, j->symbol.precision());
    jW /= user.valueofcol;

    portVariance += 2.0 * iW * jW * c * iVvol * jVvol;
  }
  portVariance += std::pow(iW, 2) * std::pow(iVvol, 2);
}
return portVariance;
}

double vigor::l_portVarianceCol(name usern)
{

  const auto& user = _user.get( usern.value, "User not found18" );  

  double portVariance = 0.0;
  for ( auto i = user.l_collateral.begin(); i != user.l_collateral.end(); ++i ) {
    
    t_series stats(name("datapreprocx"),name(issuerfeed[i->symbol]).value);
    auto itr = stats.find(1);
    double iVvol = (double)itr->vol/volPrecision;
    double iW = (double)itr->price[0] / pricePrecision;
    iW *= i->amount / std::pow(10.0, i->symbol.precision()); 
    iW /= user.l_valueofcol;

  for (auto j = i + 1; j != user.l_collateral.end(); ++j ) {
    double c = (double)itr->correlation_matrix.at(j->symbol)/corrPrecision;

    t_series statsj(name("datapreprocx"),name(issuerfeed[j->symbol]).value);
    auto itr = statsj.find(1);
    double jVvol = (double)itr->vol/volPrecision;
    double jW = (double)itr->price[0] / pricePrecision;
    jW *= j->amount / std::pow(10.0, j->symbol.precision());
    jW /= user.l_valueofcol;

    portVariance += 2.0 * iW * jW * c * iVvol * jVvol;
  }
  portVariance += std::pow(iW, 2) * std::pow(iVvol, 2);
}
return portVariance;
}

double vigor::portVarianceIns(name usern)
{

  const auto& user = _user.get( usern.value, "User not found19" );  

  double portVariance = 0.0;
  for ( auto i = user.insurance.begin(); i != user.insurance.end(); ++i ) {
    
    t_series stats(name("datapreprocx"),name(issuerfeed[i->symbol]).value);
    auto itr = stats.find(1);
    double iVvol = (double)itr->vol/volPrecision;
    double iW = (double)itr->price[0] / pricePrecision;
    iW *= i->amount / std::pow(10.0, i->symbol.precision()); 
    iW /= user.valueofins;

  for (auto j = i + 1; j != user.insurance.end(); ++j ) {
    double c = (double)itr->correlation_matrix.at(j->symbol)/corrPrecision;

    t_series statsj(name("datapreprocx"),name(issuerfeed[j->symbol]).value);
    auto itr = statsj.find(1);
    double jVvol = (double)itr->vol/volPrecision;
    double jW = (double)itr->price[0] / pricePrecision;
    jW *= j->amount / std::pow(10.0, j->symbol.precision());
    jW /= user.valueofins;

    portVariance += 2.0 * iW * jW * c * iVvol * jVvol;
  }
  portVariance += std::pow(iW, 2) * std::pow(iVvol, 2);
}
return portVariance;
}

double vigor::portVarianceIns()
{
  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  double portVariance = 0.0;

  for ( auto i = gstats.insurance.begin(); i != gstats.insurance.end(); ++i ) {

    t_series stats(name("datapreprocx"),name(issuerfeed[i->symbol]).value);
    auto itr = stats.find(1);
    double iVvol = (double)itr->vol/volPrecision;
    double iW = (double)itr->price[0] / pricePrecision;
    iW *= i->amount / std::pow(10.0, i->symbol.precision());
    iW /=  gstats.valueofins;

    for (auto j = i + 1; j != gstats.insurance.end(); ++j ) {
      double c = (double)itr->correlation_matrix.at(j->symbol)/corrPrecision;

      t_series stats(name("datapreprocx"),name(issuerfeed[j->symbol]).value);
      auto itr = stats.find(1);
      double jVvol = (double)itr->vol/volPrecision;
      double jW = (double)itr->price[0] / pricePrecision;
      jW *= j->amount / std::pow(10.0, j->symbol.precision());
      jW /=  gstats.valueofins; 

      portVariance += 2.0 * iW * jW * c * iVvol * jVvol;
    }
    portVariance += std::pow(iW, 2) * std::pow(iVvol, 2);
  }
  return portVariance;
}

void vigor::stressins()
{
  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  double portVariance = portVarianceIns();

  double stressins = std::log(1.0 + (((std::exp(-1.0*(std::pow(-1.0*std::sqrt(2.0)*erfc_inv(2.0*alphatest),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-alphatest)) * std::sqrt(portVariance)));// model suggested percentage loss that the total insurance asset portfolio would experience in a stress event.
  gstats.stressins = stressins;
  gstats.svalueofins = (1.0 - stressins) * gstats.valueofins; // model suggested dollar value of the total insurance asset portfolio in a stress event.

  double stressinsavg = std::log(1.0 + (((std::exp(-1.0*(std::pow(-1.0*std::sqrt(2.0)*erfc_inv(2.0*0.5),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-0.5)) * std::sqrt(portVariance))); // model suggested percentage loss that the total insurance asset portfolio would experience in a stress event.
  gstats.svalueofinsavg = (1.0 - stressinsavg) * gstats.valueofins; // model suggested dollar value of the total insurance asset portfolio on average in stressed markets

  gstats.l_svalueofins = (1.0 + stressins) * gstats.valueofins; // model suggested dollar value of the total insurance asset portfolio in a stress event.
  gstats.l_svalueofinsavg = (1.0 + stressinsavg) * gstats.valueofins; // model suggested dollar value of the total insurance asset portfolio on average in stressed markets

  _globals.set(gstats, _self);
}

void vigor::risk()
{
  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  // market value of assets and liabilities from the perspective of insurers

  // normal markets
  double mva_n = gstats.valueofins; //market value of insurance assets in normal markets, includes the reserve which is implemented as an insurer, collateral is not an asset of the insurers
  double mvl_n = 0; // no upfront is paid for tes, and insurers can walk away at any time, debt is not a liability of the insurers
  
  //stressed markets
  double mva_s = gstats.svalueofins;
  double mvl_s = gstats.svalueofcole;

  double own_n = mva_n - mvl_n; // own funds normal markets
  double own_s = mva_s - mvl_s; // own funds stressed markets
  
  double scr = own_n - own_s; // solvency capial requirement is the amount of insurance assets required to survive a sress event
  
  double solvency = own_n / scr; // solvency, represents capital adequacy to back the stablecoin

  gstats.solvency = solvency;
  gstats.scr = scr;
  gstats.scale = std::max(std::min(solvencyTarget/solvency,maxtesscale),mintesscale);

  _globals.set(gstats, _self);
}

void vigor::l_risk()
{
  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  // market value of assets and liabilities from the perspective of insurers

  // normal markets
  double mva_n = gstats.valueofins; //market value of insurance assets in normal markets, includes the reserve which is implemented as an insurer, collateral is not an asset of the insurers
  double mvl_n = 0; // no upfront is paid for tes, and insurers can walk away at any time, debt is not a liability of the insurers
  eosio::print( "gstats.valueofins : ", gstats.valueofins, "\n");
  //stressed markets
  double mva_s = gstats.l_svalueofins;
  double mvl_s = gstats.l_svalueofcole;
  eosio::print( "gstats.l_svalueofins: ", gstats.l_svalueofins, "\n");
  eosio::print( "gstats.l_svalueofcole : ", gstats.l_svalueofcole, "\n");
  double own_n = mva_n - mvl_n; // own funds normal markets
  double own_s = mva_s - mvl_s; // own funds stressed markets
  
  double l_scr = std::max(own_n - own_s,20.0); // solvency capial requirement is the amount of insurance assets required to survive a sress event
  
  eosio::print( "l_scr: ",l_scr, "\n");
  double l_solvency = own_n / l_scr; // solvency, represents capital adequacy to back the stablecoin

  gstats.l_solvency = l_solvency;
  gstats.l_scr = l_scr;
  gstats.l_scale = std::max(std::min(l_solvencyTarget/l_solvency,maxtesscale),mintesscale);

  _globals.set(gstats, _self);
}


void vigor::pricing(name usern) {
/* premium payments in exchange for contingient payoff in 
 * the event that a price threshhold is breached
*/
  const auto& user = _user.get( usern.value, "User not found20" );  
  
  check(_globals.exists(), "globals not found");
  globalstats gstats = _globals.get();

  double ivol = user.volcol * gstats.scale; // market determined implied volaility
                      
  double istresscol = std::log(((std::exp(user.stresscol)-1.0) * gstats.scale) + 1.0);

  double payoff = std::max(  0.0,
    1.0 * (user.debt.amount / std::pow(10.0,4)) - user.valueofcol * (1.0 - istresscol)
  );
  double T = 1.0;
  double d = ((std::log(user.valueofcol / (user.debt.amount/std::pow(10.0,4)))) + (-std::pow(ivol,2)/2.0) * T)/ (ivol * std::sqrt(T));

  double tesprice = std::min(std::max( mintesprice * gstats.scale,
    ((payoff * std::erfc(d / std::sqrt(2.0)) / 2.0) / (user.debt.amount / std::pow(10.0, 4))))*calibrate,maxtesprice);

  if (user.debt.amount == 0)
    tesprice = 0.0;

  tesprice /= 1.6 * (user.creditscore / 800.0); // credit score of 500 means no discount or penalty.

  double premiums = tesprice * (user.debt.amount / std::pow(10.0,4)); 

  gstats.premiums += premiums - user.premiums; // total dollar amount of premiums all borrowers would pay in one year to insure their collateral
  
  _globals.set(gstats, _self);

  _user.modify(user, _self, [&]( auto& modified_user) { 
    modified_user.tesprice = tesprice; // annualized rate borrowers pay in periodic premiums to insure their collateral
    modified_user.istresscol = istresscol; // market determined implied percentage loss that the user collateral portfolio would experience in a stress event.
    modified_user.lastupdate = current_time_point();
    modified_user.premiums = premiums; // dollar amount of premiums borrowers would pay in one year to insure their collateral
  });
}

void vigor::l_pricing(name usern) {
/* premium payments in exchange for contingient payoff in 
 * the event that a price threshhold is breached
*/
  const auto& user = _user.get( usern.value, "User not found21" );  
  
  check(_globals.exists(), "globals not found");
  globalstats gstats = _globals.get();

  double l_ivol = user.l_volcol * gstats.l_scale; // market determined implied volaility
  double l_istresscol = std::log(((std::exp(user.l_stresscol)-1.0) * gstats.l_scale) + 1.0);

  double payoff = std::max(  0.0,
    1.0 * user.l_valueofcol * (1.0 + l_istresscol) - (user.l_debt.amount / std::pow(10.0,4))
  );
  double T = 1.0;
  double d = ((std::log(user.l_valueofcol / (user.l_debt.amount/std::pow(10.0,4)))) + (-std::pow(l_ivol,2)/2.0) * T)/ (l_ivol * std::sqrt(T));

  double l_tesprice = std::min(std::max( mintesprice * gstats.l_scale,
    ((payoff * std::erfc((-1.0 * d) / std::sqrt(2.0)) / 2.0) / (user.l_debt.amount / std::pow(10.0, 4))))*calibrate,maxtesprice);

  if (user.l_debt.amount == 0)
    l_tesprice = 0.0;

  l_tesprice /= 1.6 * (user.creditscore / 800.0); // credit score of 500 means no discount or penalty.

  double l_premiums = l_tesprice * (user.l_debt.amount / std::pow(10.0,4)); 

  gstats.l_premiums += l_premiums - user.l_premiums; // total dollar amount of premiums all borrowers would pay in one year to insure their collateral
  
  _globals.set(gstats, _self);

  _user.modify(user, _self, [&]( auto& modified_user) { 
    modified_user.l_tesprice = l_tesprice; // annualized rate borrowers pay in periodic premiums to insure their collateral
    modified_user.l_istresscol = l_istresscol; // market determined implied percentage loss that the user collateral portfolio would experience in a stress event.
    modified_user.lastupdate = current_time_point();
    modified_user.l_premiums = l_premiums; // dollar amount of premiums borrowers would pay in one year to insure their collateral
  });
}

double vigor::stressinsx(name usern) { // same as stressins, but remove the specified user

  const auto& user = _user.get( usern.value, "User not found21" );  

  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  double portVariancex = 0.0;

  for ( auto i = gstats.insurance.begin(); i != gstats.insurance.end(); ++i ) {

    t_series stats(name("datapreprocx"),name(issuerfeed[i->symbol]).value);
    auto itr = stats.find(1);
    double iVvol = (double)itr->vol/volPrecision;
    double iW = (double)itr->price[0] / pricePrecision;
    double uW = (double)itr->price[0] / pricePrecision;
    iW *= i->amount / std::pow(10.0, i->symbol.precision());
    for ( auto u = user.insurance.begin(); u != user.insurance.end(); ++u ) {
      if (u->symbol==i->symbol){
        uW *= u->amount / std::pow(10.0, i->symbol.precision());
        iW -= uW;
        break;
      }
    }
      iW /=  (gstats.valueofins - user.valueofins);

    for (auto j = i + 1; j != gstats.insurance.end(); ++j ) {
      double c = (double)itr->correlation_matrix.at(j->symbol)/corrPrecision;

      t_series stats(name("datapreprocx"),name(issuerfeed[j->symbol]).value);
      auto itr = stats.find(1);
      double jVvol = (double)itr->vol/volPrecision;
      double jW = (double)itr->price[0] / pricePrecision;
      double uW = (double)itr->price[0] / pricePrecision;
      jW *= j->amount / std::pow(10.0, j->symbol.precision());
      for ( auto u = user.insurance.begin(); u != user.insurance.end(); ++u ) {
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

  double stressinsx = std::log(1.0 + (((std::exp(-1.0*(std::pow(-1.0*std::sqrt(2.0)*erfc_inv(2.0*alphatest),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-alphatest)) * std::sqrt(portVariancex))); // model suggested percentage loss that the total insurance asset portfolio (ex the specified user) would experience in a stress event.
  double svalueofinsx = (1.0 - stressinsx) * (gstats.valueofins  - user.valueofins); // model suggested dollar value of the total insurance asset portfolio (ex the specified user) in a stress event.
  
  return svalueofinsx;
}

double vigor::riskx(name usern)
{ // same as risk, but remove remove the specified user
  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  // market value of assets and liabilities from the perspective of insurers

  // normal markets
  double mva_n = gstats.valueofins; //market value of insurance assets in normal markets, collateral is not an asset of the insurers
  double mvl_n = 0; // no upfront is paid for tes, and insurers can walk away at any time, debt is not a liability of the insurers
  
  //stressed markets
  double svalueofinsx = stressinsx(usern);
  double mva_s = svalueofinsx;
  double mvl_s = gstats.svalueofcole;

  double own_n = mva_n - mvl_n;
  double own_s = mva_s - mvl_s;
 
  double scr = own_n - own_s;
  
  double solvencyx = own_n / scr;

  return solvencyx; // solvency without the specified insurer
}

double vigor::l_riskx(name usern)
{ // same as risk, but remove remove the specified user
  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  // market value of assets and liabilities from the perspective of insurers

  // normal markets
  double mva_n = gstats.valueofins; //market value of insurance assets in normal markets, collateral is not an asset of the insurers
  double mvl_n = 0; // no upfront is paid for tes, and insurers can walk away at any time, debt is not a liability of the insurers
  
  //stressed markets
  const auto& user = _user.get( usern.value, "User not foundl_31" );  
  double svalueofinsx = stressinsx(usern);
  double stressinsx = 1.0 - (svalueofinsx / (gstats.valueofins  - user.valueofins)); // model suggested dollar value of the total insurance asset portfolio (ex the specified user) in a stress event.
  double l_svalueofinsx = (1.0 + stressinsx) * (gstats.valueofins  - user.valueofins); // model suggested dollar value of the total insurance asset portfolio in a stress event.
  double mva_s = l_svalueofinsx;
  double mvl_s = gstats.l_svalueofcole;

  double own_n = mva_n - mvl_n;
  double own_s = mva_s - mvl_s;
 
  double scr = std::max(own_n - own_s,20.0);
  
  double solvencyx = own_n / scr;

  return solvencyx; // solvency without the specified insurer
}

double vigor::RM() {
// sum of weighted marginal contribution to risk (solvency), used for rescaling
  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  double smctr = 0;
  for ( auto it = _user.begin(); it != _user.end(); it++ ) {
    if (it->usern.value == name("finalreserve").value) // exclude the reserve because it only absorbs bailout after all insurers are wiped out, handled in reserve() method
      continue;
    double solvencyx = riskx(it->usern);
    double dRMdw =  (gstats.solvency - solvencyx);
    double w =  it->valueofins / gstats.valueofins;
    smctr +=  w * dRMdw;
  }
  return smctr;
}

double vigor::l_RM() {
// sum of weighted marginal contribution to risk (solvency), used for rescaling
  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  double smctr = 0;
  for ( auto it = _user.begin(); it != _user.end(); it++ ) {
    if (it->usern.value == name("finalreserve").value) // exclude the reserve because it only absorbs bailout after all insurers are wiped out, handled in reserve() method
      continue;
    double solvencyx = l_riskx(it->usern);
    double dRMdw =  (gstats.l_solvency - solvencyx);
    double w =  it->valueofins / gstats.valueofins;
    smctr +=  w * dRMdw;
  }
  return smctr;
}

void vigor::l_pcts(name usern, double RM) { // percent contribution to solvency

  if (usern.value == name("finalreserve").value) // exclude the reserve because it only absorbs bailout after all insurers are wiped out, handled in reserve() method
    return;
  const auto& user = _user.get( usern.value, "User not foundl_22" );
  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  double solvencyx = l_riskx(usern);
  double w =  user.valueofins / gstats.valueofins;
  double dRMdw =  (gstats.l_solvency - solvencyx);

  double pcts;
  if (RM==0.0)
    pcts =  0.0;
  else
    pcts =  w * dRMdw / RM;

  _user.modify(user, _self, [&]( auto& modified_user) { 
    modified_user.l_pcts = pcts; // percent contribution to solvency (weighted marginal contribution to risk (solvency) rescaled by sum of that
    });
    
}

void vigor::pcts(name usern, double RM) { // percent contribution to solvency

  if (usern.value == name("finalreserve").value) // exclude the reserve because it only absorbs bailout after all insurers are wiped out, handled in reserve() method
    return;
  const auto& user = _user.get( usern.value, "User not found22" );
  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  double solvencyx = riskx(usern);
  double w =  user.valueofins / gstats.valueofins;
  double dRMdw =  (gstats.solvency - solvencyx);

  double pcts;
  if (RM==0.0)
    pcts =  0.0;
  else
    pcts =  w * dRMdw / RM;

  _user.modify(user, _self, [&]( auto& modified_user) { 
    modified_user.pcts = pcts; // percent contribution to solvency (weighted marginal contribution to risk (solvency) rescaled by sum of that
    });
    
}

void vigor::payfee(name usern) {

  auto &user = _user.get( usern.value, "User not found23" );

  check(_globals.exists(), "no global table exists yet");

  globalstats gstats = _globals.get();
  
  bool late = true;

  uint64_t amt = 0;

  symbol vig =symbol("VIG", 10);

  asset amta = asset(amt, vig);
  

  uint32_t dsec = current_time_point().sec_since_epoch() - user.lastupdate.sec_since_epoch() + 1; //+1 to protect against 0

  uint32_t T = (uint32_t)(360.0 * 24.0 * 60.0 * (60.0 / (double)dsec));

  // calculating token swap pay
  double tespay = (user.debt.amount / std::pow(10.0, 4)) * (std::pow((1 + user.tesprice), (1.0 / T)) - 1); // $ amount user must pay over time T
  
  
  timer::feeclock _clock;
  eosio::time_point_sec st;  // start time
  eosio::time_point_sec et;  // expiry time
    
    auto it = user.collateral.begin();


    bool found = false;
  
    while ( !found && it++ != user.collateral.end() )
      found = (it-1)->symbol == vig; //User collateral type found
  
    t_series stats(name("datapreprocx"),name(issuerfeed[vig]).value);


    auto itr = stats.find(1);

     // number of VIG*10e4 user must pay over time T
    amta.amount = uint64_t(( tespay * std::pow(10.0, 4) ) / ((double)itr->price[0] / pricePrecision));

      if (!found){

                auto function = [&] (auto user, auto st, auto rt, auto _clock, auto _user, auto _self){
                
                    if(user.starttime == DEFAULT_TIME && user.expiry_time == DEFAULT_TIME)
                    {
                      //set clock time 
                      _clock.set_time(current_time_point().sec_since_epoch());
                      //set the starttimer 
                      st = _clock.get_time();
                      //set the expiry_timer
                      et = timer::expirydate(_clock);
                      
                      
                      _user.modify(user, _self, [&]( auto& modified_user) { 
                           // modify the user starttime
                           user.starttime = st;
                           // modify the user expiry_time
                           user.expiry_time = et;
                           // late pays gets accumulated here
                       }); 
                      
                    }
                    else if(user.starttime < user.expiry_time)
                    {
                   
                   
                    }
                    else if(user.starttime >= user.expiry_time)
                    {
                     
                     
                    }
               };
        
      }
      else {
        if (amta.amount > (it-1)->amount)
        {     

          /*_user.modify(user, _self, [&]( auto& modified_user) { // withdraw fee
            modified_user.latepays += 1;
          });*/
          
        }
        else if (amta.amount > 0){ 
         

          _user.modify(user, _self, [&]( auto& modified_user) { // withdraw fee
            modified_user.feespaid.amount += amta.amount;
            if (amta.amount == (it-1)->amount)  
              modified_user.collateral.erase(it-1); 
            else {
            modified_user.collateral[(it-1) - user.collateral.begin()] -= amta;
            }
          });

           // the global table is updated
          for ( auto itr = gstats.collateral.begin(); itr != gstats.collateral.end(); ++itr )
            if ( itr->symbol == vig ) {
              if (gstats.collateral[itr - gstats.collateral.begin()].amount - amta.amount > 0) {
                gstats.collateral[itr - gstats.collateral.begin()].amount -= amta.amount;
                gstats.valueofcol -= tespay;
              }
              else {
                gstats.collateral.erase(itr-1);
                gstats.valueofcol = 0.0;
              }
              break;
            }
          late = false;
        }
      }
  
  if (!late) {  // this block of code concerns the insurers
    uint64_t res = (uint64_t)(std::pow(10.0, 4)*(amta.amount/std::pow(10.0, 4) * reservecut));
    
    amta.amount = (uint64_t)(std::pow(10.0, 4)*(amta.amount/std::pow(10.0, 4) * (1.0-reservecut)));
    for ( auto itr = _user.begin(); itr != _user.end(); ++itr ) {
    double weight = itr->pcts; //eosio::print( "percent contribution to risk : ", weight, "\n");
      if ( weight > 0.0 ) {
        asset viga = asset(amta.amount * weight, vig);
        found = false;
        auto it = itr->insurance.begin();
        while ( !found && it++ != itr->insurance.end() )
          found = (it-1)->symbol == vig;
        if (!found && amta.amount > 0)
            _user.modify(itr, _self, [&]( auto& modified_user) { // deposit fee
              modified_user.insurance.push_back(viga);
              });
        else
            if (amta.amount > 0) {
              _user.modify( itr, _self, [&]( auto& modified_user ) { // deposit fee
              modified_user.insurance[(it-1) - itr->insurance.begin()] += viga;
              });
            }
        found = false;
        auto itg = gstats.insurance.begin();   
        while ( !found && itg++ != gstats.insurance.end() )
          found = (itg-1)->symbol == vig;  
        if (!found && amta.amount > 0) {
          gstats.insurance.push_back(viga);
          gstats.valueofins += tespay;
        }
        else if (amta.amount > 0){
          gstats.insurance[(itg-1) - gstats.insurance.begin()] += viga;
          gstats.valueofins += tespay;
        }       
      }
    }
  _globals.set(gstats, _self);
  }
  
}


void vigor::update(name usern) 
{
  auto &user = _user.get(usern.value, "User not found1");
  globalstats gstats = _globals.get();

  double valueofins = 0.0;
  double valueofcol = 0.0;
  
  for ( auto it = user.insurance.begin(); it != user.insurance.end(); ++it ) {
    t_series stats(name("datapreprocx"),name(issuerfeed[it->symbol]).value);
    auto itr = stats.find(1);
    valueofins += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( (double)itr->price[0] / pricePrecision );
  }
  for ( auto it = user.collateral.begin(); it != user.collateral.end(); ++it ){
    t_series statsj(name("datapreprocx"),name(issuerfeed[it->symbol]).value);
    auto itr = statsj.find(1);
    valueofcol += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( (double)itr->price[0] / pricePrecision );
  }

  _user.modify( user, _self, [&]( auto& modified_user ) { // Update value of collateral
    modified_user.valueofins = valueofins;
    modified_user.valueofcol = valueofcol;

  double l_valueofcol = 0.0;
  
  for ( auto it = user.l_collateral.begin(); it != user.l_collateral.end(); ++it ){
    t_series statsj(name("datapreprocx"),name(issuerfeed[it->symbol]).value);
    auto itr = statsj.find(1);
    l_valueofcol += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( (double)itr->price[0] / pricePrecision );
  }

  _user.modify( user, _self, [&]( auto& modified_user ) { // Update value of collateral
    modified_user.l_valueofcol = l_valueofcol;
  });
  });
}

void vigor::updateglobal() 
{
  check(_globals.exists(), "globals not found");
  globalstats gstats = _globals.get();

  double valueofins = 0.0;
  double valueofcol = 0.0;
  
  for ( auto it = gstats.insurance.begin(); it != gstats.insurance.end(); ++it ) {
    t_series stats(name("datapreprocx"),name(issuerfeed[it->symbol]).value);
    auto itr = stats.find(1);
    valueofins += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( (double)itr->price[0] / pricePrecision );
  }
  for ( auto it = gstats.collateral.begin(); it != gstats.collateral.end(); ++it ){
    t_series statsj(name("datapreprocx"),name(issuerfeed[it->symbol]).value);
    auto itr = statsj.find(1);
    valueofcol += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( (double)itr->price[0] / pricePrecision );
  }

  gstats.valueofins = valueofins;
  gstats.valueofcol = valueofcol;

  double l_valueofcol = 0.0;
  
  for ( auto it = gstats.l_collateral.begin(); it != gstats.l_collateral.end(); ++it ){
    t_series statsj(name("datapreprocx"),name(issuerfeed[it->symbol]).value);
    auto itr = statsj.find(1);
    l_valueofcol += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( (double)itr->price[0] / pricePrecision );
  }

  gstats.l_valueofcol = l_valueofcol;

  gstats.lastupdate = current_time_point();

  _globals.set(gstats, _self);
}

void vigor::performance(name usern) 
{
  auto &user = _user.get(usern.value, "User not found1");
  globalstats gstats = _globals.get();

  double cut = user.pcts*(1.0-reservecut);
  if (usern.value==name("finalreserve").value)
    cut = reservecut;

  double earnrate = 0.0;
  if (user.valueofins!=0.0)
    earnrate = (cut*gstats.premiums)/user.valueofins; // annualized rate of return on user portfolio of insurance crypto assets

  _user.modify( user, _self, [&]( auto& modified_user ) { // Update value of collateral
    modified_user.earnrate = earnrate;
  });
}


void vigor::l_performance(name usern) 
{
  auto &user = _user.get(usern.value, "User not found1");
  globalstats gstats = _globals.get();

  double cut = user.l_pcts*(1.0-reservecut);
  if (usern.value==name("finalreserve").value)
    cut = reservecut;

  double earnrate = 0.0;
  if (user.valueofins!=0.0)
    earnrate = (cut*gstats.l_premiums)/user.valueofins; // annualized rate of return on user portfolio of insurance crypto assets

  _user.modify( user, _self, [&]( auto& modified_user ) { // Update value of collateral
    modified_user.l_earnrate = earnrate;
  });
}

void vigor::performanceglobal() 
{
  globalstats gstats = _globals.get();

  gstats.raroc = (gstats.premiums - gstats.svalueofcoleavg)/gstats.scr; // RAROC risk adjusted return on capital. expected return on capital employed. (Revenues - Expected Loss) / SCR
  
  gstats.earnrate = 0.0;
  if (gstats.valueofins!=0.0)
    gstats.earnrate = gstats.premiums/gstats.valueofins; // annualized rate of return on total portfolio of insurance crypto assets

  _globals.set(gstats, _self);

}

void vigor::l_performanceglobal() 
{
  globalstats gstats = _globals.get();

  gstats.l_raroc = (gstats.l_premiums - gstats.l_svalueofcoleavg)/gstats.l_scr; // RAROC risk adjusted return on capital. expected return on capital employed. (Revenues - Expected Loss) / SCR
  
  gstats.l_earnrate = 0.0;
  if (gstats.valueofins!=0.0)
    gstats.l_earnrate = gstats.l_premiums/gstats.valueofins; // annualized rate of return on total portfolio of insurance crypto assets

  _globals.set(gstats, _self);

}

void vigor::reserve()
{
  globalstats gstats = _globals.get();
  auto &user = _user.get(name("finalreserve").value, "finalreserve not found");

  _user.modify(user, _self, [&]( auto& modified_user) {
      if (gstats.valueofins == user.valueofins)
        modified_user.pcts = 1.0; // trigger reserve to accept bailouts
      else
        modified_user.pcts = 0.0; // reserve does not participate in bailouts unless insurers are wiped out.
    });
}

void vigor::bailout(name usern)
{
  eosio::print( "usern : ", usern, "\n");
  auto &user = _user.get(usern.value, "User not found13");
   asset debt = user.debt;
  bool selfbailout = false;
  uint64_t n = 0; // count to identify last insurer, who will absorb dust
  uint64_t dust = 0;  // small amounts below asset precision
  uint64_t numinsurers = 0;
  double tmp;
  for ( auto itr = _user.begin(); itr != _user.end(); ++itr )
      if (itr->pcts > 0.0)
      numinsurers += 1;
  for(int m=1; m<=2; m++){
    if (m==2)
      selfbailout = true;
    double sumpcts = 0.0;
    for ( auto itr = _user.begin(); itr != _user.end(); ++itr ) {
      globalstats gstats = _globals.get();
      if (selfbailout && itr->usern.value != usern.value) // bailout self last, to make the math easier using percentages
          continue;
      if (!selfbailout && itr->usern.value == usern.value)
          continue;
      if (itr->pcts > 0.0) {
        n += 1;
        // all insurers participate to recap bad debt, each insurer taking ownership of a fraction of the imparied collateral and debt; insurer participation is based on their percent contribution to solvency
        uint64_t debtshare = debt.amount * itr->pcts; // insurer share of failed loan debt, in stablecoin
        eosio::print( "debtshare : ", debtshare, "\n");
        eosio::print( "itr->valueofins : ", itr->valueofins, "\n");
        eosio::print( "user.valueofcol : ", user.valueofcol, "\n");
        double W1 = std::min(user.valueofcol*itr->pcts,debtshare/std::pow(10.0, 4)); // insurer share of impaired collateral, in dollars
        eosio::print( "W1 : ", W1, "\n");
        double s1 = user.volcol/std::sqrt(52); // volatility of the impaired collateral portfolio, weekly
        eosio::print( "s1 : ", s1, "\n");
        double s2 = std::sqrt(portVarianceIns(itr->usern)/52.0); // volatility of the particular insurers insurance portfolio, weekly
        eosio::print( "s2 : ", s2, "\n");
        double w1 = W1/((debtshare/std::pow(10.0, 4))*(1.0+std::max(s1,s2))); // estimated percentage weight of recapped loan collateral covered by impaired collateral
        eosio::print( "w1 : ", w1, "\n");
        double sp = std::sqrt(std::pow(w1,2)*std::pow(s1,2) + std::pow(1.0-w1,2)*std::pow(s2,2) + 2.0*w1*(1.0-w1)*s1*s2); // estimated volatility of recapped loan collateral portfolio including a minimum amount of insurance assets
        // insurers auotmatically convert some of their insurance assets into collateral, and combined with the impaired collateral recapitalizes the bad debt
        // recapReq: required amount of insurance assets to be converted to collateral to recap the failed loan such that the overcollateralization amount becomes equivalent to a 1 standard deviation weekly move of the new recapped collateral portfolio
        eosio::print( "sp : ", sp, "\n");
        double recapReq = std::min((((debtshare/std::pow(10.0, 4))*(1.0+sp)) - W1)/itr->valueofins,1.0); // recapReq as a percentage of the insurers insurance assets
        eosio::print( "recapReq : ", recapReq, "\n");
        eosio::print( "itr->usern : ", itr->usern, "\n");
        eosio::print( "itr->pcts : ", itr->pcts, "\n");
        double pcts = itr->pcts*(1.0/(1.0-sumpcts));
        sumpcts += itr->pcts;
        eosio::print( "sumpcts : ", sumpcts, "\n");
        eosio::print( "pcts : ", pcts, "\n");
        // assign ownership of the impaired collateral and debt to the insurers
        for ( auto c = user.collateral.begin(); c != user.collateral.end(); ++c ) {
          asset amt = *c;
          tmp = amt.amount * pcts;
          amt.amount *= pcts;
          _user.modify(user, _self, [&]( auto& modified_user) {
                if (n==numinsurers) 
                  amt.amount += modified_user.collateral[c - user.collateral.begin()].amount - amt.amount; // adjustment for dust, so that the amount allocated to last insurer brings the collateral to zero
                modified_user.collateral[c - user.collateral.begin()] -= amt;
                eosio::print( "user collateral ", modified_user.collateral[c - user.collateral.begin()]," amt ", amt,"\n");
          });
          for ( auto it = itr->collateral.begin(); it != itr->collateral.end(); ++it ) {
            if (it->symbol == amt.symbol) {
              _user.modify(itr, _self, [&]( auto& modified_user) {
                modified_user.collateral[it - itr->collateral.begin()] += amt;
                eosio::print( "insurer collateral ", modified_user.collateral[it - itr->collateral.begin()]," amt ", amt,"\n");
              });
              amt.amount = 0;
              break;
            }
        }
          if (amt.amount > 0) 
            _user.modify(itr, _self, [&]( auto& modified_user) {
              eosio::print( "insurer collateral push_back ", amt,"\n");
              modified_user.collateral.push_back(amt);
            });
        }

        _user.modify(user, _self, [&]( auto& modified_user) {
          modified_user.collateral.erase(
              std::remove_if(modified_user.collateral.begin(), modified_user.collateral.end(),
                    [](const asset & o) { return o.amount==0; }),
              modified_user.collateral.end());
        });

        _user.modify(user, _self, [&]( auto& modified_user) {
                if (n==numinsurers)
                  debtshare += modified_user.debt.amount - debtshare; // the amount allocated to last insurer should bring the debt to zero otherwise it is dust leftover
                modified_user.debt.amount -= debtshare;
                eosio::print( "modified_user.debt.amount ", modified_user.debt.amount, " debtshare", debtshare,"\n");
        });

        _user.modify(itr, _self, [&]( auto& modified_user) {
          modified_user.debt.amount += debtshare;
        });

        //convert some insurance into collateral, recapitalizing the bad debt
        for ( auto i = itr->insurance.begin(); i != itr->insurance.end(); ++i ) {
          asset amt = *i;
          amt.amount *= recapReq;
          
          _user.modify(itr, _self, [&]( auto& modified_user) {
                modified_user.insurance[i - itr->insurance.begin()] -= amt;
                eosio::print( "insurer insurance ", modified_user.insurance[i - itr->insurance.begin()]," amt ", amt,"\n");
          });

          bool found = false;
          auto itg = gstats.insurance.begin();   
          while ( !found && itg++ != gstats.insurance.end() )
            found = (itg-1)->symbol == i->symbol;
          if (found && amt.amount > 0)
            gstats.insurance[(itg-1) - gstats.insurance.begin()] -= amt;

         found = false;
         for ( auto it = itr->collateral.begin(); it != itr->collateral.end(); ++it )
            if ( it->symbol == amt.symbol ) {
              _user.modify(itr, _self, [&]( auto& modified_user) {
                modified_user.collateral[it - itr->collateral.begin()] += amt;
                eosio::print( "insurer collateral ", modified_user.collateral[it - itr->collateral.begin()]," amt ", amt,"\n");
              });
              found = true;
              break;
            }
          if (!found) 
            _user.modify(itr, _self, [&]( auto& modified_user) {
              eosio::print( "insurer collateral push_back ", amt,"\n");
              modified_user.collateral.push_back(amt);
            });

          found = false;
          itg = gstats.collateral.begin();   
          while ( !found && itg++ != gstats.collateral.end() )
            found = (itg-1)->symbol == amt.symbol;  
          if (!found && amt.amount > 0) {
            gstats.collateral.push_back(amt);
          }
          else if (amt.amount > 0){
            gstats.collateral[(itg-1) - gstats.collateral.begin()] += amt;
          }

        }
          _user.modify(itr, _self, [&]( auto& modified_user) {
          modified_user.insurance.erase(
              std::remove_if(modified_user.insurance.begin(), modified_user.insurance.end(),
                    [](const asset & o) { return o.amount==0; }),
              modified_user.insurance.end());
        });
          gstats.insurance.erase(
              std::remove_if(gstats.insurance.begin(), gstats.insurance.end(),
                    [](const asset & o) { return o.amount==0; }),
              gstats.insurance.end());
          gstats.collateral.erase(
              std::remove_if(gstats.collateral.begin(), gstats.collateral.end(),
                    [](const asset & o) { return o.amount==0; }),
              gstats.collateral.end());
      }
      _globals.set(gstats, _self);
    }
  }
}



extern "C" {
  void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if((code==name("eosio.token").value ||
        code==name("vig111111111").value ||
        code==name("dummytokensx").value) && action==name("transfer").value) {
      eosio::execute_action(name(receiver),name(code), &vigor::assetin);
    }
    if (code == receiver) {
      switch (action) { 
          EOSIO_DISPATCH_HELPER(vigor, (create)(assetout)(issue)(transfer)(open)(close)(retire)(setsupply) )
      }    
    }
    eosio_exit(0);
  }
}
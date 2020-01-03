#include <vigor.hpp>

void vigor::doupdate()
{
      //_user.flush();
      //require_auth(_self);
      ///eosio::print( "update called.", "\n");
      for ( auto it = _user.begin(); it != _user.end(); it++ )
        update(it->usern);
      updateglobal();

     //_user.flush();
     //for ( auto it = _user.begin(); it != _user.end(); it++ ) {
    //   if ( it->debt.amount > 0 || it->l_valueofcol > 0.0  ) 
        // payfee(it->usern);
    // }

  //    for ( auto it = _user.begin(); it != _user.end(); it++ ) 
   //     update(it->usern);
    //  updateglobal();

    bool exitbailout;
    bool onetime = false;
    for (int i=1; i<10; i++){
      eosio::print( "bailout loop count: ", i, "\n");
      exitbailout = true;
      for ( auto it = _user.begin(); it != _user.end(); it++ ) {

        //allow bailout/recollateralize the reserve, but only one time
        if (onetime && (it->usern.value == name("finalreserve").value))
          continue; 
        if (it->usern.value == name("finalreserve").value)
          onetime = true;
        
        //auto &user = _user.get(it->usern.value,"User not found14");
        /*  if (it->debt.amount > 0 && it->latepays > 4) {
            _user.modify(user, _self, [&]( auto& modified_user) {
              modified_user.latepays = 0; 
              modified_user.recaps += 1;
            });
            bailout(it->usern);
            exitbailout=false;
            break;
          }*/
          if (( it->debt.amount / std::pow(10.0, 4) ) > it->valueofcol ) {
            _user.modify(it, _self, [&]( auto& modified_user) {
              modified_user.recaps += 1;
            });
            bailout(it->usern);
            //_user.flush();
            exitbailout=false;
            break;
          }
          else if (it->l_valueofcol > ( it->l_debt.amount / std::pow(10.0, 4) )) {
            _user.modify(it, _self, [&]( auto& modified_user) {
              modified_user.recaps += 1;
            });
            bailoutup(it->usern);
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
      //_user.flush();
      
      for ( auto it = _user.begin(); it != _user.end(); it++ )
        stresscol(it->usern);
        //_user.flush();
      
      for ( auto it = _user.begin(); it != _user.end(); it++ )
        l_stresscol(it->usern);
        //_user.flush();

      stressins();
      //_user.flush();
      l_stressins();

      risk();
      //_user.flush();
      l_risk();
      //_user.flush();

      for ( auto it = _user.begin(); it != _user.end(); it++ ) 
        pricing(it->usern);
      //_user.flush(); 

      for ( auto it = _user.begin(); it != _user.end(); it++ ) 
        l_pricing(it->usern);
      //_user.flush(); 

      double rm = RM();
      //_user.flush(); 
      for ( auto it = _user.begin(); it != _user.end(); it++ )
        pcts(it->usern,rm);
      //_user.flush(); 

      double l_rm = l_RM();
      //_user.flush(); 
      for ( auto it = _user.begin(); it != _user.end(); it++ )
        l_pcts(it->usern,l_rm);
      //_user.flush(); 

      reserve();
      //_user.flush(); 

      for ( auto it = _user.begin(); it != _user.end(); it++ ) 
        performance(it->usern);
      //_user.flush(); 

      for ( auto it = _user.begin(); it != _user.end(); it++ ) 
        l_performance(it->usern);
      //_user.flush(); 

      performanceglobal();
      //_user.flush(); 
      l_performanceglobal();
      //_user.flush(); 

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
      for ( auto it = _user.begin(); it != _user.end(); it++ ) {
        if ( it->debt.amount > 0 || it->l_valueofcol > 0.0  ) 
          payfee(it->usern);
      }

        if(memo.c_str() == string("collateral")){
                // Transfer stablecoin into user for use as collateral to borrow crypto
               
                auto itr = _user.find(from.value);
                if ( itr == _user.end() ) {
                  itr = _user.emplace(_self, [&](auto& new_user) {
                    new_user.usern = from;
                    new_user.l_debt = quantity;
                  });
                } else {
                  //auto &user = *itr;
                  _user.modify(itr, _self, [&]( auto& modified_user) {
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

                _user.modify(itr, _self, [&]( auto& modified_user) {
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
            auto useritr = _user.find(from.value);

            check(user.debt.amount >= quantity.amount, "Payment too high");
            
            globalstats gstats;
            if (_globals.exists())
              gstats = _globals.get();
      
            _user.modify(useritr, _self, [&]( auto& modified_user) {
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

  // VIG precision swapped from 4 to 10
  if (assetin.symbol == symbol("VIG", 4))
    assetin = swap_precision::swapprecision(assetin);
  
  require_auth( from );
  check( from != to, "cannot transfer to self" );
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
  
  for ( auto it = _user.begin(); it != _user.end(); it++ ) {
    if ( it->debt.amount > 0 || it->l_valueofcol > 0.0  ) 
      payfee(it->usern);
  }

  auto itr = _user.find(from.value);
  if ( itr == _user.end() ) {
    
    itr = _user.emplace(_self, [&](auto& new_user) {
      new_user.usern = from;
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

  if (memo.c_str() == string("insurance")) {

    // transfer tokens into insurance (not stablecoin)
    ///eosio::print( "transfer tokens into insurance (not stablecoin)","\n");
    globalstats gstats;
    if (_globals.exists())
      gstats = _globals.get();

    auto it = user.insurance.begin();

    while ( !found && it++ != user.insurance.end() )
        found = (it-1)->symbol == assetin.symbol; 

    _user.modify(itr, _self, [&]( auto& modified_user) {
      if (!found)
        modified_user.insurance.push_back(assetin);
      else
        modified_user.insurance[(it-1) - user.insurance.begin()] += assetin;
    }); found = false;
    it = gstats.insurance.begin();
    while ( !found && it++ != gstats.insurance.end() )
      found = (it-1)->symbol == assetin.symbol;
    if ( !found )
      gstats.insurance.push_back(assetin);
    else
      gstats.insurance[(it-1) - gstats.insurance.begin()] += assetin;
    _globals.set(gstats, _self);
  }
  else if (memo.c_str() == string("collateral")) {

    // transfer tokens into collateral (not stablecoin)
    ///eosio::print( "transfer tokens into collateral (not stablecoin)","\n");
    globalstats gstats;
    if (_globals.exists())
      gstats = _globals.get();

    auto it = user.collateral.begin();
    while ( !found && it++ != user.collateral.end() )
      found = (it-1)->symbol == assetin.symbol; 

    _user.modify(itr, _self, [&]( auto& modified_user) {
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
    _globals.set(gstats, _self);
  } 
  else if (memo.c_str() == string("payback borrowed token")) {
    payback_borrowed_token(from, assetin);
    _user.modify(itr, _self, [&]( auto& modified_user) {
      modified_user.l_collateral.erase(
          std::remove_if(modified_user.l_collateral.begin(), modified_user.l_collateral.end(),
                [](const asset & o) { return o.amount==0; }),
          modified_user.l_collateral.end());
    });
  }
  doupdate();
}

void vigor::payback_borrowed_token(name from, asset  assetin) {
  globalstats gstats;
  if (_globals.exists())
    gstats = _globals.get();
  auto itr = _user.find(from.value);
  auto &user = *itr;

    //payback borrowed token into user l_collateral (not stablecoin)
    eosio::print( "payback borrowed token into user l_collateral (not stablecoin)","\n");
    auto itc = user.l_collateral.begin();
    bool found = false;
    while ( !found && itc++ != user.l_collateral.end() )
      found = (itc-1)->symbol == assetin.symbol;
    if (!found)
      check(false,"Can't payback that asset; not found in user borrows");
    else
      check(user.l_collateral[(itc-1) - user.l_collateral.begin()] >= assetin,"Payback amount too high.");
    found = false;
    //loop through lending receipts in reinvestment to identify which ones to pay off
    eosio::print( "loop through lending receipts in reinvestment to identify which ones to pay off","\n");
    asset locatesremaining = assetin;
    asset paymentasset = asset( 0, symbol("VIGOR", 4) );
    asset amt = assetin;
    double pct;
     auto &re = _user.get(name("reinvestment").value, "reinvestment not found");
     auto reitr = _user.find(name("reinvestment").value);
      eosio::print( "re.l_lrtoken.size() ", re.l_lrtoken.size(), "\n");  
      if (!re.l_lrtoken.empty()) {
          for ( auto it = re.l_lrtoken.begin(); it != re.l_lrtoken.end(); ++it ) {
            if (locatesremaining.amount==0)
              break;
            if (it->symbol == assetin.symbol && re.l_lrname[it - re.l_lrtoken.begin()].value == user.usern.value) {
              amt.amount = std::min(re.l_lrtoken[it - re.l_lrtoken.begin()].amount, locatesremaining.amount);
              pct = (double)amt.amount/(double)re.l_lrtoken[it - re.l_lrtoken.begin()].amount;
              eosio::print( "pct ", pct,"\n");
              locatesremaining -= amt;
              eosio::print( "locatesremaining ", locatesremaining,"\n");
              _user.modify(reitr, _self, [&]( auto& modified_user) {

              // subtract assetin from the lending receipt
              eosio::print( "subtract assetin from the lending receipt","\n");
              if (modified_user.l_lrtoken[it - re.l_lrtoken.begin()].amount - amt.amount == 0.0){
                paymentasset = modified_user.l_lrpayment[it - re.l_lrtoken.begin()];
                modified_user.l_lrtoken[it - re.l_lrtoken.begin()].amount = 0;
                modified_user.l_lrpayment[it - re.l_lrtoken.begin()].amount = 0;
                modified_user.l_lrname[it - re.l_lrtoken.begin()] = name("delete");
                eosio::print( "erase l_lrtoken", amt,"\n");
                eosio::print( "erase paymentasset ", paymentasset,"\n");
              } else {
                modified_user.l_lrtoken[it - re.l_lrtoken.begin()] -= amt;
                paymentasset.amount = pct*modified_user.l_lrpayment[it - re.l_lrtoken.begin()].amount;
                modified_user.l_lrpayment[it - re.l_lrtoken.begin()] -= paymentasset;
                eosio::print( "paymentasset ", paymentasset,"\n");
                eosio::print( "pct ", pct,"\n");
                eosio::print( "lender l_lrpayment ", modified_user.l_lrpayment[it - re.l_lrtoken.begin()]  ," amt ", paymentasset,"\n");
                eosio::print( "lender l_lrtoken ", modified_user.l_lrtoken[it - re.l_lrtoken.begin()] ," amt ", amt,"\n");
              }
              });
             // subtract located asset from borrower l_collateral
              eosio::print( "subtract located asset from borrower l_collateral","\n");
              _user.modify(itr, _self, [&]( auto& modified_user) {
                modified_user.l_collateral[(itc-1) - user.l_collateral.begin()] -= amt;
              eosio::print( "modified_user.l_collateral[(itc-1) - user.l_collateral.begin()] ", modified_user.l_collateral[(itc-1) - user.l_collateral.begin()],"\n");
              });
              // subtract the located asset from the global l_collateral
              eosio::print( "subtract the located asset from the global l_collateral","\n");
              found = false;
              auto itg = gstats.l_collateral.begin();
              while ( !found && itg++ != gstats.l_collateral.end() )
                found = (itg-1)->symbol == amt.symbol;
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
/* The reinvestment feature disabled until further research is completed
              // subtract payment asset from reinvestment insurance
              eosio::print( "subtract payment asset from reinvestment insurance","\n");
              found = false;
              auto itre = re.insurance.begin();
              while ( !found && itre++ != re.insurance.end() )
                found = (itre-1)->symbol == paymentasset.symbol;  
              if (found) {
                _user.modify(re, _self, [&]( auto& modified_user) {
                if (modified_user.insurance[(itre-1) - re.insurance.begin()].amount - paymentasset.amount == 0)
                  modified_user.insurance.erase(itre-1);
                else 
                  modified_user.insurance[(itre-1) - re.insurance.begin()] -= paymentasset;
                });
              }
              else
                check(false,"payment asset not found in reinvestment account");
              
              // subtract payment asset from global insurance
              eosio::print( "subtract payment asset from global insurancee","\n");
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
              // don't add the payment amount to user l_debt (so user still sees it)
              // don't add the payment amount to global l_totaldebt
*/

              //add located asset to global insurance
              eosio::print( "add located asset to global insurance","\n");
              found = false;
              itg = gstats.insurance.begin();
              while ( !found && itg++ != gstats.insurance.end() )
                found = (itg-1)->symbol == amt.symbol;
              if ( !found ) {
                gstats.insurance.push_back(amt);
                eosio::print( "push_back", amt,"\n");
              }
              else {
                gstats.insurance[(itg-1) - gstats.insurance.begin()] += amt;
                eosio::print( "gstats.insurance[(itg-1) - gstats.insurance.begin()]", gstats.insurance[(itg-1) - gstats.insurance.begin()],"\n");
              }
              // don't add the located token to any particular insurer, just add it to global insurance
              _globals.set(gstats, _self);
          }
          }
              _user.modify(reitr, _self, [&]( auto& modified_user) { //removed vector elements with zero amount
              modified_user.l_lrtoken.erase(
                  std::remove_if(modified_user.l_lrtoken.begin(), modified_user.l_lrtoken.end(),
                        [](const asset & o) { return o.amount==0; }),
                  modified_user.l_lrtoken.end());
              });
              _user.modify(reitr, _self, [&]( auto& modified_user) { //removed vector elements with zero amount
              modified_user.l_lrpayment.erase(
                  std::remove_if(modified_user.l_lrpayment.begin(), modified_user.l_lrpayment.end(),
                        [](const asset & o) { return o.amount==0; }),
                  modified_user.l_lrpayment.end());
              });
              _user.modify(reitr, _self, [&]( auto& modified_user) { //removed vector elements with zero amount
              modified_user.l_lrname.erase(
                  std::remove_if(modified_user.l_lrname.begin(), modified_user.l_lrname.end(),
                        [](const name & o) { return o.value==name("delete").value; }),
                  modified_user.l_lrname.end());
              });
      }
    eosio::print( "locatesremaining.amount ", locatesremaining.amount,"\n");
    check(locatesremaining.amount==0,"Not enough locates receipts found");
    }

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

  for ( auto it = _user.begin(); it != _user.end(); it++ ) {
    if ( it->debt.amount > 0 || it->l_valueofcol > 0.0 ) 
      payfee(it->usern);
  }

  check(_globals.exists(), "globals don't exist");
  globalstats gstats = _globals.get();
  bool found = false;

  // VIG precision swapped from 4 to 10
  if (assetout.symbol == symbol("VIG", 4))
  assetout = swap_precision::swapprecision(assetout);

  auto &user = _user.get( usern.value,"User not found16" );
  auto useritr = _user.find( usern.value );

  if ( memo.c_str() == string("borrow") && assetout.symbol == symbol("VIGOR", 4) ) {
    // borrow stablecoins against crypto as collateral
    asset debt = user.debt + assetout;

    // if overcollateralization is C then leverage L = 1 / ( 1 - ( 1 / C ) )
    check( user.valueofcol >= 1.11 * ( debt.amount / std::pow(10.0, 4) ),
    "Collateral must exceed borrowings by 1.11" );
    
    _user.modify(useritr, _self, [&]( auto& modified_user) {
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
        if (it->symbol == assetout.symbol) { // User insurance type found
          check( it->amount >= assetout.amount,
          "Insufficient insurance assets available." );
          if ( it->amount - assetout.amount == 0 )
            _user.modify(useritr, _self, [&]( auto& modified_user) {
              modified_user.insurance.erase(it);
            });
          else 
            _user.modify(useritr, _self, [&]( auto& modified_user) {
              modified_user.insurance[it - user.insurance.begin()] -= assetout;
            });
          found = true;
          break;
        }
      if (found)
      for ( auto it = gstats.insurance.begin(); it != gstats.insurance.end(); ++it )
        if ( it->symbol == assetout.symbol ) {
          if ( it->amount - assetout.amount == 0 )
            gstats.insurance.erase(it);
          else
            gstats.insurance[it - gstats.insurance.begin()] -= assetout;
          break;
        }
      _globals.set(gstats, _self);
    }
    else if ( memo.c_str() == string("collateral") && assetout.symbol != symbol("VIGOR", 4) ) {
      // withdraw tokens from collateral
      for ( auto it = user.collateral.begin(); it != user.collateral.end(); ++it )
        if ( it->symbol == assetout.symbol ) { //User collateral type found
          check( it->amount >= assetout.amount,
          "Insufficient collateral assets available." );
          
          double valueofasset = assetout.amount / std::pow(10.0, it->symbol.precision());

          t_series stats(name("datapreproc2"),name(issuerfeed[assetout.symbol]).value);
          auto itr = stats.find(1);
          check(itr != stats.end(),"asset not found in the datapreprocessor, or precision invalid");
          valueofasset *= (double)itr->price[0] / pricePrecision;

          double valueofcol = user.valueofcol - valueofasset;

          check( valueofcol >= 1.11 * ( user.debt.amount / std::pow(10.0, 4) ),
          "Collateral must exceed borrowings by 1.11"   );
          
          if ( it->amount - assetout.amount == 0 )
            _user.modify(useritr, _self, [&]( auto& modified_user) {
              modified_user.collateral.erase(it);
            });
          else
            _user.modify(useritr, _self, [&]( auto& modified_user) {
              modified_user.collateral[it - user.collateral.begin()] -= assetout;
            });
          found = true;
          break;
        }
      if (found)
      for ( auto it = gstats.collateral.begin(); it != gstats.collateral.end(); ++it )
        if ( it->symbol == assetout.symbol ) {
          if ( it->amount - assetout.amount == 0 )
            gstats.collateral.erase(it);
          else
            gstats.collateral[it - gstats.collateral.begin()] -= assetout;
          break;
        }
      _globals.set(gstats, _self);
    }
    else if ( memo.c_str() == string("borrow") && assetout.symbol != symbol("VIGOR", 4) ) {
    // borrow tokens against stablecoin as collateral

    // Example: user has 50 VIGOR in l_debt as collateral against which user requests borrow 10 EOS transfered out
    // locate crypto for borrow in the insurance pool
    // a lending receipt is created into reinvestment account: ex: 10 EOS @ 30 VIGOR to user: l_lrtoken @ l_lrpayment to l_lrname
    // 10 EOS booked into user (and global) l_collateral as a borrow, and 10 EOS subtracted from global insurance
    // this line requires further research -> 30 VIGOR booked into reinvestment (and global) insurance to earn VIG

    auto &user = _user.get(usern.value, "User not found");
    globalstats gstats = _globals.get();

    t_series stats(name("datapreproc2"),name(issuerfeed[assetout.symbol]).value);
    auto itrp = stats.find(1);
    check(itrp != stats.end(),"asset not found in the datapreprocessor, or precision invalid");
    double valueofassetout = (assetout.amount) / std::pow(10.0, assetout.symbol.precision()) * 
                  ( (double)itrp->price[0] / pricePrecision );

    ///eosio::print( "valueofassetout : ", valueofassetout, "\n");
    ///eosio::print( "user.l_valueofcol : ", user.l_valueofcol, "\n");
    ///eosio::print( "user.l_debt.amount / std::pow(10.0, 4) ) / 1.11 = ", ( user.l_debt.amount / std::pow(10.0, 4) ) / 1.11, "\n");

    check( user.l_valueofcol + valueofassetout <= ( user.l_debt.amount / std::pow(10.0, 4) ) / 1.11,
         "Collateral must exceed borrowings by 1.111: " );

    // can't borrow from the finalreserve for new borrows, so record the amount to remove it from gstats.insurance
    auto &finalreserve = _user.get(name("finalreserve").value, "finalreserve not found");
    auto it = finalreserve.insurance.begin();
    asset lr = assetout;
    while ( !found && it++ != finalreserve.insurance.end() )
       found = (it-1)->symbol == assetout.symbol;
    if (found)
      lr.amount = (it-1)->amount;
    else
      lr.amount = 0;

    // can't borrow from the reinvestment for new borrows, so record the amount to remove it from gstats.insurance
    found = false;
    auto &reinvestment = _user.get(name("reinvestment").value, "reinvestment not found");
    auto reinvestmentitr = _user.find(name("reinvestment").value);
    it = reinvestment.insurance.begin();
    asset ri = assetout;
    while ( !found && it++ != reinvestment.insurance.end() )
       found = (it-1)->symbol == assetout.symbol;
    if (found)
      ri.amount = (it-1)->amount;
    else
      ri.amount = 0;

    ///eosio::print( "find lends_outstanding in global l_collateral","\n");
    found = false;
    it = gstats.l_collateral.begin();
    asset lends_outstanding = assetout;
    lends_outstanding.amount =0;
    while ( !found && it++ != gstats.l_collateral.end() )
      found = (it-1)->symbol == assetout.symbol;
    if (found)
      lends_outstanding.amount = gstats.l_collateral[(it-1) - gstats.l_collateral.begin()].amount;
    ///eosio::print( "lends_outstanding: ", lends_outstanding,"\n");

    // locate the requested asset in the global insurance for lending
    ///eosio::print( "locate the requested asset in the global insurance for lending: ", assetout,"\n");
    found = false;
    it = gstats.insurance.begin();
    asset paymentasset = asset( 0, symbol("VIGOR", 4) );
    while ( !found && it++ != gstats.insurance.end() )
      found = (it-1)->symbol == assetout.symbol;
    if (found) {
            ///eosio::print( "available: ", gstats.insurance[(it-1) - gstats.insurance.begin()].amount - lr.amount - ri.amount, " ", assetout.symbol, "\n");
      if (((gstats.insurance[(it-1) - gstats.insurance.begin()].amount - lr.amount - ri.amount)*(1.0-maxlends) - lends_outstanding.amount - assetout.amount) >= 0.0) {
          ///eosio::print( "there is sufficient supply in global insurance for lending: (available - lends_outstanding - assetout) >= available * maxlends","\n");
          // create or modify existing lending receipts
          found = false;
          auto itr = reinvestment.l_lrname.begin();
          while ( !found && itr++ != reinvestment.l_lrname.end() )
            found = ((itr-1)->value == user.usern.value && reinvestment.l_lrtoken[(itr-1) - reinvestment.l_lrname.begin()].symbol == assetout.symbol);
          _user.modify(reinvestmentitr, _self, [&]( auto& modified_user) {
            paymentasset.amount = std::pow(10.0, 4)*((assetout.amount) / std::pow(10.0, assetout.symbol.precision()) * ( (double)itrp->price[0] / pricePrecision ));
            if (!found){
              // create a lending receipt on the reinvestment account
              ///eosio::print( "create a lending receipt on the reinvestment account: ", usern, " ", assetout, " ", paymentasset ,"\n");
              modified_user.l_lrtoken.push_back(assetout);
              modified_user.l_lrpayment.push_back(paymentasset);
              modified_user.l_lrname.push_back(usern);
            }
            else {
              // add to the existing lending receipt on the reinvestment account
              ///eosio::print( "add to the existing lending receipt on the reinvestment account: ", usern, " ", assetout, " ", paymentasset ,"\n");
              modified_user.l_lrtoken[(itr-1) - reinvestment.l_lrname.begin()].amount += assetout.amount;
              modified_user.l_lrpayment[(itr-1) - reinvestment.l_lrname.begin()].amount += paymentasset.amount;
              ///eosio::print( "modified_user.l_lrtoken[(itr-1) - reinvestment.l_lrname.begin()].amount: ", modified_user.l_lrtoken[(itr-1) - reinvestment.l_lrname.begin()].amount,"\n");
              ///eosio::print( "modified_user.l_lrpayment[(itr-1) - reinvestment.l_lrname.begin()].amount ", modified_user.l_lrpayment[(itr-1) - reinvestment.l_lrname.begin()].amount,"\n");
            }
          }); found = false;
/* The reinvestment feature disabled until further research is completed
          // add payment asset to reinvestment insurance so it earns reinvestment rate
          eosio::print( "add payment asset to reinvestment insurance so it earns reinvestment rate: ", paymentasset,"\n");
          found = false;
          auto it = reinvestment.insurance.begin();
          while ( !found && it++ != reinvestment.insurance.end() )
            found = (it-1)->symbol == paymentasset.symbol;
          _user.modify(reinvestment, _self, [&]( auto& modified_user) {
            if (!found){
              modified_user.insurance.push_back(paymentasset);
             eosio::print( "push_back: ", paymentasset,"\n");
            }
            else {
              modified_user.insurance[(it-1) - reinvestment.insurance.begin()] += paymentasset;
              eosio::print( "modified_user.insurance[(it-1) - reinvestment.insurance.begin()]: ", modified_user.insurance[(it-1) - reinvestment.insurance.begin()],"\n");
            }
          }); found = false;

          // add payment asset to global insurance
          eosio::print( "add payment asset to global insurance: ", paymentasset,"\n");
          it = gstats.insurance.begin();
          while ( !found && it++ != gstats.insurance.end() )
            found = (it-1)->symbol == paymentasset.symbol;
          if ( !found ) {
            gstats.insurance.push_back(paymentasset);
            eosio::print( "push_back: ", paymentasset,"\n");
          }
          else {
            gstats.insurance[(it-1) - gstats.insurance.begin()] += paymentasset;
            eosio::print( "gstats.insurance[(it-1) - gstats.insurance.begin()]: ", gstats.insurance[(it-1) - gstats.insurance.begin()],"\n");
          }
          // don't subtract the payment amount from user l_debt (so user still sees it)
          // don't subtract the payment amount from global l_totaldebt
*/
          // add the located asset to the global l_collateral
          ///eosio::print( "add the located asset to the global l_collateral: ", assetout,"\n");
          found = false;
          auto itg = gstats.l_collateral.begin();
          while ( !found && itg++ != gstats.l_collateral.end() )
            found = (itg-1)->symbol == assetout.symbol;
          if ( !found ) {
            gstats.l_collateral.push_back(assetout);
            ///eosio::print( "push_back: ", assetout,"\n");
          }
          else {
            gstats.l_collateral[(itg-1) - gstats.l_collateral.begin()] += assetout;
            ///eosio::print( "gstats.l_collateral[(itg-1) - gstats.l_collateral.begin()]: ", gstats.l_collateral[(itg-1) - gstats.l_collateral.begin()],"\n");
          }

          // add the located asset to the borrower l_collateral
          ///eosio::print( "add the located asset to the borrower l_collateral: ", assetout,"\n");
          found = false;
          it = user.l_collateral.begin();
          while ( !found && it++ != user.l_collateral.end() )
            found = (it-1)->symbol == assetout.symbol;
          _user.modify(useritr, _self, [&]( auto& modified_user) {
            if (!found) {
              modified_user.l_collateral.push_back(assetout);
              ///eosio::print( "push_back: ", assetout,"\n");
            }
            else {
              modified_user.l_collateral[(it-1) - user.l_collateral.begin()] += assetout;
              ///eosio::print( " modified_user.l_collateral[(it-1) - user.l_collateral.begin()]: ",  modified_user.l_collateral[(it-1) - user.l_collateral.begin()],"\n");
            }
          });

          // subtract the located asset from the global insurance
          ///eosio::print( "subtract the located asset from the global insurance","\n");
          found = false;
          itg = gstats.insurance.begin();
          while ( !found && itg++ != gstats.insurance.end() )
            found = (itg-1)->symbol == assetout.symbol;
          ///eosio::print( "found", found,"\n");
          if ( !found )
            check(false,"payment asset not found in global insurance");
          else {
            if (gstats.insurance[(itg-1) - gstats.insurance.begin()].amount - assetout.amount == 0){
              ///eosio::print( "gstats.insurance[(itg-1) - gstats.insurance.begin()] erased", gstats.insurance[(itg-1) - gstats.insurance.begin()],"\n");
              gstats.insurance.erase(itg-1);
              } 
            else {
              gstats.insurance[(itg-1) - gstats.insurance.begin()] -= assetout;
              ///eosio::print( "gstats.insurance[(itg-1) - gstats.insurance.begin()]", gstats.insurance[(itg-1) - gstats.insurance.begin()],"\n");
            }
          }
          // don't subtract the located asset from any particular insurer, just subtract from global insurance
      } else {
          check(false,"Can't locate enough to borrow"); 
      }
    }
    _globals.set(gstats, _self);
    found = true;
    
  } else if ( memo.c_str() == string("collateral") && assetout.symbol == symbol("VIGOR", 4) ) {
    // withdraw stablecoins from collateral 

    asset l_debt = user.l_debt - assetout;

    // if overcollateralization is C then leverage L = 1 / ( 1 - ( 1 / C ) )
    check( user.l_valueofcol * 1.11 <= ( l_debt.amount / std::pow(10.0, 4) ),
    "Collateral must exceed borrowings by 1.11"  );
    found = true;
    
    _user.modify(useritr, _self, [&]( auto& modified_user) {
      modified_user.l_debt = l_debt;
    });
    gstats.l_totaldebt -= assetout;

    _globals.set(gstats, _self);

    }
    
    check(found, "asset not found in user");
    ///eosio::print( "transfer borrowed tokens to user: ", assetout, " ", usern, "\n");
    // VIG precision swapped from 10 to 14
    if (assetout.symbol == symbol("VIG", 10))
      assetout = swap_precision::swapprecision(assetout);
    action( permission_level{_self, name("active")},
            issueracct[assetout.symbol], name("transfer"),
            std::make_tuple(_self, usern, assetout, memo
          )).send();
  }

    doupdate();
}

void vigor::stresscol(name usern) {

  const auto& user = _user.get( usern.value, "User not found17" );
  const auto& useritr = _user.find( usern.value);  
  
  check(_globals.exists(), "globals not found");
  globalstats gstats = _globals.get();

  double portVariance = portVarianceCol(usern);

  double stresscol = -1.0*(std::exp(-1.0*(((std::exp(-1.0*(std::pow(NormalCDFInverse(alphatest),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-alphatest)) * std::sqrt(portVariance)))-1.0);

  double svalueofcol = ((1.0 - stresscol) * user.valueofcol);
  double svalueofcole = std::max( 0.0,
    user.debt.amount / std::pow(10.0, 4) - ((1.0 - stresscol) * user.valueofcol)
  );
  gstats.svalueofcole += svalueofcole - user.svalueofcole; // model suggested dollar value of the sum of all insufficient collateral in a stressed market

  double stresscolavg = -1.0*(std::exp(-1.0*(((std::exp(-1.0*(std::pow(NormalCDFInverse(0.5),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-0.5)) * std::sqrt(portVariance)))-1.0);
  double svalueofcoleavg = std::max( 0.0,
    user.debt.amount / std::pow(10.0, 4) - ((1.0 - stresscolavg) * user.valueofcol)
  );

  gstats.svalueofcoleavg += svalueofcoleavg - user.svalueofcoleavg; // model suggested dollar value of the sum of all insufficient collateral on average in stressed markets, expected loss
  
  _globals.set(gstats, _self);

  _user.modify(useritr, _self, [&]( auto& modified_user) { 
    modified_user.volcol = std::sqrt(portVariance); // volatility of the user collateral portfolio
    modified_user.stresscol = stresscol; // model suggested percentage loss that the user collateral portfolio would experience in a stress event.
    modified_user.svalueofcol = svalueofcol; // model suggested dollar value of the user collateral portfolio in a stress event.
    modified_user.svalueofcole = svalueofcole; // model suggested dollar amount of insufficient collateral of a user loan in a stressed market.
    modified_user.svalueofcoleavg = svalueofcoleavg; // model suggested dollar amount of insufficient collateral of a user loan on average in stressed markets, expected loss
  });

}

void vigor::l_stresscol(name usern) {

  const auto& user = _user.get( usern.value, "User not foundl_17" );
  const auto useritr = _user.find( usern.value );
  
  check(_globals.exists(), "globals not found");
  globalstats gstats = _globals.get();

  double l_portVariance = l_portVarianceCol(usern);
 
  double l_stresscol = std::exp(((std::exp(-1.0*(std::pow(NormalCDFInverse(alphatest),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-alphatest)) * std::sqrt(l_portVariance))-1.0;
 
  double l_svalueofcol = ((1.0 + l_stresscol) * user.l_valueofcol);
  double l_svalueofcole = std::max( 0.0,
    ((1.0 + l_stresscol) * user.l_valueofcol) - user.l_debt.amount / std::pow(10.0, 4)
  );
  gstats.l_svalueofcole += l_svalueofcole - user.l_svalueofcole; // model suggested dollar value of the sum of all insufficient collateral in a stressed market

  double l_stresscolavg = std::exp(((std::exp(-1.0*(std::pow(NormalCDFInverse(0.5),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-0.5)) * std::sqrt(l_portVariance))-1.0;
  double l_svalueofcoleavg = std::max( 0.0,
    ((1.0 + l_stresscolavg) * user.l_valueofcol) - user.l_debt.amount / std::pow(10.0, 4)
  );

  gstats.l_svalueofcoleavg += l_svalueofcoleavg - user.l_svalueofcoleavg; // model suggested dollar value of the sum of all insufficient collateral on average in stressed markets, expected loss
  
  _globals.set(gstats, _self);

  _user.modify(useritr, _self, [&]( auto& modified_user) { 
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
    
    t_series stats(name("datapreproc2"),name(issuerfeed[i->symbol]).value);
    auto itr = stats.find(1);
    double iVvol = (double)itr->vol/volPrecision;
    double iW = (double)itr->price[0] / pricePrecision;
    iW *= i->amount / std::pow(10.0, i->symbol.precision()); 
    iW /= user.valueofcol;

  for (auto j = i + 1; j != user.collateral.end(); ++j ) {
    double c = (double)itr->correlation_matrix.at(j->symbol)/corrPrecision;

    t_series statsj(name("datapreproc2"),name(issuerfeed[j->symbol]).value);
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
    
    t_series stats(name("datapreproc2"),name(issuerfeed[i->symbol]).value);
    auto itr = stats.find(1);
    double iVvol = (double)itr->vol/volPrecision;
    double iW = (double)itr->price[0] / pricePrecision;
    iW *= i->amount / std::pow(10.0, i->symbol.precision()); 
    iW /= user.l_valueofcol;

  for (auto j = i + 1; j != user.l_collateral.end(); ++j ) {
    double c = (double)itr->correlation_matrix.at(j->symbol)/corrPrecision;

    t_series statsj(name("datapreproc2"),name(issuerfeed[j->symbol]).value);
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

double vigor::portVarianceIns(name usern, double valueofins)
{
  const auto& user = _user.get( usern.value, "User not found19" );  
  
  double portVariance = 0.0;
  for ( auto i = user.insurance.begin(); i != user.insurance.end(); ++i ) {
    t_series stats(name("datapreproc2"),name(issuerfeed[i->symbol]).value);
    auto itr = stats.find(1);
    double iVvol = (double)itr->vol/volPrecision;
    double iW = (double)itr->price[0] / pricePrecision;
    iW *= i->amount / std::pow(10.0, i->symbol.precision()); 
    iW /= valueofins;

  for (auto j = i + 1; j != user.insurance.end(); ++j ) {
    double c = (double)itr->correlation_matrix.at(j->symbol)/corrPrecision;

    t_series statsj(name("datapreproc2"),name(issuerfeed[j->symbol]).value);
    auto itr = statsj.find(1);
    double jVvol = (double)itr->vol/volPrecision;
    double jW = (double)itr->price[0] / pricePrecision;
    jW *= j->amount / std::pow(10.0, j->symbol.precision());
    jW /= valueofins;

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

    t_series stats(name("datapreproc2"),name(issuerfeed[i->symbol]).value);
    auto itr = stats.find(1);
    double iVvol = (double)itr->vol/volPrecision;
    double iW = (double)itr->price[0] / pricePrecision;
    iW *= i->amount / std::pow(10.0, i->symbol.precision());
    iW /=  (gstats.valueofins);

    for (auto j = i + 1; j != gstats.insurance.end(); ++j ) {
      double c = (double)itr->correlation_matrix.at(j->symbol)/corrPrecision;

      t_series stats(name("datapreproc2"),name(issuerfeed[j->symbol]).value);
      auto itr = stats.find(1);
      double jVvol = (double)itr->vol/volPrecision;
      double jW = (double)itr->price[0] / pricePrecision;
      jW *= j->amount / std::pow(10.0, j->symbol.precision());
      jW /=  (gstats.valueofins); 

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

  double stressins = -1.0*(std::exp(-1.0*(((std::exp(-1.0*(std::pow(NormalCDFInverse(alphatest),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-alphatest)) * std::sqrt(portVariance)))-1.0);// model suggested percentage loss that the total insurance asset portfolio would experience in a stress event.
  gstats.stressins = stressins;
  gstats.svalueofins = (1.0 - stressins) * gstats.valueofins; // model suggested dollar value of the total insurance asset portfolio in a stress event.

  double stressinsavg = -1.0*(std::exp(-1.0*(((std::exp(-1.0*(std::pow(NormalCDFInverse(0.5),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-0.5)) * std::sqrt(portVariance)))-1.0); // model suggested percentage loss that the total insurance asset portfolio would experience in a stress event.
  gstats.svalueofinsavg = (1.0 - stressinsavg) * gstats.valueofins; // model suggested dollar value of the total insurance asset portfolio on average in stressed markets

  _globals.set(gstats, _self);
}

void vigor::l_stressins()
{
  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  double portVariance = portVarianceIns();

  double l_stressins = std::exp(((std::exp(-1.0*(std::pow(NormalCDFInverse(alphatest),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-alphatest)) * std::sqrt(portVariance))-1.0;// model suggested percentage gain that the total insurance asset portfolio would experience in an upside stress event.
  //gstats.l_stressins = l_stressins;
  gstats.l_svalueofins = (1.0 + l_stressins) * gstats.valueofins; // model suggested dollar value of the total insurance asset portfolio in a stress event.

  double l_stressinsavg = std::exp(((std::exp(-1.0*(std::pow(NormalCDFInverse(0.5),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-0.5)) * std::sqrt(portVariance))-1.0; // model suggested percentage gain that the total insurance asset portfolio would experience in an upside stress event.
  gstats.l_svalueofinsavg = (1.0 + l_stressinsavg) * gstats.valueofins; // model suggested dollar value of the total insurance asset portfolio on average in stressed markets

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
  ///eosio::print( "gstats.valueofins : ", gstats.valueofins, "\n");
  //stressed markets
  double mva_s = gstats.l_svalueofins;
  double mvl_s = gstats.l_svalueofcole;
  ///eosio::print( "gstats.l_svalueofins: ", gstats.l_svalueofins, "\n");
  ///eosio::print( "gstats.l_svalueofcole : ", gstats.l_svalueofcole, "\n");
  double own_n = mva_n - mvl_n; // own funds normal markets
  double own_s = mva_s - mvl_s; // own funds stressed markets
  
  double l_scr = std::max(own_n + own_s,0.0); // solvency capial requirement is the amount of insurance assets required to survive a stress event
  
  ///eosio::print( "l_scr: ",l_scr, "\n");
  double l_solvency = l_scr / own_n; // solvency, represents capital adequacy to back the stablecoin

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
  const auto useritr = _user.find( usern.value);  
  
  check(_globals.exists(), "globals not found");
  globalstats gstats = _globals.get();

  double ivol = user.volcol * gstats.scale; // market determined implied volaility
                      
  double istresscol = -1.0*(std::exp(-1.0*(-1.0*std::log(1.0+user.stresscol*-1.0) * gstats.scale))-1.0);

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

  _user.modify(useritr, _self, [&]( auto& modified_user) { 
    modified_user.tesprice = tesprice; // annualized rate borrowers pay in periodic premiums to insure their collateral
    modified_user.istresscol = istresscol; // market determined implied percentage loss that the user collateral portfolio would experience in a stress event.
    modified_user.premiums = premiums; // dollar amount of premiums borrowers would pay in one year to insure their collateral
  });
}

void vigor::l_pricing(name usern) {
/* premium payments in exchange for contingient payoff in 
 * the event that a price threshhold is breached
*/
  const auto& user = _user.get( usern.value, "User not found21" );  
  const auto useritr = _user.find( usern.value);  
  
  check(_globals.exists(), "globals not found");
  globalstats gstats = _globals.get();

  double l_ivol = user.l_volcol * gstats.l_scale; // market determined implied volaility
  double l_istresscol = std::exp(std::log(1.0+user.l_stresscol) * gstats.l_scale)-1.0;

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

  _user.modify(useritr, _self, [&]( auto& modified_user) { 
    modified_user.l_tesprice = l_tesprice; // annualized rate borrowers pay in periodic premiums to insure their collateral
    modified_user.l_istresscol = l_istresscol; // market determined implied percentage loss that the user collateral portfolio would experience in a stress event.
    modified_user.l_premiums = l_premiums; // dollar amount of premiums borrowers would pay in one year to insure their collateral
  });
}

double vigor::stressinsx(name usern) { // same as stressins, but remove the specified user

  const auto& user = _user.get( usern.value, "User not found21" );  

  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  double portVariancex = 0.0;

  for ( auto i = gstats.insurance.begin(); i != gstats.insurance.end(); ++i ) {

    t_series stats(name("datapreproc2"),name(issuerfeed[i->symbol]).value);
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

      t_series stats(name("datapreproc2"),name(issuerfeed[j->symbol]).value);
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

  double stressinsx = -1.0*(std::exp(-1.0*(((std::exp(-1.0*(std::pow(NormalCDFInverse(alphatest),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-alphatest)) * std::sqrt(portVariancex)))-1.0); // model suggested percentage loss that the total insurance asset portfolio (ex the specified user) would experience in a stress event.
  double svalueofinsx = (1.0 - stressinsx) * (gstats.valueofins  - user.valueofins); // model suggested dollar value of the total insurance asset portfolio (ex the specified user) in a stress event.
  
  return svalueofinsx;
}

double vigor::l_stressinsx(name usern) { // same as l_stressins, but remove the specified user

  const auto& user = _user.get( usern.value, "User not found21" );  

  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  double portVariancex = 0.0;

  for ( auto i = gstats.insurance.begin(); i != gstats.insurance.end(); ++i ) {

    t_series stats(name("datapreproc2"),name(issuerfeed[i->symbol]).value);
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

      t_series stats(name("datapreproc2"),name(issuerfeed[j->symbol]).value);
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

  double l_stressinsx = std::exp(((std::exp(-1.0*(std::pow(NormalCDFInverse(alphatest),2.0))/2.0)/(std::sqrt(2.0*M_PI)))/(1.0-alphatest)) * std::sqrt(portVariancex))-1.0; // model suggested percentage loss that the total insurance asset portfolio (ex the specified user) would experience in a stress event.
  double l_svalueofinsx = (1.0 + l_stressinsx) * (gstats.valueofins  - user.valueofins); // model suggested dollar value of the total insurance asset portfolio (ex the specified user) in a stress event.
  
  return l_svalueofinsx;
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
  double mva_n = gstats.valueofins; //market value of insurance assets in normal markets, includes the reserve which is implemented as an insurer, collateral is not an asset of the insurers
  double mvl_n = 0; // no upfront is paid for tes, and insurers can walk away at any time, debt is not a liability of the insurers

  //stressed markets
  double l_svalueofinsx = l_stressinsx(usern);
  double mva_s = l_svalueofinsx;
  double mvl_s = gstats.l_svalueofcole;
  ///eosio::print( "gstats.l_svalueofins: ", gstats.l_svalueofins, "\n");
  ///eosio::print( "gstats.l_svalueofcole : ", gstats.l_svalueofcole, "\n");
  double own_n = mva_n - mvl_n; // own funds normal markets
  double own_s = mva_s - mvl_s; // own funds stressed markets
  
  double l_scr = std::max(own_n + own_s,0.0); // solvency capial requirement is the amount of insurance assets required to survive a stress event
  
  ///eosio::print( "l_scr: ",l_scr, "\n");
  double l_solvencyx = l_scr / own_n; // solvency, represents capital adequacy to back the stablecoin

  return l_solvencyx; // solvency without the specified insurer
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
    double w =  it->valueofins / (gstats.valueofins + gstats.l_valueofcol);
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
    double w =  it->valueofins / (gstats.valueofins + gstats.l_valueofcol);
    smctr +=  w * dRMdw;
  }
  return smctr;
}

void vigor::l_pcts(name usern, double RM) { // percent contribution to solvency

  if (usern.value == name("finalreserve").value) // exclude the reserve because it only absorbs bailout after all insurers are wiped out, handled in reserve() method
    return;
  const auto& user = _user.get( usern.value, "User not foundl_22" );
  const auto useritr = _user.find( usern.value);  
  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  double solvencyx = l_riskx(usern);
  double w =  user.valueofins / (gstats.valueofins + gstats.l_valueofcol);
  double dRMdw =  (gstats.l_solvency - solvencyx);

  double pcts;
  if (RM==0.0)
    pcts =  0.0;
  else
    pcts =  w * dRMdw / RM;

  _user.modify(useritr, _self, [&]( auto& modified_user) { 
    modified_user.l_pcts = pcts; // percent contribution to solvency (weighted marginal contribution to risk (solvency) rescaled by sum of that
    });
    
}

void vigor::pcts(name usern, double RM) { // percent contribution to solvency

  if (usern.value == name("finalreserve").value) // exclude the reserve because it only absorbs bailout after all insurers are wiped out, handled in reserve() method
    return;
  const auto& user = _user.get( usern.value, "User not found22" );
  const auto useritr = _user.find( usern.value);  
  check( _globals.exists(), "no global table exists yet" );
  globalstats gstats = _globals.get();

  double solvencyx = riskx(usern);
  double w =  user.valueofins / (gstats.valueofins + gstats.l_valueofcol);
  double dRMdw =  (gstats.solvency - solvencyx);

  double pcts;
  if (RM==0.0)
    pcts =  0.0;
  else
    pcts =  w * dRMdw / RM;

  _user.modify(useritr, _self, [&]( auto& modified_user) { 
    modified_user.pcts = pcts; // percent contribution to solvency (weighted marginal contribution to risk (solvency) rescaled by sum of that
    });
    
}

// this method set the expiry date of the missed payments window
eosio::time_point_sec vigor::expirydate(eosio::time_point ctp){
    static const uint32_t now = ctp.sec_since_epoch();
    static const uint32_t r = now % hours(24).to_seconds();
    static const time_point_sec expiry_date = (time_point_sec)(now - r + (7 * hours(24).to_seconds()));
    return expiry_date;
  }


void vigor::payfee(name usern) {

  auto &user = _user.get( usern.value, "User not found23" );
  auto useritr = _user.find(usern.value);

  check(_globals.exists(), "no global table exists yet");

  globalstats gstats = _globals.get();
  
  bool updateglobal = true; // flag to upadte the global stats table
  bool late = true; 

  uint64_t amt = 0;
   
  symbol vig =symbol("VIG", 10);

  // the amount in vig that gets paid back
  asset amta = asset(amt, vig);
  asset l_amta = asset(amt, vig);

  time_point ctp = current_time_point();
  microseconds dmsec = ctp.time_since_epoch() - user.lastupdate.time_since_epoch(); 
  
  _user.modify(user, _self, [&]( auto& modified_user) { 
    modified_user.lastupdate = ctp;
  });

  // T is a converted time value
  double T = 360.0 * 24.0 * 60.0 * 60.0 * (1000000.0 / (double)dmsec.count());

  // calculating token swap pay
  double tespay = (user.debt.amount / std::pow(10.0, 4)) * (std::pow((1.0 + user.tesprice), (1.0 / T)) - 1);
  double l_tespay = user.l_valueofcol * (std::pow((1.0 + user.l_tesprice), (1.0 / T)) - 1);

  eosio::time_point_sec st;  // start time
  eosio::time_point_sec et;  // expiry time

  t_series stats(name("datapreproc2"),name(issuerfeed[vig]).value);

  auto itr = stats.find(1);

  // number of VIG user must pay over time T
  amta.amount = uint64_t(( tespay * std::pow(10.0, 10) ) / ((double)itr->price[0] / pricePrecision));
  l_amta.amount = uint64_t(( l_tespay * std::pow(10.0, 10) ) / ((double)itr->price[0] / pricePrecision));

  eosio::print( "usern : ", usern, " ,amta ", amta, "\n");
  eosio::print( "usern : ", usern, " ,l_amta ", l_amta, "\n");
  bool found = false;
  auto it = user.collateral.begin();
  while ( !found && it++ != user.collateral.end() ) 
      found = (it-1)->symbol == vig; //User collateral type found

    if(!found){
                if(user.starttime == st && user.expiry_time == st)
                {
                    //NO_VIG_AND_CLOCK_HAS_NOT_STARTED;    
                    //PAYMENTS;
                  st = (eosio::time_point_sec)(ctp.sec_since_epoch());
                  et = expirydate(ctp);
                  _user.modify(useritr, _self, [&]( auto& modified_user) {
                      modified_user.latepays += amta.amount + l_amta.amount;
                      modified_user.starttime = st;
                      modified_user.expiry_time = et;
                  });  
                }else{
                    
                    if(user.starttime < user.expiry_time )
                    {
                        //NO_VIG_AND_CLOCK_HAS_ALREADY_STARTED;
                        //MISSED_PAYMENTS;
                      _user.modify(useritr, _self, [&]( auto& modified_user) {
                            modified_user.latepays = amta.amount + l_amta.amount;
                      }); 
                     }
                     else if(user.starttime >= user.expiry_time)
                     {                  
                        //NO_VIG_AND_CLOCK_HAS_EXPIRED;
                        //MISSED_PAYMENTS;

                         // BAILOUT
                        // DELIQUENCY_FEE
                        // ia. the deliquency fee is being repaid this can happen over stages
                        // ib. the deliquency fee has been repaid fully
                        // iia. bailout is being repaid
                        // iib. bailout has been repaid in full
                        _user.modify(useritr, _self, [&]( auto& modified_user) {  
                                //  reset clock
                                modified_user.starttime = st;
                                modified_user.expiry_time = st; 
                                modified_user.latepays = 0;
                          });
                        
                    }
                }
    }
    else {
      
        if (user.latepays + amta.amount + l_amta.amount> (it-1)->amount)
        { 
                    if(user.starttime == st && user.expiry_time == st)

                    {
                        //NOT_ENOUGH_VIG_TO_MAKE_FULL_PAYMENT;
                        //PAYMENTS;
                        uint64_t diff = (amta.amount + l_amta.amount) - (it-1)->amount;
                        amta.amount = (it-1)->amount;
                        l_amta.amount = 0;
                        st = (eosio::time_point_sec)(ctp.sec_since_epoch());
                        et = expirydate(ctp);

                        _user.modify(useritr, _self, [&]( auto& modified_user) {
                              modified_user.feespaid.amount += amta.amount;
                              modified_user.latepays = diff; 
                              modified_user.collateral.erase(it-1);
                              modified_user.starttime = (eosio::time_point_sec)(ctp.sec_since_epoch());
                              modified_user.expiry_time = et;
                          }); 
                        updateglobal = false;

                    }else{
                        if(user.starttime < user.expiry_time )
                        {
                            //PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_ALREADY_STARTED;
                            //MISSED_PAYMENTS;
                            uint64_t diff = (user.latepays + (amta.amount + l_amta.amount)) - (it-1)->amount;
                            amta.amount = (it-1)->amount;
                            l_amta.amount = 0;

                            _user.modify(useritr, _self, [&]( auto& modified_user) {
                                  modified_user.feespaid.amount += amta.amount;
                                  modified_user.latepays = diff; 
                                  modified_user.collateral.erase(it-1);
                            });
                            updateglobal = false;
                        }
                        else if(user.starttime >= user.expiry_time)
                        {
                            //PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_HAS_EXPIRED;
                            //MISSED_PAYMENTS;
                             // BAILOUT
                            // DELIQUENCY_FEE
                            // ia. the deliquency fee is being repaid this can happen over stages
                            // ib. the deliquency fee has been repaid fully
                            // iia. bailout is being repaid
                            // iib. bailout has been repaid in full
                            _user.modify(useritr, _self, [&]( auto& modified_user) {  
                                    modified_user.starttime = st;
                                    modified_user.expiry_time = st; 
                                    modified_user.latepays = 0;
                                    modified_user.collateral.erase(it-1);
                              });
                        }
                    }
        }else if (user.latepays + amta.amount + l_amta.amount > 0){
                    if(user.starttime == st && user.expiry_time == st)
                    {
                       //NORMAL_PAYMENTS;
                       //PAYMENTS;
                        _user.modify(useritr, _self, [&]( auto& modified_user) {
                        modified_user.feespaid.amount += amta.amount + l_amta.amount;
                        if ((amta.amount + l_amta.amount) == (it-1)->amount)  
                          modified_user.collateral.erase(it-1); 
                        else 
                          modified_user.collateral[(it-1) - user.collateral.begin()] -= (amta + l_amta); 
                        });
                        updateglobal = false;  
                    }else if(user.starttime < user.expiry_time ){
                           //PAYMENT_OF_VIG_MADE_THAT_COVERS_MISSED_PAYMENTS_AND_CLOCK_HAS_ALREADY_STARTED;
                           //MISSED_PAYMENTS;                     

                          _user.modify(useritr, _self, [&]( auto& modified_user) {
                              modified_user.feespaid.amount += (it-1)->amount;
                              modified_user.starttime = st;
                              modified_user.expiry_time = st; 
                              modified_user.latepays = 0;
                              if (amta.amount == (it-1)->amount)  
                                modified_user.collateral.erase(it-1); 
                              else 
                                modified_user.collateral[(it-1) - user.collateral.begin()] -= (amta + l_amta);  
                              });
                          }
                            updateglobal = false;
        }
        }

          if(!updateglobal) {
              uint64_t n = 0; // count to identify last insurer, who will absorb dust
              uint64_t numinsurers = 0;
              asset viga;
              asset vigaremaining = asset(amta.amount + l_amta.amount, vig);
              for ( auto itr = _user.begin(); itr != _user.end(); ++itr )
                  if (itr->pcts > 0.0 || itr->l_pcts > 0.0 || itr->usern.value==name("finalreserve").value)
                    numinsurers += 1;
              for ( auto itr = _user.begin(); itr != _user.end(); ++itr ) {
                  if (itr->pcts > 0.0 || itr->l_pcts > 0.0 || itr->usern.value==name("finalreserve").value) {
                  eosio::print( "vigaremaining : ", vigaremaining, "\n");  
                  n += 1;
                  if (itr->usern.value==name("finalreserve").value)
                    viga = asset(amta.amount * reservecut, vig) + asset(l_amta.amount * reservecut, vig);
                  else
                    viga = asset(amta.amount * itr->pcts*(1.0-reservecut), vig) + asset(l_amta.amount * itr->l_pcts*(1.0-reservecut), vig);
                  if (n==numinsurers)
                    viga = vigaremaining; // adjustment for dust, so that the amount allocated to last insurer brings the vigaremaining to zero
                  vigaremaining -= viga;
                  found = false;
                  auto it = itr->insurance.begin();
                  while ( !found && it++ != itr->insurance.end() )
                    found = (it-1)->symbol == vig;
                  if (!found && viga.amount > 0)
                      _user.modify(itr, _self, [&]( auto& modified_user) {
                        modified_user.insurance.push_back(viga);
                        });
                  else if (viga.amount > 0) {
                        _user.modify( itr, _self, [&]( auto& modified_user ) {
                        modified_user.insurance[(it-1) - itr->insurance.begin()] += viga;
                        });
                      }
                }
              }
                for ( auto itr = gstats.collateral.begin(); itr != gstats.collateral.end(); ++itr )
                  if ( itr->symbol == vig ) {
                    if (gstats.collateral[itr - gstats.collateral.begin()].amount - (amta.amount + l_amta.amount)> 0)
                      gstats.collateral[itr - gstats.collateral.begin()].amount -= (amta.amount + l_amta.amount);
                    else 
                      gstats.collateral.erase(itr-1);
                    break;
                  }
                found = false;
                auto itg = gstats.insurance.begin();  
                while ( !found && itg++ != gstats.insurance.end() )
                  found = (itg-1)->symbol == vig;
                if ( !found )
                  gstats.insurance.push_back(asset(amta.amount + l_amta.amount, vig));
                else
                  gstats.insurance[(itg-1) - gstats.insurance.begin()] += asset(amta.amount + l_amta.amount, vig);
                _globals.set(gstats, _self);
            }
}

void vigor::update(name usern) 
{
  auto &user = _user.get(usern.value, "User not found1");
  auto useritr = _user.find(usern.value);

  double valueofins = 0.0;
  double valueofcol = 0.0;
  
  for ( auto it = user.insurance.begin(); it != user.insurance.end(); ++it ) {
    t_series stats(name("datapreproc2"),name(issuerfeed[it->symbol]).value);
    auto itr = stats.find(1);
    valueofins += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( (double)itr->price[0] / pricePrecision );
  }
  for ( auto it = user.collateral.begin(); it != user.collateral.end(); ++it ){
    t_series statsj(name("datapreproc2"),name(issuerfeed[it->symbol]).value);
    auto itr = statsj.find(1);
    valueofcol += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( (double)itr->price[0] / pricePrecision );
  }

  _user.modify( useritr, _self, [&]( auto& modified_user ) { // Update value of collateral
    modified_user.valueofins = valueofins;
    modified_user.valueofcol = valueofcol;

    double l_valueofcol = 0.0;
    
    for ( auto it = user.l_collateral.begin(); it != user.l_collateral.end(); ++it ){
      t_series statsj(name("datapreproc2"),name(issuerfeed[it->symbol]).value);
      auto itr = statsj.find(1);
      l_valueofcol += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                    ( (double)itr->price[0] / pricePrecision );
    }

    _user.modify( useritr, _self, [&]( auto& modified_user ) { // Update value of collateral
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
    t_series stats(name("datapreproc2"),name(issuerfeed[it->symbol]).value);
    auto itr = stats.find(1);
    valueofins += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( (double)itr->price[0] / pricePrecision );
  }
  for ( auto it = gstats.collateral.begin(); it != gstats.collateral.end(); ++it ){
    t_series statsj(name("datapreproc2"),name(issuerfeed[it->symbol]).value);
    auto itr = statsj.find(1);
    valueofcol += (it->amount) / std::pow(10.0, it->symbol.precision()) * 
                  ( (double)itr->price[0] / pricePrecision );
  }

  gstats.valueofins = valueofins;
  gstats.valueofcol = valueofcol;

  double l_valueofcol = 0.0;
  
  for ( auto it = gstats.l_collateral.begin(); it != gstats.l_collateral.end(); ++it ){
    t_series statsj(name("datapreproc2"),name(issuerfeed[it->symbol]).value);
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
  auto useritr = _user.find(usern.value);
  globalstats gstats = _globals.get();

  double cut = user.pcts*(1.0-reservecut);
  if (usern.value==name("finalreserve").value)
    cut = reservecut;

  double earnrate = 0.0;
  if (user.valueofins!=0.0)
    earnrate = (cut*gstats.premiums)/user.valueofins; // annualized rate of return on user portfolio of insurance crypto assets

  _user.modify( useritr, _self, [&]( auto& modified_user ) { // Update value of collateral
    modified_user.earnrate = earnrate;
  });
}


void vigor::l_performance(name usern) 
{
  auto &user = _user.get(usern.value, "User not found1");
  auto useritr = _user.find(usern.value);
  globalstats gstats = _globals.get();

  double cut = user.l_pcts*(1.0-reservecut);
  if (usern.value==name("finalreserve").value)
    cut = reservecut;

  double earnrate = 0.0;
  if (user.valueofins!=0.0)
    earnrate = (cut*gstats.l_premiums)/user.valueofins; // annualized rate of return on user portfolio of insurance crypto assets

  _user.modify( useritr, _self, [&]( auto& modified_user ) { // Update value of collateral
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
  auto useritr = _user.find(name("finalreserve").value);

  _user.modify(useritr, _self, [&]( auto& modified_user) {
      if (std::abs(gstats.valueofins + gstats.l_valueofcol - user.valueofins) < 0.0000000001)
        modified_user.pcts = 1.0; // trigger reserve to accept bailouts
      else
        modified_user.pcts = 0.0; // reserve does not participate in bailouts unless insurers are wiped out.
    });
}

void vigor::bailout(name usern)
{
  eosio::print( "usern : ", usern, "\n");
  auto &user = _user.get(usern.value, "User not found13");
  auto useritr = _user.find(usern.value);
   asset debt = user.debt;
  bool selfbailout = false;
  uint64_t n = 0; // count to identify last insurer, who will absorb dust
  uint64_t numinsurers = 0;
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
        int64_t debtshare = debt.amount * itr->pcts; // insurer share of failed loan debt, in stablecoin
        eosio::print( "user.debt : ", user.debt, "\n");
        eosio::print( "user.valueofcol : ", user.valueofcol, "\n");
        eosio::print( "itr->usern : ", itr->usern, "\n");
        eosio::print( "debtshare : ", debtshare, "\n");
        eosio::print( "itr->valueofins : ", itr->valueofins, "\n");
        double W1 = std::min(user.valueofcol*itr->pcts,debtshare/std::pow(10.0, 4)); // insurer share of impaired collateral, in dollars
        eosio::print( "itr->pcts : ", itr->pcts, "\n");
        double pcts = itr->pcts*(1.0/(1.0-sumpcts));
        sumpcts += itr->pcts;
        eosio::print( "sumpcts : ", sumpcts, "\n");
        eosio::print( "pcts : ", pcts, "\n");
        // assign ownership of the impaired collateral and debt to the insurers

        // subtract impaired collateral from user collateral
        for ( auto c = user.collateral.begin(); c != user.collateral.end(); ++c ) {
          asset amt = *c;
          amt.amount *= pcts;
          _user.modify(useritr, _self, [&]( auto& modified_user) {
                if (n==numinsurers) 
                  amt.amount += modified_user.collateral[c - user.collateral.begin()].amount - amt.amount; // adjustment for dust, so that the amount allocated to last insurer brings the collateral to zero
                modified_user.collateral[c - user.collateral.begin()] -= amt;
                eosio::print( "subtract impaired collateral from user collateral ", modified_user.collateral[c - user.collateral.begin()]," amt ", amt,"\n");
          });

          // subtract impaired collateral from global collateral
          bool found = false;
          auto itg = gstats.collateral.begin();   
          while ( !found && itg++ != gstats.collateral.end() )
            found = (itg-1)->symbol == amt.symbol;
          if (found)
            gstats.collateral[(itg-1) - gstats.collateral.begin()] -= amt;
          eosio::print( "subtract impaired collateral from global collateral ",gstats.collateral[(itg-1) - gstats.collateral.begin()],"\n");

          // add impaired collateral to insurers insurance
          eosio::print( "add impaired collateral to insurers insurance: ", amt,"\n");
          found = false;
          auto it = itr->insurance.begin();
          while ( !found && it++ != itr->insurance.end() )
            found = (it-1)->symbol == amt.symbol;
          _user.modify(itr, _self, [&]( auto& modified_user) {
            if (!found) {
              modified_user.insurance.push_back(amt);
              eosio::print( "push_back: ", amt,"\n");
            }
            else {
              modified_user.insurance[(it-1) - itr->insurance.begin()] += amt;
              eosio::print( " modified_user.insurance[(it-1) - itr->insurance.begin()]: ",  modified_user.insurance[(it-1) -itr->insurance.begin()],"\n");
            }
          });
          // add impaired collateral to global insurance
          found = false;
          itg = gstats.insurance.begin();  
          while ( !found && itg++ != gstats.insurance.end() )
            found = (itg-1)->symbol == amt.symbol;
          if ( !found )
            gstats.insurance.push_back(amt);
          else
            gstats.insurance[(itg-1) - gstats.insurance.begin()] += amt;
        }
        // subtract debt from users debt
          _user.modify(useritr, _self, [&]( auto& modified_user) {
                  if (n==numinsurers)
                    debtshare += modified_user.debt.amount - debtshare; // the amount allocated to last insurer should bring the debt to zero otherwise it is dust leftover
                  modified_user.debt.amount -= debtshare;
                  eosio::print( "modified_user.debt.amount ", modified_user.debt.amount, " debtshare", debtshare,"\n");
          });        
          // add debt to insurers debt
          _user.modify(itr, _self, [&]( auto& modified_user) {
                  modified_user.debt.amount += debtshare;
                  eosio::print( "modified_user.debt.amount ", modified_user.debt.amount, " debtshare", debtshare,"\n");
          });
        // cleanup empty vector elements
        _user.modify(useritr, _self, [&]( auto& modified_user) {
          modified_user.collateral.erase(
              std::remove_if(modified_user.collateral.begin(), modified_user.collateral.end(),
                    [](const asset & o) { return o.amount==0; }),
              modified_user.collateral.end());
        });
        gstats.collateral.erase(
            std::remove_if(gstats.collateral.begin(), gstats.collateral.end(),
            [](const asset & o) { return o.amount==0; }),
            gstats.collateral.end());

        //if insurer has VIGOR then use it to cancel inurers share of the bad debt
          double valueofins = itr->valueofins + W1; //adjusted value of insurance to include the newly acquired collateral
          for ( auto i = itr->insurance.begin(); i != itr->insurance.end(); ++i ) {
          if (i->symbol==symbol("VIGOR", 4)) {
          asset amt = *i;
          amt.amount = std::min(amt.amount,debtshare);
          valueofins -= (amt.amount / std::pow(10.0, 4)); //adjusted value of insurance to include cancelling some debt
          debtshare -= amt.amount; // remaining debtshare after cancelling some debt
          
          // subtract VIGOR from insurers insurance, and global insurance, and insurers debt
          _user.modify(itr, _self, [&]( auto& modified_user) {
                modified_user.insurance[i - itr->insurance.begin()] -= amt;
                modified_user.debt -= amt;
                eosio::print( "insurer insurance ", modified_user.insurance[i - itr->insurance.begin()]," amt ", amt,"\n");
          });
          bool found = false;
          auto itg = gstats.insurance.begin();   
          while ( !found && itg++ != gstats.insurance.end() )
            found = (itg-1)->symbol == i->symbol;
          if (found)
            gstats.insurance[(itg-1) - gstats.insurance.begin()] -= amt;

          // subtract VIGOR from global debt
          gstats.totaldebt -= amt;      

          //clear the debt from circulating supply
          action(permission_level{_self, name("active")}, _self, 
              name("retire"), std::make_tuple(amt,  std::string("Retire VIGOR insurance used to clear debt for user ") + itr->usern.to_string())
              ).send();
          }
        }

        // cleanup empty vector elements
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

        _globals.set(gstats, _self);

        if (debtshare==0 || valueofins==0.0) //no debt left to be recapped, or no insurance assets available to recap
          continue;

        // insurers auotmatically convert some of their insurance assets into collateral to recapitalize the bad debt
        // recapReq: required amount of insurance assets to be converted to collateral to recap the failed loan; overcollateralize to cover a 1 standard deviations monthly event
        //_user.flush();
        eosio::print( "valueofins : ", valueofins, "\n");
        eosio::print( "itr->usern : ", itr->usern, "\n");
        eosio::print( "itr->insurance.size() : ", itr->insurance.size(), "\n");
        double sp = std::sqrt(portVarianceIns(itr->usern,valueofins)/12.0); // volatility of the particular insurers insurance portfolio, monthly
        eosio::print( "sp : ", sp, "\n");
        double recapReq = std::min((((debtshare/std::pow(10.0, 4))*(1.0+1.0*sp)))/valueofins,1.0); // recapReq as a percentage of the insurers insurance assets
        eosio::print( "recapReq : ", recapReq, "\n");

        // subtract the recap amount from the insurers insurance, and global insurance
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

          // add the recap amount to the insurers collateral to recap the debt, and to the global collateral
          found = false;
          for ( auto it = itr->collateral.begin(); it != itr->collateral.end(); ++it ) {
            if ( it->symbol == amt.symbol ) {
              _user.modify(itr, _self, [&]( auto& modified_user) {
                modified_user.collateral[it - itr->collateral.begin()] += amt;
                eosio::print( "insurer collateral ", modified_user.collateral[it - itr->collateral.begin()]," amt ", amt,"\n");
              });
              found = true;
              break;
            }
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
      }
      _globals.set(gstats, _self);
    }
  }
}

void vigor::bailoutup(name usern)
{
  auto &user = _user.get(usern.value, "User not found13");
  auto useritr = _user.find(usern.value);
   asset l_debt = user.l_debt;
   asset paymentasset = asset( 0, symbol("VIGOR", 4) );
  bool selfbailout = false;
  uint64_t n = 0; // count to identify last insurer, who will absorb dust
  uint64_t numinsurers = 0;
  globalstats gstats;
  for ( auto itr = _user.begin(); itr != _user.end(); ++itr )
      if (itr->l_pcts > 0.0)
      numinsurers += 1;
  for(int m=1; m<=2; m++){
    if (m==2)
      selfbailout = true;
    double sumpcts = 0.0;
    for ( auto itr = _user.begin(); itr != _user.end(); ++itr ) {
      if (selfbailout && itr->usern.value != usern.value) // bailout self last, to make the math easier using percentages
          continue;
      if (!selfbailout && itr->usern.value == usern.value)
          continue;
      if (itr->l_pcts > 0.0) {
        n += 1;
        // all insurers participate to recap borrowed l_collateral, each insurer taking ownership of a fraction of the l_collateral and l_debt; insurer participation is based on their l_pcts percent contribution to solvency
        int64_t l_debtshare = std::pow(10.0, 4)*std::min(user.l_valueofcol*itr->l_pcts,(l_debt.amount*itr->l_pcts)/std::pow(10.0, 4)); // insurer share of the the l_debt
        double W1 = user.l_valueofcol*itr->l_pcts; // insurer share of the l_collateral
        double valueofins = itr->valueofins;
        double l_pcts = itr->l_pcts*(1.0/(1.0-sumpcts));
        sumpcts += itr->l_pcts;
        eosio::print( "usern : ", usern, ", user.l_debt : ", user.l_debt, ", user.l_valueofcol : ", user.l_valueofcol, ", itr->usern : ", itr->usern, ", l_debtshare : ", l_debtshare, ", itr->valueofins : ", itr->valueofins, ", itr->l_pcts : ", itr->l_pcts, ", sumpcts : ", sumpcts, ", W1", W1, "\n");
        auto &reinvestment = _user.get(name("reinvestment").value, "reinvestment not found");
        auto reinvestmentitr = _user.find(name("reinvestment").value);
        // assign ownership of the l_collateral and l_debt to the insurers

        // subtract borrows from user l_collateral
        for ( auto c = user.l_collateral.begin(); c != user.l_collateral.end(); ++c ) {
          asset amt = *c;
          amt.amount *= l_pcts;
          _user.modify(useritr, _self, [&]( auto& modified_user) {
                if (n==numinsurers)
                  amt.amount += modified_user.l_collateral[c - user.l_collateral.begin()].amount - amt.amount; // adjustment for dust, so that the amount allocated to last insurer brings the l_collateral to zero
                eosio::print( "amt ", amt, "\n");
                modified_user.l_collateral[c - user.l_collateral.begin()] -= amt;
                eosio::print( "test","\n");
                eosio::print( "subtract borrows from user l_collateral ", modified_user.l_collateral[c - user.l_collateral.begin()],", amt ", amt,"\n");
          });

          // add borrows to insurers l_collateral
          bool found = false;
          auto it = itr->l_collateral.begin();
          while ( !found && it++ != itr->l_collateral.end() )
            found = (it-1)->symbol == amt.symbol;
          _user.modify(itr, _self, [&]( auto& modified_user) {
            if (!found) {
              modified_user.l_collateral.push_back(amt);
              eosio::print( "add borrows to insurers l_collateral push back, amt ", amt,"\n");
            }
            else {
              modified_user.l_collateral[(it-1) - itr->l_collateral.begin()] += amt;
              eosio::print( "add borrows to insurers l_collateral: ",  modified_user.l_collateral[(it-1) -itr->l_collateral.begin()],", amt ", amt,"\n");
            }
          });

          // subtract borrow from user lending receipt
          found = false;          
          auto itre = reinvestment.l_lrname.begin();
          while ( !found && itre++ != reinvestment.l_lrname.end() )
            found = ((itre-1)->value == user.usern.value && reinvestment.l_lrtoken[(itre-1) - reinvestment.l_lrname.begin()].symbol == amt.symbol);
          _user.modify(reinvestmentitr, _self, [&]( auto& modified_user) {
            if (!found){
              check( false, user.usern.to_string() + amt.to_string() + std::string(" ") + std::string(" Error, lending receipt not found for bailout. user has token in l_collateral but no lending receipt"));
            }
            else {
              paymentasset = modified_user.l_lrpayment[(itre-1) - reinvestment.l_lrname.begin()];
              if (n==numinsurers){
                modified_user.l_lrtoken[(itre-1) - reinvestment.l_lrname.begin()].amount -= amt.amount;
                check(modified_user.l_lrtoken[(itre-1) - reinvestment.l_lrname.begin()].amount==0,std::string("lending receipt is expected to have a zero l_lrtoken amount, but has: ") + modified_user.l_lrtoken[(itre-1) - reinvestment.l_lrname.begin()].to_string());
                modified_user.l_lrpayment[(itre-1) - reinvestment.l_lrname.begin()].amount -= paymentasset.amount;
                modified_user.l_lrname[(itre-1) - reinvestment.l_lrname.begin()] = name("delete");
              } else {
                paymentasset.amount *= (double)amt.amount/(double)modified_user.l_lrtoken[(itre-1) - reinvestment.l_lrname.begin()].amount;
                modified_user.l_lrtoken[(itre-1) - reinvestment.l_lrname.begin()].amount -= amt.amount;
                modified_user.l_lrpayment[(itre-1) - reinvestment.l_lrname.begin()].amount -= paymentasset.amount;
              }
              eosio::print( "subtract borrow from user lending receipt: ", user.usern, " ", amt, " ", paymentasset, ", l_lrpayment ", modified_user.l_lrpayment[(itre-1) - reinvestment.l_lrname.begin()], ", l_lrtoken ", modified_user.l_lrtoken[(itre-1) - reinvestment.l_lrname.begin()], "\n");
            }
              });; found = false;

          // add borrow to insurer lending receipt
          itre = reinvestment.l_lrname.begin();
          while ( !found && itre++ != reinvestment.l_lrname.end() )
            found = ((itre-1)->value == itr->usern.value && reinvestment.l_lrtoken[(itre-1) - reinvestment.l_lrname.begin()].symbol == amt.symbol);
          _user.modify(reinvestmentitr, _self, [&]( auto& modified_user) {
            if (!found){
              modified_user.l_lrtoken.push_back(amt);
              modified_user.l_lrpayment.push_back(paymentasset);
              modified_user.l_lrname.push_back(itr->usern);
               eosio::print( "add borrow to insurer lending receipt, create: ", itr->usern, " ", amt, " ", paymentasset, " push back", "\n");
            }
            else {
              modified_user.l_lrtoken[(itre-1) - reinvestment.l_lrname.begin()].amount += amt.amount;
              modified_user.l_lrpayment[(itre-1) - reinvestment.l_lrname.begin()].amount += paymentasset.amount;
              eosio::print( "add borrow to insurer lending receipt, modify: ", itr->usern, " ", amt, " ", paymentasset, " l_lrpayment ", modified_user.l_lrpayment[(itre-1) - reinvestment.l_lrname.begin()].amount, " l_lrtoken ", modified_user.l_lrtoken[(itre-1) - reinvestment.l_lrname.begin()], "\n");
              }
          }); 

          // recap method 1: if the insurer has the borrowed token type in their insurance then use it to payback_borrowed_token
          eosio::print( "recap method 1: if the insurer has the borrowed token type in their insurance then use it to payback_borrowed_token", "\n");
          // move insurance into l_collateral
          for ( auto i = itr->insurance.begin(); i != itr->insurance.end(); ++i ) {
            if (i->symbol==amt.symbol) {
              asset amti = *i;
              amti.amount = std::min(amti.amount,amt.amount);

              // subtract borrow from insurers insurance and global insurance
              eosio::print( "subtract borrow from insurers insurance and global insurance. amti ", amti,"\n");
              _user.modify(itr, _self, [&]( auto& modified_user) {
              if (modified_user.insurance[i - itr->insurance.begin()].amount - amti.amount == 0)
                modified_user.insurance.erase(i);
              else
                modified_user.insurance[i - itr->insurance.begin()] -= amti;
              });

              bool found = false;
              gstats = _globals.get();
              auto itg = gstats.insurance.begin();   
              while ( !found && itg++ != gstats.insurance.end() )
                found = (itg-1)->symbol == i->symbol;
              if (found && amti.amount > 0) {
                if (gstats.insurance[(itg-1) - gstats.insurance.begin()].amount - amti.amount == 0)
                  gstats.insurance.erase(itg-1);
                else 
                  gstats.insurance[(itg-1) - gstats.insurance.begin()] -= amti;
              }
              _globals.set(gstats, _self);

              t_series stats(name("datapreproc2"),name(issuerfeed[i->symbol]).value);
              auto itp = stats.find(1);
              valueofins -= (amti.amount) / std::pow(10.0, i->symbol.precision()) * ( (double)itp->price[0] / pricePrecision );//adjusted value of insurance to include cancelling some of the borrow
              W1 -= (amti.amount) / std::pow(10.0, i->symbol.precision()) * ( (double)itp->price[0] / pricePrecision );//adjusted value of the share of l_collateral to include cancelling some of the borrow
              eosio::print( "payback_borrowed_token ", itr->usern, " ", amti,"\n");
              payback_borrowed_token(itr->usern, amti);
            }
          }
        }

          _user.modify(itr, _self, [&]( auto& modified_user) {
            modified_user.l_collateral.erase(
                std::remove_if(modified_user.l_collateral.begin(), modified_user.l_collateral.end(),
                      [](const asset & o) { return o.amount==0; }),
                modified_user.l_collateral.end());
          });
          
          // subtract l_debt from users l_debt
          _user.modify(useritr, _self, [&]( auto& modified_user) {
                  if (n==numinsurers)
                    l_debtshare += modified_user.l_debt.amount - l_debtshare; // the amount allocated to last insurer should bring the l_debt to zero otherwise it is dust leftover
                  modified_user.l_debt.amount -= l_debtshare;
                  eosio::print( "subtract l_debt from users l_debt ", modified_user.l_debt.amount, " l_debtshare", l_debtshare,"\n");
          });        
          // add l_debt to insurers l_debt
          _user.modify(itr, _self, [&]( auto& modified_user) {
                  modified_user.l_debt.amount += l_debtshare;
                  eosio::print( "add l_debt to insurers l_debt ", modified_user.l_debt.amount, " l_debtshare", l_debtshare,"\n");
          });

        _user.modify(useritr, _self, [&]( auto& modified_user) {
          modified_user.l_collateral.erase(
              std::remove_if(modified_user.l_collateral.begin(), modified_user.l_collateral.end(),
                    [](const asset & o) { return o.amount==0; }),
              modified_user.l_collateral.end());
        });

        _user.modify(reinvestmentitr, _self, [&]( auto& modified_user) { //removed vector elements with zero amount
        modified_user.l_lrtoken.erase(
            std::remove_if(modified_user.l_lrtoken.begin(), modified_user.l_lrtoken.end(),
                  [](const asset & o) { return o.amount==0; }),
            modified_user.l_lrtoken.end());
        });
        _user.modify(reinvestmentitr, _self, [&]( auto& modified_user) { //removed vector elements with zero amount
        modified_user.l_lrpayment.erase(
            std::remove_if(modified_user.l_lrpayment.begin(), modified_user.l_lrpayment.end(),
                  [](const asset & o) { return o.amount==0; }),
            modified_user.l_lrpayment.end());
        });
        _user.modify(reinvestmentitr, _self, [&]( auto& modified_user) { //removed vector elements with zero amount
        modified_user.l_lrname.erase(
            std::remove_if(modified_user.l_lrname.begin(), modified_user.l_lrname.end(),
                  [](const name & o) { return o.value==name("delete").value; }),
            modified_user.l_lrname.end());
        });

        // recap method 2: If insurer has VIGOR in insurance then use it as l_debt to collateralize insurers share of adjusted l_collateral W1
        eosio::print( "recap method 2: If insurer has VIGOR in insurance then use it as l_debt to collateralize insurers share of adjusted l_collateral W1", "\n");
        // move VIGOR from insurance into l_debt
        double sp = user.l_volcol/std::sqrt(12.0); // volatility of the insurers share of the users l_collateral portfolio, monthly
        double vigorneeded = std::max((W1*(1.0+sp)-(l_debtshare/std::pow(10.0, 4))),0.0); // need this many VIGOR to recap the borrowed risky tokens
        asset vigorneededa = asset( (uint64_t)(vigorneeded*std::pow(10.0, 4)), symbol("VIGOR", 4) );

        for ( auto i = itr->insurance.begin(); i != itr->insurance.end(); ++i ) {
        if (i->symbol==symbol("VIGOR", 4)) {
        asset amt = *i;
        amt.amount = std::min(amt.amount,vigorneededa.amount);
        valueofins -= (amt.amount / std::pow(10.0, 4)); //adjusted value of insurance to include moving some VIGOR into l_debt
        vigorneededa -= amt; // adjusted value, need this many VIGOR to recap the borrowed risky tokens
        
        // subtract VIGOR from insurers insurance and add to insurers l_debt
        eosio::print( "subtract VIGOR from insurers insurance and add to insurers l_debt","\n");
        _user.modify(itr, _self, [&]( auto& modified_user) {
            if (modified_user.insurance[i - itr->insurance.begin()].amount - amt.amount == 0)
                modified_user.insurance.erase(i);
            else
                modified_user.insurance[i - itr->insurance.begin()] -= amt;
            modified_user.l_debt += amt;
            });
        // subtract VIGOR from global insurance
        bool found = false;
        gstats = _globals.get();
        auto itg = gstats.insurance.begin();   
        while ( !found && itg++ != gstats.insurance.end() )
          found = (itg-1)->symbol == i->symbol;
        if (found && amt.amount > 0) {
          if (gstats.insurance[(itg-1) - gstats.insurance.begin()].amount - amt.amount == 0)
            gstats.insurance.erase(itg-1);
          else 
            gstats.insurance[(itg-1) - gstats.insurance.begin()] -= amt;
        }
        // add VIGOR to global l_debt
        gstats.l_totaldebt += amt;
        _globals.set(gstats, _self);
        break;
        }
        }

        // recap method 3: borrow VIGOR against insurance to recap the borrowed tokens
        eosio::print( "recap method 3: borrow VIGOR against insurance to recap the borrowed tokens", "\n");
        if (vigorneededa.amount==0 || W1==0 || valueofins==0.0)  // check if recap is still needed
          continue;
        // vigorneededa.amount==0.0 l_debtshare is sufficient to collateralize insurers share of adjusted l_collateral W1
        // W1==0 no borrows remain to be recapped
        // valueofins==0.0 no insurance assets available to recap the borrowed tokens
        double spi = std::sqrt(portVarianceIns(itr->usern,valueofins)/12.0); // volatility of the insurer insurance portfolio, monthly
        double recapReq =  std::min(((1.0+spi)*(vigorneededa.amount/std::pow(10.0, 4)))/valueofins,1.0); // amount of collateral needed, against which the insurer can borrow VIGOR to recap the borrowed risky tokens, as a pct of insurers insurance
        eosio::print( "recapReq ", recapReq, ", sp ", sp, ", spi ", spi, "\n");
        
        // subtract the recap amount from the insurers insurance, and global insurance
        for ( auto i = itr->insurance.begin(); i != itr->insurance.end(); ++i ) {
          asset amt = *i;
          amt.amount *= recapReq;
          
          _user.modify(itr, _self, [&]( auto& modified_user) {
                modified_user.insurance[i - itr->insurance.begin()] -= amt;
                eosio::print( "subtract the recap amount from the insurers insurance, and global insurance ", modified_user.insurance[i - itr->insurance.begin()]," amt ", amt,"\n");
          });
          bool found = false;
          auto itg = gstats.insurance.begin();   
          while ( !found && itg++ != gstats.insurance.end() )
            found = (itg-1)->symbol == i->symbol;
          if (found && amt.amount > 0)
            gstats.insurance[(itg-1) - gstats.insurance.begin()] -= amt;

          // add the recap amount to the insurers collateral, and to the global collateral
          found = false;
          for ( auto it = itr->collateral.begin(); it != itr->collateral.end(); ++it ) {
            if ( it->symbol == amt.symbol ) {
              _user.modify(itr, _self, [&]( auto& modified_user) {
                modified_user.collateral[it - itr->collateral.begin()] += amt;
                eosio::print( "add the recap amount to the insurers collateral, and to the global collateral ", modified_user.collateral[it - itr->collateral.begin()]," amt ", amt,"\n");
              });
              found = true;
              break;
            }
          }
          if (!found) 
            _user.modify(itr, _self, [&]( auto& modified_user) {
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

        //borrow VIGOR debt against the recapReq in the insurers collateral and move it to l_debt to recap the insurers l_collateral
        _user.modify(itr, _self, [&]( auto& modified_user) {
          modified_user.debt += vigorneededa;
          modified_user.l_debt += vigorneededa;
        });
        gstats.totaldebt += vigorneededa;
        gstats.l_totaldebt += vigorneededa;

        action( permission_level{_self, name("active")},
        _self, name("issue"), std::make_tuple(
          _self, vigorneededa, std::string("VIGOR issued to ") + _self.to_string()
        )).send();

        _globals.set(gstats, _self);
      }
    }
  }
}

double vigor::RationalApproximation(double t)
{
    // Abramowitz and Stegun formula 26.2.23.
    // The absolute value of the error should be less than 4.5 e-4.
    double c[] = {2.515517, 0.802853, 0.010328};
    double d[] = {1.432788, 0.189269, 0.001308};
    return t - ((c[2]*t + c[1])*t + c[0]) / 
               (((d[2]*t + d[1])*t + d[0])*t + 1.0);
}

double vigor::NormalCDFInverse(double p)
{
    if (p <= 0.0 || p >= 1.0)
    {
      eosio::print( "Invalid input argument in NormalCDFInverse, must be larger than 0 but less than 1.", "\n");
    }

    // See article above for explanation of this section.
    if (p < 0.5)
    {
        // F^-1(p) = - G^-1(p)
        return -RationalApproximation( sqrt(-2.0*log(p)) );
    }
    else
    {
        // F^-1(p) = G^-1(1-p)
        return RationalApproximation( sqrt(-2.0*log(1-p)) );
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
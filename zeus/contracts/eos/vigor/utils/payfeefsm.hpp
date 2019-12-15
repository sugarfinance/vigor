
/*
*   IK Nwoke 
*   creating a finite state machine to deal with the different state, events and transitions that occur within payfee()
*/

#pragma once

#include <utility>    // using std::pair
#include "Array2d.hpp"

namespace fsmpayfee {

    // enums
    enum aistate
    {
        PAYMENTS,
        MISSED_PAYMENTS,
        END_OF_GRACE_PERIOD
    } aistate_;

    enum aievents
    {
        NORMAL_PAYMENTS,
        NOT_ENOUGH_VIG_TO_MAKE_FULL_PAYMENT,
        NO_VIG_AND_CLOCK_HAS_NOT_STARTED,
        NO_VIG_AND_CLOCK_HAS_ALREADY_STARTED,
        NO_VIG_AND_CLOCK_HAS_EXPIRED,
        PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_ALREADY_STARTED,
        PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_HAS_EXPIRED,
        PAYMENT_OF_VIG_MADE_THAT_COVERS_MISSED_PAYMENTS_AND_CLOCK_HAS_ALREADY_STARTED,
        DELIQUENCY_FEE_PAYMENTS,
        DELIQUENCY_FEE_PAYMENTS_SETTLED
    } aievents_;

    //std::pair<int, int> machinevalues;  // this pair will contain values for state and events

    void InitFSM(Array2d<aistate>& obj, enum aistate, enum aievents)
    {
        int state;
        int event;


        // fill the machine with values first
        for( state = 0; state < 3; state++ )
        {
            for( event = 0; event < 10; event++ )
            {
                obj.GetX(state, event) = (aistate)state;
            }
        }

        // now fill the machine in with the real values
        obj.GetX(PAYMENTS, NORMAL_PAYMENTS) = PAYMENTS;
        obj.GetX(PAYMENTS, NOT_ENOUGH_VIG_TO_MAKE_FULL_PAYMENT) = MISSED_PAYMENTS;
        obj.GetX(PAYMENTS, NO_VIG_AND_CLOCK_HAS_NOT_STARTED) =  MISSED_PAYMENTS;
        obj.GetX(MISSED_PAYMENTS, NO_VIG_AND_CLOCK_HAS_ALREADY_STARTED) = MISSED_PAYMENTS;
        obj.GetX(MISSED_PAYMENTS, NO_VIG_AND_CLOCK_HAS_EXPIRED) = END_OF_GRACE_PERIOD;
        obj.GetX(MISSED_PAYMENTS, PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_ALREADY_STARTED) = MISSED_PAYMENTS;
        obj.GetX(MISSED_PAYMENTS, PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_HAS_EXPIRED) = END_OF_GRACE_PERIOD;
        obj.GetX(MISSED_PAYMENTS,  PAYMENT_OF_VIG_MADE_THAT_COVERS_MISSED_PAYMENTS_AND_CLOCK_HAS_ALREADY_STARTED) = PAYMENTS;
        obj.GetX(END_OF_GRACE_PERIOD, DELIQUENCY_FEE_PAYMENTS) = END_OF_GRACE_PERIOD;
        obj.GetX(END_OF_GRACE_PERIOD, DELIQUENCY_FEE_PAYMENTS_SETTLED) = PAYMENTS;
    } 
 


    // transition functions
    // normal payments
    std::pair<int, int> np(Array2d<aistate>& _obj_, int currentstate)
    {
        // fsm_machine.GetX(PAYMENTS, NORMAL_PAYMENTS) = PAYMENTS;
    
        int localstate =_obj_.GetX(currentstate, NORMAL_PAYMENTS);
        int localevent = _obj_.GetY(NORMAL_PAYMENTS); //hardcoded -change later!
        return std::make_pair(localstate, localevent);
    }
      
    // NOT_ENOUGH_VIG_TO_MAKE_FULL_PAYMENT,
    std::pair<int, int> nevtomfp(Array2d<aistate>& _obj_, int currentstate)
    {
        // fsm_machine.GetX(PAYMENTS, NOT_ENOUGH_VIG_TO_MAKE_FULL_PAYMENT) = MISSED_PAYMENTS;

        int localstate =_obj_.GetX(currentstate, NOT_ENOUGH_VIG_TO_MAKE_FULL_PAYMENT);
        int localevent = _obj_.GetY(NOT_ENOUGH_VIG_TO_MAKE_FULL_PAYMENT); //hardcoded -change later!
        return std::make_pair(localstate, localevent);
    }

    // NO_VIG_AND_CLOCK_HAS_NOT_STARTED
    std::pair<int, int> nvachns(Array2d<aistate>& _obj_, int currentstate)
    {
        //fsm_machine.GetX(PAYMENTS, NO_VIG_AND_CLOCK_HAS_NOT_STARTED) =  MISSED_PAYMENTS;

        int localstate =_obj_.GetX(currentstate, NO_VIG_AND_CLOCK_HAS_NOT_STARTED);
        int localevent = _obj_.GetY(NO_VIG_AND_CLOCK_HAS_NOT_STARTED); //hardcoded -change later!
        return std::make_pair(localstate, localevent);
    }


    // NO_VIG_AND_CLOCK_HAS_ALREADY_STARTED
    std::pair<int, int> nvachas(Array2d<aistate>& _obj_, int currentstate)
    {
        // fsm_machine.GetX(MISSED_PAYMENTS, NO_VIG_AND_CLOCK_HAS_ALREADY_STARTED) = MISSED_PAYMENTS;

        int localstate =_obj_.GetX(currentstate, NO_VIG_AND_CLOCK_HAS_ALREADY_STARTED);
        int localevent = _obj_.GetY(NO_VIG_AND_CLOCK_HAS_ALREADY_STARTED); //hardcoded -change later!
        return std::make_pair(localstate, localevent);
    }
 
    //  NO_VIG_AND_CLOCK_HAS_EXPIRED
    std::pair<int, int> nvache(Array2d<aistate>& _obj_, int currentstate)
    {
        // fsm_machine.GetX(MISSED_PAYMENTS, NO_VIG_AND_CLOCK_HAS_EXPIRED) = END_OF_GRACE_PERIOD;

        int localstate =_obj_.GetX(currentstate, NO_VIG_AND_CLOCK_HAS_EXPIRED);
        int localevent = _obj_.GetY(NO_VIG_AND_CLOCK_HAS_EXPIRED); //hardcoded -change later!
        return std::make_pair(localstate, localevent);
    }

    // PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_ALREADY_STARTED
    std::pair<int, int>povmbnetmafracas(Array2d<aistate>& _obj_, int currentstate)
    {
        // fsm_machine.GetX(MISSED_PAYMENTS, PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_ALREADY_STARTED) = MISSED_PAYMENTS;

        int localstate =_obj_.GetX(currentstate, PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_ALREADY_STARTED);
        int localevent = _obj_.GetY(PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_ALREADY_STARTED); //hardcoded -change later!
        return std::make_pair(localstate, localevent);
    }

    // PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_HAS_EXPIRED
    std::pair<int, int> povmbnetmafrache(Array2d<aistate>& _obj_, int currentstate)
    {
        // fsm_machine.GetX(MISSED_PAYMENTS, PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_HAS_EXPIRED) = END_OF_GRACE_PERIOD;

        int localstate =_obj_.GetX(currentstate, PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_HAS_EXPIRED);
        int localevent = _obj_.GetY(PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_HAS_EXPIRED); //hardcoded -change later!
        return std::make_pair(localstate, localevent);
    }

    // PAYMENT_OF_VIG_MADE_THAT_COVERS_MISSED_PAYMENTS_AND_CLOCK_HAS_ALREADY_STARTED
    std::pair<int, int> povmtcmpachas(Array2d<aistate>& _obj_, int currentstate)
    {
        // fsm_machine.GetX(MISSED_PAYMENTS,  PAYMENT_OF_VIG_MADE_THAT_COVERS_MISSED_PAYMENTS_AND_CLOCK_HAS_ALREADY_STARTED) = PAYMENTS;

        int localstate =_obj_.GetX(currentstate,  PAYMENT_OF_VIG_MADE_THAT_COVERS_MISSED_PAYMENTS_AND_CLOCK_HAS_ALREADY_STARTED);
        int localevent = _obj_.GetY( PAYMENT_OF_VIG_MADE_THAT_COVERS_MISSED_PAYMENTS_AND_CLOCK_HAS_ALREADY_STARTED); //hardcoded -change later!
        return std::make_pair(localstate, localevent);
    }

    // DELIQUENCY_FEE_PAYMENTS
    std::pair<int, int> dfp(Array2d<aistate>& _obj_, int currentstate)
    {
        //  fsm_machine.GetX(END_OF_GRACE_PERIOD, DELIQUENCY_FEE_PAYMENTS) = END_OF_GRACE_PERIOD;
   
        int localstate =_obj_.GetX(currentstate,  DELIQUENCY_FEE_PAYMENTS);
        int localevent = _obj_.GetY(DELIQUENCY_FEE_PAYMENTS); //hardcoded -change later!
        return std::make_pair(localstate, localevent);
    }

    //  DELIQUENCY_FEE_PAYMENTS_SETTLED
std::pair<int, int> dfps(Array2d<aistate>& _obj_, int currentstate)
    {
        // fsm_machine.GetX(END_OF_GRACE_PERIOD, DELIQUENCY_FEE_PAYMENTS_SETTLED) = PAYMENTS;

        int localstate =_obj_.GetX(currentstate, DELIQUENCY_FEE_PAYMENTS_SETTLED);
        int localevent = _obj_.GetY(DELIQUENCY_FEE_PAYMENTS_SETTLED); //hardcoded -change later!
        return std::make_pair(localstate, localevent);
    }


};
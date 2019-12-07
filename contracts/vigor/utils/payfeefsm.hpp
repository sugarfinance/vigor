// creating a finite state machine to deal with the different state, events and transitions that occur within payfee()

#pragma once

#include "Array2d.hpp"

namespace fsm_payfee {

    // enums
    enum aistate
    {
        PAYMENTS,
        MISSED_PAYMENTS,
        END_OF_GRACE_PERIOD
    };

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
    };

    // create the Array2d object
    Array2d<aistate> fsm_machine(3, 10);

    // a variable that selects the current state
    int g_currentstate = PAYMENTS;
    int g_currentevents = NORMAL_PAYMENTS;

    // algorithm
    void InitFSM()
    {
        int state;
        int event;

        // fill the machine with values first
        for( state = 0; state < 3; state++ )
        {
            for( event = 0; event < 7; event++ )
            {
                fsm_machine.GetX(state, event) = (aistate)state;
            }
        }

        // now fill the machine in with the real values
        fsm_machine.GetX(PAYMENTS, NORMAL_PAYMENTS) = PAYMENTS;
        fsm_machine.GetX(PAYMENTS, NOT_ENOUGH_VIG_TO_MAKE_FULL_PAYMENT) = MISSED_PAYMENTS;
        fsm_machine.GetX(PAYMENTS, NO_VIG_AND_CLOCK_HAS_NOT_STARTED) =  MISSED_PAYMENTS;
        fsm_machine.GetX(MISSED_PAYMENTS, NO_VIG_AND_CLOCK_HAS_ALREADY_STARTED) = MISSED_PAYMENTS;
        fsm_machine.GetX(MISSED_PAYMENTS, NO_VIG_AND_CLOCK_HAS_EXPIRED) = END_OF_GRACE_PERIOD;
        fsm_machine.GetX(MISSED_PAYMENTS, PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_ALREADY_STARTED) = MISSED_PAYMENTS;
        fsm_machine.GetX(MISSED_PAYMENTS, PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_HAS_EXPIRED) = END_OF_GRACE_PERIOD;
        fsm_machine.GetX(MISSED_PAYMENTS,  PAYMENT_OF_VIG_MADE_THAT_COVERS_MISSED_PAYMENTS_AND_CLOCK_HAS_ALREADY_STARTED) = PAYMENTS;
        fsm_machine.GetX(END_OF_GRACE_PERIOD, DELIQUENCY_FEE_PAYMENTS) = END_OF_GRACE_PERIOD;
        fsm_machine.GetX(END_OF_GRACE_PERIOD, DELIQUENCY_FEE_PAYMENTS_SETTLED) = PAYMENTS;

    }

    // transition phase
    // normal payments
    int np()
    {
        // fsm_machine.GetX(PAYMENTS, NORMAL_PAYMENTS) = PAYMENTS;
        g_currentstate = fsm_machine.GetX(g_currentstate, NORMAL_PAYMENTS);
        return g_currentstate;
    }

    // NOT_ENOUGH_VIG_TO_MAKE_FULL_PAYMENT,
    int nevtomfp()
    {
        // fsm_machine.GetX(PAYMENTS, NOT_ENOUGH_VIG_TO_MAKE_FULL_PAYMENT) = MISSED_PAYMENTS;
        g_currentstate = fsm_machine.GetX(g_currentstate,  NO_VIG_AND_CLOCK_HAS_NOT_STARTED);
        return g_currentstate;
    } 

    // NO_VIG_AND_CLOCK_HAS_NOT_STARTED
    int nvachns()
    {
        //fsm_machine.GetX(PAYMENTS, NO_VIG_AND_CLOCK_HAS_NOT_STARTED) =  MISSED_PAYMENTS;
        g_currentstate = fsm_machine.GetX(g_currentstate, NO_VIG_AND_CLOCK_HAS_NOT_STARTED);
        return g_currentstate;
    } 

    // NO_VIG_AND_CLOCK_HAS_ALREADY_STARTED
    int nvachas()
    {
        // fsm_machine.GetX(MISSED_PAYMENTS, NO_VIG_AND_CLOCK_HAS_ALREADY_STARTED) = MISSED_PAYMENTS;
        g_currentstate = fsm_machine.GetX(g_currentstate, NOT_ENOUGH_VIG_TO_MAKE_FULL_PAYMENT);
        return g_currentstate;
    }
 
    //  NO_VIG_AND_CLOCK_HAS_EXPIRED
    int nvache()
    {
        // fsm_machine.GetX(MISSED_PAYMENTS, NO_VIG_AND_CLOCK_HAS_EXPIRED) = END_OF_GRACE_PERIOD;
        g_currentstate =  fsm_machine.GetX(g_currentstate,  NO_VIG_AND_CLOCK_HAS_EXPIRED);
        return g_currentstate;
    }

    // PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_ALREADY_STARTED
    int povmbnetmafracas()
    {
        // fsm_machine.GetX(MISSED_PAYMENTS, PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_ALREADY_STARTED) = MISSED_PAYMENTS;
        g_currentstate = fsm_machine.GetX(g_currentstate, PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_ALREADY_STARTED);
        return g_currentstate;
    }

    // PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_HAS_EXPIRED
    int povmbnetmafrache()
    {
        // fsm_machine.GetX(MISSED_PAYMENTS, PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_HAS_EXPIRED) = END_OF_GRACE_PERIOD;
        g_currentstate = fsm_machine.GetX(g_currentstate, PAYMENT_OF_VIG_MADE_BUT_NOT_ENOUGH_TO_MAKE_A_FULL_REPAYMENT_AND_CLOCK_HAS_EXPIRED);
        return g_currentstate;
    }

    // PAYMENT_OF_VIG_MADE_THAT_COVERS_MISSED_PAYMENTS_AND_CLOCK_HAS_ALREADY_STARTED
    int povmtcmpachas()
    {
        // fsm_machine.GetX(MISSED_PAYMENTS,  PAYMENT_OF_VIG_MADE_THAT_COVERS_MISSED_PAYMENTS_AND_CLOCK_HAS_ALREADY_STARTED) = PAYMENTS;
        g_currentstate = fsm_machine.GetX(g_currentstate,  PAYMENT_OF_VIG_MADE_THAT_COVERS_MISSED_PAYMENTS_AND_CLOCK_HAS_ALREADY_STARTED);
        return g_currentstate;
    }

    // DELIQUENCY_FEE_PAYMENTS
    void dfp()
    {
        //  fsm_machine.GetX(END_OF_GRACE_PERIOD, DELIQUENCY_FEE_PAYMENTS) = END_OF_GRACE_PERIOD;
        g_currentstate =  fsm_machine.GetX(g_currentstate, DELIQUENCY_FEE_PAYMENTS);
    }

    //  DELIQUENCY_FEE_PAYMENTS_SETTLED
    void dfps()
    {
        // fsm_machine.GetX(END_OF_GRACE_PERIOD, DELIQUENCY_FEE_PAYMENTS_SETTLED) = PAYMENTS;
        g_currentstate = fsm_machine.GetX(g_currentstate, DELIQUENCY_FEE_PAYMENTS_SETTLED);
    }

};
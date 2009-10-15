/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2007 Chris Kenyon
 Copyright (C) 2009 StatPro Italia srl

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file zerocouponinflationswap.hpp
    \brief Zero-coupon inflation-indexed swap
*/

#ifndef quantlib_zciis_hpp
#define quantlib_zciis_hpp

#include <ql/instruments/inflationswap.hpp>

namespace QuantLib {

    //! Zero-coupon inflation-indexed swap
    /*! Quoted as a fixed rate \f$ K \f$.  At start:
        \f[
        P_n(0,T) N [(1+K)^{T}-1] =
        P_n(0,T) N \left[ \frac{I(T)}{I(0)} -1 \right]
        \f]
        where \f$ T \f$ is the maturity time, \f$ P_n(0,t) \f$ is the
        nominal discount factor at time \f$ t \f$, \f$ N \f$ is the
        notional, and \f$ I(t) \f$ is the inflation index value at
        time \f$ t \f$.
    */
    class ZeroCouponInflationSwap : public InflationSwap {
      public:
        class arguments;
        class engine;
        ZeroCouponInflationSwap(const Date& start,
                                const Date& maturity,
                                const Period& lag,
                                Rate fixedRate,
                                const Calendar& calendar,
                                BusinessDayConvention convention,
                                const DayCounter& dayCounter);

        //! \name Inspectors
        //@{
        //! \f$ K \f$ in the above formula.
        Rate fixedRate() const;
        //@}

        //! \name Instrument interface
        //@{
        void setupArguments(PricingEngine::arguments*) const;
        //@}

      protected:
        Rate fixedRate_;
    };


    class ZeroCouponInflationSwap::arguments : public InflationSwap::arguments {
      public:
        Rate fixedRate;
        void validate() const;
    };

    class ZeroCouponInflationSwap::engine
        : public GenericEngine<ZeroCouponInflationSwap::arguments,
                               ZeroCouponInflationSwap::results> {};

}


#endif
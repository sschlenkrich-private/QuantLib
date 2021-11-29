/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
Copyright (C) 2018 Sebastian Schlenkrich

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


#ifndef quantlib_vanillalocalvolmodel_hpp
#define quantlib_vanillalocalvolmodel_hpp

#include <vector>
#include <string>

#include <ql/types.hpp>
#include <ql/errors.hpp>

namespace QuantLib {

    class VanillaLocalVolModel {
    private:
        // input parameters
        Time T_;                    // time to expiry (in years)
        Real S0_;                   // forward
        Real sigmaATM_;             // ATM normal volatility as basis for straddle calculation
        std::vector<Real> Sp_;      // S_i with S_i > S0
        std::vector<Real> Sm_;      // S_-i with S_-i < S0
        std::vector<Real> Mp_;      // slope on intervall [S_i-1, S_i)
        std::vector<Real> Mm_;      // slope on intervall (S_-i, S_-[i-1]]
        // calculated parameters
        Real  straddleATM_;          // ATM straddle price
        Real  sigma0_;               // local vol at S0, i.e. sigma(S0)
        std::vector<Real>  sigmaP_;  // sigma(Sp_[i])
        std::vector<Real>  sigmaM_;  // sigma(Sm_[i])
        std::vector<Real>  Xp_;      // X_i with X_i > 0
        std::vector<Real>  Xm_;      // X_-i with X_-i < 0
        // adjusters
        Real mu_;      // in-the-model adjuster for forward
        Real alpha_;   // out-of-the-model adjuster for straddle
        Real nu_;      // out-of-the-model adjuster for forward
        // numerical accuracy parameters (maybe expose to user...)
        Real    extrapolationStdevs_;  // number of stdevs used as lower and upper cutoff, default 10
        Size    maxCalibrationIters_;  // number of iterations for forward/sigma0 calibration
        Size    onlyForwardCalibrationIters_;  // we may have some initial iterations only calibrating forward, this is intended to stabilise calibration
        Real    sigma0Tol_;            // tolerance for sigma convergence
        Real    S0Tol_;                // tolerance for forward convergence
        bool    adjustATM_;            // apply post-calibration ATM adjuster
        bool    useInitialMu_;
        Real    initialMu_;														
        // we may want some debug information for the calibration process
        bool                     enableLogging_;
        std::vector<std::string> logging_;
 
        // we have two constructors and want to make sure the setup is consistent
        void initializeDeepInTheModelParameters();

        // determine the lower and upper bounds for integration
        inline Real lowerBoundX() const { return -extrapolationStdevs_ * sqrt(T_) + mu_; }
        inline Real upperBoundX() const { return extrapolationStdevs_ * sqrt(T_) + mu_; }

        // this is an unsafe method specifying the vol function sigma(S) on the individual segments
        Real localVol(bool isRightWing, Size k, Real S);

        // this is an unsafe method specifying the underlying level S(x) on the individual segments
        Real underlyingS(bool isRightWing, Size k, Real x);

        // this is an unsafe method specifying the underlying level x(S) on the individual segments
        Real underlyingX(bool isRightWing, Size k, Real S);

        // this is an unsafe method specifying the primitive function F(x) = \int [alpha S(x) + nu] p(x) dx
        // on the individual segments
        Real primitiveF(bool isRightWing, Size k, Real x);

        // this is an unsafe method specifying the primitive function F(x) = \int [alpha S(x) + nu]^2 p(x) dx
        // on the individual segments
        Real primitiveFSquare(bool isRightWing, Size k, Real x);

        // this is an unsafe method to calculate the S-grid for a given x-grid
        // it is intended as a preprocessing step in conjunction with smile interplation
        // validity of the model is ensured by proceeding it with updateLocalVol()
        void calculateSGrid();

        // use ODE solution to determine x-grid and sigma-grid taking into account constraints of
        // positive local volatility and local vol extrapolation
        void updateLocalVol();

        // calibrate \mu and \sigma_0 based on given S_0 and \sigma_ATM
        void calibrateATM();

        // calculate post-ATM-calibration adjusters
        void adjustATM();

    public:
        // construct model based on S-grid
      VanillaLocalVolModel(Time T,
                           Real S0,
                           Real sigmaATM,
                           const std::vector<Real>& Sp,
                           const std::vector<Real>& Sm,
                           const std::vector<Real>& Mp,
                           const std::vector<Real>& Mm,
                           // controls for calibration
                           Size maxCalibrationIters = 5,
                           Size onlyForwardCalibrationIters = 0,
                           bool adjustATMFlag = true,
                           bool enableLogging = false,
                           bool useInitialMu = false,
                           Real initialMu = 0.0);

      // construct model based on x-grid
      VanillaLocalVolModel(Time T,
                           Real S0,
                           Real sigmaATM,
                           Real sigma0,
                           const std::vector<Real>& Xp,
                           const std::vector<Real>& Xm,
                           const std::vector<Real>& Mp,
                           const std::vector<Real>& Mm,
                           // controls for calibration
                           Size maxCalibrationIters = 5,
                           Size onlyForwardCalibrationIters = 0,
                           bool adjustATMFlag = true,
                           bool enableLogging = false,
                           bool useInitialMu = false,
                           Real initialMu = 0.0);

      // inspectors

      inline std::vector<std::string> logging() { return logging_; }
      inline Time timeToExpiry() const { return T_; }
      inline Real forward() const { return S0_; }
      inline Real sigmaATM() const { return sigmaATM_; }
      inline Real alpha() const { return alpha_; }
      inline Real mu() const { return mu_; }
      inline Real nu() const { return nu_; }
      inline Size maxCalibrationIters() const { return maxCalibrationIters_; }
      inline Size onlyForwardCalibrationIters() const { return onlyForwardCalibrationIters_; }
      inline bool adjustATMFlag() const { return adjustATM_; }
      inline bool enableLogging() const { return enableLogging_; }
      inline bool useInitialMu() const { return useInitialMu_; }
      inline Real initialMu() const { return initialMu_; }

      // attributes in more convenient single-vector format

      const std::vector<Real> underlyingX();
      const std::vector<Real> underlyingS();
      const std::vector<Real> localVol();
      const std::vector<Real> localVolSlope();

      // model function evaluations

      const Real localVol(Real S);
      const Real underlyingS(Real x);

      // calculating expectations - that is the actual purpose of that model

      // calculate the forward price of an OTM option
      const Real expectation(bool isRightWing, Real strike);

      // calculate the forward price of an OTM power option with payoff 1_{S>K}(S-K)^2
      const Real variance(bool isRightWing, Real strike);

    };


}


#endif  /* ifndef quantlib_vanillalocalvolmodel_hpp_hpp */

/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2006, 2007, 2008 Ferdinando Ametrano
 Copyright (C) 2000, 2001, 2002, 2003 RiskMap srl
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

#include <ql/time/schedule.hpp>
#include <ql/time/imm.hpp>
#include <ql/settings.hpp>

namespace QuantLib {

    namespace {

        Date nextTwentieth(const Date& d, DateGeneration::Rule rule) {
            Date result = Date(20, d.month(), d.year());
            if (result < d)
                result += 1*Months;
            if (rule == DateGeneration::TwentiethIMM ||
                rule == DateGeneration::OldCDS) {
                Month m = result.month();
                if (m % 3 != 0) { // not a main IMM nmonth
                    Integer skip = 3 - m%3;
                    result += skip*Months;
                }
            }
            return result;
        }

    }


    Schedule::Schedule(const std::vector<Date>& dates,
                       const Calendar& calendar,
                       BusinessDayConvention convention)
    : fullInterface_(false),
      tenor_(Period()), calendar_(calendar),
      convention_(convention),
      terminationDateConvention_(convention),
      rule_(DateGeneration::Forward), endOfMonth_(false),
      finalIsRegular_(true),
      dates_(dates) {}

    Schedule::Schedule(const Date& effectiveDate,
                       const Date& terminationDate,
                       const Period& tenor,
                       const Calendar& calendar,
                       BusinessDayConvention convention,
                       BusinessDayConvention terminationDateConvention,
                       DateGeneration::Rule rule,
                       bool endOfMonth,
                       const Date& firstDate,
                       const Date& nextToLastDate)
    : fullInterface_(true),
      tenor_(tenor), calendar_(calendar),
      convention_(convention),
      terminationDateConvention_(terminationDateConvention),
      rule_(rule), endOfMonth_(endOfMonth),
      firstDate_(firstDate), nextToLastDate_(nextToLastDate),
      finalIsRegular_(true)
    {
        // sanity checks
        QL_REQUIRE(effectiveDate != Date(), "null effective date");
        QL_REQUIRE(terminationDate != Date(), "null termination date");
        QL_REQUIRE(effectiveDate < terminationDate,
                   "effective date (" << effectiveDate
                   << ") later than or equal to termination date ("
                   << terminationDate << ")");

        if (tenor.length()==0)
            rule_ = DateGeneration::Zero;
        else
            QL_REQUIRE(tenor.length()>0,
                       "non positive tenor (" << tenor << ") not allowed");

        if (firstDate != Date()) {
            switch (rule_) {
              case DateGeneration::Backward:
              case DateGeneration::Forward:
                QL_REQUIRE(firstDate > effectiveDate &&
                           firstDate < terminationDate,
                           "first date (" << firstDate <<
                           ") out of [effective (" << effectiveDate <<
                           "), termination (" << terminationDate <<
                           ")] date range");
                // we should ensure that the above condition is still
                // verified after adjustment
                break;
              case DateGeneration::ThirdWednesday:
                  QL_REQUIRE(IMM::isIMMdate(firstDate, false),
                             "first date (" << firstDate <<
                             ") is not an IMM date");
                break;
              case DateGeneration::Zero:
              case DateGeneration::Twentieth:
              case DateGeneration::TwentiethIMM:
              case DateGeneration::OldCDS:
                QL_FAIL("first date incompatible with " << rule_ <<
                        " date generation rule");
              default:
                QL_FAIL("unknown rule (" << Integer(rule_) << ")");
            }
        }
        if (nextToLastDate != Date()) {
            switch (rule_) {
              case DateGeneration::Backward:
              case DateGeneration::Forward:
                QL_REQUIRE(nextToLastDate > effectiveDate &&
                           nextToLastDate < terminationDate,
                           "next to last date (" << nextToLastDate <<
                           ") out of [effective (" << effectiveDate <<
                           "), termination (" << terminationDate <<
                           ")] date range");
                // we should ensure that the above condition is still
                // verified after adjustment
                break;
              case DateGeneration::ThirdWednesday:
                QL_REQUIRE(IMM::isIMMdate(nextToLastDate, false),
                           "next-to-last date (" << nextToLastDate <<
                           ") is not an IMM date");
                break;
              case DateGeneration::Zero:
              case DateGeneration::Twentieth:
              case DateGeneration::TwentiethIMM:
              case DateGeneration::OldCDS:
                QL_FAIL("next to last date incompatible with " << rule_ <<
                        " date generation rule");
              default:
                QL_FAIL("unknown rule (" << Integer(rule_) << ")");
            }
        }


        // calendar needed for endOfMonth adjustment
        Calendar nullCalendar = NullCalendar();
        Integer periods = 1;
        Date seed, exitDate;
        switch (rule_) {

          case DateGeneration::Zero:
            tenor_ = 0*Years;
            dates_.push_back(effectiveDate);
            dates_.push_back(terminationDate);
            isRegular_.push_back(true);
            break;

          case DateGeneration::Backward:

            dates_.push_back(terminationDate);

            seed = terminationDate;
            if (nextToLastDate != Date()) {
                dates_.insert(dates_.begin(), nextToLastDate);
                Date temp = nullCalendar.advance(seed,
                    -periods*tenor_, convention, endOfMonth);
                if (temp!=nextToLastDate)
                    isRegular_.insert(isRegular_.begin(), false);
                else
                    isRegular_.insert(isRegular_.begin(), true);
                seed = nextToLastDate;
            }

            exitDate = effectiveDate;
            if (firstDate != Date())
                exitDate = firstDate;

            while (true) {
                Date temp = nullCalendar.advance(seed,
                    -periods*tenor_, convention, endOfMonth);
                if (temp < exitDate) {
                    if (firstDate != Date() &&
                        (calendar.adjust(dates_.front(),convention)!=
                         calendar.adjust(firstDate,convention))) {
                        dates_.insert(dates_.begin(), firstDate);
                        isRegular_.insert(isRegular_.begin(), false);
                    }
                    break;
                } else {
                    dates_.insert(dates_.begin(), temp);
                    isRegular_.insert(isRegular_.begin(), true);
                    ++periods;
                }
            }

            if (endOfMonth && calendar.isEndOfMonth(seed))
                convention=Preceding;

            if (calendar.adjust(dates_.front(),convention)!=
                calendar.adjust(effectiveDate,convention)) {
                dates_.insert(dates_.begin(), effectiveDate);
                isRegular_.insert(isRegular_.begin(), false);
            }
            break;

          case DateGeneration::Twentieth:
          case DateGeneration::TwentiethIMM:
          case DateGeneration::ThirdWednesday:
          case DateGeneration::OldCDS:
            QL_REQUIRE(!endOfMonth,
                       "endOfMonth convention incompatible with " << rule_ <<
                       " date generation rule");
          // fall through
          case DateGeneration::Forward:

            dates_.push_back(effectiveDate);

            seed = effectiveDate;

            if (firstDate!=Date()) {
                dates_.push_back(firstDate);
                Date temp = nullCalendar.advance(seed, periods*tenor_,
                                                 convention, endOfMonth);
                if (temp!=firstDate)
                    isRegular_.push_back(false);
                else
                    isRegular_.push_back(true);
                seed = firstDate;
            } else if (rule_ == DateGeneration::Twentieth ||
                       rule_ == DateGeneration::TwentiethIMM ||
                       rule_ == DateGeneration::OldCDS) {
                Date next20th = nextTwentieth(effectiveDate, rule_);
                if (rule_ == DateGeneration::OldCDS) {
                    // distance rule inforced in natural days
                    static const BigInteger stubDays = 30;
                    if (next20th - effectiveDate < stubDays) {
                        // +1 will skip this one and get the next
                        next20th = nextTwentieth(next20th + 1, rule_);
                    }
                }
                if (next20th != effectiveDate) {
                    dates_.push_back(next20th);
                    isRegular_.push_back(false);
                    seed = next20th;
                }
            }

            exitDate = terminationDate;
            if (nextToLastDate != Date())
                exitDate = nextToLastDate;

            while (true) {
                Date temp = nullCalendar.advance(seed, periods*tenor_,
                                                 convention, endOfMonth);
                if (temp > exitDate) {
                    if (nextToLastDate != Date() &&
                        (calendar.adjust(dates_.back(),convention)!=
                         calendar.adjust(nextToLastDate,convention))) {
                        dates_.push_back(nextToLastDate);
                        isRegular_.push_back(false);
                    }
                    break;
                } else {
                    dates_.push_back(temp);
                    isRegular_.push_back(true);
                    ++periods;
                }
            }

            if (endOfMonth && calendar.isEndOfMonth(seed))
                convention=Preceding;

            if (calendar.adjust(dates_.back(),terminationDateConvention)!=
                calendar.adjust(terminationDate,terminationDateConvention)) {
                if (rule_ == DateGeneration::Twentieth ||
                    rule_ == DateGeneration::TwentiethIMM ||
                    rule_ == DateGeneration::OldCDS) {
                    dates_.push_back(nextTwentieth(terminationDate, rule_));
                    isRegular_.push_back(true);
                } else {
                    dates_.push_back(terminationDate);
                    isRegular_.push_back(false);
                }
            }

            break;

          default:
            QL_FAIL("unknown rule (" << Integer(rule_) << ")");
        }

        // adjustments
        if (rule_==DateGeneration::ThirdWednesday)
            for (Size i=1; i<dates_.size()-1; ++i)
                dates_[i] = Date::nthWeekday(3, Wednesday,
                                             dates_[i].month(),
                                             dates_[i].year());

        // first date not adjusted for CDS schedules
        if (rule_ != DateGeneration::OldCDS)
            dates_[0] = calendar.adjust(dates_[0], convention);
        for (Size i=1; i<dates_.size()-1; ++i)
            dates_[i] = calendar.adjust(dates_[i], convention);

        // termination date is NOT adjusted as per ISDA
        // specifications, unless otherwise specified in the
        // confirmation of the deal or unless we're creating a CDS
        // schedule
        if (terminationDateConvention != Unadjusted
            || rule_ == DateGeneration::Twentieth
            || rule_ == DateGeneration::TwentiethIMM
            || rule_ == DateGeneration::OldCDS) {
            dates_.back() = calendar.adjust(dates_.back(),
                                            terminationDateConvention);
        }
    }

    std::vector<Date>::const_iterator
    Schedule::lower_bound(const Date& refDate) const {
        Date d = (refDate==Date() ?
                  Settings::instance().evaluationDate() :
                  refDate);
        return std::lower_bound(dates_.begin(), dates_.end(), d);
    }

    Date Schedule::nextDate(const Date& refDate) const {
        std::vector<Date>::const_iterator res = lower_bound(refDate);
        if (res!=dates_.end())
            return *res;
        else
            return Date();
    }

    Date Schedule::previousDate(const Date& refDate) const {
        std::vector<Date>::const_iterator res = lower_bound(refDate);
        if (res!=dates_.begin())
            return *(--res);
        else
            return Date();
    }

    bool Schedule::isRegular(Size i) const {
        QL_REQUIRE(fullInterface_, "full interface not available");
        QL_REQUIRE(i<=isRegular_.size() && i>0,
                   "index (" << i << ") must be in [1, " <<
                   isRegular_.size() <<"]");
        return isRegular_[i-1];
    }


    MakeSchedule::MakeSchedule()
    : rule_(DateGeneration::Backward), endOfMonth_(false) {}

    MakeSchedule& MakeSchedule::from(const Date& effectiveDate) {
        effectiveDate_ = effectiveDate;
        return *this;
    }

    MakeSchedule& MakeSchedule::to(const Date& terminationDate) {
        terminationDate_ = terminationDate;
        return *this;
    }

    MakeSchedule& MakeSchedule::withTenor(const Period& tenor) {
        tenor_ = tenor;
        return *this;
    }

    MakeSchedule& MakeSchedule::withFrequency(Frequency frequency) {
        tenor_ = Period(frequency);
        return *this;
    }

    MakeSchedule& MakeSchedule::withCalendar(const Calendar& calendar) {
        calendar_ = calendar;
        return *this;
    }

    MakeSchedule& MakeSchedule::withConvention(BusinessDayConvention conv) {
        convention_ = conv;
        return *this;
    }

    MakeSchedule& MakeSchedule::withTerminationDateConvention(
                                                BusinessDayConvention conv) {
        terminationDateConvention_ = conv;
        return *this;
    }

    MakeSchedule& MakeSchedule::withRule(DateGeneration::Rule r) {
        rule_ = r;
        return *this;
    }

    MakeSchedule& MakeSchedule::forwards() {
        rule_ = DateGeneration::Forward;
        return *this;
    }

    MakeSchedule& MakeSchedule::backwards() {
        rule_ = DateGeneration::Backward;
        return *this;
    }

    MakeSchedule& MakeSchedule::endOfMonth(bool flag) {
        endOfMonth_ = flag;
        return *this;
    }

    MakeSchedule& MakeSchedule::withFirstDate(const Date& d) {
        firstDate_ = d;
        return *this;
    }

    MakeSchedule& MakeSchedule::withNextToLastDate(const Date& d) {
        nextToLastDate_ = d;
        return *this;
    }

    MakeSchedule::operator Schedule() const {
        // check for mandatory arguments
        QL_REQUIRE(effectiveDate_ != Date(), "effective date not provided");
        QL_REQUIRE(terminationDate_ != Date(), "termination date not provided");
        QL_REQUIRE(tenor_, "tenor/frequency not provided");

        // set dynamic defaults:
        BusinessDayConvention convention;
        // if a convention was set, we use it.
        if (convention_) {
            convention = *convention_;
        } else {
            if (!calendar_.empty()) {
                // ...if we set a calendar, we probably want it to be used;
                convention = Following;
            } else {
                // if not, we don't care.
                convention = Unadjusted;
            }
        }

        BusinessDayConvention terminationDateConvention;
        // if set explicitly, we use it;
        if (terminationDateConvention_) {
            terminationDateConvention = *terminationDateConvention_;
        } else {
            // Unadjusted as per ISDA specification
            terminationDateConvention = convention;
        }

        Calendar calendar = calendar_;
        // if no calendar was set...
        if (calendar.empty()) {
            // ...we use a null one.
            calendar = NullCalendar();
        }

        return Schedule(effectiveDate_, terminationDate_, *tenor_, calendar,
                        convention, terminationDateConvention,
                        rule_, endOfMonth_, firstDate_, nextToLastDate_);
    }

}
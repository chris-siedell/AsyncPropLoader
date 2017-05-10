//
//  SimpleChrono.hpp
//  HSerial
//
//  Created by admin on 5/5/17.
//  Copyright © 2017 Chris Siedell. All rights reserved.
//

#ifndef SimpleChrono_hpp
#define SimpleChrono_hpp

#include <chrono>


namespace simple {

    
    typedef std::chrono::steady_clock SteadyClock;

    typedef std::chrono::time_point<simple::SteadyClock> SteadyTimePoint;

    typedef std::chrono::milliseconds Milliseconds;
    typedef std::chrono::microseconds Microseconds;
    

    /*!
     \brief Converts floating point seconds to simple::Milliseconds.
     */
    inline simple::Milliseconds millisecondsFromFloatSeconds(float seconds) {
        return Milliseconds(static_cast<long long>(seconds * 1.0e3f));
    }

    /*!
     \brief Converts simple::Milliseconds to floating point seconds.
     */
    inline float floatSecondsFromMilliseconds(simple::Milliseconds milliseconds) {
        return milliseconds.count() / 1.0e3f;
    }

}


#endif /* SimpleChrono_hpp */

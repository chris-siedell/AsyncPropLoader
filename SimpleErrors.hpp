//
//  SimpleErrors.hpp
//  HSerial
//
//  Created by admin on 5/5/17.
//  Copyright © 2017 Chris Siedell. All rights reserved.
//

#ifndef SimpleErrors_hpp
#define SimpleErrors_hpp

#include <stdexcept>


namespace simple {

    /*!
     \brief An exception for being busy.
     */
    class IsBusyError : public std::runtime_error {
    public:
        IsBusyError() : std::runtime_error("The thing is busy.") {}
        IsBusyError(const std::string& description) : std::runtime_error(description) {}
    };

    /*!
     \brief An exception for timeouts.
     */
    class TimeoutError : public std::runtime_error {
    public:
        TimeoutError() : std::runtime_error("Time limit exceeded.") {}
        TimeoutError(const std::string& description) : std::runtime_error(description) {}
    };

}


#endif /* SimpleErrors_hpp */

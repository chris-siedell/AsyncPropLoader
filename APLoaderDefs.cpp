//
//  APLoaderDefs.cpp
//  libserial loader 001
//
//  Created by admin on 2/8/17.
//  Copyright © 2017 Chris Siedell. All rights reserved.
//

#include "APLoaderDefs.hpp"

#include <cassert>
#include <iomanip>


namespace APLoader {
    

#pragma mark - ResetLine Enum

    bool resetLineIsValid(ResetLine resetLine) {
        // Probably not necessary since an enum class is used.
        return resetLine == ResetLine::DTR || resetLine == ResetLine::RTS || resetLine == ResetLine::Callback;
    }

    std::string strForResetLine(ResetLine resetLine) {
        switch (resetLine) {
            case ResetLine::DTR:
                return "DTR";
            case ResetLine::RTS:
                return "RTS";
            case ResetLine::Callback:
                return "callback";
            default:
                return "unknown";
        }
    }


#pragma mark - Status Enum

    std::string strForStatus(Status status) {
        switch (status) {
            case Status::Resetting:
                return "resetting";
            case Status::EstablishingCommunications:
                return "establishing communications";
            case Status::SendingCommandAndImage:
                return "sending command and image";
            case Status::WaitingForChecksumStatus:
                return "waiting for checksum status";
            case Status::WaitingForEEPROMProgrammingStatus:
                return "waiting for EEPROM programming status";
            case Status::WaitingForEEPROMVerificationStatus:
                return "waiting for EEPROM verification status";
            default:
                return "unknown";
        }
    }


#pragma mark - Action Enum

    bool actionIsValid(Action action) {
        // Necessary, since this test considers None invalid.
        return action == Action::Shutdown || action == Action::LoadRAM || action == Action::ProgramEEPROMThenShutdown || action == Action::ProgramEEPROMThenRun || action == Action::Restart;
    }

    std::string strForAction(Action action) {
        switch (action) {
            case Action::Shutdown:
                return "shutdown";
            case Action::LoadRAM:
                return "load RAM";
            case Action::ProgramEEPROMThenShutdown:
                return "program EEPROM then shutdown";
            case Action::ProgramEEPROMThenRun:
                return "program EEPROM then run";
            case Action::Restart:
                return "restart";
            case Action::None:
                return "none";
            default:
                return "unknown";
        }
    }

    bool actionRequiresImage(Action action) {
        return action == Action::LoadRAM || action == Action::ProgramEEPROMThenShutdown || action == Action::ProgramEEPROMThenRun;
    }

    uint32_t commandForAction(Action action) {
        switch (action) {
            case Action::Shutdown:
                return 0;
            case Action::LoadRAM:
                return 1;
            case Action::ProgramEEPROMThenShutdown:
                return 2;
            case Action::ProgramEEPROMThenRun:
                return 3;
            default:
                return 0xffffffff;
        }
    }


#pragma mark - ErrorCode Enum

    std::string strForErrorCode(ErrorCode errorCode) {
        switch (errorCode) {
            case ErrorCode::None:
                return "none";
            case ErrorCode::Cancelled:
                return "cancelled";
            case ErrorCode::FailedToObtainPortAccess:
                return "failed to obtain port access";
            case ErrorCode::FailedToOpenPort:
                return "failed to open port";
            case ErrorCode::FailedToFlushOutput:
                return "failed flush output";
            case ErrorCode::FailedToSetBaudrate:
                return "failed to set baudrate";
            case ErrorCode::FailedToSetTimeout:
                return "failed to set timeout";
            case ErrorCode::FailedToSetBytesize:
                return "failed to set bytesize";
            case ErrorCode::FailedToSetParity:
                return "failed to set parity";
            case ErrorCode::FailedToSetStopbits:
                return "failed to set stopbits";
            case ErrorCode::FailedToSetFlowcontrol:
                return "failed to set flowcontrol";
            case ErrorCode::FailedToReset:
                return "failed to reset";
            case ErrorCode::FailedToFlushInput:
                return "failed to flush input";
            case ErrorCode::FailedToSendInitialBytes:
                return "failed to send initial bytes";
            case ErrorCode::FailedToReceivePropAuthentication:
                return "failed to receive Propeller authentication";
            case ErrorCode::FailedToAuthenticateProp:
                return "failed to authenticate Propeller";
            case ErrorCode::FailedToReceiveChipVersion:
                return "failed to receive chip version";
            case ErrorCode::FailedToDecodeChipVersion:
                return "failed to decode chip version";
            case ErrorCode::UnsupportedChipVersion:
                return "unsupported chip version";
            case ErrorCode::FailedToSendCommand:
                return "failed to send command";
            case ErrorCode::FailedToEncodeImageSize:
                return "failed to encode image size";
            case ErrorCode::FailedToSendImageSize:
                return "failed to send image size";
            case ErrorCode::FailedToSendImage:
                return "failed to send image";
            case ErrorCode::FailedToSendStatusPrompt:
                return "failed to send status prompt";
            case ErrorCode::FailedToReceiveChecksumStatus:
                return "failed to receive checksum status";
            case ErrorCode::PropReportsChecksumError:
                return "Propeller reports checksum error";
            case ErrorCode::FailedToReceiveEEPROMProgrammingStatus:
                return "failed to receive EEPROM programming status";
            case ErrorCode::PropReportsEEPROMProgrammingError:
                return "Propeller reports EEPROM programming error";
            case ErrorCode::FailedToReceiveEEPROMVerificationStatus:
                return "failed to receive EEPROM verification status";
            case ErrorCode::PropReportsEEPROMVerificationError:
                return "Propeller reports EEPROM verification error";
            case ErrorCode::UnhandledException:
                return "BUG: unhandled exception";
            default:
                return "unknown";
        }
    }
    
    
} // namespace APLoader


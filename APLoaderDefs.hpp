//
//  APLoaderDefs.hpp
//  libserial loader 001
//
//  Created by admin on 2/8/17.
//  Copyright © 2017 Chris Siedell. All rights reserved.
//

#ifndef APLoaderDefs_hpp
#define APLoaderDefs_hpp

#include <chrono>
#include <cstddef>
#include <string>
#include <sstream>

#include "SimpleChrono.hpp"


namespace APLoader {
    

#pragma mark - ResetCallback

    /*!
     \brief Defines a function that performs a user implemented Propeller reset.

     This callback might be useful in situations where user code can use GPIO pins of a board
     such as a Raspberry PI but modifying this code is not an option.

     User code must provide a callback that manually performs the reset. The callback is expected
     to drop the reset line low, hold it low for resetDuration, and then raise the line and return.
     
     The callback is called on the worker thread created for performing the loader action.

     The loader will begin its boot wait immediately after the callback returns (unless the action
     being performed is a reset, in which case it will finish the action).

     Exceptions thrown from this callback are caught and will cause the loader to abort the action.
     
     \see AsyncPropLoader::setResetLine, AsyncPropLoader::setResetCallback
     */
    typedef void (*ResetCallback)(const simple::Milliseconds& resetDuration);


#pragma mark - ResetLine Enum

    /*!
     \brief The control lines that the loader may use to trigger a Propeller reset.
     
     The Callback option defers the responsibility of toggling the reset line to user code.
     
     \see AsyncPropLoader::setResetLine, AsyncPropLoader::setResetLineCallback,
     APLoader::ResetLineCallback, resetLineIsValid, strForResetLine
     */
    enum class ResetLine {
        DTR,
        RTS,
        Callback,
    };

    /*!
     \brief Indicates if the given reset line has a valid value.
     */
    bool resetLineIsValid(ResetLine resetLine);

    /*!
     \brief Returns a string describing the given reset line.
     */
    std::string strForResetLine(ResetLine resetLine);


#pragma mark - Status Enum

    /*!
     \brief These identify the status of the loader when performing an action.
     
     These status values are reported to user via the StatusMonitor::loaderUpdate() callback.
     
     \see strForStatus, StatusMonitor::loaderUpdate
     */
    enum class Status {
        Resetting,
        EstablishingCommunications,
        SendingCommandAndImage,
        WaitingForChecksumStatus,
        WaitingForEEPROMProgrammingStatus,
        WaitingForEEPROMVerificationStatus
    };

    /*!
     \brief Returns a string describing the given loader status.
     */
    std::string strForStatus(Status status);


#pragma mark - Action Enum

    /*!
     \brief These identify the actions the loader may perform.
     
     Action identifiers are passed to user code in the StatusMonitor::loaderWillBegin()
     callback, and in the ActionSummary struct.
     
     The Shutdown, LoadRAM, ProgramEEPROMThenShutdown, and ProgramEEPROMThenRun actions involve
     interacting with the Propeller's booter program.

     Restart just means to toggle the reset line without interacting with the booter program. In
     this case the Propeller should eventually attempt to run from the EEPROM.

     \see actionIsValid, actionRequiresImage, strForAction, commandForAction,
     ActionSummary::action
     */
    enum class Action {
        None,
        Shutdown,
        LoadRAM,
        ProgramEEPROMThenShutdown,
        ProgramEEPROMThenRun,
        Restart
    };

    /*!
     \brief Indicates if the given action is a valid, non-None action.
     */
    bool actionIsValid(Action action);

    /*!
     \brief Returns a string describing the given action.
     */
    std::string strForAction(Action action);

    /*!
     \brief Indicates if the action requires an image.
     */
    bool actionRequiresImage(Action action);

    /*!
     \brief Returns the command number for a given action.
     
     This is the number used to issue a command to the Propeller's booter program.
     
     For actions that don't have a corresponding command (e.g. Action::Restart) this function
     returns 0xffffffff, which if sent to the Propeller will cause it to shutdown.
     */
    uint32_t commandForAction(Action action);


#pragma mark - ErrorCode Enum

    /*!
     \brief Identifies the primary reason a loader action has failed.
     
     An error code is passed to the StatusMonitor::loaderHasFinishedWithError() callback. It is
     also part of the ActionSummary struct.
     
     \see strForErrorCode, ActionSummary::errorCode
     */
    enum class ErrorCode {
        None,
        Cancelled,
        FailedToObtainPortAccess,               // Another controller is using the port, and refuses to relinquish it.
        FailedToOpenPort,
        FailedToFlushOutput,
        FailedToSetBaudrate,
        FailedToSetTimeout,                     // Specifically, the serial port's read and write timeouts.
        FailedToSetBytesize,
        FailedToSetParity,
        FailedToSetStopbits,
        FailedToSetFlowcontrol,
        FailedToReset,
        FailedToFlushInput,
        FailedToSendInitialBytes,
        FailedToReceivePropAuthentication,      // The authentication data was not received.
        FailedToAuthenticateProp,               // The authentication data was received, but it was not correct.
        FailedToReceiveChipVersion,             // The chip version was not received.
        FailedToDecodeChipVersion,              // The chip version was received, but was not encoded in valid 3BP.
        UnsupportedChipVersion,                 // The chip version was received, but is not supported.
        FailedToSendCommand,
        FailedToEncodeImageSize,
        FailedToSendImageSize,
        FailedToSendImage,
        FailedToSendStatusPrompt,               // A transmission prompt necessary to get a status code could not be sent.
        FailedToReceiveChecksumStatus,
        PropReportsChecksumError,
        FailedToReceiveEEPROMProgrammingStatus,
        PropReportsEEPROMProgrammingError,
        FailedToReceiveEEPROMVerificationStatus,
        PropReportsEEPROMVerificationError,
        UnhandledException                      // A bug AsyncPropLoader.
    };

    /*!
     \brief Returns a string describing the given error code.
     */
    std::string strForErrorCode(ErrorCode errorCode);


#pragma mark - ActionSummary Struct

    /*!
     \brief Contains performance information about a loader action. May change.
     
     (Note: After further testing I may reduce or eliminate this profiler information.)
     
     A summary struct is passed to the StatusMonitor::loaderHasFinishedOK() and
     StatusMonitor::loaderHasFinishedWithError() callbacks.
     */
    struct ActionSummary {

        /*!
         \name Basic Information
         */
        /// \{

        /*!
         \brief The action performed.
         */
        Action action;

        /*!
         \brief Indicates if the action was successful.
         \see errorCode
         */
        bool wasSuccessful;

        /*!
         \brief Identifies the type of error if the action was unsuccessful.
         \see wasSuccessful
         */
        ErrorCode errorCode;

        /*!
         \brief The baudrate used when performing the action.
         \see AsyncPropLoader::setBaudrate
         */
        uint32_t baudrate;

        /*!
         \brief The reset duration used when performing the action, in milliseconds.
         \see AsyncPropLoader::setResetDuration
         */
        long long resetDuration;

        /*!
         \brief The boot wait duration used when performing the action, in milliseconds.
         \see AsyncPropLoader::setBootWaitDuration
         */
        long long bootWaitDuration;

        /*!
         \brief The size of the image, in bytes.
         */
        size_t imageSize;

        /*!
         \brief The size of the encoded image, in bytes.
         
         This is the number of bytes required to transmit the 3-Bit-Protocol encoded image.
         */
        size_t encodedImageSize;

        /// \} /Basic Information

        /*!
         \name Timings
         
         Times are in floating point seconds.
         */
        /// \{

        float totalTime;    // Sum of All Stages
        float stage1Time;   // Stage 1: Preparation
        float stage2Time;   // Stage 2: Reset and Wait
        float stage2aTime;  //      2a: Reset
        float stage2bTime;  //      2b: Wait
        float stage3Time;   // Stage 3: Establish Communications
        float stage4Time;   // Stage 4: Send Command and Payload
        float stage4aTime;  //      4a: Send Command

        /*!
         \brief Stage 4b: Send Payload
         
         In this implementation Stage 5 actually begins while some of the payload is still
         being sent over the wire (but all of it has been buffered). So stage4bTime will slightly
         shorter than the true time and and stage5Time will be slightly longer. The deviation
         should be approximately EarlyStage4Return.
         */
        float stage4bTime;

        /*!
         \brief Stage 5: Wait for Checksum Status

         In this implementation Stage 5 actually begins while some of the payload is still
         being sent over the wire (but all of it has been buffered). So stage4bTime will slightly
         shorter than the true time and and stage5Time will be slightly longer. The deviation
         should be approximately EarlyStage4Return.
         */
        float stage5Time;

        float stage6Time;   // Stage 6: Wait for EEPROM Programming Status
        float stage7Time;   // Stage 7: Wait for EEPROM Verification Status
        float encodingTime; // Image encoding is part of Stage 1.

        /// \} /Timings

        void reset() {

            action = Action::None;
            wasSuccessful = false;
            errorCode = ErrorCode::None;
            baudrate = 0;
            resetDuration = 0;
            bootWaitDuration = 0;
            imageSize = 0;
            encodedImageSize = 0;

            totalTime = 0.0f;

            stage1Time = 0.0f;
            stage2Time = 0.0f;
            stage2aTime = 0.0f;
            stage2bTime = 0.0f;
            stage3Time = 0.0f;
            stage4Time = 0.0f;
            stage4aTime = 0.0f;
            stage4bTime = 0.0f;
            stage5Time = 0.0f;
            stage6Time = 0.0f;
            stage7Time = 0.0f;

            encodingTime = 0.0f;
        }
    };


#pragma mark - StatusMonitor

    class AsyncPropLoader;

    /*!
     \brief Defines an object used to follow the activity of AsyncPropLoader.
     */
    class StatusMonitor {
    public:

        /*!
         \brief Called when an action is about to begin.

         Guarantee: If loaderWillBegin is called then loaderHasFinished will be called.

         Note: loaderUpdate might never be called.

         Do not call AsyncPropLoader::cancelAndWait() or AsyncPropLoader::waitUntilFinished()
         from this callback -- it will lock up the thread. Calling AsyncPropLoader::cancel()
         is OK.
         
         todo: consider having those functions test against above situation and throw

         Called on a worker thread, unique for each action -- not the main thread.

         __Important__: This function may not throw exceptions.
         */
        virtual void loaderWillBegin(AsyncPropLoader& loader, APLoader::Action action, float secondsTakenSoFar, float estimatedTotalSeconds) noexcept {}

        /*!
         \brief Called when the status of the loader has changed.

         estimatedTotalSeconds may change between calls. It will always be greater than
         secondsTakenSoFar.

         This callback should return quickly. While it is executing the loader is idle. If the
         loader is idle for too long (approximately 100 milliseconds) the Propeller will reboot.
         Consider redispatching to work to another thread.

         Do not call AsyncPropLoader::cancelAndWait() or AsyncPropLoader::waitUntilFinished()
         from this callback -- it will lock up the thread. Calling AsyncPropLoader::cancel()
         is OK.
         
         todo: consider having those functions test against above situation and throw

         Called on a worker thread, unique for each action -- not the main thread.

         __Important__: This function may not throw exceptions.
         */
        virtual void loaderUpdate(AsyncPropLoader& loader, APLoader::Status status, float secondsTakenSoFar, float estimatedTotalSeconds) noexcept {}

        /*!
         \brief Called when the action has finished.

         If the action finished properly then errorCode will be ErrorCode::None and the
         errorDetails string will be empty.

         When this callback is called the action is finished. AsyncPropLoader::isBusy() will
         return `false` (unless another action has already begun). Any threads that were blocked
         on the action (using AsyncPropLoader::cancelAndWait() or
         AsyncPropLoader::waitUntilFinished()) will have already been unblocked.

         Guarantee: loaderWillBegin() for subsequent actions will not
         be called until this callback returns.

         Called on a worker thread, unique for each action -- not the main thread.

         __Important__: This function may not throw exceptions.
         */
        virtual void loaderHasFinished(AsyncPropLoader& loader, APLoader::ErrorCode errorCode, const std::string& errorDetails, const APLoader::ActionSummary& summary) noexcept {}
    };


} // namespace APLoader


#endif /* APLoaderDefs_hpp */

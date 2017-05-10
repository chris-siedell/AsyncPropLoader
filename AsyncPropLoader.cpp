//
//  AsyncPropLoader.cpp
//  libserial loader 001
//
//  Created by admin on 1/31/17.
//  Copyright © 2017 Chris Siedell. All rights reserved.
//

#include "AsyncPropLoader.hpp"

#include <cassert>
#include <thread>
#include <sstream>
#include <iomanip>

#include "APLoaderInternal.hpp"
#include "HSerialExceptions.hpp"
#include "SimpleErrors.hpp"
#include "ThreeBitProtocolEncoder.hpp"

using simple::SteadyClock;
using simple::SteadyTimePoint;
using simple::Milliseconds;
using simple::Microseconds;

using hserial::HSerialController;
using hserial::HSerialPort;


// todo: remove profiler after final testing


namespace APLoader {

    static const std::string EmptyString;
    static const std::vector<uint8_t> EmptyImage;


#pragma mark - AsyncPropLoader

    AsyncPropLoader::AsyncPropLoader(hserial::HSerialPort port) : HSerialController(port) {

        // 87382 is the size of 32 KBytes of encoded zeroes (the worst case).
        a_encodedImage.reserve(87382);
    }

    AsyncPropLoader::AsyncPropLoader(const std::string& deviceName) : AsyncPropLoader(HSerialPort(deviceName)) {}

    AsyncPropLoader::~AsyncPropLoader() {
        cancelAndWait(Milliseconds(0)); // wait indefinitely
        removeFromAccess();
    }

    std::string AsyncPropLoader::getControllerType() const {
        return "AsyncPropLoader";
    }


#pragma mark - Loader Actions

    void AsyncPropLoader::restart() {
        startAction(Action::Restart, EmptyImage);
    }

    void AsyncPropLoader::shutdown() {
        startAction(Action::Shutdown, EmptyImage);
    }

    void AsyncPropLoader::loadRAM(const std::vector<uint8_t>& image) {
        startAction(Action::LoadRAM, image);
    }

    void AsyncPropLoader::programEEPROM(const std::vector<uint8_t>& image, bool runAfterwards) {
        if (runAfterwards) {
            startAction(Action::ProgramEEPROMThenRun, image);
        } else {
            startAction(Action::ProgramEEPROMThenShutdown, image);
        }
    }


#pragma mark - Action Control

    bool AsyncPropLoader::isBusy() const {
        return a_action.load() != Action::None;
    }

    void AsyncPropLoader::cancel() {
        std::lock_guard<std::mutex> lock(a_mutex);
        // Setting isCancelled to true when not busy is meaningless, but not harmful.
        // However, a_mutex must be locked.
        a_isCancelled.store(true);
    }

    void AsyncPropLoader::cancelAndWait(const Milliseconds& timeout) {
        // Cancelling and starting to wait must be performed with a_mutex continuously locked
        //  so that we can be sure that the action we are waiting on is the action we
        //  just cancelled (since a_counter is incremented when a_mutex is locked).
        std::unique_lock<std::mutex> lock(a_mutex);
        if (!isBusy()) return;
        a_isCancelled.store(true);
        waitUntilFinishedInternal(lock, timeout);
    }

    void AsyncPropLoader::waitUntilFinished(const Milliseconds& timeout) {
        // Same comments from cancelAndWait apply here.
        std::unique_lock<std::mutex> lock(a_mutex);
        if (!isBusy()) return;
        waitUntilFinishedInternal(lock, timeout);
    }

    void AsyncPropLoader::waitUntilFinishedInternal(std::unique_lock<std::mutex>& lock, const Milliseconds& timeout) {
        // This helper function expects the provided lock to use a_mutex and to be locked.
        uint32_t originalActionCounter = a_counter;
        auto predicate = [this, originalActionCounter]() {
            // Returns true when the action we are waiting on is finished. This occurs if:
            //  1. isBusy() is true and the a_counter has changed (if another action has
            //     started it means the action we were waiting on has finished).
            //  2. isBusy() is false.
            if (isBusy()) {
                return a_counter != originalActionCounter;
            } else {
                return true;
            }
        };
        if (timeout.count() <= 0) {
            a_finishedCondition.wait(lock, predicate);
        } else {
            bool success = a_finishedCondition.wait_for(lock, timeout, predicate);
            if (!success) {
                throw simple::TimeoutError("Timeout occurred while waiting for the action to finish.");
            }
        }
    }


#pragma mark - Settings

    uint32_t AsyncPropLoader::getBaudrate() {
        return baudrate.load();
    }

    void AsyncPropLoader::setBaudrate(uint32_t _baudrate) {
        if (_baudrate > MaxBaudrate) {
            std::stringstream ss;
            ss << "Baudrate may not exceed " << MaxBaudrate << ".";
            throw std::invalid_argument(ss.str());
        }
        baudrate.store(_baudrate);
    }

    ResetLine AsyncPropLoader::getResetLine() {
        return resetLine.load();
    }

    void AsyncPropLoader::setResetLine(ResetLine _resetLine) {
        if (!resetLineIsValid(_resetLine)) {
            std::stringstream ss;
            ss << "Invalid reset line value: " << static_cast<int>(_resetLine) << ".";
            throw std::invalid_argument(ss.str());
        }
        resetLine.store(_resetLine);
    }

    ResetCallback AsyncPropLoader::getResetCallback() {
        return resetCallback.load();
    }

    void AsyncPropLoader::setResetCallback(ResetCallback _resetCallback) {
        resetCallback.store(_resetCallback);
    }

    Milliseconds AsyncPropLoader::getResetDuration() {
        return resetDuration.load();
    }

    void AsyncPropLoader::setResetDuration(const Milliseconds& _resetDuration) {
        if (_resetDuration.count() < 1) throw std::invalid_argument("Reset duration may not be less than 1 ms.");
        if (_resetDuration.count() > 100) throw std::invalid_argument("Reset duration may not be greater than 100 ms.");
        resetDuration.store(_resetDuration);
    }

    Milliseconds AsyncPropLoader::getBootWaitDuration() {
        return bootWaitDuration.load();
    }

    void AsyncPropLoader::setBootWaitDuration(const Milliseconds& _bootWaitDuration) {
        if (_bootWaitDuration.count() > 150) throw std::invalid_argument("Boot wait duration may not be greater than 150 ms.");
        else if (_bootWaitDuration.count() < 50) throw std::invalid_argument("Boot wait duration may not be less than 50 ms.");
        bootWaitDuration.store(_bootWaitDuration);
    }

    StatusMonitor* AsyncPropLoader::getStatusMonitor() {
        return statusMonitor.load();
    }

    void AsyncPropLoader::setStatusMonitor(StatusMonitor* _monitor) {
        statusMonitor.store(_monitor);
    }


#pragma mark - [Internal] Action Lifecycle Functions

    void AsyncPropLoader::startAction(Action action, const std::vector<uint8_t>& image) {
        // Called by a public action function (e.g. loadRAM).

        if (!actionIsValid(action)) {
            assert(false);
            std::stringstream ss;
            ss << "Invalid action specified (" << static_cast<int>(action) << ").";
            throw std::logic_error(ss.str());
        }

        std::lock_guard<std::mutex> lock(a_mutex);

        // Do not continue if an action is already in progress.
        if (isBusy()) {
            std::stringstream ss;
            ss << "The loader is busy. " << strForCurrentActivity();
            throw simple::IsBusyError(ss.str());
        }

        // Lock in the settings.
        a_baudrate = baudrate.load();
        a_resetLine = resetLine.load();
        a_resetCallback = resetCallback.load();
        a_resetDuration = resetDuration.load();
        a_bootWaitDuration = bootWaitDuration.load();
        a_statusMonitor = statusMonitor.load();

        a_counter += 1;

        Profiler profiler;
        profiler.start(action, a_baudrate, a_resetDuration, a_bootWaitDuration);

        if (actionRequiresImage(action)) {
            profiler.willStartEncodingImage(image.size());
            a_imageSizeInLongs = verifyAndEncodeImage(image, a_encodedImage); // copies the image data
            profiler.finishedEncodingImage(a_encodedImage.size());
        }

        // The action will proceed -- no exceptions from this point on.
        // Design note: by setting a_action to a non-None value before calling makeActive we
        //  ensure that once the controller is made active it can not be made inactive until
        //  the action finishes (see willMakeInactive).
        a_isCancelled.store(false);
        a_lastCheckpoint.store("launching thread");
        a_action.store(action);

        std::thread thread(&AsyncPropLoader::actionThread, this, action, profiler);
        thread.detach();
    }

    void AsyncPropLoader::actionThread(Action action, Profiler profiler) {
        try {
            actionWillBegin(profiler, action);
            a_performAction(profiler, action); // may throw ActionError
            actionWillFinish(profiler, ErrorCode::None, EmptyString);
        } catch (const ActionError& e) {
            actionWillFinish(profiler, e.errorCode, e.what());
        } catch (const std::exception& e) {
            // This shouldn't happen -- these exceptions should be caught earlier and rethrown
            //  as ActionError.
            assert(false);
            std::stringstream ss;
            ss << strForCurrentActivity() << " Error: " << e.what();
            actionWillFinish(profiler, ErrorCode::UnhandledException, ss.str());
        } catch (...) {
            // This shouldn't happen either.
            assert(false);
            std::stringstream ss;
            ss << strForCurrentActivity() << " Non-standard exception.";
            actionWillFinish(profiler, ErrorCode::UnhandledException, ss.str());
        }
    }

    void AsyncPropLoader::actionWillBegin(Profiler& profiler, Action action) {
        // The a_callbackOrderEnforcingMutex blocks this thread until the previous action's
        //  loaderHasFinished callback returns.
        std::lock_guard<std::mutex> lock(a_callbackOrderEnforcingMutex);
        if (a_statusMonitor) {
            a_statusMonitor->loaderWillBegin(*this, action, profiler.summary.totalTime, profiler.getEstimatedTotalTime()); // noexcept
        }
    }

    void AsyncPropLoader::actionWillFinish(Profiler& profiler, APLoader::ErrorCode errorCode, const std::string& errorDetails) {

        if (errorCode == ErrorCode::None) {
            profiler.endOK();
        } else {
            profiler.endWithError(errorCode);
        }

        // After finishAction is called a new action may begin immediately. Therefore we need to
        //  copy variables used for the last callback.
        StatusMonitor* monitor = a_statusMonitor;
        a_summaryCopy = profiler.summary;

        // Locking a_callbackOrderEnforcingMutex prevents loaderWillBegin (for the next action)
        //  from being called until loaderHasFinished returns.
        std::lock_guard<std::mutex> lock(a_callbackOrderEnforcingMutex);

        finishAction();

        if (monitor) {
            monitor->loaderHasFinished(*this, errorCode, errorDetails, a_summaryCopy);
        }
    }

    void AsyncPropLoader::finishAction() {

        std::unique_lock<std::mutex> lock(a_mutex);
        a_lastCheckpoint.store("finished");
        a_action.store(Action::None);
        lock.unlock();
        
        a_finishedCondition.notify_all();
    }


#pragma mark - [Internal] Action Work Functions

    void AsyncPropLoader::a_performAction(Profiler& profiler, Action action) {
        // Called by actionThread.

        // A normal return indicates success. Otherwise ActionError is thrown.

        // Stage 1: Preparation
        a_stage1_preparation(profiler);

        // Stage 2: Reset
        a_callStatusMonitorLoaderUpdate(profiler, Status::Resetting);
        a_stage2a_reset(profiler);
        if (action == Action::Restart) return;
        a_stage2b_waitAfterReset(profiler);

        // Stage 3: Establish Communications
        a_callStatusMonitorLoaderUpdate(profiler, Status::EstablishingCommunications);
        a_stage3_establishComms(profiler);

        // Stage 4: Send Command and Image
        a_callStatusMonitorLoaderUpdate(profiler, Status::SendingCommandAndImage);
        a_stage4a_sendCommand(profiler);
        if (action == Action::Shutdown) return;
        a_stage4b_sendImage(profiler);

        // Stage 5: Wait for Checksum Status
        a_callStatusMonitorLoaderUpdate(profiler, Status::WaitingForChecksumStatus);
        a_stage5_waitForChecksumStatus(profiler);
        if (action == Action::LoadRAM) return;

        // Stage 6: Wait for EEPROM Programming Status
        a_callStatusMonitorLoaderUpdate(profiler, Status::WaitingForEEPROMProgrammingStatus);
        a_stage6_waitForEEPROMProgrammingStatus(profiler);

        // Stage 7: Wait for EEPROM Verification Status
        a_callStatusMonitorLoaderUpdate(profiler, Status::WaitingForEEPROMVerificationStatus);
        a_stage7_waitForEEPROMVerificationStatus(profiler);
    }

    void AsyncPropLoader::a_stage1_preparation(Profiler& profiler) {

        a_checkPoint("obtaining serial port access");

        // The call to makeActive is guaranteed to make the controller active or to throw. If
        //  the controller is made active it will stay active until the action is finished (see
        //  willMakeInactive).
        try {
            makeActive();
        } catch (const std::exception& e) {
            if (!isActive()) {
                throw ActionError(ErrorCode::FailedToObtainPortAccess, e.what());
            } else {
                // Since the controller is active keep going.
            }
        }

        a_checkPoint("opening port");

        try {
            ensureOpen();
        } catch (const std::exception& e) {
            throw ActionError(ErrorCode::FailedToOpenPort, e.what());
        }

        a_checkPoint("flushing output buffer");

        try {
            // Using flush since flushOutput not available on Windows as of February 2017.
            flush();
        } catch (const std::exception& e) {
            throw ActionError(ErrorCode::FailedToFlushOutput, e.what());
        }

        a_checkPoint("updating port settings");

        a_updatePortSettings();

        profiler.endStage1();
    }

    void AsyncPropLoader::a_stage2a_reset(Profiler& profiler) {

        a_checkPoint("resetting the Propeller");

        a_doReset();

        profiler.endStage2a();
    }

    void AsyncPropLoader::a_stage2b_waitAfterReset(Profiler& profiler) {

        a_checkPoint("waiting for Propeller to boot up");

        // Since the maximum reasonable boot wait duration is somewhere around 150 ms we
        //  won't bother breaking this sleep down into smaller sleeps for cancellation checks.
        std::this_thread::sleep_for(a_bootWaitDuration);

        a_checkPoint("flushing input buffer");

        // Flush input buffer after reset and presence wait.
        try {
            // Using flush since flushInput not available on Windows as of February 2017.
            flush();
        } catch (const std::exception& e) {
            throw ActionError(ErrorCode::FailedToFlushInput, e.what());
        }

        profiler.endStage2b();
    }

    void AsyncPropLoader::a_stage3_establishComms(Profiler& profiler) {

        a_checkPoint("sending initial bytes");

        // Includes calibration, host auth, and 258 transmission prompts for prop auth and chip version.
        SteadyTimePoint initTimeoutTime = a_sendBytes(InitBytes, ErrorCode::FailedToSendInitialBytes);

        a_checkPoint("authenticating Propeller chip");

        // The prop auth bytes and version should be available immediately after the drain time
        //  for InitBytes, plus some margin.
        initTimeoutTime += InitBytesTimeout;

        // Receive prop auth bytes.
        a_receiveBytes(a_buffer, PropAuthBytes.size(), initTimeoutTime, ErrorCode::FailedToReceivePropAuthentication);

        // Verify prop auth bytes.
        if (PropAuthBytes != a_buffer) {
            throw ActionError(ErrorCode::FailedToAuthenticateProp, "unexpected bytes received from propeller");
        }

        a_checkPoint("verifying Propeller chip version");

        // Receive chip version.
        a_receiveBytes(a_buffer, 4, initTimeoutTime, ErrorCode::FailedToReceiveChipVersion);

        // Decode chip version.
        uint8_t version;
        try {
            std::vector<uint8_t>::iterator iter = a_buffer.begin();
            version = decode3BPByte(iter, a_buffer.end());
        } catch (const std::exception& e) {
            throw ActionError(ErrorCode::FailedToDecodeChipVersion, e.what());
        }

        // Verify chip version.
        if (version != 1) {
            std::stringstream ss;
            ss << "Unrecognized chip version: " << static_cast<int>(version) << ".";
            throw ActionError(ErrorCode::UnsupportedChipVersion, ss.str());
        }

        profiler.endStage3();
    }

    void AsyncPropLoader::a_stage4a_sendCommand(Profiler& profiler) {

        a_checkPoint("sending command");

        // Pick the pre-encoded command.
        const std::vector<uint8_t>* encodedCommand;
        Action action = a_action.load();
        switch (action) {
            case Action::Shutdown:
                encodedCommand = &EncodedShutdown;
                break;
            case Action::LoadRAM:
                encodedCommand = &EncodedLoadRAM;
                break;
            case Action::ProgramEEPROMThenShutdown:
                encodedCommand = &EncodedProgramEEPROMThenShutdown;
                break;
            case Action::ProgramEEPROMThenRun:
                encodedCommand = &EncodedProgramEEPROMThenRun;
                break;
            default:
                // Program logic should prevent such commands from reaching this point.
                assert(false);
                std::stringstream ss;
                ss << "The action " << strForAction(action) << " is invalid at this stage.";
                throw ActionError(ErrorCode::FailedToSendCommand, ss.str());
        }

        // Send the encoded command -- sending for stage 4 starts with this call, so the
        //  drain time will be set here and adjusted as additional bytes are sent.
        a_stage4DrainTime = a_sendBytes(*encodedCommand, ErrorCode::FailedToSendCommand);

        profiler.endStage4a();
    }

    void AsyncPropLoader::a_stage4b_sendImage(Profiler& profiler) {

        a_checkPoint("sending image size");

        // Encode image size.
        try {
            ThreeBitProtocolEncoder encoder(a_buffer);
            encoder.encodeLong(static_cast<uint32_t>(a_imageSizeInLongs));
        } catch (const std::exception& e) {
            throw ActionError(ErrorCode::FailedToEncodeImageSize, e.what());
        }

        // Send the encoded image size.
        a_sendBytes(a_buffer, ErrorCode::FailedToSendImageSize);

        a_checkPoint("sending image");

        a_sendBytes(a_encodedImage, ErrorCode::FailedToSendImage);

        // a_stage4DrainTime was originally set for sending the encoded command at the start of
        //  this stage. To get the correct drain time we need to add the transmission times for
        //  the encoded image size (in a_buffer) and the encoded image (in a_encodedImage).
        a_stage4DrainTime += a_transitDuration(a_buffer.size() + a_encodedImage.size());

        // Wait until most of the image has been sent. This avoids buffering an excessive number of
        //  checksum status transmission prompts.
        a_waitUntil(a_stage4DrainTime - EarlyStage4Return);

        profiler.endStage4b();
    }

    void AsyncPropLoader::a_stage5_waitForChecksumStatus(Profiler& profiler) {

        a_checkPoint("waiting for checksum status");

        bool status = a_receiveStatus(ChecksumStatusTimeout, ErrorCode::FailedToReceiveChecksumStatus);

        a_checkPoint("checking checksum status");

        // true means failure
        if (status) {
            throw ActionError(ErrorCode::PropReportsChecksumError, "Data may have been corrupted in transmission.");
        }

        profiler.endStage5();
    }

    void AsyncPropLoader::a_stage6_waitForEEPROMProgrammingStatus(Profiler& profiler) {

        a_checkPoint("waiting for EEPROM programming status");

        bool status = a_receiveStatus(EEPROMProgrammingStatusTimeout, ErrorCode::FailedToReceiveEEPROMProgrammingStatus);

        a_checkPoint("checking EEPROM programming status");

        // true means failure
        if (status) {
            throw ActionError(ErrorCode::PropReportsEEPROMProgrammingError, "EEPROM may be absent or incorrectly connected.");
        }

        profiler.endStage6();
    }

    void AsyncPropLoader::a_stage7_waitForEEPROMVerificationStatus(Profiler& profiler) {

        a_checkPoint("waiting for EEPROM verification status");

        bool status = a_receiveStatus(EEPROMVerificationStatusTimeout, ErrorCode::FailedToReceiveEEPROMVerificationStatus);
        
        a_checkPoint("checking EEPROM verification status");
        
        // true means failure
        if (status) {
            throw ActionError(ErrorCode::PropReportsEEPROMVerificationError, "EEPROM may be read-only or malfunctioning.");
        }
        
        a_checkPoint("finishing up");
        
        profiler.endStage7();
    }


#pragma mark - [Internal] Action Thread Helper Functions

    SteadyTimePoint AsyncPropLoader::a_sendBytes(const std::vector<uint8_t>& bytes, ErrorCode potentialError) {

        size_t totalToSend = bytes.size();
        const uint8_t* data = bytes.data();

        if (totalToSend == 0) {
            throw ActionError(potentialError, "BUG: bytes in a_sendBytes should not be empty.");
        }

        Microseconds transitDuration = a_transitDuration(totalToSend);

        SteadyTimePoint now = SteadyClock::now();
        SteadyTimePoint drainTime = now + transitDuration; // assumes immediate start and uninterrupted transmission
        SteadyTimePoint responsivenessTimeoutTime = now + a_responsivenessTimeout(transitDuration);

        size_t numSent = 0;

        while (true) {

            a_throwIfCancelled();

            try {
                numSent += write(&data[numSent], totalToSend - numSent);
            } catch (const std::exception& e) {
                std::stringstream ss;
                ss << "Writing to the port failed. Error: " << e.what();
                throw ActionError(potentialError, ss.str());
            }

            if (numSent >= totalToSend) break;

            if (responsivenessTimeoutTime < SteadyClock::now()) {
                throw ActionError(potentialError, "The port was unresponsive.");
            }
        }
        
        return drainTime;
    }

    void AsyncPropLoader::a_receiveBytes(std::vector<uint8_t>& buffer, size_t totalToReceive, const SteadyTimePoint& timeoutTime, ErrorCode potentialError) {

        if (totalToReceive == 0) {
            throw ActionError(potentialError, "BUG: totalToReceive in a_receiveBytes should not be zero.");
        }

        buffer.resize(totalToReceive);
        uint8_t* data = buffer.data();

        size_t numReceived = 0;

        while (true) {

            a_throwIfCancelled();

            try {
                numReceived += read(&data[numReceived], totalToReceive - numReceived);
            } catch (const std::exception& e) {
                std::stringstream ss;
                ss << "Reading from the port failed. Error: " << e.what();
                throw ActionError(potentialError, ss.str());
            }

            if (numReceived >= totalToReceive) break;

            // Check for timeout.
            // This check does not occur more frequently than the timeout on the read call,
            //  which is set to CancellationCheckInterval. So we may end up going past timeoutTime
            //  by that amount (default is 100 ms). This is not a problem for AsyncPropLoader.
            // Also, we defer the first timeout check until after the first read. Again, this
            //  shouldn't be a problem for AsyncPropLoader.
            if (timeoutTime < SteadyClock::now()) {
                throw ActionError(potentialError, "Timeout occured.");
            }
        }
    }

    bool AsyncPropLoader::a_receiveStatus(const simple::Milliseconds& timeout, APLoader::ErrorCode potentialError) {

        // todo: Consider implementing the following error cases:
        //  - numAvailable > 1 on a not final stage, and
        //  - an impossibly early status code for the EEPROM stages. It would still be
        //    desirable to continuously send prompts for the benefit of indicator leds and
        //    logic analyzers. Receiving a status code too early is probably a sign that the
        //    prop has rebooted. It might even send a success code with the first byte.

        SteadyTimePoint timeoutTime = SteadyClock::now() + timeout;

        while (true) {

            a_throwIfCancelled();

            // Send the status prompt.
            try {
                const uint8_t prompt = 0x29;
                write(&prompt, 1);
            } catch (const std::exception& e) {
                std::stringstream ss;
                ss << "Writing to the port failed. Error: " << e.what();
                throw ActionError(potentialError, ss.str());
            }

            std::this_thread::sleep_for(StatusPromptInterval);

            // Check for status.
            size_t numAvailable;
            try {
                numAvailable = available();
            } catch (const std::exception& e) {
                std::stringstream ss;
                ss << "Getting available bytes failed. Error: " << e.what();
                throw ActionError(potentialError, ss.str());
            }

            // Read the status.
            if (numAvailable > 0) {

                uint8_t buffer;
                size_t numReceived;

                try {
                    numReceived = read(&buffer, 1);
                } catch (const std::exception& e) {
                    std::stringstream ss;
                    ss << "Reading from the port failed. Error: " << e.what();
                    throw ActionError(potentialError, ss.str());
                }

                if (numReceived == 1) {
                    if (buffer == 0xff) {
                        // Status code is 1 (failure).
                        return true;
                    } else if (buffer == 0xfe) {
                        // Status code is 0 (success).
                        return false;
                    } else {
                        // Unexpected byte.
                        std::stringstream ss;
                        ss << std::setw(2) << std::uppercase << std::hex;
                        ss << "Received unexpected byte: 0x" << static_cast<int>(buffer) << ".";
                        throw ActionError(potentialError, ss.str());
                    }
                } else {
                    // Two reasons not to allow another loop:
                    // - this situation is not expected, and probably indicates an error (but I am
                    //   not certain of this), and
                    // - the read call has presumably timed out, which at the default setting of
                    //   100 ms means the propeller might have rebooted already.
                    throw ActionError(potentialError, "Port reported bytes available but returned none.");
                }
            }

            // Check for timeout.
            // This occurs at StatusPromptInterval.
            // todo: is this too much? certainly it is not necessary
            if (timeoutTime < SteadyClock::now()) {
                throw ActionError(potentialError, "Timeout occured.");
            }
        }
    }

    void AsyncPropLoader::a_callStatusMonitorLoaderUpdate(Profiler& profiler, Status status) {
        if (a_statusMonitor) {
            a_statusMonitor->loaderUpdate(*this, status, profiler.summary.totalTime, profiler.getEstimatedTotalTime()); // noexcept
        }
    }

    void AsyncPropLoader::a_updatePortSettings() {

        try {
            HSerialController::setBaudrate(a_baudrate, true);
        } catch (const std::exception& e) {
            throw ActionError(ErrorCode::FailedToSetBaudrate, e.what());
        }

        try {
            HSerialController::setTimeout(SerialTimeout, true);
        } catch (const std::exception& e) {
            throw ActionError(ErrorCode::FailedToSetTimeout, e.what());
        }

        try {
            HSerialController::setBytesize(serial::eightbits, true);
        } catch (const std::exception& e) {
            throw ActionError(ErrorCode::FailedToSetBytesize, e.what());
        }

        try {
            HSerialController::setParity(serial::parity_none, true);
        } catch (const std::exception& e) {
            throw ActionError(ErrorCode::FailedToSetParity, e.what());
        }

        try {
            HSerialController::setStopbits(serial::stopbits_one, true);
        } catch (const std::exception& e) {
            throw ActionError(ErrorCode::FailedToSetStopbits, e.what());
        }

        try {
            HSerialController::setFlowcontrol(serial::flowcontrol_none, true);
        } catch (const std::exception& e) {
            throw ActionError(ErrorCode::FailedToSetFlowcontrol, e.what());
        }
    }

    void AsyncPropLoader::a_doReset() {
        if (a_resetLine == ResetLine::DTR) {
            setDTR(true);
            std::this_thread::sleep_for(a_resetDuration);
            setDTR(false);
        } else if (a_resetLine == ResetLine::RTS) {
            setRTS(true);
            std::this_thread::sleep_for(a_resetDuration);
            setRTS(false);
        } else if (a_resetLine == ResetLine::Callback) {
            if (!a_resetCallback) {
                throw ActionError(ErrorCode::FailedToReset, "Reset callback option selected, but no callback provided.");
            }
            try {
                a_resetCallback(a_resetDuration);
            } catch (const std::exception& e) {
                throw ActionError(ErrorCode::FailedToReset, e.what());
            } catch (...) {
                throw ActionError(ErrorCode::FailedToReset, "Reset callback failed with non-standard error.");
            }
        } else {
            std::stringstream ss;
            ss << "Invalid reset line specified (" << static_cast<int>(a_resetLine) << ").";
            throw ActionError(ErrorCode::FailedToReset, ss.str());
        }
    }

    void AsyncPropLoader::a_throwIfCancelled() {
        if (a_isCancelled.load()) {
            throw ActionError(ErrorCode::Cancelled, strForCurrentActivity());
        }
    }

    void AsyncPropLoader::a_checkPoint(const char* description) {
        a_throwIfCancelled();
        a_lastCheckpoint.store(description);
    }

    void AsyncPropLoader::a_waitUntil(const SteadyTimePoint& waitTime) {

        SteadyTimePoint now = SteadyClock::now();
        auto timeRemaining = waitTime - now;

        while (timeRemaining.count() > 0) {

            a_throwIfCancelled();

            if (timeRemaining < CancellationCheckInterval) {
                std::this_thread::sleep_for(timeRemaining);
                a_throwIfCancelled();
                return;
            } else {
                std::this_thread::sleep_for(CancellationCheckInterval);
            }

            now = SteadyClock::now();
            timeRemaining = waitTime - now;
        }
    }

    Microseconds AsyncPropLoader::a_transitDuration(size_t numBytes) {
        long long n = numBytes * 10000000.0f / a_baudrate;
        if (n < 1) n = 1;
        return Microseconds(n);
    }

    Milliseconds AsyncPropLoader::a_responsivenessTimeout(Microseconds transitDuration) {
        return Milliseconds(std::max(static_cast<long long>(ResponsivenessMultiplier * transitDuration.count() / 1000.0f),
                                     MinResponsivenessTimeout.count()));
    }


#pragma mark - [Internal] Miscellaneous Functions

    std::string AsyncPropLoader::strForCurrentActivity() {
        // See documentation about having action and checkpoint being consistent.
        Action action = a_action.load();
        const char* checkpoint = a_lastCheckpoint.load();
        if (action == Action::None) {
            return "Loader is idle.";
        } else {
            std::stringstream ss;
            ss << "Action: " << strForAction(action) << ". Last checkpoint: "
            << (checkpoint ? checkpoint : "unknown") << ".";
            return ss.str();
        }
    }


#pragma mark - HSerialController Transition Callback

    void AsyncPropLoader::willMakeInactive() {

        std::lock_guard<std::mutex> lock(a_mutex);

        if (isBusy()) {
            std::stringstream ss;
            ss << "The loader is busy. " << strForCurrentActivity();
            throw hserial::ControllerRefuses(*this, ss.str());
        }

        // Use the default implementation to fulfill obligations.
        HSerialController::willMakeInactive();

        // In some controllers it may be necessary to keep a mutex locked over the transition.
        //  (This would involve implementing the didCancelMakeInactive and didMakeInactive callbacks
        //  to unlock the mutex.) Locking is not necessary for this controller. If an action starts
        //  after this point it won't do anything with the serial port before its makeActive call
        //  (in a_stage1_preparation), at which point it either succeeds in making the controller
        //  active again, or it fails, throwing an exception that aborts the action.
    }


} // namespace APLoader


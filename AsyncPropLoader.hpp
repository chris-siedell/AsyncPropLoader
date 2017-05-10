//
//  AsyncPropLoader.hpp
//  libserial loader 001
//
//  Created by admin on 1/31/17.
//  Copyright © 2017 Chris Siedell. All rights reserved.
//

#ifndef AsyncPropLoader_hpp
#define AsyncPropLoader_hpp


#include "HSerialController.hpp"
#include "APLoaderDefs.hpp"
#include "SimpleChrono.hpp"


// todo: add internal_docs tags for User vs Complete documentation

namespace APLoader {


#pragma mark - AsyncPropLoader

    /*!
     \brief A serial port controller used to program and control a Parallax Propeller P8X32A
     microcontroller.
     */
    class AsyncPropLoader : public hserial::HSerialController {

    public:

        /*!
         \brief Creates a loader using the given serial port.
         \see AsyncPropLoader(const std::string& deviceName)
         */
        AsyncPropLoader(hserial::HSerialPort port);

        /*!
         \brief Creates a loader using the given serial port, identified by its device name.
         \see AsyncPropLoader(HSerialPort port)
         */
        AsyncPropLoader(const std::string& deviceName);

        virtual ~AsyncPropLoader();

        AsyncPropLoader() = delete;
        AsyncPropLoader(const AsyncPropLoader&) = delete;
        AsyncPropLoader& operator=(const AsyncPropLoader&) = delete;
        AsyncPropLoader(AsyncPropLoader&&) = delete;
        AsyncPropLoader& operator=(AsyncPropLoader&&) = delete;

        /*!
         \brief Returns `"AsyncPropLoader"`.
         */
        virtual std::string getControllerType() const override;


#pragma mark - Loader Actions

        /*!
         \name Loader Actions
         */
        /// \{

        /*!
         \brief Restarts the Propeller.
         
         This action just toggles the reset control line and finishes. The Propeller still needs
         some time to go through its boot process before it will start running the
         code on the EEPROM.
         
         The action is performed asynchronously.
         Use an APLoader::StatusMonitor object to follow the progress of the action.
         It may be cancelled with cancel() or cancelAndWait().

         \throws simple::IsBusyError Thrown if there is an action already in progress.
         \see APLoader::StatusMonitor, shutdown, loadRAM, programEEPROM
         */
        void restart();

        /*!
         \brief Shuts down the Propeller.
         
         This action resets the Propeller and then issues a command for it to enter its shutdown
         mode.
         
         The action is performed asynchronously.
         Use an APLoader::StatusMonitor object to follow the progress of the action.
         It may be cancelled with cancel() or cancelAndWait().

         \throws simple::IsBusyError Thrown if there is an action already in progress.
         \see APLoader::StatusMonitor, restart, loadRAM, programEEPROM
         */
        void shutdown();

        /*!
         \brief Loads the given image into RAM and runs it.
         
         The image data is copied before returning.

         The action is performed asynchronously.
         Use an APLoader::StatusMonitor object to follow the progress of the action.
         It may be cancelled with cancel() or cancelAndWait().

         \throws std::invalid_argument Thrown if the image is empty, its size exceeds 32768, or
         it has an incorrect checksum.
         \throws simple::IsBusyError Thrown if there is an action already in progress.
         \see APLoader::StatusMonitor, restart, shutdown, programEEPROM
         */
        void loadRAM(const std::vector<uint8_t>& image);

        /*!
         \brief Programs the EEPROM with the given image.
         
         The runAfterwards flag indicates whether to run the image or to
         shutdown after programming the EEPROM.
         
         The image data is copied before returning.

         The action is performed asynchronously.
         Use an APLoader::StatusMonitor object to follow the progress of the action.
         It may be cancelled with cancel() or cancelAndWait().

         \throws std::invalid_argument Thrown if the image is empty, its size exceeds 32768, or
         it has an incorrect checksum.
         \throws simple::IsBusyError Thrown if there is an action already in progress.
         \see APLoader::StatusMonitor, restart, shutdown, loadRAM
         */
        void programEEPROM(const std::vector<uint8_t>& image, bool runAfterwards = true);

        /// \} /Loader Actions


#pragma mark - Action Control

        /*!
         \name Action Control
         */
        /// \{

        /*!
         \brief Indicates if an action is in progress.
         */
        bool isBusy() const;

        /*!
         \brief Blocks until the current action finishes or timeout occurs.
         
         It will return immediately if no action is being performed.

         A timeout value of 0 disables the timeout (the function will wait indefinitely).

         \throws simple::TimeoutError Thrown if timeout occurs.
         \see cancel, cancelAndWait
         */
        void waitUntilFinished(const simple::Milliseconds& timeout = simple::Milliseconds(0));

        /*!
         \brief Cancels the action and returns without waiting for the cancellation to go into effect.
         
         Does nothing if there is no action in progress.

         \see cancelAndWait, waitUntilFinished
         */
        void cancel();

        /*!
         \brief Cancels the action and waits for it to go into effect, or until timeout occurs.
         
         It will return immediately if no action is being performed.
         
         A timeout value of 0 disables the timeout (the function will wait indefinitely).
         
         \throws simple::TimeoutError Thrown if timeout occurs.
         \see cancel, waitUntilFinished
         */
        void cancelAndWait(const simple::Milliseconds& timeout = simple::Milliseconds(0));

        /// \} /Action Control


#pragma mark - Settings

        /*!
         \name Settings
         */
        /// \{

        /*!
         \brief Gets the baudrate.
         \see setBaudrate
         */
        uint32_t getBaudrate();

        /*!
         \brief Sets the baudrate.
         
         Since the booter communicates using the 3-Bit-Protocol (3BP) the actual throughput
         is lower than would be expected.
         
         The default is 115200 bps. This is also the maximum that can be safely supported
         by the Propeller's booter program.

         \throws std::invalid_argument Thrown if the baudrate exceeds the maximum allowed rate.
         \see MaxBaudrate, getBaudrate
         */
        void setBaudrate(uint32_t baudrate);

        /*!
         \brief Gets the control line used to reset the Propeller.
         \see APLoader::ResetLine, setResetLine
         */
        APLoader::ResetLine getResetLine();

        /*!
         \brief The control line used to reset the Propeller.
         
         The default is APLoader::ResetLine::DTR.
         
         \see APLoader::ResetLine, getResetLine
         */
        void setResetLine(APLoader::ResetLine resetLine);

        /*!
         \brief Gets the reset callback.
         \see APLoader::ResetCallback, setResetCallback
         */
        APLoader::ResetCallback getResetCallback();

        /*!
         \brief Sets the reset callback.
         
         This is the function that the loader will call to reset the Propeller when 
         ResetLine::Callback is chosen as the reset line. This allows user
         code to manually perform the reset when the Propeller's reset line is connected to
         something other than the serial port's RTS or DTR control lines.

         The default is NULL. It must not be NULL if ResetLine::Callback is selected.
         
         See the ResetCallback definition for the the callback's requirements.
         
         \see APLoader::ResetCallback, getResetCallback
         */
        void setResetCallback(APLoader::ResetCallback resetCallback);

        /*!
         \brief Gets the reset duration.
         \see setResetDuration
         */
        simple::Milliseconds getResetDuration();

        /*!
         \brief Sets the reset duration.
         
         The reset duration is the approximate length of time that the loader holds the reset
         line low to initiate a reset.
         
         The default is 10 milliseconds.

         \throws std::invalid_argument Thrown if the duration is outside of reasonable limits.
         \see getResetDuration
         */
        void setResetDuration(const simple::Milliseconds& resetDuration);

        /*!
         \brief Gets the boot wait duration.
         \see setBootWaitDuration
         */
        simple::Milliseconds getBootWaitDuration();

        /*!
         \brief Sets the boot wait duration.
         
         The boot wait duration is the approximate length in time that the loader waits
         between raising the reset line and initiating communications. In this interval
         the Propeller is restarting and beginning its booter program.
         
         The default is 100 milliseconds.

         \throws std::invalid_argument Thrown if the duration is outside of reasonable limits.
         \see getBootWaitDuration
         */
        void setBootWaitDuration(const simple::Milliseconds& bootWaitDuration);

        /*!
         \brief Gets the status monitor.
         \see setStatusMonitor
         */
        APLoader::StatusMonitor* getStatusMonitor();

        /*!
         \brief Sets the status monitor.

         A status monitor object is used to follow the progress of the loader using
         callbacks.

         The default is NULL.

         \see APLoader::StatusMonitor, getStatusMonitor
         */
        void setStatusMonitor(APLoader::StatusMonitor* monitor);

        /// \} /Settings


#pragma mark - Constants

        /*!
         \brief The maximum baudrate the loader will operate at.

         Analysis of the Propeller's booter program determined that 115200 bps is the fastest
         commonly supported baudrate that can be used reliably over the entire RCFAST frequency
         range, given a large allowance for jitter (±10%).

         Even though it might work -- or appear to work -- exceeding 115200 bps is unwise because
         the booter program uses a relatively weak error detection mechanism (a one byte checksum
         for a 32 Kbyte image). If faster loading is desired then a bootstrapping loader should
         be used.
        
         See the comments for ThreeBitProtocolEncoder::MaxBaudrate for more details. 
         
         Note that this limit must not exceed the assumed limit used to prepare
         APLoader::InitBytes.
         
         \see ThreeBitProtocolEncoder::MaxBaudrate, APLoader::InitBytes
         */
        static const uint32_t MaxBaudrate = 115200;

        /// \} /Constants


    protected:


#pragma mark - [Internal] Constants

        /*!
         \name [Internal] Constants
         */
        /// \{

        /*!
         \brief Determines responsiveness to cancellation.

         CancellationCheckInterval specifies approximately how often the loader should check to see
         if the action has been cancelled. For efficiency this shouldn't be too low, but for
         responsiveness it shouldn't be too high
         */
        const simple::Milliseconds CancellationCheckInterval {100};

        /*!
         \brief Timeout for getting the Propeller authentication and version bytes.
         
         The Propeller sends its authentication and version bytes simultaneously with the
         transmission prompts, so as soon as InitBytes is sent (drained) that data should
         be available. However, some margin should be allowed for the hardware and drivers to make
         the bytes available.
         
         \see APLoader::InitBytes
         */
        const simple::Milliseconds InitBytesTimeout {1000};

        /*!
         \brief A constant that helps determine when stage 4 (sending the command and image) ends.

         The a_sendBytes function returns as soon as all of its bytes have been buffered. This may
         be much earlier than the bytes actually being sent over the wire. If we're sending a large
         image and didn't wait after a_sendBytes the checksum status stage would start too early.
         This could result in a significant number of checksum status transmission prompts
         being buffered for transmission before the Propeller has even received the image.

         a_sendBytes returns an estimated drain time based on the assumption that transmission
         begins immediately and continues without interruption. The loader will wait until the drain
         time for sending the command and image -- minus this constant -- before ending this stage.
         This is insurance against the drain time being over-estimated.

         Sending timely status prompts is critical. It will take the Propeller approximately
         50 to 130 ms after receiving the last bit of the image to calculate the checksum. After
         this it will wait about 100 ms for a prompt before aborting the serial loading process
         and then attempting to boot from EEPROM.

         \see ChecksumStatusTimeout
         */
        const simple::Milliseconds EarlyStage4Return {100};

        /*!
         \brief The interval between sending transmission prompts to the Propeller when waiting for
         a status code.

         StatusPromptInterval specifies approximately how long to wait between sending status
         transmission prompts to the Propeller. The Propeller needs a transmission prompt
         to send the status code after doing the checksum comparison, EEPROM programming, and EEPROM
         verification steps.

         The Propeller must receive a prompt within about 100 ms after being ready to send a status
         code, otherwise it will abort the serial loading process and attempt to boot from EEPROM.
         Therefore this interval must not be too high. Keep in mind there is some overhead and
         unpredictability in the sleeping and serial functions.

         10-20 milliseconds seems to be reasonable.
         */
        const simple::Milliseconds StatusPromptInterval {10};

        /*!
         \brief Timeout for receiving a checksum status code.

         I observed 84 milliseconds between the last 3BP encoded image bit to the checksum status
         on a Propeller running at 13 MHz. This implies a minimum safe timeout of 140 milliseconds
         at 8 MHz.

         Keep in mind that the loader can only guess when the last image bit was sent (using the
         estimated drain time after sending the encoded image), so the checksum status timeout
         should have some extra time added to it. This extra time should take into account
         EarlyStage4Return.
         
         \see EarlyStage4Return
         */
        const simple::Milliseconds ChecksumStatusTimeout {1500};

        /*!
         \brief Timeout for receiving an EEPROM programming status code.

         I observed 3.4 seconds from the checksum status to the EEPROM programming status on a
         Propeller running at 13 MHz. This implies a minimum safe timeout of 5.6 seconds at 8 MHz.
         */
        const simple::Milliseconds EEPROMProgrammingStatusTimeout {6000};

        /*!
         \brief Timeout for receiving an EEPROM verification status code.

         I observed 1.2 seconds between the programming status to the verification status on a
         Propeller running at 13 MHz. This implies a minimum safe timeout of 2.0 seconds at 8 MHz.
         */
        const simple::Milliseconds EEPROMVerificationStatusTimeout {2500};

        /*!
         \brief Helps determine the responsiveness timeout used for sending bytes.

         If write calls to the serial port aren't keeping pace with the baudrate then
        something is wrong -- the port is unresponsive.

         \see a_responsivenessTimeout
         */
        const float ResponsivenessMultiplier = 1.5f;

        /*! \copydoc ResponsivenessMultiplier */
        const simple::Milliseconds MinResponsivenessTimeout {1000};

        /*!
         \brief The serial::Timeout struct used with the serial port.

         Using the simpleTimeout template means that the interbyte timeout is disabled, and that
         the timeout for read and write calls is CancellationCheckInterval milliseconds.

         Note: unable to use const since serial's setTimeout takes a non-const reference.
         
         \see CancellationCheckInterval
         */
        serial::Timeout SerialTimeout = serial::Timeout::simpleTimeout(static_cast<uint32_t>(CancellationCheckInterval.count()));
        
        /// \} /[Internal] Constants


#pragma mark - [Internal] Profiler

        class Profiler; // implemented in APLoaderInternal.hpp/cpp


#pragma mark - [Internal] Action Lifecycle Functions

        /*!
         \name [Internal] Action Lifecycle Functions
         
         These are upper level functions used during an action's lifecycle.
         */
        /// \{

        /*!
         \brief The helper function called by the action initiating functions (e.g. loadRAM).
         
         This function does some preparation and creates the worker thread.
         
         \see actionThread
         */
        void startAction(APLoader::Action action, const std::vector<uint8_t>& image);

        /*!
         \brief The entry function for the thread created to perform the action.
         */
        void actionThread(APLoader::Action action, Profiler profiler);

        /*!
         \brief Called from actionThread just before performing the action.

         Notifies the status monitor that the action will begin.
        */
        void actionWillBegin(Profiler& profiler, APLoader::Action action);

        /*!
         \brief Called from actionThread when the action should be finished.

         Calls finishAction and notifies the status monitor.
         */
        void actionWillFinish(Profiler& profiler, ErrorCode errorCode, const std::string& errorDetails);

        /*!
         \brief Officially finishes the action. Called from actionWillFinish.
         
         Sets a_action to None and notifies waiting threads.
         */
        void finishAction();

        /// \} /[Internal] Action Lifecycle Functions


#pragma mark - [Internal] Action Work Functions

        /*!
         \name [Internal] Action Work Functions

         These functions do the work of the action. They begin with "a_", which indicates that
         they are expected to be used on the action thread and to throw only ActionError.
         */
        /// \{

        /*!
         \brief Called from actionThread. This is the main function for performing the action.

         This function delegates work to the a_stageN_* functions.

         This function also notifies the status monitor with progress updates.

         A normal return indicates success. Otherwise, it throws ActionError.
         */
        void a_performAction(Profiler& profiler, APLoader::Action action);

        void a_stage1_preparation(Profiler& profiler);
        void a_stage2a_reset(Profiler& profiler);
        void a_stage2b_waitAfterReset(Profiler& profiler);
        void a_stage3_establishComms(Profiler& profiler);
        void a_stage4a_sendCommand(Profiler& profiler);
        void a_stage4b_sendImage(Profiler& profiler);
        void a_stage5_waitForChecksumStatus(Profiler& profiler);
        void a_stage6_waitForEEPROMProgrammingStatus(Profiler& profiler);
        void a_stage7_waitForEEPROMVerificationStatus(Profiler& profiler);

        /// \} /[Internal] Action Work Functions


#pragma mark - [Internal] Action Thread Helper Functions

        /*!
         \name Action Thread Helper Functions
         
         These functions begin with "a_", indicating that they should be called from
         a_performAction or subcalls. They throw only ActionError.
         */
        /// \{

        /*!
         \brief Either sends the bytes or throws.
         \see a_responsivenessTimeout
         */
        simple::SteadyTimePoint a_sendBytes(const std::vector<uint8_t>& bytes, APLoader::ErrorCode potentialError);

        /*!
         \brief Either receives the request number of bytes before timeoutTime or throws.
         */
        void a_receiveBytes(std::vector<uint8_t>& buffer, size_t totalToReceive, const simple::SteadyTimePoint& timeoutTime, APLoader::ErrorCode potentialError);

        /*!
         \brief Receives a status code from the Propeller.

         There are three stages where the Propeller reports a status code: verifying the checksum,
         programming the EEPROM, and verifying the programmed image.

         This function sends the required transmission prompts at StatusPromptInterval. The first
         byte received should encode the status.
         
         Important note: the Propeller returns a status code of 0 for success and 1 for failure.
         So the return value is the inversion of a success flag.
         
         \see StatusPromptInterval, ChecksumStatusTimeout, EEPROMProgrammingStatusTimeout, EEPROMVerificationStatusTimeout
         */
        bool a_receiveStatus(const simple::Milliseconds& timeout, APLoader::ErrorCode potentialError);

        /*!
         \brief Calls the status monitor's update callback.
         */
        void a_callStatusMonitorLoaderUpdate(Profiler& profiler, APLoader::Status status);

        /*!
         \brief Applies the loader's settings to the serial port.
         */
        void a_updatePortSettings();

        /*!
         \brief Performs the reset.
         */
        void a_doReset();

        /*!
         \brief Throws if isCancelled is true.
         */
        void a_throwIfCancelled();

        /*!
         \brief Does a cancellation check and registers a checkpoint.
         */
        void a_checkPoint(const char* description);

        /*!
         \brief Waits until the given time, periodically checking for cancellation.
         */
        void a_waitUntil(const simple::SteadyTimePoint& waitTime);

        /*!
         \brief The time taken (NB: in microseconds) to transmit the bytes at the current baudrate.
         */
        simple::Microseconds a_transitDuration(size_t numBytes);

        /*!
         \brief Calculates the responsiveness timeout given a transit duration.
         */
        simple::Milliseconds a_responsivenessTimeout(simple::Microseconds transitDuration);

        /// \} /[Internal] Action Thread Helper Functions


#pragma mark - [Internal] Miscellaneous Functions

        /*!
         \name [Internal] Miscellaneous Functions
         */
        /// \{

        /*!
         \brief Used to implement waitUntilFinished and cancelAndWait.

         The provided lock should use a_mutex and it should already be locked.

         \see waitUntilFinished, cancelAndWait
         */
        void waitUntilFinishedInternal(std::unique_lock<std::mutex>& lock, const simple::Milliseconds& timeout);

        /*!
         \brief Composes a string describing what the loader is currently doing.

         It is safe to call this function from any thread, but to guarantee that the string is
         logically consistent (it's action and last checkpoint are meaningful together) then
         a_mutex should be locked when called, or the call should come from a_performAction
         or subcalls.

         If the loader is idle the string is `"Loader is idle."`. Otherwise the string
         has the form <tt>"Action: <current action>. Last checkpoint: <last checkpoint>."</tt>.
         */
        std::string strForCurrentActivity();

        /// \} /[Internal] Miscellaneous Functions


#pragma mark - [Internal] Setting Variables

        /*!
         \name [Internal] Setting Variables
         
         These are the backing variables for the public getter/setters. They do not affect
         an action in progress.
         */
        /// \{

        std::atomic<uint32_t> baudrate {MaxBaudrate};
        std::atomic<APLoader::ResetLine> resetLine {APLoader::ResetLine::DTR};
        std::atomic<APLoader::ResetCallback> resetCallback {NULL};
        std::atomic<simple::Milliseconds> resetDuration {simple::Milliseconds(10)};
        std::atomic<simple::Milliseconds> bootWaitDuration {simple::Milliseconds(100)};
        std::atomic<APLoader::StatusMonitor*> statusMonitor {NULL};

        /// \} /[Internal] Setting Variables


#pragma mark - [Internal] Action Settings

        /*!
         \name Internal Action Settings
         
         The settings may change at any time, so values are locked-in at the beginning of an
         action (in startAction). Publicly changing a setting during an action will have no effect
         until the next action.
         */
        /// \{

        uint32_t a_baudrate;
        APLoader::ResetLine a_resetLine;
        APLoader::ResetCallback a_resetCallback;
        simple::Milliseconds a_resetDuration;
        simple::Milliseconds a_bootWaitDuration;
        APLoader::StatusMonitor* a_statusMonitor;

        /// \} /[Internal] Action Settings


#pragma mark - [Internal] Loader State

        /*!
         \name [Internal] Loader State
         */
        /// \{

        /*!
         \brief The primary mutex for protecting loader state and coordinating actions.

         This mutex is used with:
         - a_action (w),
         - a_isCancelled (w),
         - a_counter (r+w), and
         - a_finishedCondition.
         
         \see a_action, a_isCancelled, a_counter, a_finishedCondition
         */
        std::mutex a_mutex;

        /*!
         \brief The action being performed;

         This value of this property determines if the loader is busy.
         
         Changes must be performed with a_mutex locked to coordinate actions.
         It may be read without mutex protection.
         
         Used as part of the predicate for a_finishedCondition.

         \see a_mutex, a_finishedCondition, strForCurrentActivity
         */
        std::atomic<APLoader::Action> a_action {APLoader::Action::None};

        /*!
         \brief Flag used to signify that the action has been cancelled.

         This variable is meaningful only if a_action is non-None.

         a_mutex must be locked when setting this flag.
         It may be read without mutex protection.
         
         \see a_mutex
         */
        std::atomic_bool a_isCancelled {false};

        /*!
         \brief Uniquely identifies each action.
         
         This counter is used to identify actions to ensure that waitUntilFinished and
         cancelAndWait behave correctly. Essentially, if an action is cancelled another thread might
         sneak in and start a new action before these functions are aware that the action they
         were waiting on has finished. (As far as I know there's no guarantee that the
         waiting condition variable will be the first to get the mutex). Therefore, we need
         something besides the isBusy() to indicate that the given action has finished.
        
         a_counter should be used only when a_mutex is locked. It is incremented in
         startAction.
         
         Used as part of the predicate for a_finishedCondition.

         \see waitUntilFinishedInternal, a_mutex, a_finishedCondition
         */
        uint32_t a_counter = 0;

        /*!
         \brief Used to notify blocked threads that an action has finished.
         
         This condition variable is used in waitUntilFinishedInternal to receive notification
         that an action has finished.

         The predicate for this condition depends on both a_action and a_counter.
         It uses a_mutex.
         
         \see waitUntilFinishedInternal, a_mutex, a_action, a_counter
         */
        std::condition_variable a_finishedCondition;

        /*!
         \brief Stores the last reported checkpoint during an action.

         Writing to a_lastCheckpoint is not protected by a mutex -- it is acceptable for
         strForCurrentActivity to report a slightly out-of-date last checkpoint.

         \see strForCurrentActivity
         */
        std::atomic<const char*> a_lastCheckpoint {"no action performed yet"};

        /*!
         \brief Prevents the status monitor's callbacks from being called out of order.

         The loaderHasFinished callback is called after the action is finished, when isBusy()
         will return `false`. This means that another action may begin at any time,
         including before the previous action's loaderHasFinished has been called.

         This mutex is used to
         prevent the next action's loaderWillBegin callback from being called until the
         previous action's loaderHasFinished callback has returned. Effectively, the next
         action is blocked until the previous action returns from its callback.
         
         This coordination is required since each action spawns it own thread.
         */
        std::mutex a_callbackOrderEnforcingMutex;

        /// \} /[Internal] Loader State


#pragma mark - [Internal] Action Parameters

        /*!
         \name Action Parameters
         
         These are the parameters associated with an action. They are set in startAction under
         a_mutex protection, and then used in subcalls of a_performAction.
         */
        /// \{

        /*!
         \brief Holds the 3BP encoded image.
         */
        std::vector<uint8_t> a_encodedImage;

        /*!
         \brief The number of longs in the encoded image.

         If the provided image size is not a multiple of four it is padded at the end
         with NUL bytes.
         */
        size_t a_imageSizeInLongs;

        /*!
         \brief The command number that the Propeller associates with the action.
         
         \see APLoader::commandForAction
         */
        uint32_t a_command;


#pragma mark - [Internal] Miscellaneous Action Variables

        /*!
         \name Miscellaneous Action Variables

         These variables are used by the action threaad.
         */

        /*!
         \brief A multipurpose buffer used during the loading process.
         */
        std::vector<uint8_t> a_buffer;

        /*!
         \brief A copy of the summary data for the actionDidFinish callback.
         */
        ActionSummary a_summaryCopy;

        /*!
         \brief Holds the drain time for Stage 4 between a_stage4a* and a_stage4b* calls.
         */
        simple::SteadyTimePoint a_stage4DrainTime;

        /// \} /Miscellaneous Action Variables


#pragma mark - HSerialController Transition Callback

        /*!
         \name HSerialController Transition Callback
        */
        /// \{

        /*!
         \brief Called by %HSerial library when this controller is being made inactive.
         
         This callback refuses inactivaction if there is an action in progress.
         */

        virtual void willMakeInactive() override;

        /// \} /Transition Callbacks

    };


} // namespace APLoader


#endif /* AsyncPropLoader_hpp */


//
//  APLoaderInternal.hpp
//  libserial loader 001
//
//  Created by admin on 2/8/17.
//  Copyright © 2017 Chris Siedell. All rights reserved.
//

#ifndef APLoaderInternal_hpp
#define APLoaderInternal_hpp

#include <chrono>
#include <vector>

#include "AsyncPropLoader.hpp"


namespace APLoader {


#pragma mark - Communications Stuff

    /*!
     \name Communications Stuff
     */
    /// \{

    /*!
     \brief Prepared data for initiating communications with the Propeller boot program.

     InitBytes includes the calibration pulses, the 250 encoded host authentication bits, the
     transmission prompts (0xAD) to receive 250 Propeller authentication bits, and the transmission
     prompts to receive the 8 version bits.

     This prepared data must not be transmitted at baudrates
     faster than 115200 bps.

     \see PropAuthBytes, decode3BPByte, ThreeBitProtocolEncoder::MaxBaudrate
     */
    const std::vector<uint8_t> InitBytes =
    {0xf9,0x4a,0x25,0xd5,0x4a,0xd5,0x92,0x95,0x4a,0x92,0xd5,0x92,0xca,0xca,0x4a,
        0x95,0xca,0xd2,0x92,0xa5,0xa9,0xc9,0x4a,0x49,0x49,0x2a,0x25,0x49,0xa5,0x4a,
        0xaa,0x2a,0xa9,0xca,0xaa,0x55,0x52,0xaa,0xa9,0x29,0x92,0x92,0x29,0x25,0x2a,
        0xaa,0x92,0x92,0x55,0xca,0x4a,0xca,0xca,0x92,0xca,0x92,0x95,0x55,0xa9,0x92,
        0x2a,0xd2,0x52,0x92,0x52,0xca,0xd2,0xca,0x2a,0xff,0xad,0xad,0xad,0xad,0xad,
        0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,
        0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,
        0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,
        0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,
        0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,
        0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,
        0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,
        0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,
        0xad,0xad,0xad,0xad};

    /*!
     \brief Prepared data for authenticating the Propeller chip.

     PropAuthBytes contains the encoded prop authentication bits that should be received in
     response to sending InitBytes. (After receiving these 125 authentication bytes, 4 more bytes
     should be received that encode the 8-bit chip version number.)

     \see InitBytes
     */
    const std::vector<uint8_t> PropAuthBytes =
    {0xee,0xce,0xce,0xcf,0xef,0xcf,0xee,0xef,0xcf,0xcf,0xef,0xef,0xcf,0xce,0xef,
        0xcf,0xee,0xee,0xce,0xee,0xef,0xcf,0xce,0xee,0xce,0xcf,0xee,0xee,0xef,0xcf,
        0xee,0xce,0xee,0xce,0xee,0xcf,0xef,0xee,0xef,0xce,0xee,0xee,0xcf,0xee,0xcf,
        0xee,0xee,0xcf,0xef,0xce,0xcf,0xee,0xef,0xee,0xee,0xee,0xee,0xef,0xee,0xcf,
        0xcf,0xef,0xee,0xce,0xef,0xef,0xef,0xef,0xce,0xef,0xee,0xef,0xcf,0xef,0xcf,
        0xcf,0xce,0xce,0xce,0xcf,0xcf,0xef,0xce,0xee,0xcf,0xee,0xef,0xce,0xce,0xce,
        0xef,0xef,0xcf,0xcf,0xee,0xee,0xee,0xce,0xcf,0xce,0xce,0xcf,0xce,0xee,0xef,
        0xee,0xef,0xef,0xcf,0xef,0xce,0xce,0xef,0xce,0xee,0xce,0xef,0xce,0xce,0xee,
        0xcf,0xcf,0xce,0xcf,0xcf};

    /*!
     \brief The 3BP encoded command to shutdown.
     */
    const std::vector<uint8_t> EncodedShutdown = {0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0xF2};

    /*!
     \brief The 3BP encoded command to load the image into RAM and then run.
     */
    const std::vector<uint8_t> EncodedLoadRAM = {0xC9, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0xF2};

    /*!
     \brief The 3BP encoded command to program the EEPROM and then shutdown.
     */
    const std::vector<uint8_t> EncodedProgramEEPROMThenShutdown = {0xCA, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0xF2};

    /*!
     \brief The 3BP encoded command to program the EEPROM and then run.
     */
    const std::vector<uint8_t> EncodedProgramEEPROMThenRun = {0x25, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0x92, 0xFE};

    /*!
     \brief Decodes a 3-Bit-Protocol encoded byte.

     The byte is assumed to be from the Propeller in response to four 0xAD transmission prompts.

     This function is used for decoding the chip version.

     It takes four bytes from the iterator.

     \throws std::runtime_error Thrown if there are not enough bytes, or if an unexpected byte
     is encountered.
     */
    uint8_t decode3BPByte(std::vector<uint8_t>::iterator& iter, const std::vector<uint8_t>::iterator end);

    /*!
     \brief Verifies that image is valid, and encodes it in 3BP format into encodedImage.

     Returns the number of longs in the encoded image.

     \throws std::invalid_argument Thrown if the image is too small, too big, or has an invalid
     checksum.
     \see ThreeBitProtocolEncoder::encodeBytesAsLongs
     */
    size_t verifyAndEncodeImage(const std::vector<uint8_t>& image, std::vector<uint8_t>& encodedImage);
    
    /// \} /Communications Stuff


#pragma mark - ActionError

    /*!
     \brief An internal exception thrown to abort an action on the action thread.

     This exception is eventually caught in AsyncPropLoader::actionThread and leads to the status
     monitor's loaderHasFinishedWithError being called. There is a fixed list of primary errors
     which is reported as the error code. Secondary information about the error is provided in the
     what() string.

     \see APLoader::ErrorCode
     */
    class ActionError : public std::runtime_error {
    public:
        ActionError(APLoader::ErrorCode errorCode, const std::string& details) : errorCode(errorCode), std::runtime_error(details) {}
        /*!
         \brief The primary error, as an enum constant.
         
         Additional information about the error is in the what() string.
         */
        const APLoader::ErrorCode errorCode;
    };
    

#pragma mark - Profiler

    /*!
     \brief Keeps track of the performance of an action and provides timing estimates of future
     stages. May be removed.
     
     After further testing of AsyncPropLoader the Profiler may be reduced or removed.
     */
    class AsyncPropLoader::Profiler {

    public:

        Profiler() {}

        /*!
         \brief Contains information about the action's performance.
         
         The information will be complete after endOK or endWithError is called. Until then the
         information is current up the last stage completed.
         */
        APLoader::ActionSummary summary;

        /*!
         \brief The estimated total time for completing the action, in floating point seconds.
         
         The estimate is incomplete until finishedEncodingImage is called (assuming the action
         requires an image).
         */
        float getEstimatedTotalTime();

        /*!
         \name Update Functions
         
         These functions allow the profiler to track the action. They must be called in the
         correct order (as listed).
         */
        /// \{

        void start(APLoader::Action action, uint32_t baudrate, const simple::Milliseconds& resetDuration, const simple::Milliseconds& bootWaitDuration);

        /*!
         \brief Called if the action requires an image.
         */
        void willStartEncodingImage(size_t imageSize);

        /*!
         \brief Called if the action requires an image.
         
         encodedImageSize the size of the byte buffer holding the encoded image -- not the size
         of the original image.
         */
        void finishedEncodingImage(size_t encodedImageSize);

        void endStage1();
        void endStage2a();
        void endStage2b();
        void endStage3();
        void endStage4a();
        void endStage4b();
        void endStage5();
        void endStage6();
        void endStage7();

        /*!
         \brief Either endOK or endWithError must be called.
         */
        void endOK();

        /*!
         \brief Either endOK or endWithError must be called.
         */
        void endWithError(APLoader::ErrorCode errorCode);

        /// \} /Update Functions

    private:

        enum class Stage {
            Stage1,
            Stage2a,
            Stage2b,
            Stage3,
            Stage4a,
            Stage4b,
            Stage5,
            Stage6,
            Stage7,
            Finished,
        };

        void incrementStage(Stage& stage);

        Stage currStage;
        
        std::chrono::time_point<std::chrono::steady_clock> encodingStart;
        std::chrono::time_point<std::chrono::steady_clock> stageStart;

        /*!
         \brief Called in start.
         */
        void startTiming();

        /*!
         \brief Called from the end* functions.

         Reports the time since the last stageTime or startTiming call (like the lap feature
         of a stopwatch).
         */
        float stageTime();
};


} // namespace APLoader


#endif /* APLoaderInternal_hpp */

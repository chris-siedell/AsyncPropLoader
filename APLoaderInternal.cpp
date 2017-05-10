//
//  APLoaderInternal.cpp
//  libserial loader 001
//
//  Created by admin on 2/8/17.
//  Copyright © 2017 Chris Siedell. All rights reserved.
//

#include "APLoaderInternal.hpp"

#include <cassert>
#include <cmath>
#include <sstream>
#include <iomanip>

#include "ThreeBitProtocolEncoder.hpp"

using std::chrono::duration;
using std::chrono::duration_cast;

using simple::SteadyClock;
using simple::SteadyTimePoint;
using simple::Milliseconds;


namespace APLoader {


#pragma mark - Communications Stuff

    uint8_t decode3BPByte(std::vector<uint8_t>::iterator& iter, const std::vector<uint8_t>::iterator end)  {
        uint8_t decodedByte = 0;
        for (int i = 0; i < 4; ++i) {
            if (iter == end) {
                throw std::runtime_error("Insufficient bytes.");
            }
            decodedByte >>= 2;
            uint8_t byte = *iter++;
            if (byte == 0xCE) continue;
            else if (byte == 0xCF) decodedByte |= 0x40;
            else if (byte == 0xEE) decodedByte |= 0x80;
            else if (byte == 0xEF) decodedByte |= 0xC0;
            else {
                std::stringstream ss;
                ss << std::setw(2) << std::uppercase << std::hex;
                ss << "Unexpected byte: 0x" << int(byte) << ".";
                throw std::runtime_error(ss.str());
            }
        }
        return decodedByte;
    }

    size_t verifyAndEncodeImage(const std::vector<uint8_t>& image, std::vector<uint8_t>& encodedImage) {

        // todo: revise
        if (image.size() == 0) {
            throw std::invalid_argument("Image is too small to be valid.");
        }

        if (image.size() > 32768) {
            std::stringstream ss;
            ss << "Image size (" << image.size() << ") exceeds the Propeller's hub RAM size (32768).";
            throw std::invalid_argument(ss.str());
        }

        // todo: verify checksum
        // Remember to account for automatic stack bottom.

        ThreeBitProtocolEncoder encoder(encodedImage);
        return encoder.encodeBytesAsLongs(image);
    }


#pragma mark - Profiler
    
    void AsyncPropLoader::Profiler::start(APLoader::Action action, uint32_t baudrate, const Milliseconds& resetDuration, const Milliseconds& bootWaitDuration) {
        currStage = Stage::Stage1;
        startTiming();
        summary.reset();
        summary.action = action;
        summary.baudrate = baudrate;
        summary.resetDuration = resetDuration.count();
        summary.bootWaitDuration = bootWaitDuration.count();
    }

    void AsyncPropLoader::Profiler::willStartEncodingImage(size_t imageSize) {
        summary.imageSize = imageSize;
        encodingStart = SteadyClock::now();
    }

    void AsyncPropLoader::Profiler::finishedEncodingImage(size_t encodedImageSize) {
        SteadyTimePoint now = SteadyClock::now();
        summary.encodingTime = (duration_cast<duration<float>>(now - encodingStart)).count();
        summary.encodedImageSize = encodedImageSize;
    }

    float AsyncPropLoader::Profiler::getEstimatedTotalTime() {
        float secondsPerByte = 10.0f / summary.baudrate;
        float estimate = summary.totalTime;
        switch (currStage) {
            case Stage::Stage1:     // Stage 1: Preparation
                estimate += 0.1f;   //  using 0.1f just to guarantee estimate is non-zero
            case Stage::Stage2a:    // Stage 2a: Reset
                estimate += summary.resetDuration / 1000.0f;
                if (summary.action == Action::Restart) break;
            case Stage::Stage2b:    // Stage 2b: Wait After Reset
                estimate += summary.bootWaitDuration / 1000.0f;
            case Stage::Stage3:     // Stage 3: Establish Comms
                estimate +=  InitBytes.size() * secondsPerByte;
            case Stage::Stage4a:    // Stage 4a: Send Command
                // The actual time for this stage is insignificant (just sending 4 bytes).
                if (summary.action == Action::Shutdown) break;
            case Stage::Stage4b:    // Stage 4b: Send Image
                estimate += summary.encodedImageSize * secondsPerByte;
            case Stage::Stage5:     // Stage 5: Wait for Checksum Status
                estimate += 0.1f;   //  approx 0.1 seconds at 12 MHz
                if (summary.action == Action::LoadRAM) break;
            case Stage::Stage6:     // Stage 6: Wait for EEPROM Programming Status
                estimate += 3.7f;   //  approx 3.7 seconds at 12 MHz
            case Stage::Stage7:     // Stage 7: Wait for EEPROM Verification Status
                estimate += 1.3f;   //  approx 1.3 seconds at 12 MHz
            case Stage::Finished:
                estimate += 0.0f;
        }
        return estimate;
    }

    void AsyncPropLoader::Profiler::endStage1() {
        assert(currStage == Stage::Stage1);
        incrementStage(currStage);
        summary.stage1Time = stageTime();
        summary.totalTime += summary.stage1Time;
    }
        
    void AsyncPropLoader::Profiler::endStage2a() {
        assert(currStage == Stage::Stage2a);
        incrementStage(currStage);
        summary.stage2aTime = stageTime();
        summary.stage2Time = summary.stage2aTime;
        summary.totalTime += summary.stage2aTime;
    }

    void AsyncPropLoader::Profiler::endStage2b() {
        assert(currStage == Stage::Stage2b);
        incrementStage(currStage);
        summary.stage2bTime = stageTime();
        summary.stage2Time += summary.stage2bTime;
        summary.totalTime += summary.stage2bTime;
    }

    void AsyncPropLoader::Profiler::endStage3() {
        assert(currStage == Stage::Stage3);
        incrementStage(currStage);
        summary.stage3Time = stageTime();
        summary.totalTime += summary.stage3Time;
    }

    void AsyncPropLoader::Profiler::endStage4a() {
        assert(currStage == Stage::Stage4a);
        incrementStage(currStage);
        summary.stage4aTime = stageTime();
        summary.stage4Time = summary.stage4aTime;
        summary.totalTime += summary.stage4aTime;
    }

    void AsyncPropLoader::Profiler::endStage4b() {
        assert(currStage == Stage::Stage4b);
        incrementStage(currStage);
        summary.stage4bTime = stageTime();
        summary.stage4Time += summary.stage4bTime;
        summary.totalTime += summary.stage4bTime;
    }

    void AsyncPropLoader::Profiler::endStage5() {
        assert(currStage == Stage::Stage5);
        incrementStage(currStage);
        summary.stage5Time = stageTime();
        summary.totalTime += summary.stage5Time;
    }

    void AsyncPropLoader::Profiler::endStage6() {
        assert(currStage == Stage::Stage6);
        incrementStage(currStage);
        summary.stage6Time = stageTime();
        summary.totalTime += summary.stage6Time;
    }

    void AsyncPropLoader::Profiler::endStage7() {
        assert(currStage == Stage::Stage7);
        summary.stage7Time = stageTime();
        summary.totalTime += summary.stage7Time;
    }

    void AsyncPropLoader::Profiler::endOK() {
        currStage = Stage::Finished;
        summary.wasSuccessful = true;
    }

    void AsyncPropLoader::Profiler::endWithError(APLoader::ErrorCode errorCode) {
        switch (currStage) {
            case Stage::Stage1:
                endStage1();
                break;
            case Stage::Stage2a:
                endStage2a();
                break;
            case Stage::Stage2b:
                endStage2b();
                break;
            case Stage::Stage3:
                endStage3();
                break;
            case Stage::Stage4a:
                endStage4a();
                break;
            case Stage::Stage4b:
                endStage4b();
                break;
            case Stage::Stage5:
                endStage5();
                break;
            case Stage::Stage6:
                endStage6();
                break;
            case Stage::Stage7:
                endStage7();
                break;
            default:
                assert(false);
        }
        currStage = Stage::Finished;
        summary.wasSuccessful = false;
        summary.errorCode = errorCode;
    }

    void AsyncPropLoader::Profiler::incrementStage(Stage& stage) {
        if (stage < Stage::Stage1) {
            // Stage is invalid.
            assert(false);
        } else if (stage < AsyncPropLoader::Profiler::Stage::Stage7) {
            stage = static_cast<Stage>( static_cast<int>(stage) + 1 );
        } else {
            // Can't go past stage 7.
            assert(false);
        }
    }

    void AsyncPropLoader::Profiler::startTiming() {
        stageStart = SteadyClock::now();
    }

    float AsyncPropLoader::Profiler::stageTime() {
        SteadyTimePoint now = SteadyClock::now();
        float time = (duration_cast<duration<float>>(now - stageStart)).count();
        stageStart = now;
        return time;
    }

} // namespace APLoader


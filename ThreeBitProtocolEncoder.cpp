//
//  ThreeBitProtocolEncoder.cpp
//  libserial loader 001
//
//  Created by admin on 2/6/17.
//  Copyright © 2017 Chris Siedell. All rights reserved.
//

#include "ThreeBitProtocolEncoder.hpp"

#include <cassert>
#include <sstream>


ThreeBitProtocolEncoder::ThreeBitProtocolEncoder(std::vector<uint8_t>& _buffer) : buffer(_buffer) {
    buffer.clear();
    bitPos = 0;
    currByte = 0xff; // Begin with all bits high (except for the start bit, of course).
}

void ThreeBitProtocolEncoder::encodeLong(uint32_t longValue) {
    encodeLongInternal(longValue);
    pushCurrByteIfNotEmpty();
}

size_t ThreeBitProtocolEncoder::encodeBytesAsLongs(const std::vector<uint8_t>& bytes) {
    size_t numBytes = bytes.size();
    size_t numLongs = numBytes / 4; // numLongs is initially the number of full (not padded) longs.
    size_t index = 0;
    const uint8_t* data = bytes.data();
    for (int i = 0; i < numLongs; i++) {
        assert((numBytes - index) > 3);
        uint32_t longValue = 0;
        longValue |= data[index++];
        longValue |= data[index++] << 8;
        longValue |= data[index++] << 16;
        longValue |= data[index++] << 24;
        encodeLongInternal(longValue);
    }
    if (index < numBytes) {
        // There are remainder bytes.
        assert((numBytes - index) < 4);
        numLongs++; // Now numLongs is the total number of longs, including the last padded long.
        uint32_t longValue = 0;
        longValue |= data[index++];
        if (index < numBytes) longValue |= data[index++] << 8;
        if (index < numBytes) longValue |= data[index++] << 16;
        encodeLongInternal(longValue);
    }
    pushCurrByteIfNotEmpty();
    return numLongs;
}

void ThreeBitProtocolEncoder::encodeLongInternal(uint32_t longValue) {
    uint8_t bit;
    for (int i = 0; i < 31; i++) {
        bit = longValue & 1;
        longValue >>= 1;
        encodeBit(bit, IntraLongIdleTime);
    }
    bit = longValue & 1;
    encodeBit(bit, InterLongIdleTime);
}

void ThreeBitProtocolEncoder::encodeBit(uint8_t bit, size_t idleBits) {

    assert(idleBits > 0);
    assert(idleBits < 9);

    if (bitPos >= 10) {
        pushCurrByteIfNotEmpty();
    }

    if (bitPos == 0) {
        // Starting at the stop bit means we can guarantee all valid idleBits values.
        if (bit == 0) {
            // Clear lower bit of currByte to extend the start bit into a long pulse.
            currByte &= 0xfe;
            bitPos = 2 + idleBits;
            return;
        } else {
            // Use implicit short pulse of start bit.
            bitPos = 1 + idleBits;
            return;
        }
    } else {
        if (bit == 0) {
            size_t newPos = bitPos + 2 + idleBits;
            if (newPos > 10) {
                // Need to move to next byte to guarantee requested idleBits.
                pushCurrByteIfNotEmpty();
                encodeBit(bit, idleBits);
                return;
            } else {
                // Bit 0 = long pulse (2 bit periods).
                currByte &= ~(3 << (bitPos - 1));
                bitPos = newPos;
                return;
            }
        } else {
            size_t newPos = bitPos + 1 + idleBits;
            if (newPos > 10) {
                // Need to move to next byte to guarantee requested idleBits.
                pushCurrByteIfNotEmpty();
                encodeBit(bit, idleBits);
                return;
            } else {
                // Bit 1 = short pulse (1 bit period).
                currByte &= ~(1 << (bitPos - 1));
                bitPos = newPos;
                return;
            }
        }
    }
}

void ThreeBitProtocolEncoder::pushCurrByteIfNotEmpty() {
    if (bitPos == 0) {
        // The default 'empty' byte (0xff) has an implicitly encoded 1 (from the stop bit) so
        //  pushing the 'empty' byte would put garbage data into the buffer.
        return;
    }
    buffer.push_back(currByte);
    bitPos = 0;
    currByte = 0xff;
}



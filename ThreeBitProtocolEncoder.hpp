//
//  ThreeBitProtocolEncoder.hpp
//  libserial loader 001
//
//  Created by admin on 2/6/17.
//  Copyright © 2017 Chris Siedell. All rights reserved.
//

#ifndef ThreeBitProtocolEncoder_hpp
#define ThreeBitProtocolEncoder_hpp

#include <vector>

/*!
 \brief A class for encoding data in the 3-Bit-Protocol (3BP) used by the Propeller bootloader.
 
 In 3BP a 1 is encoded as a short low pulse and a 0 is encoded as a long low pulse. When idle --
 not transmitting encoded bits -- the line should stay high. The Propeller determines the
 difference between a short and long pulse using two calibration pulses sent at the beginning
 of communications.

 This encoder packs encoded data into the provided buffer for 8N1 asynchronous serial transmission.
 It uses a single 0 bit for a short low pulse and two consecutive 0 bits for a long low pulse. It
 takes into consideration the implied start bit, and it tries to pack bits as tightly as
 possible. It uses a longer high idle period between bits of different longs (four byte values)
 since the Propeller does extra work after receiving a long. This supports a faster baudrate for
 reliable communications with the Propeller's booter program, which uses the RCFAST
 clock mode (8 MHz - 20 MHz).
 
 Its output can be transmitted at up to 115200 bps. See ThreeBitProtocolEncoder::MaxBaudrate for details.
 */

class ThreeBitProtocolEncoder {

public:

    /*!
     \brief Creates an encoder which puts its encoded data into the provided buffer.
     
     The encoder begins by clearing the buffer.
     */
    ThreeBitProtocolEncoder(std::vector<uint8_t>& buffer);

    /*!
     \brief Appends the encoded four byte value to the buffer.
     */
    void encodeLong(uint32_t longValue);

    /*!
     \brief Appends the encoded bytes to the buffer.
     
     Bytes are encoded in groups of four -- a 'long' on the Propeller. If the size of bytes is
     not a multiple of four then the end is implicitly padded with sufficient NUL bytes.
     
     The Propeller uses little-endian byte order.
     
     The return value is the number of longs encoded.
     */
    size_t encodeBytesAsLongs(const std::vector<uint8_t>& bytes);

    /*!
     \brief The maximum guaranteed safe baudrate for trasmitting data encoded by this class
     to the Propeller bootloader.
     
     The limit of 115200 bps was chosen after close analysis of the Propeller's booter program. Two
     aspects of receiving data were considered: pulse duration and interpulse timing.
     
     __Pulse Duration__

     The Propeller determines the duration of a pulse by counting loops while
     the rx line is low. Then it compares that loop count to a threshold to classify the bit as
     a 0 or a 1. When the loop count number is low (i.e. the baudrate is high) the Propeller may
     not always correctly classify a bit (it would tend to change a 1 to a 0). If one and two bit
     periods are used for the short and long pulses -- as in this encoder -- then 133 kbps is the
     maximum safe baudrate (assuming up to ±10% jitter and an 8 MHz clock).
     
     The following plot shows that given a fast enough baudrate the Propeller may not always
     correctly classify a bit.
     
     \image html pulse_analysis_1T_2T_10pct.png
     
     60 clocks corresponds to 133 kbps at 8 MHz.
     The plot was generated using [propBooterAnalysis.py](propBooterAnalysis.py).

     __Interpulse Timing__

     The Propeller does work after receiving one encoded bit and before
     being able to receive the next encoded bit. If one bit period is used between bits of the
     same long, and two bit periods are used between bits of different longs, then the
     maximum safe baudrate is 150 kbps (assuming up to ±10% jitter and an 8 MHz clock).
     
     The following table gives the minimum number of clocks of high idle between events for
     reliable communications. It is based on a count of instructions from the booter
     detecting that the rx line has gone high to being ready to detect the rx line going low.
     Worst case assumptions are used: being 8 clocks late in detecting the rising edge, and
     hub instructions taking 23 clocks.
     
     | Interval                                 | Clocks | Requires 2T at 115200 |
     |------------------------------------------|--------|-----------------------|
     | between host auth bits                   | 60     |                       |
     | from host auth to prop auth              | 84     | Y                     |
     | between prop auth bits                   | 48     |                       |
     | from prop auth to version                | 79     | Y                     |
     | between version bits                     | 44     |                       |
     | from version to command                  | 52     |                       |
     | from command to length                   | 80     | Y                     |
     | from length to payload                   | 72     | Y                     |
     | between bits of a payload long           | 44     |                       |
     | between bits of different payload longs  | 95     | Y                     |
     
     At 115200 bps and 8 MHz a nominal bit period is 69.4 clocks. So any interval longer than
     this requires at least two bit periods of high idle.

     __Conclusion__

     115200 bps was chosen because it is the fastest commonly supported baudrate below these
     two limits.
     */
    static const uint32_t MaxBaudrate = 115200;

private:

    /*!
     \brief Internal function used to encode a long (four bytes) of data.

     encodeLongInternal differs from encodeLong (the public function) in that currByte is not
     automatically pushed to the buffer at the end -- currByte is left open for additional
     encoded bits. This achieves higher density packing if multiple longs are encoded.

     Note that currByte must be pushed onto the buffer before returning from a public function,
     otherwise the last few encoded bits might not be in the buffer.
     */
    void encodeLongInternal(uint32_t longValue);

    /*!
     \brief Internal function to encode a single bit.

     idleBits specifies the minimum guaranteed duration of the high idle after the encoded bit
     pulse, in bit periods.

     idleBits must be in the range [1, 8].
     */
    void encodeBit(uint8_t bit, size_t idleBits);

    /*!
     \brief Pushes currByte onto the buffer.
     
     An 'empty' currByte (i.e. bitPos = 0) is not pushed onto the buffer since the implicit
     start bit pulse would result in garbage data being encoded.
     */
    void pushCurrByteIfNotEmpty();

    /*!
     \brief The data buffer. Cleared in the constructor.
     */
    std::vector<uint8_t>& buffer;

    /*!
     \brief The position for the next encoded pulse within currByte. Position zero refers to the
     start bit.
     */
    size_t bitPos;

    /*!
     \brief The current byte where encoded bits are added. Pushed onto the buffer when full.
     */
    uint8_t currByte;

    /*!
     \brief The number of bit periods of high idle between encoded bit pulses of the same long.
     */
    const size_t IntraLongIdleTime = 1;

    /*!
     \brief The number of bit periods of high idle between encoded bit pulses of different longs.
     
     This must be 2+ to reliably support 115200 bps since the Propeller does extra work between
     receiving longs.
     */
    const size_t InterLongIdleTime = 2;
};

#endif /* ThreeBitProtocolEncoder_hpp */

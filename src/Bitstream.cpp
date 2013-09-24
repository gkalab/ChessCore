//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Bitstream.cpp: Blob Bitstream interface class implementation.
//

#include <string.h>
#include <ChessCore/Bitstream.h>
#include <ChessCore/Log.h>

using namespace std;

namespace ChessCore {
const char *Bitstream::m_classname = "Bitstream";

Bitstream::Bitstream(Blob &blob) :
    m_blob(blob),
    m_readOnly(false),
    m_readOffset(0),
    m_readBit(7),
    m_writeOffset(0),
    m_writeBit(7)
{
}

Bitstream::Bitstream(const Blob &blob) :
    m_blob(const_cast<Blob &> (blob)),
    m_readOnly(true),
    m_readOffset(0),
    m_readBit(7),
    m_writeOffset(0),
    m_writeBit(7)
{
}

Bitstream::~Bitstream() {
}

bool Bitstream::read(uint32_t &value, unsigned numBits) {
#define READ_BYTE() \
    if (m_readOffset >= m_blob.length()) { \
        logerr("No more data at offset %u (blob length=%u)", m_readOffset, m_blob.length()); \
        return false; \
    } \
    byte = *(m_blob.data() + m_readOffset)

    value = 0;
    uint8_t byte;

    READ_BYTE();

    while (numBits > 0) {
        value <<= 1;
        value |= (byte >> m_readBit) & 1;

        if (m_readBit == 0) {
            m_readBit = 7;
            m_readOffset++;

            if (numBits > 1) {
                READ_BYTE();
            }
        } else {
            m_readBit--;
        }

        numBits--;
    }

    return true;

#undef READ_BYTE
}

bool Bitstream::write(uint32_t value, unsigned numBits) {
#define READ_BYTE() \
    if (m_writeOffset >= m_blob.allocatedLength()) { \
        logerr("No more data at offset %u (blob length=%u)", m_writeOffset, m_blob.allocatedLength()); \
        return false; \
    } \
    byte = *(m_blob.data() + m_writeOffset)

#define WRITE_BYTE() \
    if (m_writeOffset >= m_blob.allocatedLength()) { \
        logerr("No more space at offset %u (blob length=%u)", m_writeOffset, m_blob.allocatedLength()); \
        return false; \
    } \
    *(m_blob.data() + m_writeOffset) = byte

    if (m_readOnly) {
        LOGERR << "Cannot write to a read-only bitstream";
        return false;
    }

    unsigned newBitLength = (m_writeOffset * 8) + (7 - m_writeBit) + numBits;
    unsigned newLength = (newBitLength / 8) + ((newBitLength & 7) == 0 ? 0 : 1);

    if (newLength > m_blob.allocatedLength()) {
        if (!m_blob.reserve(newLength)) {
            LOGERR << "Failed to reserve " << newLength << " bytes in blob";
            return false;
        }
    }

    // As we aren't using Blob::add() to add the bytes, explicitly set the length
    // of the blob ourselves
    if (newLength > m_blob.length())
        m_blob.setLength(newLength);

    uint8_t byte;
    bool needsFlush = false;

    READ_BYTE();

    while (numBits > 0) {
        byte |= ((value >> (numBits - 1)) & 1) << m_writeBit;
        needsFlush = true;

        if (m_writeBit == 0) {
            WRITE_BYTE();
            byte = 0;
            m_writeOffset++;
            m_writeBit = 7;
            needsFlush = false;
        } else {
            m_writeBit--;
        }

        numBits--;
    }

    if (needsFlush) {
        WRITE_BYTE();
    }

    return true;

#undef READ_BYTE
#undef WRITE_BYTE
}
}   // namespace ChessCore

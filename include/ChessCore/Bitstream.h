//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Bitstream.h: Blob Bitstream interface class definition.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Blob.h>

namespace ChessCore {
class CHESSCORE_EXPORT Bitstream {
private:
    static const char *m_classname;

protected:
    Blob &m_blob;
    bool m_readOnly;                // If initialised with const Blob &
    unsigned m_readOffset;          // Read offset (byte count)
    unsigned m_readBit;             // Read bit within offset
    unsigned m_writeOffset;         // Write offset (byte count)
    unsigned m_writeBit;            // Write bit within offset

public:
    Bitstream(Blob &blob);
    Bitstream(const Blob &blob);
    virtual ~Bitstream();

    unsigned length() const {
        return m_blob.length();
    }

    unsigned allocatedLength() const {
        return m_blob.allocatedLength();
    }

    bool isEmpty() const {
        return m_blob.isEmpty();
    }

    bool isReadOnly() const {
        return m_readOnly;
    }

    unsigned readOffset() const {
        return m_readOffset;
    }

    unsigned readBit() const {
        return m_readBit;
    }

    unsigned writeOffset() const {
        return m_writeOffset;
    }

    unsigned writeBit() const {
        return m_writeBit;
    }

    void reset() {
        m_readOffset = 0;
        m_readBit = 7;
        m_writeOffset = 0;
        m_writeBit = 7;
    }

    /**
     * Read the specified number of bits from the current read bit offset in the blob
     * and into to the unsigned integer.
     *
     * @param value The value to read.
     * @param numBits The number of bits to read.
     *
     * @return true if the integer was read successfully, else false.
     */
    bool read(uint32_t &value, unsigned numBits);

    /**
     * Write the specified number of bits from an unsigned integer, into the blob
     * at the current write offset, allocating additional space as required.
     *
     * @param value The value to write.
     * @param numBits The number of bits to write.
     *
     * @return Boolean success code.
     */
    bool write(uint32_t value, unsigned numBits);
};
} // namespace ChessCore

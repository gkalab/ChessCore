//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Blob.h: Binary Large OBject class definition.
//

#pragma once

#include <ChessCore/ChessCore.h>

namespace ChessCore {

class CHESSCORE_EXPORT Blob {
private:
    static const char *m_classname;

protected:
    uint8_t *m_data;
    unsigned m_length;
    unsigned m_allocatedLength;
    bool m_ownsMemory;

public:
    Blob();

    // Same as calling set(data, length, false); the blob won't own the memory
    Blob(uint8_t *data, unsigned length);

    ~Blob();

    uint8_t *data() {
        return m_data;
    }

    const uint8_t *data() const {
        return m_data;
    }

    const uint8_t *end() const {
        return m_data + m_length;
    }

    unsigned length() const {
        return m_length;
    }

    void setLength(unsigned length) {
        if (length <= m_allocatedLength)
            m_length = length;
    }

    unsigned allocatedLength() const {
        return m_allocatedLength;
    }

    bool ownsMemory() const {
        return m_ownsMemory;
    }

    bool isEmpty() const {
        return m_data == 0 || m_length == 0;
    }

    /**
     * Reserve the specified number of bytes to avoid unncessary realloc's when
     * adding data to the blob.  If the blob doesn't currently own the memory,
     * then the request will be rejected.
     *
     * Note that this method does not set the length of the blob, so if you are doing:
     *
     *    blob.reserve(1000);
     *    ::read(fd, blob.data(), 1000);
     *
     * Then don't forget:
     *
     *    blob.setLength(1000);
     *
     * @param length The total amount to reserve.
     *
     * @return true if the specified number of bytes was reserved, else false.
     */
    bool reserve(unsigned length);

    /**
     * Set the data, without assuming ownership of it.
     *
     * @param data The data buffer.
     * @param length The length of the data buffer.
     * @param copy If true then copy the data and take ownership, else
     * if false then just track the external memory.
     *
     * @return true if the data was set successfully, else false.
     */
    bool set(const uint8_t *data, unsigned length, bool copy = true);

    /**
     * Append data to the data buffer.  Data cannot be added unless the Blob owns the
     * existing data.
     *
     * @param data The data to add.
     * @param length The length of the data buffer.
     *
     * @return true if the data was added successfully, else false.
     */
    bool add(const uint8_t *data, unsigned length);

    /**
     * Free the data stored in the blob, if the blob owns it.
     */
    void free();

    /**
     * Format the binary data as a string.
     */
    void toString(std::string &dest) const;

    /**
     * Dump the data to a string.
     */
    std::string dump() const;

    friend CHESSCORE_EXPORT std::ostream &operator<<(std::ostream &os, const Blob &blob);
};

} // namespace ChessCore

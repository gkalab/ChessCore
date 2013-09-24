//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Blob.cpp: Binary Large OBject class implementation.
//

#define VERBOSE_LOGGING 0

#include <stdlib.h>
#include <string.h>
#include <iomanip>
#include <sstream>
#include <new>
#include <ChessCore/Blob.h>
#include <ChessCore/Util.h>
#include <ChessCore/Log.h>

using namespace std;

namespace ChessCore {
const char *Blob::m_classname = "Blob";

Blob::Blob() :
    m_data(0),
    m_length(0),
    m_allocatedLength(0),
    m_ownsMemory(false)
{
}

Blob::Blob(uint8_t *data, unsigned length) :
    m_data(data),
    m_length(length),
    m_allocatedLength(0),
    m_ownsMemory(false)
{
}

Blob::~Blob() {
    free();
}

bool Blob::reserve(unsigned length) {
    if (length == 0) {
        LOGWRN << "Requested to reserve 0 bytes?";
        return true;    // Ignore
    }

    if (!m_data) {
        ASSERT(!m_length);
        ASSERT(!m_allocatedLength);
        m_data = (uint8_t *)::malloc(length);

        if (m_data == 0) {
            LOGERR << "Failed to allocate " << length << " bytes of memory";
            return false;
        }

        memset(m_data, 0, length);
        m_allocatedLength = length;
        m_ownsMemory = true;
        LOGVERBOSE << "Reserved " << m_allocatedLength << " bytes";
    } else {
        if (!ownsMemory()) {
            LOGERR << "Cannot reserve more as the blob does not own the memory";
            return false;
        }

        if (length < m_allocatedLength) {
            LOGERR << "Reserve length (" << length <<
                ") is smaller than length already allocated (" << m_allocatedLength;
            return false;
        }

        m_data = (uint8_t *)::realloc(m_data, length);
        if (!m_data) {
            LOGERR << "Failed to reallocate " << length << " bytes of memory";
            return false;
        }

        memset(m_data + m_allocatedLength, 0, length - m_allocatedLength);
        m_allocatedLength = length;
        LOGVERBOSE << "Reserved " << length << " bytes";
    }

    return true;
}

bool Blob::set(const uint8_t *data, unsigned length, bool copy /*=true*/) {
    free();

    if (!data || !length)
        return true; // That's OK

    if (copy) {
        m_data = (uint8_t *)::malloc(length);
        if (!m_data) {
            LOGERR << "Failed to allocate " << length << " bytes of memory";
            return false;
        }

        ::memcpy(m_data, data, length);
        m_length = length;
        m_allocatedLength = length;
        m_ownsMemory = true;
        LOGVERBOSE << "Copied " << m_allocatedLength << " bytes";
    } else {
        m_data = const_cast<uint8_t *> (data);
        m_length = length;
        m_allocatedLength = length;
        m_ownsMemory = false;
    }

    return true;
}

bool Blob::add(const uint8_t *data, unsigned length) {
    if (!m_data) {
        ASSERT(!m_length);
        ASSERT(!m_allocatedLength);
        m_data = (uint8_t *)::malloc(length);
        if (!m_data) {
            LOGERR << "Failed to allocate " << length << " bytes of memory";
            return false;
        }

        ::memcpy(m_data, data, length);
        m_length = length;
        m_allocatedLength = length;
        m_ownsMemory = true;
    } else {
        if (!m_ownsMemory) {
            LOGERR << "Cannot add data as the blob does not own the memory";
            return false;
        }

        ASSERT(m_ownsMemory);

        if (m_length + length > m_allocatedLength) {
            m_data = (uint8_t *)::realloc(m_data, m_length + length);
            if (!m_data) {
                LOGERR << "Failed to reallocate " << (m_length + length) << " bytes of memory";
                return false;
            }

            LOGVERBOSE << "Allocated an extra " << (m_length + length) - m_allocatedLength << " bytes";
            m_allocatedLength = m_length + length;
        }

        ::memcpy(m_data + m_length, data, length);
        m_length += length;
    }

    return true;
}

void Blob::free() {
    if (m_ownsMemory && m_data && m_allocatedLength)
        ::free(m_data);

    m_data = 0;
    m_length = 0;
    m_allocatedLength = 0;
    m_ownsMemory = false;
}

void Blob::toString(string &dest) const {
    stringstream oss;

    if (m_data != 0 && m_length > 0) {
        const uint8_t *p = m_data;
        for (unsigned i = 0; i < m_length; i++)
            oss << Util::hexChar(*p++);
    }

    dest = oss.str();
}

string Blob::dump() const {
    return Util::formatData(m_data, m_length);
}

std::ostream &operator<<(std::ostream &os, const Blob &blob) {
    os << blob.dump();
    return os;
}

}   // namespace ChessCore

//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Epd.h: Extended Position Description (EPD) class definitions.
//

#pragma once

#include <ChessCore/ChessCore.h>
#include <ChessCore/Position.h>
#include <istream>

namespace ChessCore {
class CHESSCORE_EXPORT EpdOp {
private:
    static const char *m_classname;

public:
    enum Type {
        OP_NONE = 0,
        OP_STRING = 1,
        OP_INTEGER = 2,
        OP_MOVE = 3,
        OP_EVAL = 4
    };

    enum Eval {
        EVAL_NONE = 0,
        EVAL_W_DECISIVE_ADV = 1,
        EVAL_W_CLEAR_ADV = 2,
        EVAL_W_SLIGHT_ADV = 3,
        EVAL_EQUAL = 4,
        EVAL_B_SLIGHT_ADV = 5,
        EVAL_B_CLEAR_ADV = 6,
        EVAL_B_DECISIVE_ADV = 7,
        NUM_EVALS = EVAL_B_DECISIVE_ADV
    };

protected:
    std::string m_opcode;
    Type m_type;
    union {
        const char *str;
        int64_t integer;
        const Move *move; // This has to be a pointer as it has a non-trivial constructor (shrug)
        Eval evl;
    } m_operand;

public:
    inline EpdOp() {
        m_type = OP_NONE;
    }

    inline virtual ~EpdOp() {
        free();
    }

    //
    // Free data
    //
    void free();

    inline const std::string &opcode() const {
        return m_opcode;
    }

    inline void setOpcode(const char *opcode) {
        m_opcode = opcode;
    }

    inline Type type() const {
        return m_type;
    }

    void setOperandNone();

    inline const char *operandString() const {
        return m_operand.str;
    }

    void setOperandString(const char *str);

    inline int64_t operandInteger() const {
        return m_operand.integer;
    }

    void setOperandInteger(int64_t integer);

    inline const Move &operandMove() const {
        return *(m_operand.move);
    }

    void setOperandMove(const Move &move);

    inline Eval operandEval() const {
        return m_operand.evl;
    }

    void setOperandEval(const Eval evl);

    static std::string formatEval(Eval evl);

    inline std::string formatEval() {
        return formatEval(m_operand.evl);
    }
};

class CHESSCORE_EXPORT Epd {
private:
    static const char *m_classname;

protected:
    struct EpdOpType {
        const char *opcode;
        EpdOp::Type type;
    };

    static EpdOpType m_epdOpTypes[];
    static size_t m_numEpdOpTypes;

    Position m_position;
    std::vector<EpdOp *> m_ops;
    unsigned m_lineNum;
    std::vector<EpdOp *>::const_iterator m_findOpIt;

public:
    //
    // Constructors
    //
    inline Epd() {
        init();
    }

    //
    // Destructor
    //
    virtual ~Epd();

    //
    // Initialisers
    //
    void init();

    //
    // Free EPD operands
    //
    void freeOps();

    //
    // Parse an EPD.
    //
    bool parse(const std::string &epdline, unsigned lineNum);

protected:
    //
    // Parse an EPD operation.
    //
    bool parseEpdOp(const std::string &optext);

    //
    // Split an EPD line
    //
    static unsigned split(const std::string &line, std::vector<std::string> &parts);

public:

    inline const Position &pos() const {
        return m_position;
    }

    //
    // The number of EPD operands stored.
    //
    inline size_t numOps() const {
        return m_ops.size();
    }

    //
    // Return the specified EPD operand.
    //
    inline const EpdOp *op(size_t index) const {
        return const_cast<const EpdOp *> (m_ops[index]);
    }

    inline unsigned lineNum() const {
        return m_lineNum;
    }

    //
    // Find an EPD operand.
    //
    inline const EpdOp *findFirstOp(const std::string &opcode) {
        return findOp(opcode, m_ops.begin());
    }

    inline const EpdOp *findNextOp(const std::string &opcode) {
        return findOp(opcode, ++m_findOpIt);
    }

protected:
    const EpdOp *findOp(const std::string &opcode, std::vector<EpdOp *>::const_iterator startOp);

public:

    //
    // Test if the EPD contains one or more move-related operations.
    //
    bool hasMoveOps();

    //
    // Test that a move passes the defined move operations.
    //
    bool checkMoveOps(const Move &move);

    //
    // Test if the EPD contains one or more eval-related operations.
    //
    bool hasEvalOps();

    //
    // Dump an EPD to a string.
    //
    std::string dump() const;

    friend CHESSCORE_EXPORT std::ostream &operator<<(std::ostream &os, const Epd &epd);
};

class CHESSCORE_EXPORT EpdFile {
private:
    static const char *m_classname;

protected:
    std::vector<Epd *> m_epds;

public:
    //
    // Constructors.
    //
    inline EpdFile() {
        init();
    }

    //
    // Destructors.
    //
    virtual ~EpdFile();

    //
    // Initialisers.
    //
    void init();

    //
    // Free EPDs.
    //
    void freeEpds();

    //
    // The number of EPDs stored.
    //
    inline size_t numEpds() const {
        return m_epds.size();
    }

    //
    // Return the specified EPD.
    //
    // TODO: Return const epd *
    //
    inline Epd *epd(size_t index) const {
        return m_epds[index];
    }

    //
    // Read EPD entries from a file.
    //
    bool readFromFile(const std::string &filename);

    //
    // Read EPD entries from a string.
    //
    bool readFromString(const std::string &data);

private:
    bool read(std::istream &stream);

};
} // namespace ChessCore

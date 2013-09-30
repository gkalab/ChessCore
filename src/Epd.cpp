//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// Epd.cpp: Extended Position Description (EPD) class implementations.
//

#if FREEBSD
#define __ISO_C_VISIBLE 1999
#define __LONG_LONG_SUPPORTED
#endif

#include <ChessCore/Epd.h>
#include <ChessCore/Log.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>

using namespace std;

namespace ChessCore {

// ==========================================================================
// class EpdOp
// ==========================================================================

const char *EpdOp::m_classname = "EpdOp";

struct Epd::EpdOpType Epd::m_epdOpTypes[] = {
    {"acn",         EpdOp::OP_INTEGER},
    {"acs",         EpdOp::OP_INTEGER},
    {"am",          EpdOp::OP_MOVE},
    {"bm",          EpdOp::OP_MOVE},
    {"c0",          EpdOp::OP_STRING},
    {"c1",          EpdOp::OP_STRING},
    {"c2",          EpdOp::OP_STRING},
    {"c3",          EpdOp::OP_STRING},
    {"c4",          EpdOp::OP_STRING},
    {"c5",          EpdOp::OP_STRING},
    {"c6",          EpdOp::OP_STRING},
    {"c7",          EpdOp::OP_STRING},
    {"c8",          EpdOp::OP_STRING},
    {"c9",          EpdOp::OP_STRING},
    {"ce",          EpdOp::OP_INTEGER},
    {"dm",          EpdOp::OP_INTEGER},
    {"draw_accept", EpdOp::OP_NONE},
    {"draw_claim",  EpdOp::OP_NONE},
    {"draw_offer",  EpdOp::OP_NONE},
    {"draw_reject", EpdOp::OP_NONE},
    {"eco",         EpdOp::OP_STRING},
    {"eval",        EpdOp::OP_EVAL},               // Non-standard eval test
    {"fmvn",        EpdOp::OP_INTEGER},
    {"hmvc",        EpdOp::OP_INTEGER},
    {"id",          EpdOp::OP_STRING},
    {"nic",         EpdOp::OP_STRING},
    {"noop",        EpdOp::OP_NONE},
    {"perft1",      EpdOp::OP_INTEGER},            // Non-standard perft test, depth = 1
    {"perft2",      EpdOp::OP_INTEGER},            // Non-standard perft test, depth = 2
    {"perft3",      EpdOp::OP_INTEGER},            // Non-standard perft test, depth = 3
    {"perft4",      EpdOp::OP_INTEGER},            // Non-standard perft test, depth = 4
    {"perft5",      EpdOp::OP_INTEGER},            // Non-standard perft test, depth = 5
    {"perft6",      EpdOp::OP_INTEGER},            // Non-standard perft test, depth = 6
    {"perft7",      EpdOp::OP_INTEGER},            // Non-standard perft test, depth = 7
    {"perft8",      EpdOp::OP_INTEGER},            // Non-standard perft test, depth = 8
    {"perft9",      EpdOp::OP_INTEGER},            // Non-standard perft test, depth = 9
    {"pm",          EpdOp::OP_MOVE},
    {"pv",          EpdOp::OP_MOVE},
    {"rc",          EpdOp::OP_INTEGER},
    {"resign",      EpdOp::OP_NONE},
    {"sm",          EpdOp::OP_MOVE},
    {"tcgs",        EpdOp::OP_STRING},
    {"tcri",        EpdOp::OP_STRING},
    {"tcsi",        EpdOp::OP_STRING},
    {"v0",          EpdOp::OP_STRING},
    {"v1",          EpdOp::OP_STRING},
    {"v2",          EpdOp::OP_STRING},
    {"v3",          EpdOp::OP_STRING},
    {"v4",          EpdOp::OP_STRING},
    {"v5",          EpdOp::OP_STRING},
    {"v6",          EpdOp::OP_STRING},
    {"v7",          EpdOp::OP_STRING},
    {"v8",          EpdOp::OP_STRING},
    {"v9",          EpdOp::OP_STRING}
};

size_t Epd::m_numEpdOpTypes = sizeof(Epd::m_epdOpTypes) / sizeof(Epd::m_epdOpTypes[0]);

void EpdOp::free() {
    if (m_type == OP_STRING)
        ::free((void *)m_operand.str);
    else if (m_type == OP_MOVE)
        delete m_operand.move;

    m_type = OP_NONE;
}

void EpdOp::setOperandNone() {
    m_type = OP_NONE;
}

void EpdOp::setOperandString(const char *str) {
    m_operand.str = strdup(str);
    m_type = OP_STRING;
}

void EpdOp::setOperandInteger(int64_t integer) {
    m_operand.integer = integer;
    m_type = OP_INTEGER;
}

void EpdOp::setOperandMove(const Move &move) {
    m_operand.move = new Move(move);
    m_type = OP_MOVE;
}

void EpdOp::setOperandEval(const Eval eval) {
    m_operand.evl = eval;
    m_type = OP_EVAL;
}

string EpdOp::formatEval(Eval eval) {
    switch (eval) {
    case EVAL_W_DECISIVE_ADV:
        return "+-";

    case EVAL_W_CLEAR_ADV:
        return "+/-";

    case EVAL_W_SLIGHT_ADV:
        return "+/=";

    case EVAL_EQUAL:
        return "=";

    case EVAL_B_SLIGHT_ADV:
        return "=/+";

    case EVAL_B_CLEAR_ADV:
        return "-/+";

    case EVAL_B_DECISIVE_ADV:
        return "-+";

    default:
        return "???";
    }
}

// ==========================================================================
// class Epd
// ==========================================================================

const char *Epd::m_classname = "Epd";

Epd::~Epd() {
    freeOps();
}

void Epd::init() {
    m_position.init();
    m_ops.clear();
    m_lineNum = 0;
    m_findOpIt = m_ops.end();
}

void Epd::freeOps() {
    for (vector<EpdOp *>::iterator it = m_ops.begin(); it != m_ops.end(); ++it) {
        EpdOp *op = *it;
        delete op;
    }

    m_ops.clear();
}

bool Epd::parse(const string &epdline, unsigned lineNum) {
    vector<string> fields;
    unsigned numFields = split(epdline, fields);

    m_lineNum = lineNum;

    if (numFields < 4) {
        LOGERR << "line " << lineNum << ": expected at least 4 fields; got " << numFields;
        return false;
    }

    if (m_position.setFromFen(fields[0].c_str(), fields[1].c_str(), fields[2].c_str(),
                              fields[3].c_str(), 0, 0) != Position::LEGAL) {
        LOGERR << "line " << lineNum << ": invalid position data";
        return false;
    }

    for (unsigned i = 4; i < numFields; i++)
        if (!parseEpdOp(fields[i])) {
            LOGERR << "line " << lineNum << ": failed to process EPD operand '" << fields[i] << "'";
            freeOps();
            return false;
        }

    return true;
}

bool Epd::parseEpdOp(const string &optext) {
    unsigned i;

    vector<string> fields;
    char *endp;
    EpdOp *op;
    int64_t integer;
    Move move;

    unsigned numFields = Util::splitLine(optext, fields);

    if (numFields == 0)
        return false;

    // Force opcode to lowercase
    transform(fields[0].begin(), fields[0].end(), fields[0].begin(), ::tolower);

    // Determine the type of operand
    EpdOp::Type type = EpdOp::OP_NONE;

    for (i = 0; i < m_numEpdOpTypes; i++)
        if (fields[0] == m_epdOpTypes[i].opcode) {
            type = m_epdOpTypes[i].type;
            break;
        }

    if (i == m_numEpdOpTypes) {
        LOGERR << "Unsupported EPD opcode '" << fields[0] << "'";
        return false;
    }

    if (numFields == 1 && type == EpdOp::OP_NONE) {
        // No operand; all done
        op = new EpdOp;
        op->setOpcode(fields[0].c_str());
        op->setOperandNone();
        m_ops.push_back(op);
    } else {
        // One or more operands (each operand creates a new EpdOp)
        for (i = 1; i < numFields; i++) {
            op = new EpdOp;
            op->setOpcode(fields[0].c_str());

            switch (type) {
            case EpdOp::OP_STRING:
                op->setOperandString(fields[i].c_str());
                break;

            case EpdOp::OP_INTEGER:
                integer = strtoll(fields[i].c_str(), &endp, 10);

                if (*endp != '\0') {
                    LOGERR << "Invalid integer '" << fields[i] << "'";
                    delete op;
                    return false;
                }

                op->setOperandInteger(integer);
                break;

            case EpdOp::OP_MOVE:

                if (!move.parse(m_position, fields[i])) {
                    LOGERR << "Failed to parse move '" << fields[i] << "'";
                    delete op;
                    return false;
                }

                op->setOperandMove(move);
                break;

            case EpdOp::OP_EVAL:

                if (fields[i] == "+-")
                    op->setOperandEval(EpdOp::EVAL_W_DECISIVE_ADV);
                else if (fields[i] == "+/-")
                    op->setOperandEval(EpdOp::EVAL_W_CLEAR_ADV);
                else if (fields[i] == "+/=")
                    op->setOperandEval(EpdOp::EVAL_W_SLIGHT_ADV);
                else if (fields[i] == "=")
                    op->setOperandEval(EpdOp::EVAL_EQUAL);
                else if (fields[i] == "=/+")
                    op->setOperandEval(EpdOp::EVAL_B_SLIGHT_ADV);
                else if (fields[i] ==  "-/+")
                    op->setOperandEval(EpdOp::EVAL_B_CLEAR_ADV);
                else if (fields[i] == "-+")
                    op->setOperandEval(EpdOp::EVAL_B_DECISIVE_ADV);
                else {
                    LOGERR << "Invalid evaluation '" << fields[i] << "'";
                    delete op;
                    return false;
                }

                break;

            case EpdOp::OP_NONE:
                op->setOperandNone();
                break;
                //            default:
                //                break;
            }

            m_ops.push_back(op);
        }
    }

    return true;
}

unsigned Epd::split(const string &line, vector<string> &parts) {
#define ENDOFPART(c) ((parts.size() <= 3 && isspace(c)) || (parts.size() >= 4 && (c) == ';'))

    char inQuotes = '\0';
    const char *start, *end;

    parts.clear();

    start = end = line.c_str();

    while (*end != '\0') {
        while (*start != '\0' && isspace(*start))
            start++;

        if (*start == '\0')
            break;

        end = start;

        while (*end != '\0' && !ENDOFPART(*end)) {
            if (*end == '\'' || *end == '"') {
                inQuotes = *end++;

                while (*end != '\0' && *end != inQuotes)
                    end++;

                if (*end == inQuotes)
                    end++;
            } else {
                end++;
            }
        }

        if (end > start)
            parts.push_back(string(start, end - start));

        start = end + 1;
    }

#if 0
    LOGDBG << "line='" << line << "' parts.size()=" << parts.size();

    for (unsigned i = 0; i < parts.size(); i++)
        LOGDBG << "parts[" << i << "]='" << parts[i] << "'";
#endif // 0

    return (unsigned)parts.size();
}

const EpdOp *Epd::findOp(const string &opcode, vector<EpdOp *>::const_iterator startOp) {
    for (; startOp != m_ops.end(); startOp++) {
        EpdOp *op = *startOp;

        if (op->opcode() == opcode) {
            m_findOpIt = startOp;
            return *startOp;
        }
    }

    m_findOpIt = m_ops.end();
    return 0;
}

bool Epd::hasMoveOps() {
    return findFirstOp("bm") != 0 || findFirstOp("am") != 0;
    // "pm", "pv", etc....
}

bool Epd::checkMoveOps(const Move &move) {
    const EpdOp *op;

    // Check the move against any "bm" operations; any hit means success
    op = findFirstOp("bm");

    if (op) {
        do {
            if (move.equals(op->operandMove()))
                return true;

            op = findNextOp("bm");
        } while (op);

        return false;
    }

    // Check the move against any "am" operations; any hit means a failure
    op = findFirstOp("am");

    while (op) {
        if (move.equals(op->operandMove()))
            return false;

        op = findNextOp("am");
    }

    // All tests must have passed...
    return true;
}

bool Epd::hasEvalOps() {
    return findFirstOp("eval") != 0;
}

string Epd::dump() const {
    ostringstream oss;

    oss << m_position;

    for (vector<EpdOp *>::const_iterator it = m_ops.begin(); it != m_ops.end(); ++it) {
        EpdOp *op = *it;
        oss << op->opcode();
        oss << ' ';

        switch (op->type()) {
        case EpdOp::OP_STRING:
            oss << op->operandString();
            break;

        case EpdOp::OP_INTEGER:
            oss << dec << op->operandInteger();
            break;

        case EpdOp::OP_MOVE:
            oss << op->operandMove().san(m_position);
            break;

        case EpdOp::OP_EVAL:
            oss << op->formatEval();
            break;

        case EpdOp::OP_NONE:
            break;
            //        default:
            //            ASSERT(false);
            //            break;
        }

        oss << '\n';
    }

    return oss.str();
}

ostream &operator<<(ostream &os, const Epd &epd) {
    os << epd.dump();
    return os;
}

// ==========================================================================
// class EpdFile
// ==========================================================================

const char *EpdFile::m_classname = "EpdFile";

EpdFile::~EpdFile() {
    freeEpds();
}

void EpdFile::init() {
}

void EpdFile::freeEpds() {
    for (vector<Epd *>::iterator it = m_epds.begin(); it != m_epds.end(); ++it) {
        Epd *epd = *it;
        delete epd;
    }

    m_epds.clear();
}

bool EpdFile::read(const string &filename) {
    ifstream epdfile(filename.c_str());

    if (!epdfile.is_open()) {
        LOGERR << "Failed to open EPD file '" << filename <<
            "': " << strerror(errno) << " (" << errno << ")";
        return false;
    }

    bool retval = true;
    string buffer;
    unsigned lineNum = 0;

    do {
        getline(epdfile, buffer);

        if (epdfile.bad() || epdfile.fail()) {
            epdfile.clear();
            break;
        }

        lineNum++;

        if (buffer.empty())
            continue;

        Epd *e = new Epd;

        if (!e->parse(buffer.c_str(), lineNum)) {
            delete e;
            retval = false;
            break;
        }

        m_epds.push_back(e);
    } while (retval);

    epdfile.close();

    if (!retval)
        freeEpds();

    return retval;
}
}   // namespace ChessCore

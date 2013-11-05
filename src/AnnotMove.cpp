//
// ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
//
// AnnotMove.cpp: AnnotMove class implementation.
//

#include <ChessCore/AnnotMove.h>
#include <ChessCore/Position.h>
#include <ChessCore/Log.h>
#include <string.h>
#include <errno.h>
#include <fstream>
#include <sstream>

using namespace std;

namespace ChessCore {
// EMBEDDED_VARIATIONS how variations are generated in dumpLine()
#undef EMBEDDED_VARIATIONS

const char *AnnotMove::m_classname = "AnnotMove";

AnnotMove::AnnotMove() :
    Move(),
    m_prev(0),
    m_next(0),
    m_mainline(0),
    m_variation(0),
    m_priorPosition(0),
    m_posHash(0ULL),
    m_preAnnot(),
    m_postAnnot()
{
    clearNags();
}

AnnotMove::AnnotMove(const Move &other) :
    Move(other),
    m_prev(0),
    m_next(0),
    m_mainline(0),
    m_variation(0),
    m_priorPosition(0),
    m_posHash(0ULL),
    m_preAnnot(),
    m_postAnnot()
{
    clearNags();
}

AnnotMove::AnnotMove(const Move &other, uint64_t posHash) :
    Move(other),
    m_prev(0),
    m_next(0),
    m_mainline(0),
    m_variation(0),
    m_priorPosition(0),
    m_posHash(posHash),
    m_preAnnot(),
    m_postAnnot()
{
    clearNags();
}

AnnotMove::AnnotMove(const AnnotMove &other) :
    Move(other.move()),
    m_prev(0),
    m_next(0),
    m_mainline(0),
    m_variation(0),
    m_priorPosition(0),
    m_posHash(other.m_posHash),
    m_preAnnot(other.m_preAnnot),
    m_postAnnot(other.m_postAnnot)
{
    memcpy(m_nags, other.m_nags, sizeof(m_nags));
    if (other.m_priorPosition)
        setPriorPosition(*other.m_priorPosition);
}

AnnotMove::AnnotMove(const AnnotMove *other) :
    Move(other->move()),
    m_prev(0),
    m_next(0),
    m_mainline(0),
    m_variation(0),
    m_priorPosition(0),
    m_posHash(other->m_posHash),
    m_preAnnot(other->m_preAnnot),
    m_postAnnot(other->m_postAnnot)
{
    memcpy(m_nags, other->m_nags, sizeof(m_nags));
    if (other->m_priorPosition)
        setPriorPosition(*other->m_priorPosition);
}

AnnotMove::~AnnotMove() {
    clearPriorPosition();
}

void AnnotMove::deepDelete(AnnotMove *amove) {
    if (amove == 0)
        return;

    deepDelete(amove->m_variation);
    deepDelete(amove->m_next);
    delete amove;
}

AnnotMove *AnnotMove::deepCopy(const AnnotMove *amove) {
    AnnotMove *first = 0;

    while (amove) {
        AnnotMove *newMove = new AnnotMove(amove);

        if (first == 0)
            first = newMove;
        else
            first->addMove(newMove);

        if (amove->m_variation) {
            AnnotMove *newVar = AnnotMove::deepCopy(amove->m_variation);
            newMove->m_variation = newVar;
            newVar->m_mainline = newMove;
        }

        amove = amove->m_next;
    }

    return first;
}

void AnnotMove::removeVariations(AnnotMove *amove, vector<AnnotMove *> *removed /*=0*/) {
    while (amove) {
        if (amove->m_variation) {
            if (removed)
                removed->push_back(amove->m_variation);
            else
                deepDelete(amove->m_variation);

            amove->m_variation = 0;
        }

        amove = amove->m_next;
    }
}

AnnotMove *AnnotMove::makeMoveList(const vector<Move> &moves) {
    if (moves.size() == 0)
        return 0;

    AnnotMove *first = 0;

    for (vector<Move>::const_iterator it = moves.begin(); it != moves.end(); ++it) {
        Move m = *it;
        AnnotMove *newMove = new AnnotMove(m);

        if (first == 0)
            first = newMove;
        else
            first->addMove(newMove);
    }

    return first;
}

void AnnotMove::addMove(AnnotMove *amove) {
    ASSERT(amove->m_prev == 0);
    AnnotMove *m = lastMove();
    m->m_next = amove;
    amove->m_prev = m;
}

void AnnotMove::addVariation(AnnotMove *variation, bool atEnd /*=true*/) {
    ASSERT(variation->m_mainline == 0);

    if (atEnd) {
        AnnotMove *m = 0;

        for (m = this; m->m_variation; m = m->m_variation) {
        }

        m->m_variation = variation;
        variation->m_mainline = m;
    } else {
        if (m_variation) {
            m_variation->m_mainline = variation;
            variation->m_variation = m_variation;
        }

        m_variation = variation;
        variation->m_mainline = this;
    }
}

bool AnnotMove::promote() {
    AnnotMove *mainline = m_mainline;

    if (mainline == 0)
        return false;

    if (m_variation)
        m_variation->m_mainline = mainline;

    mainline->m_variation = m_variation;
    m_variation = mainline;
    m_mainline = mainline->m_mainline;

    if (m_mainline)
        m_mainline->m_variation = this;

    mainline->m_mainline = this;
    m_prev = mainline->m_prev;
    mainline->m_prev = 0;

    if (m_prev)
        m_prev->m_next = this;

    ASSERT(m_priorPosition == 0);

    if (mainline->m_priorPosition) {
        m_priorPosition = mainline->m_priorPosition;
        mainline->m_priorPosition = 0;
    }

    return true;
}

bool AnnotMove::demote() {
    AnnotMove *variation = m_variation;

    if (variation == 0)
        return false;

    if (m_mainline)
        m_mainline->m_variation = variation;

    variation->m_mainline = m_mainline;
    m_mainline = variation;
    m_variation = variation->m_variation;

    if (m_variation)
        m_variation->m_mainline = this;

    variation->m_variation = this;
    variation->m_prev = m_prev;
    m_prev = 0;

    if (variation->m_prev)
        variation->m_prev->m_next = variation;

    if (m_priorPosition) {
        variation->m_priorPosition = m_priorPosition;
        m_priorPosition = 0;
    }

    return true;
}

bool AnnotMove::promoteToMainline(unsigned *count /*=0*/) {
    ASSERT(m_mainline);
    ASSERT(m_prev == 0);

    unsigned c = 0;

    while (promote())
        c++;

    if (count)
        *count = c;

    return true;
}

void AnnotMove::replaceNext(AnnotMove *amove, AnnotMove **oldNext /*=0*/) {
    if (m_next && oldNext)
        *oldNext = m_next;
    else
        deepDelete(m_next);

    m_next = amove;

    if (amove)
        amove->m_prev = this;
}

AnnotMove *AnnotMove::remove(bool unlinkOnly /*=false*/) {
    AnnotMove *prev = m_prev;
    AnnotMove *mainline = m_mainline;

    if (!unlinkOnly)
        deepDelete(this);

    if (prev)
        prev->m_next = 0;

    if (mainline)
        mainline->m_variation = 0;

    return prev;
}

bool AnnotMove::restore(AnnotMove **replaced /*=0*/) {
    if (m_prev) {
        if (m_mainline) {
            logwrn("Move has m_prev and m_mainline set");
            return false;
        }

        if (m_prev->m_next)
            if (replaced)
                *replaced = m_prev->m_next;

        m_prev->m_next = this;
    } else if (m_mainline) {
        if (m_mainline->m_variation)
            if (replaced)
                *replaced = m_mainline->m_variation;

        m_mainline->m_variation = this;
    } else {
        logwrn("Move has neither m_mainline nor m_prev set");
        return false;
    }

    return true;
}

void AnnotMove::setPriorPosition(const Position &priorPosition) {
    clearPriorPosition();
    m_priorPosition = new Position(priorPosition);
}

void AnnotMove::clearPriorPosition() {
    delete m_priorPosition;
    m_priorPosition = 0;
}

bool AnnotMove::hasAnnotations() const {
    return
        !m_preAnnot.empty() ||
        !m_postAnnot.empty();
}

bool AnnotMove::lineHasAnnotations() const {
    const AnnotMove *m = this;

    while (m) {
        if (m->hasAnnotations() || m->nagCount() > 0)
            return true;

        if (m->m_variation && (m->m_variation->hasAnnotations() || m->m_variation->nagCount() > 0))
            return true;

        m = m->m_next;
    }

    return false;
}

const AnnotMove *AnnotMove::topMainline() const {
    const AnnotMove *m = this;

    while (m->m_mainline)
        m = m->m_mainline;

    return m;
}

AnnotMove *AnnotMove::topMainline() {
    AnnotMove *m = this;

    while (m->m_mainline)
        m = m->m_mainline;

    return m;
}

void AnnotMove::removeAnnotations(SavedAnnotations *savedAnnotations /*=0*/) {
    if (savedAnnotations) {
        savedAnnotations->move = this;
        savedAnnotations->preAnnot = m_preAnnot;
        savedAnnotations->postAnnot = m_postAnnot;
        memcpy(savedAnnotations->nags, m_nags, sizeof(m_nags));
    }

    m_preAnnot.clear();
    m_postAnnot.clear();
    clearNags();
}

void AnnotMove::removeLineAnnotations(vector<SavedAnnotations> *removed /*=0*/) {
    AnnotMove *m = this;
    SavedAnnotations savedAnnotations;

    while (m) {
        if (m->m_variation)
            m->m_variation->removeLineAnnotations(removed);

        if (m->hasAnnotations()) {
            m->removeAnnotations(removed ? &savedAnnotations : 0);

            if (removed)
                removed->push_back(savedAnnotations);
        }

        m = m->m_next;
    }
}

void AnnotMove::clearNags() {
    memset(m_nags, NAG_NONE, sizeof(m_nags));
}

unsigned AnnotMove::nags(Nag nags[STORED_NAGS]) const {
    unsigned count = 0, i;

    for (i = 0; i < STORED_NAGS; i++)
        nags[i] = NAG_NONE;

    for (i = 0; i < STORED_NAGS; i++)
        if (m_nags[i] != NAG_NONE)
            nags[count++] = m_nags[i];

    return count;
}

unsigned AnnotMove::setNags(const Nag *nags) {
    unsigned count = 0;

    clearNags();

    for (unsigned i = 0; i < STORED_NAGS; i++)
        if (addNag(nags[i]))
            count++;

    return count;
}

bool AnnotMove::addNag(Nag nag) {
    if (nag != NAG_NONE && !hasNag(nag)) {
        for (unsigned i = 0; i < STORED_NAGS; i++) {
            if (m_nags[i] == NAG_NONE) {
                m_nags[i] = nag;
                return true;
            }
        }
    }

    return false;
}

bool AnnotMove::hasNag(Nag nag) const {
    for (unsigned i = 0; i < STORED_NAGS; i++)
        if (m_nags[i] == (uint8_t)nag)
            return true;
    return false;
}

unsigned AnnotMove::nagCount() const {
    unsigned count = 0;
    for (unsigned i = 0; i < STORED_NAGS; i++)
        if (m_nags[i] != NAG_NONE)
            count++;
    return count;
}

void AnnotMove::saveAnnotations(SavedAnnotations &savedAnnotations) const {
    savedAnnotations.move = const_cast<AnnotMove *> (this);
    savedAnnotations.preAnnot = m_preAnnot;
    savedAnnotations.postAnnot = m_postAnnot;
    memcpy(savedAnnotations.nags, m_nags, sizeof(m_nags));
}

void AnnotMove::restoreAnnotations(const SavedAnnotations &savedAnnotations) {
    //ASSERT(savedAnnotations.move == this);
    m_preAnnot = savedAnnotations.preAnnot;
    m_postAnnot = savedAnnotations.postAnnot;
    memcpy(m_nags, savedAnnotations.nags, sizeof(m_nags));
}

AnnotMove *AnnotMove::firstMove() {
    AnnotMove *m = this;
    while (m->m_prev)
        m = m->m_prev;
    return m;
}

const AnnotMove *AnnotMove::firstMove() const {
    const AnnotMove *m = this;
    while (m->m_prev)
        m = m->m_prev;
    return m;
}

AnnotMove *AnnotMove::lastMove() {
    AnnotMove *m = this;
    while (m->m_next)
        m = m->m_next;
    return m;
}

const AnnotMove *AnnotMove::lastMove() const {
    const AnnotMove *m = this;
    while (m->m_next)
        m = m->m_next;
    return m;
}

AnnotMove *AnnotMove::previousVariation() {
    AnnotMove *m = firstMove();
    if (m->m_mainline)
        return m->m_mainline;
    return 0;
}

const AnnotMove *AnnotMove::previousVariation() const {
    const AnnotMove *m = firstMove();
    if (m->m_mainline)
        return m->m_mainline;
    return 0;
}

AnnotMove *AnnotMove::nextVariation() {
    AnnotMove *m = firstMove();
    if (m->m_variation)
        return m->m_variation;
    return 0;
}

const AnnotMove *AnnotMove::nextVariation() const {
    const AnnotMove *m = firstMove();
    if (m->m_variation)
        return m->m_variation;
    return 0;
}

unsigned AnnotMove::variationLevel() const {
    unsigned count = 0;
    const AnnotMove *m = this;

    do {
        m = m->firstMove();

        if (m->m_mainline) {
            count++;

            while (m->m_mainline)
                m = m->m_mainline;
        }
    } while (m->m_prev);

    return count;
}

bool AnnotMove::lineHasVariations() const {
    const AnnotMove *m = this;
    while (m) {
        if (m->m_variation)
            return true;
        m = m->m_next;
    }

    return false;
}

bool AnnotMove::isDescendant(const AnnotMove *amove) const {
    const AnnotMove *m = amove;
    while (m) {
        if (m == this)
            return true;

        if (m->m_variation && isDescendant(m->m_variation))
            return true;

        m = m->m_next;
    }

    return false;
}

bool AnnotMove::isDirectVariation(const AnnotMove *amove) const {
    const AnnotMove *m = amove;
    while (m) {
        if (m == this)
            return true;
        m = m->m_mainline;
    }

    return false;
}

unsigned AnnotMove::count(const AnnotMove *amove) {
    unsigned count = 0;
    while (amove) {
        count++;
        amove = amove->m_next;
    }

    return count;
}

void AnnotMove::count(const AnnotMove *amove, unsigned &moveCount, unsigned &variationCount, unsigned &symbolCount,
                      unsigned &annotationsLength) {
    while (amove) {
        moveCount++;

        unsigned nagCount = amove->nagCount();

        if (nagCount > 0)
            symbolCount += nagCount + 1; // Allow space for trailing NAG_NONE

        const string &preAnnot = amove->preAnnot();

        if (!preAnnot.empty())
            annotationsLength += (unsigned)preAnnot.length() + 1;

        const string &postAnnot = amove->postAnnot();

        if (!postAnnot.empty())
            annotationsLength += (unsigned)postAnnot.length() + 1;

        if (amove->variation()) {
            variationCount++;
            count(amove->variation(), moveCount, variationCount, symbolCount, annotationsLength);
        }

        amove = amove->next();
    }
}

unsigned AnnotMove::countRepeatedPositions(const AnnotMove *amove) {
    unsigned count = 0;
    uint64_t hash = amove->posHash();

    ASSERT(hash);

    while (amove) {
        if (amove->posHash() == hash)
            count++;

        while (amove->mainline())
            amove = amove->mainline();

        amove = amove->prev();
    }

    return count;
}

bool AnnotMove::writeToDotFile(const AnnotMove *line, const std::string &filename) {
    ofstream ofs(filename.c_str(), ios::out | ios::trunc);

    if (!ofs.is_open()) {
        logerr("Failed to open file '%s': %s", filename.c_str(), strerror(errno));
        return false;
    }

    ofs << "digraph Moves {" << endl;
    ofs << "  rankdir=TB;" << endl;

    if (!writeToDotFile(line, ofs))
        return false;

    ofs << "}" << endl;

    ofs.close();

    return true;
}

bool AnnotMove::writeToDotFile(const AnnotMove *line, ostream &os) {
    const AnnotMove *m, *oldm;
    string style;

    // Influence the generated graph by generating all the mainline nodes first
    for (m = line, oldm = 0; m; m = m->m_next) {
        os << "  N" << m << " [label=<" << m->coord() << "<BR/><FONT POINT-SIZE=\"8\">" << m << "</FONT>>];"
           << endl;

        if (oldm) {
            style = m->m_prev == oldm ? "filled" : "dotted";
            os << "  N" << oldm << " -> N" << m << " [style=" << style << ", color=black];" << endl;
        }

        oldm = m;
    }

    // And then adding the variations afterwards
    for (m = line; m; m = m->m_next) {
        if (m->m_variation) {
            if (!writeToDotFile(m->m_variation, os))
                return false;

            style = m->m_variation->m_mainline && m->m_variation->m_mainline == m ? "filled" : "dotted";

            os << "  N" << m << " -> N" << m->m_variation << " [style=" << style << ", color=blue];" << endl;
        }
    }

    return true;
}

string AnnotMove::dumpLine() const {
    stringstream ss;

    for (const AnnotMove *amove = this; amove; amove = amove->next()) {
        if (amove != this)
            ss << ' ';

        ss << amove->dump(false);

        if (amove->variation()) {
            if (amove->mainline() == 0) {
                // Top of variation tree
                for (const AnnotMove *m = amove->variation(); m; m = m->variation()) {
                    ss << " (";
                    ss << m->dumpLine();
                    ss << ")";
                }
            }
        }
    }

    return ss.str();
}


std::ostream &operator<<(std::ostream &os, const AnnotMove &move) {
    os << move.dump();
    return os;
}

}   // namespace ChessCore

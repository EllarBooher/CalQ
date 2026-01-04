#pragma once

#include "math/number.h"
#include "types.h"
#include <string>
#include <vector>

namespace calqmath
{
struct GraphChunkDebug
{
    std::string beginX;
    std::string endX;

    std::string beginY;

    // This should NEVER change
    std::string middleXDelta;

    size_t middleYCount;
    ptrdiff_t expectedMiddleYCount;

    std::string endY;
};

/*
 * Each chunk is a line-strip primitive for graphing, representing a
 * continous portion of the function.
 *
 * Points are snapped to a grid besides the endpoints which are exact depending
 * on discontinuities, domain restrictions, etc. The chunk records these
 * endpoints, with the remaining x values being explicit.
 *
 * This simplifies binary operations between chunks, which is the fundamental
 * operation of composing the final Curve for an Expression.
 */
struct GraphChunk
{
    Scalar beginX;
    // If true, indicates that the begin-boundary is excluded from the interval.
    bool beginIsOpen;
    Scalar beginY;

    // This should NEVER change
    Scalar gridXDelta;

    ptrdiff_t gridIdxBegin;
    ptrdiff_t gridIdxEnd;
    std::vector<Scalar> gridY;

    Scalar endX;
    // If true, indicates that the end-boundary is excluded from the interval.
    bool endIsOpen;
    Scalar endY;

    static auto isValid(GraphChunk const&) -> bool;
    static auto isPointLike(GraphChunk const&) -> bool;
    static auto isCompatible(GraphChunk const&, GraphChunk const&) -> bool;
    static auto expectedMiddleYCount(GraphChunk const&) -> size_t;

#ifdef CALQ_DEBUG
    /* Optional spot to store debug representation of Chunk. */
    GraphChunkDebug debug{};

    static void setDebug(calqmath::GraphChunk& chunk);
#endif
};

struct GraphCurve
{
    std::vector<GraphChunk> chunks;

    static auto generateUnitLine(
        Scalar const& min, Scalar const& max, Scalar const& middleXDelta
    ) -> GraphCurve;
};

auto mergeCurves(GraphCurve const&, GraphCurve const&, BinaryOp) -> GraphCurve;
} // namespace calqmath

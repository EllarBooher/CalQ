#pragma once

#include "math/number.h"
#include "types.h"
#include <string>

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
    Scalar endX;

    Scalar beginY;

    // This should NEVER change
    Scalar middleXDelta;

    std::vector<Scalar> middleY;

    Scalar endY;

    static auto isValid(GraphChunk const&) -> bool;
    static auto isCompatible(GraphChunk const&, GraphChunk const&) -> bool;

#ifdef CALQ_DEBUG
    /* Optional spot to store debug representation of Chunk. */
    GraphChunkDebug debug;

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

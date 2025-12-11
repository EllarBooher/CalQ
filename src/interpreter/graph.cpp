#include "graph.h"

#include "math/functions.h"
#include <cassert>
#include <optional>
#include <utility>

auto calqmath::GraphChunk::isValid(GraphChunk const& chunk) -> bool
{
    if (chunk.endX < chunk.beginX)
    {
        return false;
    }

    auto const expectedMiddleYCount{
        (Functions::floor(chunk.endX / chunk.middleXDelta)
         - Functions::floor(chunk.beginX / chunk.middleXDelta))
            .toUnsignedInt()
    };

    [[maybe_unused]]
    constexpr double SANE_SIZE_LIMIT{10000};

    assert(
        expectedMiddleYCount < SANE_SIZE_LIMIT
        && "GraphChunk size limit reached"
    );

    return chunk.middleY.size() == expectedMiddleYCount;
}

auto calqmath::GraphChunk::isCompatible(
    GraphChunk const& first, GraphChunk const& second
) -> bool
{
    return first.middleXDelta == second.middleXDelta;
}

namespace
{
using calqmath::Scalar, calqmath::BinaryOp;
auto doMath(Scalar const& left, Scalar const& right, BinaryOp const binaryOp)
    -> Scalar
{
    switch (binaryOp)
    {
    case BinaryOp::Plus:
    {
        return left + right;
    }
    case BinaryOp::Minus:
    {
        return left - right;
    }
    case BinaryOp::Multiply:
    {
        return left * right;
    }
    case BinaryOp::Divide:
    {
        return left / right;
    }
    }

    std::unreachable();
}

struct SampleInterval
{
    Scalar beginX;
    Scalar endX;

    Scalar beginY;
    Scalar endY;

    Scalar sampleX;
};
auto sampleLineUnchecked(SampleInterval const& interval) -> Scalar
{
    auto const slope = (interval.endY - interval.beginY);
    return slope * (interval.sampleX - interval.beginX) + interval.beginY;
}

using calqmath::GraphChunk, calqmath::Functions;
// Sample a GraphChunk, with bounds checking and handling sampling the
// non-uniform edges vs sampling the uniform middle.
auto sampleInterpolated(GraphChunk const& chunk, Scalar const& sampleX)
    -> std::optional<Scalar>
{
    if (sampleX < chunk.beginX || sampleX > chunk.endX)
    {
        return std::nullopt;
    }

    auto const isPointlike{chunk.beginX == chunk.endX};
    if (isPointlike)
    {
        assert(chunk.endY == chunk.beginY);
        return chunk.beginY;
    }

    ptrdiff_t const gridIdxBegin{
        Functions::floor(chunk.beginX / chunk.middleXDelta).toSignedInt() + 1
    };
    ptrdiff_t const gridIdxEnd{
        Functions::ceil(chunk.endX / chunk.middleXDelta).toSignedInt()
    };

    assert(gridIdxEnd - gridIdxBegin >= 0);
    auto const gridCount{static_cast<size_t>(gridIdxEnd - gridIdxBegin)};

    auto const hasMiddle{gridCount > 0};
    if (!hasMiddle)
    {
        return sampleLineUnchecked(
            {.beginX = chunk.beginX,
             .endX = chunk.endX,
             .beginY = chunk.beginY,
             .endY = chunk.endY,
             .sampleX = sampleX}
        );
    }

    ptrdiff_t const gridIdx{
        Functions::floor(sampleX / chunk.middleXDelta).toSignedInt()
    };

    assert(chunk.middleY.size() == gridCount);

    if (gridIdx < gridIdxBegin)
    {
        return sampleLineUnchecked(
            {.beginX = chunk.beginX,
             .endX = Scalar{gridIdxBegin} * chunk.middleXDelta,
             .beginY = chunk.beginY,
             .endY = chunk.middleY.front(),
             .sampleX = sampleX}
        );
    }

    if (gridIdx >= gridIdxEnd - 1)
    {
        return sampleLineUnchecked(
            {.beginX = Scalar{gridIdxEnd - 1} * chunk.middleXDelta,
             .endX = chunk.endX,
             .beginY = chunk.middleY.back(),
             .endY = chunk.endY,
             .sampleX = sampleX}
        );
    }

    assert(gridIdx >= gridIdxBegin);
    auto const middleIdx{static_cast<size_t>(gridIdx - gridIdxBegin)};

    return sampleLineUnchecked(
        {.beginX = Scalar{gridIdx} * chunk.middleXDelta,
         .endX = Scalar{gridIdx + 1} * chunk.middleXDelta,
         .beginY = chunk.middleY.at(middleIdx),
         .endY = chunk.middleY.at(middleIdx + 1),
         .sampleX = sampleX}
    );
}

auto mergeChunks(
    GraphChunk const& left, GraphChunk const& right, BinaryOp const binaryOp
) -> std::optional<calqmath::GraphChunk>
{
    assert(GraphChunk::isValid(left));
    assert(GraphChunk::isValid(right));
    assert(GraphChunk::isCompatible(left, right));

    auto const minX{Scalar::max(left.beginX, right.beginX)};
    auto const maxX{Scalar::min(left.endX, right.endX)};

    auto const comparison{minX <=> maxX};
    if (comparison > 0)
    {
        // No intersection
        return std::nullopt;
    }

    assert(comparison < 0 && "Unimplemented case");

    std::optional<GraphChunk> result{std::in_place};
    GraphChunk& chunk{result.value()};
    chunk.middleXDelta = left.middleXDelta;

    chunk.beginX = minX;
    chunk.endX = maxX;

    // TODO: handle NaN/Inf
    chunk.beginY = ::doMath(
        ::sampleInterpolated(left, chunk.beginX).value(),
        ::sampleInterpolated(right, chunk.beginX).value(),
        binaryOp
    );
    chunk.endY = ::doMath(
        ::sampleInterpolated(left, chunk.endX).value(),
        ::sampleInterpolated(right, chunk.endX).value(),
        binaryOp
    );

    // The global grid index of the left-most "middle" point of each chunk
    ptrdiff_t const gridOffsetLeft{
        Functions::floor(left.beginX / left.middleXDelta).toSignedInt() + 1
    };
    ptrdiff_t const gridOffsetRight(
        Functions::floor(right.beginX / right.middleXDelta).toSignedInt() + 1
    );

    /*
     * Doing floor / ceil and shifting by 1 is better than ceil / floor.
     * This is because it captures the case that beginX and endX are on the
     * grid.
     * When beginX and endX are on the grid already, we want to skip them in the
     * middle.
     */
    ptrdiff_t const gridIdxBegin{
        Functions::floor(chunk.beginX / chunk.middleXDelta).toSignedInt() + 1
    };
    ptrdiff_t const gridIdxEnd{
        Functions::ceil(chunk.endX / chunk.middleXDelta).toSignedInt() - 1
    };

    assert(gridIdxEnd > gridIdxBegin);
    assert(gridIdxBegin >= gridOffsetLeft);
    assert(gridIdxBegin >= gridOffsetRight);

    chunk.middleY.reserve((gridIdxEnd - gridIdxBegin) + 1);

    for (ptrdiff_t gridIdx = gridIdxBegin; gridIdx <= gridIdxEnd; gridIdx += 1)
    {
        auto const& leftY{left.middleY[gridIdx - gridOffsetLeft]};
        auto const& rightY{right.middleY[gridIdx - gridOffsetRight]};

        chunk.middleY.emplace_back(::doMath(leftY, rightY, binaryOp));
    }

    assert(GraphChunk::isValid(chunk));

#ifdef CALQ_DEBUG
    GraphChunk::setDebug(chunk);
#endif

    return chunk;
}
} // namespace

auto calqmath::mergeCurves(
    GraphCurve const& left, GraphCurve const& right, BinaryOp binaryOp
) -> calqmath::GraphCurve
{
    GraphCurve output{};

    for (auto const& leftChunk : left.chunks)
    {
        for (auto const& rightChunk : right.chunks)
        {
            auto chunk{::mergeChunks(leftChunk, rightChunk, binaryOp)};
            if (!chunk.has_value())
            {
                continue;
            }

            output.chunks.emplace_back(std::move(chunk).value());
        }
    }

    // TODO: cleanup chunks via sort, merge, glue false-positive discontinuities
    // etc

    return output;
}

#ifdef CALQ_DEBUG
void calqmath::GraphChunk::setDebug(GraphChunk& chunk)
{
    chunk.debug = {
        .beginX = chunk.beginX.toString(),
        .endX = chunk.endX.toString(),
        .beginY = chunk.beginY.toString(),
        .middleXDelta = chunk.middleXDelta.toString(),
        .middleYCount = chunk.middleY.size(),
        .endY = chunk.endY.toString(),
    };
}
#endif

auto calqmath::GraphCurve::generateUnitLine(
    const Scalar& min, const Scalar& max, const Scalar& middleXDelta
) -> calqmath::GraphCurve
{
    GraphCurve curve{.chunks{GraphChunk{
        .beginX{min},
        .endX{max},
        .beginY{min},
        .middleXDelta{middleXDelta},
        .middleY{},
        .endY{max},
#ifdef CALQ_DEBUG
        .debug{}
#endif
    }}};

    for (auto& chunk : curve.chunks)
    {
        ptrdiff_t const gridIdxBegin{
            calqmath::Functions::floor(chunk.beginX / chunk.middleXDelta)
                .toSignedInt()
            + 1
        };
        ptrdiff_t const gridIdxEnd{
            calqmath::Functions::ceil(chunk.endX / chunk.middleXDelta)
                .toSignedInt()
            - 1
        };

        for (ptrdiff_t gridIdx = gridIdxBegin; gridIdx <= gridIdxEnd; gridIdx++)
        {
            chunk.middleY.push_back(
                calqmath::Scalar{gridIdx} * chunk.middleXDelta
            );
        }

#ifdef CALQ_DEBUG
        GraphChunk::setDebug(chunk);
#endif

        assert(GraphChunk::isValid(chunk));
    }

    return curve;
}

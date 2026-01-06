#include "graph.h"

#include "math/functions.h"
#include <cassert>
#include <optional>
#include <ranges>
#include <utility>

auto calqmath::GraphChunk::isValid(GraphChunk const& chunk) -> bool
{
    assert(chunk.endX >= chunk.beginX);

    assert(chunk.gridXDelta > Scalar::zero());

    assert(chunk.gridIdxEnd >= chunk.gridIdxBegin);

    if (chunk.gridIdxEnd > chunk.gridIdxBegin)
    {
        assert(chunk.beginX <= Scalar{chunk.gridIdxBegin} * chunk.gridXDelta);
        assert(chunk.endX >= Scalar{chunk.gridIdxEnd - 1} * chunk.gridXDelta);

        [[maybe_unused]]
        auto const expectedMiddleYCount{chunk.gridIdxEnd - chunk.gridIdxBegin};

        // Arbitrary limit to avoid blowing up memory or compute time
        [[maybe_unused]]
        constexpr double SIZE_LIMIT{10000};

        assert(expectedMiddleYCount < SIZE_LIMIT);
        assert(expectedMiddleYCount >= 0);
        assert(chunk.gridY.size() == static_cast<size_t>(expectedMiddleYCount));
    }
    else
    {
        assert(chunk.gridIdxBegin == 0);
        assert(chunk.gridIdxEnd == 0);

        assert(chunk.gridY.empty());
    }

    return true;
}

auto calqmath::GraphChunk::isPointLike(GraphChunk const& chunk) -> bool
{
    auto const width = chunk.endX - chunk.beginX;
    return width < Scalar{"0.1"} * chunk.gridXDelta;
}

auto calqmath::GraphChunk::isCompatible(
    GraphChunk const& first, GraphChunk const& second
) -> bool
{
    return first.gridXDelta == second.gridXDelta;
}

auto calqmath::GraphChunk::expectedMiddleYCount(GraphChunk const& chunk)
    -> size_t
{
    auto const begin{
        Functions::floor(chunk.beginX / chunk.gridXDelta).toSignedInt() + 1
    };
    auto const end{
        Functions::ceil(chunk.endX / chunk.gridXDelta).toSignedInt()
    };

    return end - begin;
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

    if (GraphChunk::isPointLike(chunk))
    {
        return Scalar{"0.5"} * (chunk.beginY + chunk.endY);
    }

    if (chunk.gridY.empty())
    {
        return sampleLineUnchecked(
            {.beginX = chunk.beginX,
             .endX = chunk.endX,
             .beginY = chunk.beginY,
             .endY = chunk.endY,
             .sampleX = sampleX}
        );
    }

    auto const gridIdx{
        Functions::floor(sampleX / chunk.gridXDelta).toSignedInt()
    };

    if (gridIdx < chunk.gridIdxBegin)
    {
        return sampleLineUnchecked(
            {.beginX = chunk.beginX,
             .endX = Scalar{chunk.gridIdxBegin} * chunk.gridXDelta,
             .beginY = chunk.beginY,
             .endY = chunk.gridY.front(),
             .sampleX = sampleX}
        );
    }

    if (gridIdx >= chunk.gridIdxEnd - 1)
    {
        return sampleLineUnchecked(
            {.beginX = Scalar{chunk.gridIdxEnd - 1} * chunk.gridXDelta,
             .endX = chunk.endX,
             .beginY = chunk.gridY.back(),
             .endY = chunk.endY,
             .sampleX = sampleX}
        );
    }

    return sampleLineUnchecked(
        {.beginX = Scalar{gridIdx} * chunk.gridXDelta,
         .endX = Scalar{gridIdx + 1} * chunk.gridXDelta,
         .beginY = chunk.gridY.at(gridIdx - chunk.gridIdxBegin),
         .endY = chunk.gridY.at(gridIdx - chunk.gridIdxBegin + 1),
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

    auto const beginX{Scalar::max(left.beginX, right.beginX)};
    auto const beginIsOpen = (beginX > left.beginX && left.beginIsOpen)
                          || (beginX > right.beginX && right.beginIsOpen)
                          || (left.beginIsOpen && right.beginIsOpen);

    auto const endX{Scalar::min(left.endX, right.endX)};
    auto const endIsOpen = (endX < left.endX && left.endIsOpen)
                        || (endX < right.endX && right.endIsOpen)
                        || (left.endIsOpen && right.endIsOpen);

    auto const comparison{beginX <=> endX};
    if (comparison > 0 || (comparison == 0 && (beginIsOpen || endIsOpen)))
    {
        // No intersection
        return std::nullopt;
    }

    std::optional<GraphChunk> result{std::in_place};
    GraphChunk& chunk{result.value()};
    chunk.gridXDelta = left.gridXDelta;

    chunk.beginX = beginX;
    chunk.beginIsOpen = beginIsOpen;

    chunk.endX = endX;
    chunk.endIsOpen = endIsOpen;

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

    chunk.gridIdxBegin = std::max(left.gridIdxBegin, right.gridIdxBegin);
    chunk.gridIdxEnd = std::min(left.gridIdxEnd, right.gridIdxEnd);

    if (comparison < 0 && chunk.gridIdxBegin < chunk.gridIdxEnd)
    {
        chunk.gridY.reserve(chunk.gridIdxEnd - chunk.gridIdxBegin);

        for (auto const gridIdx :
             std::views::iota(chunk.gridIdxBegin, chunk.gridIdxEnd))
        {
            auto const& leftY{left.gridY.at(gridIdx - left.gridIdxBegin)};
            auto const& rightY{right.gridY.at(gridIdx - right.gridIdxBegin)};

            chunk.gridY.emplace_back(::doMath(leftY, rightY, binaryOp));
        }
    }
    else
    {
        chunk.gridIdxBegin = 0;
        chunk.gridIdxEnd = 0;
        chunk.gridY = {};
    }

#ifdef CALQ_DEBUG
    GraphChunk::setDebug(chunk);
#endif

    assert(GraphChunk::isValid(chunk));

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
        .middleXDelta = chunk.gridXDelta.toString(),
        .middleYCount = chunk.gridY.size(),
        .expectedMiddleYCount = chunk.gridIdxEnd - chunk.gridIdxBegin,
        .endY = chunk.endY.toString(),
    };
}
#endif

auto calqmath::GraphCurve::generateUnitLine(
    const Scalar& min, const Scalar& max, const Scalar& middleXDelta
) -> calqmath::GraphCurve
{
    ptrdiff_t const gridIdxBegin{
        calqmath::Functions::ceil(min / middleXDelta).toSignedInt()
    };
    ptrdiff_t const gridIdxEnd{
        calqmath::Functions::floor(max / middleXDelta).toSignedInt() + 1
    };

    GraphCurve curve{.chunks{GraphChunk{
        .beginX = min,
        .beginIsOpen = false,
        .beginY = min,
        .gridXDelta = middleXDelta,
        .gridIdxBegin = gridIdxBegin,
        .gridIdxEnd = gridIdxEnd,
        .gridY = {},
        .endX = max,
        .endIsOpen = false,
        .endY = max,
    }}};

    auto& chunk{curve.chunks[0]};
    for (auto gridIdx : std::views::iota(chunk.gridIdxBegin, chunk.gridIdxEnd))
    {
        chunk.gridY.push_back(Scalar{gridIdx} * chunk.gridXDelta);
    }

#ifdef CALQ_DEBUG
    GraphChunk::setDebug(chunk);
#endif

    assert(GraphChunk::isValid(chunk));

    return curve;
}

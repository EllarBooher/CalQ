#include "function_database.h"

#include "math/functions.h"
#include <cassert>
#include <ranges>

[[maybe_unused]]
constexpr char const* RESERVED_FUNCTION_NAME = "x"; // Identifier for variable

namespace
{
namespace graph
{
using calqmath::GraphCurve, calqmath::GraphChunk, calqmath::Functions,
    calqmath::Scalar;

auto id(GraphCurve const& curve) -> GraphCurve { return curve; }

auto floor(GraphCurve const& curve) -> GraphCurve
{
    GraphCurve output{};
    for (auto const& inputChunk : curve.chunks)
    {
        Scalar xFirst = inputChunk.beginX;
        Scalar yFirst = inputChunk.beginY;

        GraphChunk outputChunk;
        outputChunk.beginX = xFirst;
        outputChunk.beginY = Functions::floor(yFirst);
        outputChunk.gridXDelta = inputChunk.gridXDelta;
        outputChunk.gridIdxBegin = inputChunk.gridIdxBegin;
        outputChunk.gridY = {};
        // Finish chunk later, as we iterate the grid for possible
        // discontinuities

        for (auto gridIdx :
             std::views::iota(inputChunk.gridIdxBegin, inputChunk.gridIdxEnd))
        {
            Scalar const xSecond = Scalar{gridIdx} * inputChunk.gridXDelta;
            Scalar const ySecond =
                inputChunk.gridY[gridIdx - inputChunk.gridIdxBegin];

            Scalar const yFirstMapped = Functions::floor(yFirst);
            Scalar const ySecondMapped = Functions::floor(ySecond);

            bool const continuous{ySecondMapped == yFirstMapped};
            if (continuous)
            {
                outputChunk.gridY.push_back(ySecondMapped);
            }
            else
            {
                // solve mx+y0=y as x = (y-y0) / m, for every step this interval
                // fragments into
                Scalar const slopeInverse =
                    (xSecond - xFirst) / (ySecond - yFirst);

                [[maybe_unused]]
                bool const willCreateIntragridChunks =
                    Functions::round(ySecondMapped - yFirstMapped)
                    > Scalar{"1"};
                assert(!willCreateIntragridChunks); // TODO

                auto splitX = slopeInverse * (ySecondMapped - yFirst) + xFirst;
                if (splitX > Scalar{gridIdx} * outputChunk.gridXDelta)
                {
                    splitX = Scalar{gridIdx} * outputChunk.gridXDelta;
                }

                outputChunk.gridIdxEnd = gridIdx;

                outputChunk.endX = splitX;
                outputChunk.endY = yFirstMapped;

#ifdef CALQ_DEBUG
                GraphChunk::setDebug(outputChunk);
#endif

                assert(
                    outputChunk.endX >= Scalar{outputChunk.gridIdxEnd - 1}
                                            * outputChunk.gridXDelta
                );
                if (outputChunk.gridIdxEnd <= outputChunk.gridIdxBegin)
                {
                    outputChunk.gridY = {};
                    outputChunk.gridIdxBegin = 0;
                    outputChunk.gridIdxEnd = 0;
                }

#ifdef CALQ_DEBUG
                GraphChunk::setDebug(outputChunk);
#endif

                assert(GraphChunk::isValid(outputChunk));
                output.chunks.emplace_back(std::move(outputChunk));

                outputChunk.beginX = splitX;
                outputChunk.beginY = ySecondMapped;
                outputChunk.gridXDelta = inputChunk.gridXDelta;
                outputChunk.gridIdxBegin = gridIdx;
                outputChunk.gridY = {ySecondMapped};
            }

            yFirst = ySecond;
            xFirst = xSecond;
        }

        Scalar const ySecond = inputChunk.endY;
        Scalar const xSecond = inputChunk.endX;

        Scalar const yFirstMapped = Functions::floor(yFirst);
        Scalar const ySecondMapped = Functions::floor(ySecond);

        bool const continuous{ySecondMapped == yFirstMapped};
        if (continuous)
        {
            outputChunk.gridIdxEnd = inputChunk.gridIdxEnd;
        }
        else
        {
            // solve mx+y0=y as x = (y-y0) / m, for every step this interval
            // fragments into
            Scalar const slopeInverse = (xSecond - xFirst) / (ySecond - yFirst);

            [[maybe_unused]]
            bool const willCreateIntragridChunks =
                Functions::round(ySecondMapped - yFirstMapped) > Scalar{"1"};
            assert(!willCreateIntragridChunks); // TODO

            auto splitX = slopeInverse * (ySecondMapped - yFirst) + xFirst;
            if (splitX > Scalar{inputChunk.gridIdxEnd} * outputChunk.gridXDelta)
            {
                splitX = Scalar{inputChunk.gridIdxEnd} * outputChunk.gridXDelta;
            }

            outputChunk.gridIdxEnd = inputChunk.gridIdxEnd;
            outputChunk.endX = splitX;
            outputChunk.endY = yFirstMapped;

            assert(
                outputChunk.endX
                >= Scalar{outputChunk.gridIdxEnd - 1} * outputChunk.gridXDelta
            );
            if (outputChunk.gridIdxEnd <= outputChunk.gridIdxBegin)
            {
                outputChunk.gridY = {};
                outputChunk.gridIdxBegin = 0;
                outputChunk.gridIdxEnd = 0;
            }

#ifdef CALQ_DEBUG
            GraphChunk::setDebug(outputChunk);
#endif

            assert(GraphChunk::isValid(outputChunk));
            output.chunks.emplace_back(std::move(outputChunk));

            outputChunk.beginX = splitX;
            outputChunk.beginY = ySecondMapped;
            outputChunk.gridXDelta = inputChunk.gridXDelta;

            outputChunk.gridY = {};
            outputChunk.gridIdxBegin = 0;
            outputChunk.gridIdxEnd = 0;
        }

        outputChunk.endX = xSecond;
        outputChunk.endY = ySecondMapped;

#ifdef CALQ_DEBUG
        GraphChunk::setDebug(outputChunk);
#endif

        if (GraphChunk::isPointLike(outputChunk))
        {
            continue;
        }

        assert(GraphChunk::isValid(outputChunk));
        output.chunks.emplace_back(std::move(outputChunk));
    };
    return output;
};

auto sin(GraphCurve const& curve) -> GraphCurve
{
    GraphCurve output{};
    for (auto const& chunk : curve.chunks)
    {
        auto const transformed =
            std::views::transform(chunk.gridY, Functions::sin);

        GraphChunk outputChunk{
            .beginX = chunk.beginX,
            .beginY = Functions::sin(chunk.beginY),
            .gridXDelta = chunk.gridXDelta,
            .gridIdxBegin = chunk.gridIdxBegin,
            .gridIdxEnd = chunk.gridIdxEnd,
            .gridY = {transformed.begin(), transformed.end()},
            .endX = chunk.endX,
            .endY = Functions::sin(chunk.endY),
        };

#ifdef CALQ_DEBUG
        GraphChunk::setDebug(outputChunk);
#endif

        assert(GraphChunk::isValid(outputChunk));
        output.chunks.emplace_back(std::move(outputChunk));
    };

    return output;
}
} // namespace graph
} // namespace

namespace calqmath
{
FunctionDatabase::FunctionDatabase() = default;

auto FunctionDatabase::createWithDefaults() -> FunctionDatabase
{
    FunctionDatabase result{};

    std::vector<UnaryFunction> const functions = {
        {"id", Functions::id, ::graph::id},
        {"floor", Functions::floor, ::graph::floor},
        {"sin", Functions::sin, ::graph::sin},
        /*
        {"abs", Functions::abs},
        {"ceil", Functions::ceil},
        {"round", Functions::round},
        {"roundeven", Functions::roundeven},
        {"trunc", Functions::trunc},
        {"sqrt", Functions::sqrt},
        {"cbrt", Functions::cbrt},
        {"exp", Functions::exp},
        {"log", Functions::log},
        {"log2", Functions::log2},
        {"erf", Functions::erf},
        {"erfc", Functions::erfc},
        {"gamma", Functions::gamma},
        {"csc", Functions::csc},
        {"asin", Functions::asin},
        {"cos", Functions::cos},
        {"sec", Functions::sec},
        {"acos", Functions::acos},
        {"tan", Functions::tan},
        {"cot", Functions::cot},
        {"atan", Functions::atan},
        {"sinh", Functions::sinh},
        {"cosh", Functions::cosh},
        {"tanh", Functions::tanh},
        {"asinh", Functions::asinh},
        {"acosh", Functions::acosh},
        {"atanh", Functions::atanh},
*/
    };

    result.m_unaryFunctions =
        std::map<std::string, std::shared_ptr<UnaryFunction const>>();

    for (UnaryFunction const& function : functions)
    {
        assert(function.name != RESERVED_FUNCTION_NAME);

        result.m_unaryFunctions[function.name] =
            std::make_shared<UnaryFunction const>(function);
    }

    return result;
}

auto FunctionDatabase::lookup(std::string const& identifier) const
    -> std::optional<std::shared_ptr<UnaryFunction const>>
{
    if (!m_unaryFunctions.contains(identifier))
    {
        return std::nullopt;
    }

    return m_unaryFunctions.at(identifier);
}
} // namespace calqmath

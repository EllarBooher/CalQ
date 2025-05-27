# Populates a cache variable CLANG_TIDY_PATH with the path to the found clang-tidy executable.
# It can then be used as follows:
# set_target_properties(
#     <targets> ...
#     PROPERTIES
#         CXX_CLANG_TIDY
#             "${CLANG_TIDY_PATH}"
# )

option(CLANG_TIDY_ENABLE "Enable clang-tidy - this impacts compilation time." ON)

if (NOT CLANG_TIDY_ENABLE)
    message(STATUS "clang-tidy NOT enabled")
    return()
endif()

find_program(
    CLANG_TIDY_PATH
    NAMES
        "clang-tidy"
    DOC
        "The path of clang-tidy, ran alongside compilation. Linting rules are found in '.clang-tidy'."
)

if (NOT CLANG_TIDY_PATH)
    message(WARNING "clang-tidy enabled - unable to find program, or no path is specified")
endif()

# Populates a cache variable CLANG_TIDY with the path to the found clang-tidy executable.
# It can then be used as follows:
# set_target_properties(
#     <targets> ...
#     PROPERTIES
#         CXX_CLANG_TIDY
#             "${CLANG_TIDY}"
# )

if(CLANG_TIDY)
    # CLANG_TIDY is set to a truthy value, potentially by the user, so we should not override it.
    return()
endif()

find_program(
    CLANG_TIDY
    NAMES
        "clang-tidy"
    DOC
        "The path of clang-tidy, ran alongside compilation. Linting rules are found in '.clang-tidy'."
)

if (NOT CLANG_TIDY)
    message(WARNING "clang-tidy enabled - unable to find program. Try adding a directory containing clang-tidy to your PATH environment variable and unset the CLANG_TIDY cache variable, or manually set it to your desired clang-tidy binary. Set CLANG_TIDY_ENABLE to OFF if you do not want to use clang-tidy.")
endif()

include(FetchContent)

# --------------------------------------------------------------------

# Offline mode (uncomment next line to enable)
# set(FETCHCONTENT_FULLY_DISCONNECTED ON)

# set(FETCHCONTENT_QUIET OFF)

# --------------------------------------------------------------------

# Libraries

# --------------------------------------------------------------------

message(STATUS "FetchContent: whirl-matrix")

FetchContent_Declare(
        whirl-matrix
        GIT_REPOSITORY https://gitlab.com/whirl-framework/whirl-matrix.git
        GIT_TAG stable
)
FetchContent_MakeAvailable(whirl-matrix)

# --------------------------------------------------------------------

message(STATUS "FetchContent: wheels")

FetchContent_Declare(
        wheels
        GIT_REPOSITORY https://gitlab.com/Lipovsky/wheels.git
        GIT_TAG master
)
FetchContent_MakeAvailable(wheels)

# --------------------------------------------------------------------

message(STATUS "FetchContent: await")

FetchContent_Declare(
        await
        GIT_REPOSITORY https://gitlab.com/Lipovsky/await.git
        GIT_TAG master
)
FetchContent_MakeAvailable(await)

# --------------------------------------------------------------------

message(STATUS "FetchContent: fmtlib")

FetchContent_Declare(
        fmtlib
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 8.0.1
)
FetchContent_MakeAvailable(fmtlib)

# --------------------------------------------------------------------

message(STATUS "FetchContent: timber")

FetchContent_Declare(
        timber
        GIT_REPOSITORY https://gitlab.com/Lipovsky/timber.git
        GIT_TAG main
)
FetchContent_MakeAvailable(timber)

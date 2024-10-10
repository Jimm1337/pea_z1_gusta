include(CPM.cmake)

CPMAddPackage(
  NAME fmt
  GITHUB_REPOSITORY fmtlib/fmt
  GIT_TAG 11.0.2
  OPTIONS
    "FMT_DOC OFF"
    "FMT_TEST OFF"
)

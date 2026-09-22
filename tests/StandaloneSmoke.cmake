# Exercise the shipped binary with no sibling assets/configuration and an unrelated cwd.
string(TIMESTAMP RUN_ID "%Y%m%d-%H%M%S")
set(ISOLATED "${TEST_ROOT}/standalone-${RUN_ID}-Tiếng Việt")
file(MAKE_DIRECTORY "${ISOLATED}/program" "${ISOLATED}/working")
file(COPY_FILE "${GAME_EXE}" "${ISOLATED}/program/TaskbarFishing.exe")
foreach(MODE IN ITEMS game background)
    set(ARGS --smoke-test)
    if(MODE STREQUAL "background")
        list(APPEND ARGS --background-demo)
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$ENV{SystemRoot}/System32;$ENV{SystemRoot}"
        "LOCALAPPDATA=${ISOLATED}/profile"
        "${ISOLATED}/program/TaskbarFishing.exe" ${ARGS}
        WORKING_DIRECTORY "${ISOLATED}/working" RESULT_VARIABLE RESULT
        OUTPUT_VARIABLE LOG ERROR_VARIABLE ERR TIMEOUT 25)
    file(WRITE "${ISOLATED}/${MODE}.log" "${LOG}\n${ERR}")
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "Standalone ${MODE} failed: ${RESULT}\n${LOG}\n${ERR}")
    endif()
    set(STARTUP_LOG "${ISOLATED}/profile/TaskbarFishing/startup.log")
    if(NOT EXISTS "${STARTUP_LOG}")
        message(FATAL_ERROR "Standalone ${MODE} did not create its diagnostic log")
    endif()
    file(READ "${STARTUP_LOG}" STARTUP)
    if(NOT STARTUP MATCHES "SMOKE PASS")
        message(FATAL_ERROR "Standalone ${MODE} diagnostic log did not capture successful rendering")
    endif()
endforeach()
if(EXISTS "${ISOLATED}/program/assets" OR EXISTS "${ISOLATED}/program/sprite" OR EXISTS "${ISOLATED}/program/discord_app_id.txt"
    OR EXISTS "${ISOLATED}/program/discord_presence.json")
    message(FATAL_ERROR "Standalone test unexpectedly needed/extracted loose assets")
endif()
message(STATUS "Standalone game and all 24 backgrounds passed: ${ISOLATED}")

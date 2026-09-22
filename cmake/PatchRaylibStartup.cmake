# Taskbar Fishing changes to the pinned raylib 5.5 source. Do not enter rlgl
# without a context, or ask GLFW about a window it failed to create.
function(fishing_patch_raylib relative before after)
    set(path "${raylib_SOURCE_DIR}/${relative}")
    file(READ "${path}" source)
    string(FIND "${source}" "${after}" already_patched)
    if(NOT already_patched EQUAL -1)
        return()
    endif()
    string(FIND "${source}" "${before}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "raylib startup patch no longer matches ${relative}")
    endif()
    string(REPLACE "${before}" "${after}" source "${source}")
    file(WRITE "${path}" "${source}")
endfunction()

fishing_patch_raylib(src/rcore.c
    "    InitPlatform();"
    "    // Taskbar Fishing: failed initialization must not enter rlgl.\n    if (InitPlatform() != 0) return;")
fishing_patch_raylib(src/platforms/rcore_desktop_glfw.c
    "        // After the window was created, determine the monitor that the window manager assigned."
    "        // Taskbar Fishing: never pass a null window to monitor queries.\n        if (platform.handle == NULL)\n        {\n            TRACELOG(LOG_WARNING, \"GLFW: Failed to create window/context\");\n            glfwTerminate();\n            return -1;\n        }\n\n        // After the window was created, determine the monitor that the window manager assigned.")

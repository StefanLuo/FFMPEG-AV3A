# Custom uninstall target see:
# https://cmake.org/Wiki/CMake_FAQ#Can_I_do_.22make_uninstall.22_with_CMake.3F

# cmake_policy(SET CMP0007 NEW)
if (NOT EXISTS "D:/adm-render/install_manifest.txt")
    message(FATAL_ERROR "Cannot find install manifest: \"D:/adm-render/install_manifest.txt\"")
endif(NOT EXISTS "D:/adm-render/install_manifest.txt")

file(READ "D:/adm-render/install_manifest.txt" files)
string(REGEX REPLACE "\n" ";" files "${files}")
list(REVERSE files)
foreach (file ${files})
    message(STATUS "Uninstalling \"$ENV{DESTDIR}${file}\"")
    if (EXISTS "$ENV{DESTDIR}${file}")
        execute_process(
            COMMAND D:/工具/cmake-3.22.1-windows-x86_64/cmake-3.22.1-windows-x86_64/bin/cmake.exe -E remove "$ENV{DESTDIR}${file}"
            OUTPUT_VARIABLE rm_out
            RESULT_VARIABLE rm_retval
        )
        if(NOT ${rm_retval} EQUAL 0)
            message(FATAL_ERROR "Problem when removing \"$ENV{DESTDIR}${file}\"")
        endif (NOT ${rm_retval} EQUAL 0)
    else (EXISTS "$ENV{DESTDIR}${file}")
        message(STATUS "File \"$ENV{DESTDIR}${file}\" does not exist.")
    endif (EXISTS "$ENV{DESTDIR}${file}")
endforeach(file)

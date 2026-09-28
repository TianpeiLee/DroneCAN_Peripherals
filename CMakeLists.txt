cmake_minimum_required(VERSION 3.22.1)

# ---------- 工具链设置 ----------
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

set(CMAKE_ASM_COMPILER riscv32-wch-elf-gcc)
set(CMAKE_C_COMPILER riscv32-wch-elf-gcc)
set(CMAKE_CXX_COMPILER riscv32-wch-elf-g++)
set(CMAKE_OBJCOPY riscv32-wch-elf-objcopy)
set(CMAKE_OBJDUMP riscv32-wch-elf-objdump)

project(DroneCAN-CH32V203 VERSION 0.1.0 LANGUAGES C ASM)

# ---------- 公共编译选项 ----------
set(COMMON_COMPILE_OPTIONS
    -march=rv32imac_xw
    -mabi=ilp32
    -msmall-data-limit=8
    -msave-restore
    -fmax-errors=20
    -fmessage-length=0
    -fsigned-char
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wunused
    -Wuninitialized
)

add_compile_options(${COMMON_COMPILE_OPTIONS})

add_executable(${PROJECT_NAME}
    platform/startup/startup.S
    platform/startup/sys_init.c
    platform/startup/vector.S

    drivers/drv_can.c
    drivers/drv_tick.c
    drivers/drv_uid.c
    drivers/drv_ws2812.c
    drivers/drv_dshot.c
    drivers/drv_dshot_telem.c

    dronecan/dronecan_node.c
    dronecan/dronecan_status.c
    dronecan/dronecan_transport.c
    dronecan/dronecan_publishers.c
    dronecan/dronecan_dispatch.c
    dronecan/dronecan_params.c
    dronecan/handlers/getnodeinfo.c
    dronecan/handlers/param_getset.c
    dronecan/handlers/param_execute_opcode.c
    dronecan/handlers/dna.c
    dronecan/handlers/restart_node.c
    dronecan/handlers/file_server.c
    dronecan/handlers/firmware_update.c

    app/app_main.c
    app/app_params.c
    app/modules/light/light.c
    app/modules/esc/esc.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/app/modules/light
    ${CMAKE_CURRENT_SOURCE_DIR}/app/modules/esc
    ${CMAKE_CURRENT_SOURCE_DIR}/dronecan
    ${CMAKE_CURRENT_SOURCE_DIR}/dronecan/handlers
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX .elf
    DEBUG_POSTFIX _DEBUG
    RELEASE_POSTFIX _RELEASE
)

# ---------- wch_support 静态库 ----------
file(GLOB_RECURSE WCH_SUPPORT_SOURCES wch_support/*.c)
file(GLOB_RECURSE WCH_SUPPORT_HEADERS wch_support/*.h)

set(WCH_SUPPORT_INCLUDE_DIRS "")
foreach(header ${WCH_SUPPORT_HEADERS})
    get_filename_component(dir ${header} DIRECTORY)
    list(APPEND WCH_SUPPORT_INCLUDE_DIRS ${dir})
endforeach()
list(REMOVE_DUPLICATES WCH_SUPPORT_INCLUDE_DIRS)

add_library(wch_support STATIC ${WCH_SUPPORT_SOURCES})
target_include_directories(wch_support PUBLIC ${WCH_SUPPORT_INCLUDE_DIRS})
target_compile_options(wch_support PRIVATE ${COMMON_COMPILE_OPTIONS})

# ---------- DSDL 代码生成与 dsdlc 库 ----------
set(DSDL_GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dsdlc_generated)
set(DSDL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/DSDL/uavcan)

set(Python3_FIND_VIRTUALENV FIRST)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

execute_process(
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/dronecan_dsdlc/dronecan_dsdlc.py
            -O ${DSDL_GENERATED_DIR} ${DSDL_SOURCE_DIR}
    RESULT_VARIABLE DSDL_GEN_RESULT
    OUTPUT_VARIABLE DSDL_GEN_OUTPUT
    ERROR_VARIABLE DSDL_GEN_ERROR
)
if(NOT DSDL_GEN_RESULT EQUAL 0)
    message(FATAL_ERROR "DSDL generation failed: ${DSDL_GEN_ERROR}")
endif()

file(GLOB_RECURSE DSDL_SOURCES ${DSDL_GENERATED_DIR}/src/*.c)
add_library(dsdlc STATIC ${DSDL_SOURCES})
target_include_directories(dsdlc PUBLIC
    ${DSDL_GENERATED_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libcanard
)
target_compile_options(dsdlc PRIVATE ${COMMON_COMPILE_OPTIONS})

# ---------- libcanard 子目录 ----------
add_subdirectory(libcanard)

# ---------- 链接库 ----------
target_link_libraries(${PROJECT_NAME} PRIVATE
    dsdlc
    wch_support
    canard_tgt
    m
)

# ---------- 链接选项 ----------
target_link_options(${PROJECT_NAME} PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/linkscript.ld
    -nostartfiles
    -Wl,--print-memory-usage
    -Xlinker --gc-sections
    --specs=nano.specs
    --specs=nosys.specs
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}>
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}>
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJDUMP} $<TARGET_FILE:${PROJECT_NAME}> -S >
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.lst
    COMMENT "Generating binary, hex and disassembly"
)
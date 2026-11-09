add_executable(efxeffect_test WIN32
    efxeffect_test.cpp efxeffect_test.h
)
target_include_directories(efxeffect_test PRIVATE
    ../../../plugins/interfaces
    ../../src
)
target_compile_definitions(efxeffect_test PRIVATE
    EFX_GOLDEN_CSV="${CMAKE_CURRENT_SOURCE_DIR}/../efx_golden/efx_legacy_golden.csv"
    EFX_PRESET_DIR="${CMAKE_SOURCE_DIR}/resources/efx"
)

target_link_libraries(efxeffect_test PRIVATE
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Gui
    Qt${QT_MAJOR_VERSION}::Test
    qlcplusengine
)

if(APPLE)
    set_target_properties(efxeffect_test PROPERTIES
        MACOSX_BUNDLE FALSE
    )
endif()

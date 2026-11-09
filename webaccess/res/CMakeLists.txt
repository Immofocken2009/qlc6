project(webfiles)

# Set the files for the webfiles target
set(WEBFILES_COMMON
    common.css
    keypad.html
    networkconfig.js
    configuration.js
    websocket.js
    favicon.ico
    favicon-192x192.png
)

set(WEBFILES_QML
    simpledesk-v5.css
    simpledesk-v5.js
    webaccess-v5.css
    webaccess-v5.js
    webaccess-v5.html
)

set(WEBFILES_CLASSIC
    simpledesk.css
    simpledesk.js
    virtualconsole.css
    virtualconsole.js
)

if(qlc6)
    set(WEBFILES ${WEBFILES_COMMON})
elseif(qmlui)
    set(WEBFILES ${WEBFILES_COMMON} ${WEBFILES_QML})
else()
    set(WEBFILES ${WEBFILES_COMMON} ${WEBFILES_CLASSIC})
endif()

# Set the path for the webfiles target
set(WEBFILES_PATH "${INSTALLROOT}/${WEBFILESDIR}")

# Only perform the copy if we're building for Android
if(ANDROID)
    # Define the destination directory for the web files within the Android package
    set(WEBFILES_ASSETS_DIR "${QT_ANDROID_PACKAGE_SOURCE_DIR}/${WEBFILES_PATH}")

    # Create the web files assets directory if it doesn't exist
    file(MAKE_DIRECTORY ${WEBFILES_ASSETS_DIR})

    # Copy the web files to the assets directory
    foreach(WEBFILE IN LISTS WEBFILES)
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${WEBFILE} DESTINATION ${WEBFILES_ASSETS_DIR})
    endforeach()
endif()

# Install the webfiles target to the specified path
install(FILES ${WEBFILES} DESTINATION ${WEBFILES_PATH})

# qlc6: install the QML web files renamed from -v5 to -v6 so they can coexist
# with an installed qlcplus5 package (both would otherwise claim the same path).
if(qlc6)
    foreach(src IN LISTS WEBFILES_QML)
        string(REPLACE "-v5" "-v6" dst "${src}")
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${src}
                DESTINATION ${WEBFILES_PATH}
                RENAME ${dst})
    endforeach()
endif()

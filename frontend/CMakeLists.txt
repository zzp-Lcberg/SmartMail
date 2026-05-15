set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(FRONTEND_SOURCES
    src/main.cpp
    src/client/ServiceClient.cpp
    src/client/WebSocketHandler.cpp
    src/models/MailModel.cpp
    src/models/FolderModel.cpp
    src/views/MainWindow.cpp
    src/views/AiPanel.cpp
    src/views/EmailListView.cpp
    src/views/EmailDetailView.cpp
    src/views/EmailComposer.cpp
    src/dialogs/SettingsDialog.cpp
    src/dialogs/AccountDialog.cpp
)

set(FRONTEND_HEADERS
    src/client/ServiceClient.hpp
    src/client/WebSocketHandler.hpp
    src/models/MailModel.hpp
    src/models/FolderModel.hpp
    src/views/MainWindow.hpp
    src/views/AiPanel.hpp
    src/views/EmailListView.hpp
    src/views/EmailDetailView.hpp
    src/views/EmailComposer.hpp
    src/dialogs/SettingsDialog.hpp
    src/dialogs/AccountDialog.hpp
)

set(RESOURCE_FILES
    resources/resources.qrc
)

add_executable(SmartMailClient ${FRONTEND_SOURCES} ${FRONTEND_HEADERS} ${RESOURCE_FILES})

target_include_directories(SmartMailClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_libraries(SmartMailClient PRIVATE
    SmartMailCommon
    Qt6::Widgets
    Qt6::Network
    Qt6::WebSockets
)

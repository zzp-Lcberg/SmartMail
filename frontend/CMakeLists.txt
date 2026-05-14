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

add_executable(SmartMailClient ${FRONTEND_SOURCES} ${FRONTEND_HEADERS})

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

# 资源文件
qt_add_resources(SmartMailClient "resources"
    PREFIX "/"
    FILES
        resources/icons/placeholder.txt
)

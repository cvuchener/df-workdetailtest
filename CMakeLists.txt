cmake_minimum_required(VERSION 3.5)
project(workdetailtest)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	include(GNUInstallDirs)
	set(CMAKE_INSTALL_DATADIR ${CMAKE_INSTALL_DATADIR}/workdetailtest)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	# TODO
endif()

option(USE_EXTERNAL_DFS "Use dfs from external subdirectory" ON)
if (${USE_EXTERNAL_DFS})
	add_subdirectory(external/dfs)
else()
	find_package(dfs REQUIRED)
endif()
option(USE_EXTERNAL_DFHACK_CLIENT "Use dfhack-client from external subdirectory" ON)
if (${USE_EXTERNAL_DFHACK_CLIENT})
	add_subdirectory(external/dfhack-client-qt)
else()
	find_package(DFHackClientQt REQUIRED)
endif()
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent Qml)
find_package(QCoro6 REQUIRED COMPONENTS Core)
find_package(Protobuf REQUIRED)

option(BUILD_PORTABLE "Build as portable application (look for files in the application directory)" OFF)
if(BUILD_PORTABLE)
    add_definitions(-DBUILD_PORTABLE)
endif()
option(BUILD_DEVMODE "Build as devmode, using current source directory as source for data" OFF)
if (BUILD_DEVMODE)
    add_definitions("-DDEVMODE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/data\"")
    install(CODE "message(FATAL_ERROR \"Cannot install when built with BUILD_DEVMODE\")")
endif()

protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS
	dfhack/llmemreader/proto/llmemreader.proto
	dfhack/workdetailtest/proto/workdetailtest.proto)
qt_wrap_ui(UI_SOURCES
	ui/AboutDialog.ui
	ui/AdvancedConnectionDialog.ui
	ui/LogDock.ui
	ui/MainWindow.ui
	ui/PreferencesDialog.ui
	ui/UnitDetailsDock.ui
	ui/WorkDetailEditor.ui
	ui/WorkDetailManager.ui
)
add_executable(workdetailtest
	src/main.cpp
	src/AbstractColumn.cpp
	src/Application.cpp
	src/Columns/AttributesColumn.cpp
	src/Columns/Factory.cpp
	src/Columns/NameColumn.cpp
	src/Columns/SkillsColumn.cpp
	src/Columns/UnitFlagsColumn.cpp
	src/Columns/WorkDetailColumn.cpp
	src/Counter.cpp
	src/DFHackProcess.cpp
	src/DataRole.h
	src/DwarfFortress.cpp
	src/DwarfFortressData.cpp
	src/DwarfFortressReader.cpp
	src/FilterBar.cpp
	src/GridView.cpp
	src/GridViewManager.cpp
	src/GridViewModel.cpp
	src/GridViewStyle.cpp
	src/GridViewTabs.cpp
	src/GroupBar.cpp
	src/GroupBy.cpp
	src/Groups/Factory.cpp
	src/Groups/GroupByCreature.cpp
	src/Groups/GroupByMigration.cpp
	src/Groups/GroupByWorkDetailAssigned.cpp
	src/IconProvider.cpp
	src/LaborModel.cpp
	src/LogCategory.cpp
	src/LogDock.cpp
	src/MainWindow.cpp
	src/MessageHandler.cpp
	src/ModelMimeData.cpp
	src/ObjectList.cpp
	src/PreferencesDialog.cpp
	src/ProcessStats.cpp
	src/ScriptManager.cpp
	src/Settings.cpp
	src/StandardPaths.cpp
	src/StructuresManager.cpp
	src/Unit.cpp
	src/UnitDescriptors.cpp
	src/UnitDetails/AttributeModel.cpp
	src/UnitDetails/Dock.cpp
	src/UnitDetails/InventoryModel.cpp
	src/UnitDetails/ProgressDelegate.cpp
	src/UnitDetails/SkillModel.cpp
	src/UnitDetails/UnitDataModel.cpp
	src/UnitFilterProxyModel.cpp
	src/UnitScriptWrapper.cpp
	src/UserUnitFilters.cpp
	src/WorkDetail.cpp
	src/WorkDetailEditor.cpp
	src/WorkDetailManager.cpp
	src/WorkDetailModel.cpp
	src/WorkDetailPresets.cpp
	src/df/raws.cpp
	src/df/utils.cpp
	${PROTO_SOURCES}
	${UI_SOURCES}
)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	target_sources(workdetailtest PRIVATE src/DwarfFortress_linux.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	target_sources(workdetailtest PRIVATE src/DwarfFortress_windows.cpp)
else()
	target_sources(workdetailtest PRIVATE src/DwarfFortress_unsupported_platform.cpp)
endif()
dfs_generate_df_types(TARGET workdetailtest
	STRUCTURES ${CMAKE_CURRENT_SOURCE_DIR}/data/structures/50.12
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/df_enums
	NAMESPACE df
	TYPES
		language_name_component
		unit_labor_category
		unit_labor
		mood_type
		caste_raw_flags
		unit_flags1
		unit_flags2
		unit_flags3
		unit_flags4
		cie_add_tag_mask1
		work_detail.work_detail_flags --as work_detail_flags
		work_detail.icon --as work_detail_icon
		work_detail_mode
		histfig_entity_link_type
		histfig_hf_link_type
		occupation_type
		entity_position_flags
		profession
		job_skill_class
		job_skill
		skill_rating
		item_quality
		item_matstate
		item_type
		unit_inventory_item.mode --as unit_inventory_item_mode
		matter_state
		tool_uses
		builtin_mats
		identity_type
		physical_attribute_type
		mental_attribute_type
)
set_property(SOURCE
	${PROTO_SOURCES} ${PROTO_HEADERS}
	${CMAKE_CURRENT_BINARY_DIR}/df_enums.h
	${CMAKE_CURRENT_BINARY_DIR}/df_enums.cpp
	PROPERTY SKIP_AUTOGEN ON)
set_target_properties(workdetailtest PROPERTIES
	AUTOMOC ON
)
target_include_directories(workdetailtest PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_features(workdetailtest PRIVATE cxx_std_20)
target_link_libraries(workdetailtest
	dfs::dfs
	Qt6::Widgets
	Qt6::Concurrent
	Qt6::Qml
	QCoro::Core
	DFHackClientQt::dfhack-client-qt
	protobuf::libprotobuf
)
file(GLOB_RECURSE ICON_THEME_FILES
	FOLLOW_SYMLINKS
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	CONFIGURE_DEPENDS
	icons/*/index.theme
	icons/*/configure.svg
	icons/*/document-edit.svg
	icons/*/document-export.svg
	icons/*/document-import.svg
	icons/*/document-new.svg
	icons/*/edit-delete.svg
	icons/*/edit-table-insert-column-left.svg
	icons/*/edit-table-insert-column-right.svg
	icons/*/go-bottom.svg
	icons/*/go-down.svg
	icons/*/go-top.svg
	icons/*/go-up.svg
	icons/*/help-about.svg
	icons/*/list-add.svg
	icons/*/network-connect.svg
	icons/*/network-disconnect.svg
	icons/*/object-locked.svg
	icons/*/tab-new.svg
	icons/*/view-filter.svg
	icons/*/view-refresh.svg
)
qt_add_resources(workdetailtest "icons"
	PREFIX "/icons"
	BASE "icons"
	FILES ${ICON_THEME_FILES}
)

install(TARGETS workdetailtest
	COMPONENT Runtime)
install(DIRECTORY data/structures
	TYPE DATA
	COMPONENT Runtime
	FILES_MATCHING
	PATTERN df.*.xml
	PATTERN symbols.xml
	PATTERN .* EXCLUDE
	PATTERN tools EXCLUDE
)
install(DIRECTORY
		data/icons
		data/gridviews
		data/unit_filters
		data/workdetails
	TYPE DATA
	COMPONENT Runtime
)

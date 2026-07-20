
add_library(anchor_emitter STATIC
	anchor_emitter.cpp
)
add_library(retdec::anchor_emitter ALIAS anchor_emitter)

target_compile_features(anchor_emitter PUBLIC cxx_std_17)

target_include_directories(anchor_emitter
	PUBLIC
		$<BUILD_INTERFACE:${RETDEC_INCLUDE_DIR}>
		$<INSTALL_INTERFACE:${RETDEC_INSTALL_INCLUDE_DIR}>
)

target_link_libraries(anchor_emitter
	PUBLIC
		retdec::config
)

set_target_properties(anchor_emitter
	PROPERTIES
		OUTPUT_NAME "retdec-anchor-emitter"
)

# Install includes.
install(
	DIRECTORY ${RETDEC_INCLUDE_DIR}/retdec/anchor_emitter
	DESTINATION ${RETDEC_INSTALL_INCLUDE_DIR}/retdec
)

# Install libs.
install(TARGETS anchor_emitter
	EXPORT anchor-emitter-targets
	ARCHIVE DESTINATION ${RETDEC_INSTALL_LIB_DIR}
	LIBRARY DESTINATION ${RETDEC_INSTALL_LIB_DIR}
)

# Export targets.
install(EXPORT anchor-emitter-targets
	FILE "retdec-anchor-emitter-targets.cmake"
	NAMESPACE retdec::
	DESTINATION ${RETDEC_INSTALL_CMAKE_DIR}
)

# Install CMake files.
configure_file(
	"retdec-anchor-emitter-config.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/retdec-anchor-emitter-config.cmake"
	@ONLY
)
install(
	FILES
		"${CMAKE_CURRENT_BINARY_DIR}/retdec-anchor-emitter-config.cmake"
	DESTINATION
		"${RETDEC_INSTALL_CMAKE_DIR}"
)

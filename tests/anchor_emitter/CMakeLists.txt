
add_executable(tests-anchor_emitter
	anchor_emitter_tests.cpp
)

target_link_libraries(tests-anchor_emitter
	retdec::anchor_emitter
	retdec::config
	retdec::deps::gmock_main
)

set_target_properties(tests-anchor_emitter
	PROPERTIES
		OUTPUT_NAME "retdec-tests-anchor-emitter"
)

install(TARGETS tests-anchor_emitter
	RUNTIME DESTINATION ${RETDEC_INSTALL_TESTS_DIR}
)

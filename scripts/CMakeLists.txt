FILE(COPY ${dynamind-sewer_SOURCE_DIR}/scripts DESTINATION ${SCRIPT_PATH})

INSTALL(DIRECTORY ${dynamind-sewer_SOURCE_DIR}/scripts/
		DESTINATION "share/DynaMind/python_extensions")

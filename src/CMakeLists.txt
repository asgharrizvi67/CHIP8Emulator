add_executable (
	CHIP8Emulator
	main.cpp
	chip8.cpp
	keyboard.cpp
)

target_include_directories(CHIP8Emulator
	PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries (CHIP8Emulator
	PRIVATE SDL3::SDL3 
)

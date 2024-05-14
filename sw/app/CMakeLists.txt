# set vertical repeat to 8 for matching x8scale horizontal stretching
add_definitions(-DDVI_VERTICAL_REPEAT=8)

add_executable(
	pico-disp-ident
	main.c
)

target_link_libraries(
	pico-disp-ident
	pico_stdlib
	pico_multicore
	pico_util
	hardware_i2c
	libdvi
)

# switch UART0 assignment from pin header to USB

if (TARGET tinyusb_device)
	message ("INFO : found tinyusb device and switch UART0 to USB")
	pico_enable_stdio_usb(pico-disp-ident 1)
	pico_enable_stdio_uart(pico-disp-ident 0)
endif ()

# create map/bin/hex file etc.
pico_add_extra_outputs(pico-disp-ident)

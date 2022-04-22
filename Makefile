.PHONY: help build_all prog_fpga clean_all start_console run

INIT_APP := fw/tftp_ldr

help:
	@echo "Usage:"
	@echo "   REV A or B Pano (xc6slx150):"
	@echo "      make load      - load bit stream directly into FPGA"
	@echo "      make prog_fpga - program SPI flash"
	@echo "      make build_all - rebuild bitstream from sources (optional)"
	@echo
	@echo "   REV C Pano (xc6slx100):"
	@echo "      make PLATFORM=pano-g2-c load"
	@echo "      ..."
	@echo
	@echo "   make start_console (bit stream must be loaded)"
	@echo
	@echo "   other make targets: build_all, clean_all, run"
     
build_all:
	make -C $(INIT_APP) init_image
	make -C fpga

prog_fpga:
	make -C fpga prog_fpga

clean_all:
	make -C $(INIT_APP) clean
	make -C fpga clean

start_console:
	make -C $(INIT_APP) start_console

run:
	make -C $(INIT_APP) run

load:
	make -C $(INIT_APP) load


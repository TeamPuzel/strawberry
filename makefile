
all: build

setup:
	@rm -rf build
	@cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

build: setup
	@cd build; make

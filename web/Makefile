all:
	mkdir -p build
	emcmake cmake -S .. -B build
	cmake --build build -j

clean:
	rm -rf build
	rm -f jenn3d.js jenn3d.wasm
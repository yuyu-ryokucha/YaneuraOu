#include <emscripten.h>
#include <emscripten/wasmfs.h>
#include <iostream>

__attribute__((constructor))
void setup_wasmfs_opfs() {
    // Check if we are running in a Web Worker or Browser environment
    // Node.js will return 0, preventing unsupported OPFS calls in the Node wrapper.
    int is_web = EM_ASM_INT({
        return (typeof window !== 'undefined' || typeof importScripts !== 'undefined') ? 1 : 0;
    });

    if (is_web) {
        auto backend = wasmfs_create_opfs_backend();
        int res = wasmfs_create_directory("/opfs", 0777, backend);
        
        if (res == 0) {
            std::cout << "[WasmFS] OPFS successfully mounted at /opfs" << std::endl;
        } else {
            std::cerr << "[WasmFS] Failed to mount OPFS: " << res << std::endl;
        }
    } else {
        std::cout << "[WasmFS] Not in a web environment, skipping OPFS mount." << std::endl;
    }
}

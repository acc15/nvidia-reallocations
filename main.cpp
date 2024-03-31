#include <dlfcn.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>

enum memory_function {
    MALLOC,
    FREE,
    REALLOC
};

std::ostream& operator<<(std::ostream& s, memory_function f) {
    switch (f) {
        case MALLOC: s << "malloc"; break;
        case FREE: s << "free"; break;
        case REALLOC: s << "realloc"; break;
        default: s << "???"; break;
    }
    return s;
}

bool dl_init = false;

void nvidia_callback(memory_function fn, void *old_ptr, void* new_ptr, size_t size) {
    std::cout << fn << " " << old_ptr << " size " << size << " = " << new_ptr << std::endl;
}

int open(const char *__file, int __oflag, ...) {
    static auto libc_open = reinterpret_cast<int(*)(const char*,int)>(dlsym(RTLD_NEXT, "open"));
    int result = libc_open(__file, __oflag);
    std::cout << "open " << __file << " flags " << __oflag << " = " << result << std::endl;
    return result;
}

int close(int fd) {
    static auto libc_close = reinterpret_cast<int(*)(int)>(dlsym(RTLD_NEXT, "close"));
    std::cout << "close " << fd << std::endl;
    return libc_close(fd);
}

void* malloc(size_t size) {
    using malloc_fn = decltype(&malloc);
    static malloc_fn libc_malloc = reinterpret_cast<malloc_fn>(dlsym(RTLD_NEXT, "malloc"));
    void* result = libc_malloc(size);
    if (dl_init) {
        nvidia_callback(MALLOC, nullptr, result, size);
    }
    return result;
}

void free(void* ptr) {
    using free_fn = decltype(&free);

    static free_fn libc_free = reinterpret_cast<free_fn>(dlsym(RTLD_NEXT, "free"));

    size_t usable_size = malloc_usable_size(ptr);
    libc_free(ptr);
    if (dl_init) {
        nvidia_callback(FREE, ptr, nullptr, usable_size);
    }
}

void* realloc(void * ptr, size_t size) {
    using realloc_fn = decltype(&realloc);
    static realloc_fn libc_realloc = reinterpret_cast<realloc_fn>(dlsym(RTLD_NEXT, "realloc"));

    void* result = libc_realloc(ptr, size);
    if (dl_init) {
        nvidia_callback(REALLOC, ptr, result, size);
    }
    return result;
}

int main(int, char**) {

    const char* path = "/usr/lib/libGLX_nvidia.so.550.67";
    
    std::cout << path << " loading..." << std::endl;
    
    clock_t start = clock();
    dl_init = true;
    void* lib = dlopen(path, RTLD_NOW);
    dl_init = false;
    clock_t end = clock();

    std::cout << path << " loaded at " << lib << " in " << (end - start) << "usec" << std::endl;
    if (lib != nullptr) {
        dlclose(lib);
        std::cout << path << " unloaded" << std::endl;
    }

    return 0;
}


#ifndef __HELLO__
#define __HELLO__

typedef void (*func_void)(void);
typedef const char* (*func_const_char)(void);

typedef struct func_instance {
    const char* name;
    union {
        func_const_char const_char_ptr;
        func_void       void_ptr;
    } func;
} func_instance;

const char* hello();

#endif
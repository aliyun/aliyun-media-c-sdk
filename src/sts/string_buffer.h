#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include <stdio.h>


// Declare a string_buffer with the given name of the given maximum length
#define string_buffer(name, len)                                        \
    char name[len + 1];                                                 \
    int name##Len


// Initialize a string_buffer
#define string_buffer_initialize(sb)                                    \
    do {                                                                \
        sb[0] = 0;                                                      \
        sb##Len = 0;                                                    \
    } while (0)


// Append [len] bytes of [str] to [sb], setting [all_fit] to 1 if it fit, and
// 0 if it did not
#define string_buffer_append(sb, str, len, all_fit)                     \
    do {                                                                \
        sb##Len += snprintf(&(sb[sb##Len]), sizeof(sb) - sb##Len - 1,   \
                            "%.*s", (int) (len), str);                  \
        if (sb##Len > (int) (sizeof(sb) - 1)) {                         \
            sb##Len = sizeof(sb) - 1;                                   \
            all_fit = 0;                                                \
        }                                                               \
        else {                                                          \
            all_fit = 1;                                                \
        }                                                               \
    } while (0)


// Declare a string multibuffer with the given name of the given maximum size
#define string_multibuffer(name, size)                                  \
    char name[size];                                                    \
    int name##Size


// Initialize a string_multibuffer
#define string_multibuffer_initialize(smb)                              \
    do {                                                                \
        smb##Size = 0;                                                  \
    } while (0)


// Evaluates to the current string within the string_multibuffer
#define string_multibuffer_current(smb)                                  \
    &(smb[smb##Size])


// Adds a new string to the string_multibuffer
#define string_multibuffer_add(smb, str, len, all_fit)                  \
    do {                                                                \
        smb##Size += (snprintf(&(smb[smb##Size]),                       \
                               sizeof(smb) - smb##Size,                 \
                               "%.*s", (int) (len), str) + 1);          \
        if (smb##Size > (int) sizeof(smb)) {                            \
            smb##Size = sizeof(smb);                                    \
            all_fit = 0;                                                \
        }                                                               \
        else {                                                          \
            all_fit = 1;                                                \
        }                                                               \
    } while (0)


// Appends to the current string in the string_multibuffer.  There must be a
// current string, meaning that string_multibuffer_add must have been called
// at least once for this string_multibuffer.
#define string_multibuffer_append(smb, str, len, all_fit)               \
    do {                                                                \
        smb##Size--;                                                    \
        string_multibuffer_add(smb, str, len, all_fit);                 \
    } while (0)


#endif /* STRING_BUFFER_H */

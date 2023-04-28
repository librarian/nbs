/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "error/s2n_errno.h"
#include "utils/s2n_ensure.h"
#include "utils/s2n_result.h"
#include "utils/s2n_safety_macros.h"

/**
 * The goal of s2n_safety is to provide helpers to perform common
 * checks, which help with code readability.
 */

/**
 * Marks a case of a switch statement as able to fall through to the next case
 */
#if defined(S2N_FALL_THROUGH_SUPPORTED)
    #define FALL_THROUGH __attribute__((fallthrough))
#else
    #define FALL_THROUGH ((void) 0)
#endif

/* Returns `true` if s2n is in unit test mode, `false` otherwise */
bool s2n_in_unit_test();

/* Sets whether s2n is in unit test mode */
int s2n_in_unit_test_set(bool newval);

#define S2N_IN_INTEG_TEST (getenv("S2N_INTEG_TEST") != NULL)
#define S2N_IN_TEST       (s2n_in_unit_test() || S2N_IN_INTEG_TEST)

/* Returns 1 if a and b are equal, in constant time */
bool s2n_constant_time_equals(const uint8_t* a, const uint8_t* b, const uint32_t len);

/* Copy src to dst, or don't copy it, in constant time */
int s2n_constant_time_copy_or_dont(uint8_t* dst, const uint8_t* src, uint32_t len, uint8_t dont);

/* If src contains valid PKCS#1 v1.5 padding of exactly expectlen bytes, decode
 * it into dst, otherwise leave dst alone, in constant time.
 * Always returns zero. */
int s2n_constant_time_pkcs1_unpad_or_dont(uint8_t* dst, const uint8_t* src, uint32_t srclen, uint32_t expectlen);

/**
 * Runs _thecleanup function on _thealloc once _thealloc went out of scope
 */
#define DEFER_CLEANUP(_thealloc, _thecleanup) \
    __attribute__((cleanup(_thecleanup))) _thealloc
/**
 * Often we want to free memory on an error, but not on a success.
 * We do this by declaring a variable with DEFER_CLEANUP, then zeroing
 * that variable after success to prevent DEFER_CLEANUP from accessing
 * and freeing any memory it allocated.
 *
 * This pattern is not intuitive, so a named macro makes it more readable.
 */
#define ZERO_TO_DISABLE_DEFER_CLEANUP(_thealloc) memset(&_thealloc, 0, sizeof(_thealloc))

/* We want to apply blinding whenever `action` fails.
 * Unfortunately, because functions in S2N do not have a consistent return type, determining failure is difficult.
 * Instead, let's rely on the consistent error handling behavior of returning from a method early on error
 * and apply blinding if our tracking variable goes out of scope early.
 */
S2N_CLEANUP_RESULT s2n_connection_apply_error_blinding(struct s2n_connection** conn);
#define WITH_ERROR_BLINDING(conn, action)                                                                 \
    do {                                                                                                  \
        DEFER_CLEANUP(struct s2n_connection* _conn_to_blind = conn, s2n_connection_apply_error_blinding); \
        action;                                                                                           \
        /* The `if` here is to avoid a redundantInitialization warning from cppcheck */                   \
        if (_conn_to_blind) {                                                                             \
            _conn_to_blind = NULL;                                                                        \
        }                                                                                                 \
    } while (0)

/* Creates cleanup function for pointers from function func which accepts a pointer.
 * This is useful for DEFER_CLEANUP as it passes &_thealloc into _thecleanup function,
 * so if _thealloc is a pointer _thecleanup will receive a pointer to a pointer.*/
#define DEFINE_POINTER_CLEANUP_FUNC(type, func) \
    static inline void func##_pointer(type* p)  \
    {                                           \
        if (p && *p)                            \
            func(*p);                           \
    }                                           \
    struct __useless_struct_to_allow_trailing_semicolon__

#define s2n_array_len(array) ((array != NULL) ? (sizeof(array) / sizeof(array[0])) : 0)

int s2n_mul_overflow(uint32_t a, uint32_t b, uint32_t* out);

/**
 * Rounds "initial" up to a multiple of "alignment", and stores the result in "out".
 * Raises an error if overflow would occur.
 * NOT CONSTANT TIME.
 */
int s2n_align_to(uint32_t initial, uint32_t alignment, uint32_t* out);
int s2n_add_overflow(uint32_t a, uint32_t b, uint32_t* out);
int s2n_sub_overflow(uint32_t a, uint32_t b, uint32_t* out);

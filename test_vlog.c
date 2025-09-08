#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <cmocka.h>

#include "vlog.h"

#define MODULE1 VLM_test_vlog1
#define MODULE2 VLM_test_vlog2

FILE* MOCK_FP = (FILE*)1;

static char file_log_buffer[1024];
static char stderr_log_buffer[1024];
static char file_stash_buffer[1024];
static char stderr_stash_buffer[1024];
static char expected_stderr_log_buffer[1024];
static char expected_file_log_buffer[1024];

FILE* __wrap_fopen(const char* filename, const char* mode) {
    return MOCK_FP;
}

int __wrap_fclose(FILE* fp) {
    return 0;
}

long __wrap_ftell(FILE* fp) {
    return mock_type(long);
}

int __wrap_fseek(FILE* stream, long offset, int whence) {
    return 0;
}

void __wrap_rewind(FILE* stream) {
}

int __wrap_fileno(FILE* stream) {
    return 1;
}

int __wrap_ftruncate(int fd, off_t length) {
    return 0;
}

int __wrap_fflush(FILE* stream) {
    if(stream == MOCK_FP) {
        memcpy(file_stash_buffer, file_log_buffer, sizeof(file_log_buffer));
        memset(file_log_buffer, 0, sizeof(file_log_buffer));
    } else if(stream == stderr) {
        memcpy(stderr_stash_buffer, stderr_log_buffer, sizeof(stderr_log_buffer));
        memset(stderr_log_buffer, 0, sizeof(stderr_log_buffer));
    }
    return 0;
}

int __wrap_vfprintf(FILE* stream, const char* format, va_list ap) {
    int rc;
    int len;

    if(stream == MOCK_FP) {
        len = strlen(file_log_buffer);
        rc = vsnprintf(file_log_buffer + len, sizeof(file_log_buffer) - len, format, ap);
    } else if(stream == stderr) {
        len = strlen(stderr_log_buffer);
        rc = vsnprintf(stderr_log_buffer + len, sizeof(stderr_log_buffer) - len, format, ap);
    }

    return rc;
}

int __wrap_fprintf(FILE* stream, const char* format, ...) {
    int rc;
    va_list ap;
    int len;

    va_start(ap, format);
    if(stream == MOCK_FP) {
        len = strlen(file_log_buffer);
        rc = vsnprintf(file_log_buffer + len, sizeof(file_log_buffer) - len, format, ap);
    } else if(stream == stderr) {
        len = strlen(stderr_log_buffer);
        rc = vsnprintf(stderr_log_buffer + len, sizeof(stderr_log_buffer) - len, format, ap);
    }
    va_end(ap);

    return rc;
}

int __wrap_fputc(int c, FILE* stream) {
    int len;

    if(stream == MOCK_FP) {
        len = strlen(file_log_buffer);
        file_log_buffer[len] = (char)c;
        file_log_buffer[len + 1] = '\0';
    } else if(stream == stderr) {
        len = strlen(stderr_log_buffer);
        stderr_log_buffer[len] = (char)c;
        stderr_log_buffer[len + 1] = '\0';
    }

    return 0;
}

size_t __wrap_strftime(char* s, size_t maxsize, const char* format, const struct tm* tm) {
    if(strcmp(format, "%Y-%m-%d %H:%M:%S") == 0) {
        return snprintf(s, maxsize, "2024-01-01 12:00:00");
    } else if(strcmp(format, "%H:%M:%S") == 0) {
        return snprintf(s, maxsize, "12:00:00");
    }
    return 0;
}

int __wrap_fputs(const char* __restrict __s, FILE* __restrict __stream) {
    if(__stream == MOCK_FP) {
        strncpy(file_log_buffer, __s, sizeof(file_log_buffer));
    } else if(__stream == stderr) {
        strncpy(stderr_log_buffer, __s, sizeof(stderr_log_buffer));
    }
    return 0;
}

void test_vlog_log(int level, int module, const char* file, int line, const char* fmt, ...) {
    int len;
    va_list ap;

    snprintf(expected_stderr_log_buffer, sizeof(expected_stderr_log_buffer),
    "12:00:00 %-5s %-5s %s:%d: ", vlog_get_level_name(level),
    vlog_get_module_name(module), file, line);
    len = strlen(expected_stderr_log_buffer);
    va_start(ap, fmt);
    vsnprintf(expected_stderr_log_buffer + len,
    sizeof(expected_stderr_log_buffer) - len, fmt, ap);
    va_end(ap);
    len = strlen(expected_stderr_log_buffer);
    expected_stderr_log_buffer[len] = '\n';
    expected_stderr_log_buffer[len + 1] = '\0';

    snprintf(expected_file_log_buffer, sizeof(expected_file_log_buffer),
    "2024-01-01 12:00:00 %-5s %-5s %s:%d: ", vlog_get_level_name(level),
    vlog_get_module_name(module), file, line);
    len = strlen(expected_file_log_buffer);
    va_start(ap, fmt);
    vsnprintf(expected_file_log_buffer + len,
    sizeof(expected_file_log_buffer) - len, fmt, ap);
    va_end(ap);
    len = strlen(expected_file_log_buffer);
    expected_file_log_buffer[len] = '\n';
    expected_file_log_buffer[len + 1] = '\0';

    vlog(module, level, file, line, fmt, ap);
}

static int setup(void** state) {
    will_return_maybe(__wrap_ftell, 100);

    memset(file_log_buffer, 0, sizeof(file_log_buffer));
    memset(stderr_log_buffer, 0, sizeof(stderr_log_buffer));
    memset(file_stash_buffer, 0, sizeof(file_stash_buffer));
    memset(stderr_stash_buffer, 0, sizeof(stderr_stash_buffer));
    memset(expected_stderr_log_buffer, 0, sizeof(expected_stderr_log_buffer));
    memset(expected_file_log_buffer, 0, sizeof(expected_file_log_buffer));

    vlog_init();

    vlog_set_log_file("test.log");

    return 0;
}

static int teardown(void** state) {
    memset(file_log_buffer, 0, sizeof(file_log_buffer));
    memset(stderr_log_buffer, 0, sizeof(stderr_log_buffer));
    memset(file_stash_buffer, 0, sizeof(file_stash_buffer));
    memset(stderr_stash_buffer, 0, sizeof(stderr_stash_buffer));
    memset(expected_stderr_log_buffer, 0, sizeof(expected_stderr_log_buffer));
    memset(expected_file_log_buffer, 0, sizeof(expected_file_log_buffer));

    vlog_exit();

    return 0;
}

static void test_vlog_init(void** state) {
    will_return_maybe(__wrap_ftell, 100);

    int rc;

    vlog_init();

    rc = vlog_set_log_file("test.log");
    assert_int_equal(rc, 0);

    vlog_exit();
}

static void test_vlog_set_level(void** state) {
    will_return_maybe(__wrap_ftell, 100);

    vlog_set_levels(VLM_ANY_MODULE, VLF_ANY_FACILITY, VLL_INFO);

    test_vlog_log(VLL_DBG, MODULE1, "test.c", 10, "A debug message");
    assert_string_equal(file_log_buffer, "");
    assert_string_equal(stderr_log_buffer, "");

    test_vlog_log(VLL_INFO, MODULE2, "test.c", 20, "An info message");
    assert_string_equal(file_stash_buffer, expected_file_log_buffer);
    assert_string_equal(stderr_stash_buffer, expected_stderr_log_buffer);
}


int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_vlog_init),
        cmocka_unit_test_setup_teardown(test_vlog_set_level, setup, teardown),
    };

    // cmocka_set_message_output(CM_OUTPUT_XML);
    return cmocka_run_group_tests(tests, NULL, NULL);
}
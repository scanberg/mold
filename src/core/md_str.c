#include "md_str.h"

#include "md_common.h"
#include "md_file.h"
#include "md_allocator.h"
#include "md_log.h"

#include <string.h>

bool skip_line(str_t* in_out_str) {
    ASSERT(in_out_str);
    const char* c = (const char*)memchr(in_out_str->ptr, '\n', in_out_str->len);
    if (c) {
        in_out_str->len = in_out_str->len - (c - in_out_str->ptr);
        in_out_str->ptr = c + 1;
        return in_out_str;
    }
    return false;
}

bool peek_line(str_t* out_line, const str_t* in_str) {
    ASSERT(out_line);
    ASSERT(in_str);
    const char* beg = in_str->ptr;
    const char* end = (const char*)memchr(in_str->ptr, '\n', in_str->len);
    if (end) {
        out_line->ptr = beg;
        out_line->len = end - beg;
        return true;
    }
    return false;
}

bool extract_line(str_t* out_line, str_t* in_out_str) {
    ASSERT(out_line);
    ASSERT(in_out_str);
    if (in_out_str->len == 0) return false;

    const char* beg = in_out_str->ptr;
    const char* end = in_out_str->ptr + in_out_str->len;
    const char* c = (const char*)memchr(in_out_str->ptr, '\n', in_out_str->len);
    if (c) {
        end = c + 1;
    }
    else {
        while(0);
    }

    out_line->ptr = beg;
    out_line->len = end - beg;

    in_out_str->len = in_out_str->ptr + in_out_str->len - end;
    in_out_str->ptr = end;
    
    return true;
}

int64_t find_char(str_t str, int c) {
    // @TODO: Implement support for UTF encodings here.
    // Identify the effective width used in c and search for that.
    for (int64_t i = 0; i < (int64_t)str.len; ++i) {
        if (str.ptr[i] == c) return i;
    }
    return -1;
}

int64_t rfind_char(str_t str, int c) {
    for (int64_t i = str.len - 1; i >= 0; --i) {
        if (str.ptr[i] == c) return i;
    }
    return -1;
}

double parse_float(str_t str) {
    ASSERT(str.ptr);
    static const double pow10[16] = {
        1e+0,  1e+1,  1e+2,  1e+3,
        1e+4,  1e+5,  1e+6,  1e+7,
        1e+8,  1e+9,  1e+10, 1e+11,
        1e+12, 1e+13, 1e+14, 1e+15
    };

    const char* c = str.ptr;
    const char* end = str.ptr + str.len;

    double val = 0;
    double sign = 1;
    if (*c == '-') {
        ++c;
        sign = -1;
    }
    while (c != end && is_digit(*c)) {
        val = val * 10 + ((int)(*c) - '0');
        ++c;
    }
    if (*c != '.' || c == end) return sign * val;

    ++c; // skip '.'
    const uint32_t count = (uint32_t)(end - c);
    while (c < end && is_digit(*c)) {
        val = val * 10 + ((int)(*c) - '0');
        ++c;
    }

    return sign * val / pow10[count];
}

int64_t parse_int(str_t str) {
    int64_t val = 0;
    const char* c = str.ptr;
    const char* end = str.ptr + str.len;
    int64_t sign = 1;
    if (*c == '-') {
        ++c;
        sign = -1;
    }
    while (c != end && is_digit(*c)) {
        val = val * 10 + ((int)(*c) - '0');
        ++c;
    }
    return sign * val;
}

str_t load_textfile(str_t filename, struct md_allocator* alloc) {
    ASSERT(alloc);
    md_file_o* file = md_file_open(filename, MD_FILE_READ | MD_FILE_BINARY);
    str_t result = {0,0};
    if (file) {
        uint64_t file_size = md_file_size(file);
        char* mem = md_alloc(alloc, file_size + 1);
        if (mem) {
            uint64_t read_size = md_file_read(file, mem, file_size);

            ASSERT(read_size == file_size);
            mem[file_size] = '\0'; // Zero terminate as a nice guy

            result.ptr = mem;
            result.len = file_size;
        } else {
            md_printf(MD_LOG_TYPE_ERROR, "Could not allocate memory for file %d", 10);
        }
        md_file_close(file);
    }
    return result;
}

/*
str_t make_cstr(const char* str) {
    return (str_t){str, strlen(str)};
}
*/

str_t alloc_str(uint64_t len, struct md_allocator* alloc) {
    ASSERT(alloc);
    char* mem = md_alloc(alloc, len + 1);
    memset(mem, 0, len + 1);
    str_t str = {mem, len};
    return str;
}

void free_str(str_t str, struct md_allocator* alloc) {
    ASSERT(alloc);
    md_free(alloc, (void*)str.ptr, str.len + 1);
}

str_t copy_str(const str_t str, struct md_allocator* alloc) {
    ASSERT(alloc);
    str_t result = {0,0};
    if (str.ptr && str.len > 0) {
        char* data = md_alloc(alloc, str.len + 1);
        data[str.len] = '\0';
        memcpy(data, str.ptr, str.len);
        result.ptr = data;
        result.len = str.len;
    }
    return result;
}

// c:/folder/file.ext -> ext
str_t extract_ext(str_t path) {
    int64_t pos = rfind_char(path, '.');

    str_t res = {0};
    if (pos) {
        pos += 1; // skip '.'
        res.ptr = path.ptr + pos;
        res.len = path.len - pos;
    }
    return res;
}

// c:/folder/file.ext -> file.ext
str_t extract_file(str_t path) {
    int64_t pos = rfind_char(path, '/');
    if (pos == -1) {
        pos = rfind_char(path, '\\');
    }

    str_t res = {0};
    if (pos) {
        pos += 1; // skip slash or backslash
        res.ptr = path.ptr + pos;
        res.len = path.len - pos;
    }
    return res;
}

// c:/folder/file.ext -> c:/folder/file.
str_t extract_path_without_ext(str_t path) {
    const int64_t pos = rfind_char(path, '.');

    str_t res = {0};
    if (pos) {
        res.ptr = path.ptr;
        res.len = pos;
    }
    return res;
}

// c:/folder/file.ext -> c:/folder/
str_t extract_path_without_file(str_t path) {
    int64_t pos = rfind_char(path, '/');
    if (pos == -1) {
        pos = rfind_char(path, '\\');
    }

    str_t res = {0};
    if (pos) {
        res.ptr = path.ptr;
        res.len = pos;
    }
    return res;
}
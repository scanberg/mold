#if _MSC_VER && !__INTEL_COMPILER && !__clang__
#define _CRT_SECURE_NO_WARNINGS
#pragma warning( disable : 6011 26451 )
#endif

#include "mold_filter.h"

#include <string.h>
#include <stdio.h>

#include "core/arena_alloc.h"
#include "core/bitop.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define CLAMP(v, _min, _max) MIN(MAX(v, _min), _max)
#define DIV_UP(x) ((x + 63) / 64)
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define MAX_FUNC_ARGS 2
#define FLT_EPSILON 1E-5
#define FLT_MAX     1E+37

#ifndef ASSERT
#include <assert.h>
#define ASSERT assert
#endif

#ifdef __cplusplus
extern "C" {
#endif

void filter_func_all(uint64_t* bits, const mold_molecule* mol);
void filter_func_none(uint64_t* bits, const mold_molecule* mol);
void filter_func_protein(uint64_t* bits, const mold_molecule* mol);
void filter_func_water(uint64_t* bits, const mold_molecule* mol);
void filter_func_name(const char* str, uint64_t* bits, const mold_molecule* mol);
void filter_func_element(const char* str, uint64_t* bits, const mold_molecule* mol);
void filter_func_resname(const char* str, uint64_t* bits, const mold_molecule* mol);
void filter_func_resid(int min_range, int max_range, uint64_t* bits, const mold_molecule* mol);
void filter_func_residue(int min_range, int max_range, uint64_t* bits, const mold_molecule* mol);
void filter_func_chain(const char* str, uint64_t* bits, const mold_molecule* mol);
void filter_func_within(float min_range, float max_range, const uint64_t* in_bits, uint64_t* out_bits, const mold_molecule* mol);
void filter_func_resid_int(int val, uint64_t* bits, const mold_molecule* mol);
void filter_func_residue_int(int val, uint64_t* bits, const mold_molecule* mol);
void filter_func_within_flt(float range, const uint64_t* in_bits, uint64_t* out_bits, const mold_molecule* mol);

typedef struct str_slice_t {
    uint32_t offset;
    uint32_t length;
} str_slice_t;

typedef struct frange_t {
    float min;
    float max;
} frange_t;

typedef struct irange_t {
    int32_t min;
    int32_t max;
} irange_t;

static inline bool is_digit(char c) {
    return '0' <= c && c <= '9';
}

static inline bool is_alpha(char c) {
    return ('a' <= c && c <= 'z') ||
        ('A' <= c && c <= 'Z');
}

static inline bool is_whitespace(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

// Fast parser for floats (with assumptions)
// no leading +- (This is caught as unary operators)
// no scientific notation (e.g. 1.0e+7)
static inline double parse_float(const char* str, uint32_t len) {
    const double pow10[16] = {
        1,     1e+1,  1e+2,  1e+3,
        1e+4,  1e+5,  1e+6,  1e+7,
        1e+8,  1e+9,  1e+10, 1e+11,
        1e+12, 1e+13, 1e+14, 1e+15
    };

    double val = 0;
    const char* c = str;
    const char* end = str + len;
    while (*c != '.') {
        val = val * 10 + (*c - '0');
        ++c;
    }
    ++c; // skip '.'
    const uint32_t count = (uint32_t)(end - c);
    while (c < end) {
        val = val * 10 + (*c - '0');
        ++c;
    }

    return val / pow10[count];
}

// Fast parser for ints (with assumptions)
// no leading +- (This is caught as unary operators)
static inline int64_t parse_int(const char* str, uint32_t len) {
    int64_t val = 0;
    const char* c = str;
    const char* end = str + len;
    while (c != end && is_digit(*c)) {
        val = val * 10 + (*c - '0');
        ++c;
    }
    return val;
}

typedef enum {
    TOKEN_UNDEFINED = 0,
    TOKEN_IDENT = 128, // Reserve the first indices for character literals
    TOKEN_NOT,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_OF,
    TOKEN_IN,
    TOKEN_NUM,
    TOKEN_STR,
    TOKEN_END
} token_type_t;

typedef enum { // Sorted in precedence top to bottom
    NODE_TYPE_UNDEFINED = 0,
    NODE_TYPE_VALUE,
    NODE_TYPE_REF,
    NODE_TYPE_FUNC,
    NODE_TYPE_NOT,
    NODE_TYPE_AND,
    NODE_TYPE_OR,
} node_type_t;

typedef enum {
    VALUE_TYPE_UNDEFINED = 0,
    VALUE_TYPE_BOOL,
    VALUE_TYPE_INT,
    VALUE_TYPE_FLT,
    VALUE_TYPE_STR,
    VALUE_TYPE_INT_RNG,
    VALUE_TYPE_FLT_RNG
} value_type_t;

typedef enum {
    FUNC_TYPE_UNDEFINED = 0,
    FUNC_TYPE_VOID,
    FUNC_TYPE_STR,
    FUNC_TYPE_INT,
    FUNC_TYPE_FLT,
    FUNC_TYPE_INT_RNG,
    FUNC_TYPE_FLT_RNG,
    FUNC_TYPE_FLT_BOOL,
    FUNC_TYPE_FLT_RNG_BOOL
} func_type_t;

typedef struct token_t token_t;

struct token_t {
    token_type_t type;
    str_slice_t str;
};

typedef struct value_t {
    value_type_t type;
    union {
        uint64_t*   _bool;
        str_slice_t _str;
        float       _flt;
        int32_t     _int;
        frange_t    _frange;
        irange_t    _irange;
    };
} value_t;

typedef struct ast_node_t ast_node_t;
struct ast_node_t {
    node_type_t  type;
    value_type_t return_type;
    str_slice_t  str;
    union {
        value_t      _value;
        struct {
            uint16_t ident;
            int16_t  args;      // relative byte pointer to array of relative byte pointers to argument nodes
        } _func;
        int16_t      _child[2];  // relative byte pointers to ast_node children
    };
};

//static const int ast_node_size = sizeof(ast_node_t);
static_assert((sizeof(ast_node_t) & (sizeof(ast_node_t) - 1)) == 0, "ast_node_t size should be power of 2");

typedef struct lexer_t lexer_t;
struct lexer_t {
    const char* buff;
    uint32_t    buff_len;
    uint32_t    at;
};

typedef struct parse_context_t {
    mem_arena_t*    arena;
    const char*     expr_str;
    uint32_t        expr_len;
    lexer_t*        lexer;
    const token_t   cur_tok;
    ast_node_t*     cur_node;
} parse_context_t;

typedef struct keyword_t {
    const char*     str;
    uint32_t        len;
    token_type_t    type;
} keyword_t;

static const keyword_t keyword_table[] = {
    {0},    // idx zero as invalid
    {"or",  2, TOKEN_OR},
    {"of",  2, TOKEN_OF},           // reserved
    {"in",  2, TOKEN_IN},           // reserved
    {"and", 3, TOKEN_AND},
    {"not", 3, TOKEN_NOT},
};

typedef struct ident_t {
    const char*  str;
    uint32_t     len;
    node_type_t  type;
    func_type_t  func_type;
    void*        func_addr;
    uint32_t     arg_count;
    value_type_t arg_type[2]; // add more in the future

} ident_t;

static const ident_t ident_table[] = {
    {0},          // use idx zero as invalid
    {"x",         1, NODE_TYPE_FUNC, FUNC_TYPE_FLT_RNG,      NULL,                       1, VALUE_TYPE_FLT_RNG},
    {"y",         1, NODE_TYPE_FUNC, FUNC_TYPE_FLT_RNG,      NULL,                       1, VALUE_TYPE_FLT_RNG},
    {"z",         1, NODE_TYPE_FUNC, FUNC_TYPE_FLT_RNG,      NULL,                       1, VALUE_TYPE_FLT_RNG},
    {"all",       3, NODE_TYPE_FUNC, FUNC_TYPE_VOID,         filter_func_all,            0},
    {"none",      4, NODE_TYPE_FUNC, FUNC_TYPE_VOID,         filter_func_none,           0},
    {"mass",      4, NODE_TYPE_FUNC, FUNC_TYPE_FLT_RNG,      NULL,                       1, VALUE_TYPE_FLT_RNG},
    {"name",      4, NODE_TYPE_FUNC, FUNC_TYPE_STR,          filter_func_name,           1, VALUE_TYPE_STR},
    {"water",     5, NODE_TYPE_FUNC, FUNC_TYPE_VOID,         filter_func_water,          0},
    {"resid",     5, NODE_TYPE_FUNC, FUNC_TYPE_INT_RNG,      filter_func_resid,          1, VALUE_TYPE_INT_RNG},
    {"resid",     5, NODE_TYPE_FUNC, FUNC_TYPE_INT_RNG,      filter_func_resid_int,      1, VALUE_TYPE_INT},
    {"chain",     5, NODE_TYPE_FUNC, FUNC_TYPE_STR,          NULL,                       1, VALUE_TYPE_STR},
    {"radius",    6, NODE_TYPE_FUNC, FUNC_TYPE_FLT_RNG,      NULL,                       1, VALUE_TYPE_FLT_RNG},
    {"within",    6, NODE_TYPE_FUNC, FUNC_TYPE_FLT_RNG_BOOL, filter_func_within,         2, {VALUE_TYPE_FLT_RNG, VALUE_TYPE_BOOL}},
    {"within",    6, NODE_TYPE_FUNC, FUNC_TYPE_FLT_BOOL,     filter_func_within_flt,     2, {VALUE_TYPE_FLT, VALUE_TYPE_BOOL}},
    {"protein",   7, NODE_TYPE_FUNC, FUNC_TYPE_VOID,         filter_func_protein,        0},
    {"bfactor",   7, NODE_TYPE_FUNC, FUNC_TYPE_FLT_RNG,      NULL,                       1, VALUE_TYPE_FLT_RNG},
    {"resname",   7, NODE_TYPE_FUNC, FUNC_TYPE_STR,          filter_func_resname,        1, VALUE_TYPE_STR},
    {"residue",   7, NODE_TYPE_FUNC, FUNC_TYPE_INT_RNG,      filter_func_residue,        1, VALUE_TYPE_INT_RNG},
    {"residue",   7, NODE_TYPE_FUNC, FUNC_TYPE_INT,          filter_func_residue_int,    1, VALUE_TYPE_INT},
    {"occupancy", 8, NODE_TYPE_FUNC, FUNC_TYPE_FLT_RNG,      NULL,                       1, VALUE_TYPE_FLT_RNG},
};

static inline void enc_rel_ptr(int16_t* ptr, void* adress) {
    *ptr = (int16_t)((uint8_t*)adress - (uint8_t*)ptr);
}

static inline void* dec_rel_ptr(const int16_t* ptr) {
    return (uint8_t*)ptr + *ptr;
}

static void log_error(const char* msg) {
     fprintf(stderr, "ERROR: %s\n", msg);
}

static void log_error_with_context(const char* msg, const char* str, uint32_t len, str_slice_t carret) {
    fprintf(stderr, "ERROR: %s\n", msg);

    if(str && carret.length) {
        const uint32_t cont_w = 40; // Context width in character
        uint32_t offset = (int)((int)carret.offset  - (int)cont_w) < 0 ? 0 : (carret.offset - cont_w);
        uint32_t length = (carret.offset + carret.length + cont_w) > len ? len : (carret.offset + carret.length + cont_w);

        int carret_w = carret.length < 40 ? carret.length : 40;
        char buf[64] = {0};
        buf[0] = '^';
        for (uint32_t i = 1; i < carret.length; i++) {
            buf[i] = '_';
        }
    
        fprintf(stderr, "%.*s\n", length, str + offset);
        fprintf(stderr, "\033[91m%*s%s\033[m\n", (int)(carret.offset - offset), "", buf);
    }
}

static inline const uint32_t match_keyword(const char* str, uint32_t len) {
    for (uint32_t i = 1; i < ARRAY_SIZE(keyword_table); ++i) {
        if (keyword_table[i].len < len) {
            continue;
        }
        else if (keyword_table[i].len == len) {
            if (strncmp(keyword_table[i].str, str, len) == 0) return i;
        }
        else if (keyword_table[i].len > len) {
            return 0;
        }
    }
    return 0;
}

static inline uint32_t match_ident(const char* str, uint32_t len, uint32_t arg_count, const value_type_t arg_types[MAX_FUNC_ARGS]) {
    for (uint32_t i = 1; i < ARRAY_SIZE(ident_table); ++i) {
        ident_t ident = ident_table[i];
        if (ident.len < len) {
            continue;
        }
        else if (ident.len == len) {
            if (strncmp(ident.str, str, len) == 0 &&
                arg_count == ident.arg_count &&
                memcmp(arg_types, ident.arg_type, sizeof(value_type_t) * arg_count) == 0) {
                return i;
            }
        }
        else if (ident_table[i].len > len) {
            return 0;
        }
    }
    return 0;
}

static inline bool match_ident_str(const char* str, uint32_t len) {
    for (uint32_t i = 1; i < ARRAY_SIZE(ident_table); ++i) {
        ident_t ident = ident_table[i];
        if (ident.len < len) {
            continue;
        }
        else if (ident.len == len) {
            if (strncmp(ident.str, str, len) == 0) {
                return i;
            }
        }
        else if (ident_table[i].len > len) {
            return 0;
        }
    }
    return 0;
}

static token_t get_next_token_from_buffer(lexer_t* lexer) {
    ASSERT(lexer);
    token_t t = {0};
    t.type = TOKEN_UNDEFINED;

    char* buf = lexer->buff;
    for (uint32_t i = lexer->at; buf[i]; ++i) {
        if (is_whitespace(buf[i])) {
            continue;
        }
        else if (is_digit(buf[i])) {
            // @NOTE: Number
            t.type = TOKEN_NUM;
            t.str.offset = i++;
            while (buf[i] && is_digit(buf[i])) ++i;
            t.str.length = i - t.str.offset;
            break;
        }
        else if (is_alpha(buf[i]) || buf[i] == '_') {
            // @NOTE: Identifier, Keyword, String-literal
            t.str.offset = i++;
            while (buf[i] && (is_alpha(buf[i]) || is_digit(buf[i]) || buf[i] == '_')) ++i;
            t.str.length = i - t.str.offset;

            uint32_t idx = match_keyword(buf + t.str.offset, t.str.length);
            if (idx) {
                t.type = keyword_table[idx].type;
            } else if (match_ident_str(buf + t.str.offset, t.str.length)) {
                t.type = TOKEN_IDENT;
            } else {
                while (buf[i] && is_alpha(buf[i]) && is_digit(buf[i])) ++i;
                t.str.length = i - t.str.offset;
                t.type = TOKEN_STR;
            }
            break;
        }
        else {
            // @NOTE: Single character token
            t.type = buf[i];
            t.str.offset = i++;
            t.str.length = 1;
            break;
        }
    }

    if (t.type == TOKEN_UNDEFINED) t.type = TOKEN_END;
    return t;
}

static token_t peek_token(lexer_t* lexer) {
    return get_next_token_from_buffer(lexer);
}

static token_t next_token(lexer_t* lexer) {
    token_t tok = get_next_token_from_buffer(lexer);
    lexer->at = tok.str.offset + tok.str.length;
    return tok;
}

static bool require_token_type(lexer_t* lexer, token_type_t type, token_t* tok_ptr) {
    bool match = false;
    token_t tok = get_next_token_from_buffer(lexer);
    if (type == tok.type) {
        lexer->at = tok.str.offset + tok.str.length;
        if (tok_ptr) {
            *tok_ptr = tok;
            match = true;
        }
    }
    return tok.type == type;
}

static ast_node_t* parser_allocate_node(parse_context_t* s, node_type_t type) {
    ast_node_t* node = (ast_node_t*)arena_alloc(s->arena, sizeof(ast_node_t));
    if (!node) {
        log_error("Out of memory");
        return NULL;
    }
    node->type = type;
    return node;
}

static bool convert_node(ast_node_t* node, value_type_t dst_type) {
    ASSERT(node->type == NODE_TYPE_VALUE);
    const value_type_t src_type = node->_value.type;
    switch (dst_type) {
    case VALUE_TYPE_FLT:
        switch(src_type) {
        case VALUE_TYPE_FLT: goto finish_up;
        case VALUE_TYPE_INT: node->_value._flt = (float)node->_value._int; goto finish_up;
        default: return false;
        }
        break;
    case VALUE_TYPE_INT:
        switch(src_type) {
        case VALUE_TYPE_INT: goto finish_up;
        case VALUE_TYPE_FLT: node->_value._int = (int)node->_value._flt; goto finish_up;
        default: return false;
        }
    case VALUE_TYPE_FLT_RNG:
        switch(src_type) {
        case VALUE_TYPE_FLT_RNG: goto finish_up;
        case VALUE_TYPE_INT_RNG: {
            frange_t val = {(float)node->_value._irange.min, (float)node->_value._irange.max};
            node->_value._frange = val;
            goto finish_up;
        }
        case VALUE_TYPE_INT: {
            frange_t val = {(float)node->_value._int, (float)node->_value._int};
            node->_value._frange = val;
            goto finish_up;
        }
        case VALUE_TYPE_FLT: {
            frange_t val = {node->_value._flt, node->_value._flt};
            node->_value._frange = val;
            goto finish_up;
        }
        default: return false;
        }
    case VALUE_TYPE_INT_RNG:
        switch(src_type) {
        case VALUE_TYPE_INT_RNG: goto finish_up;
        case VALUE_TYPE_FLT_RNG: {
            irange_t val = {(int)node->_value._frange.min, (int)node->_value._frange.max};
            node->_value._irange = val;
            goto finish_up;
        }
        case VALUE_TYPE_INT: {
            irange_t val = {node->_value._int, node->_value._int};
            node->_value._irange = val;
            goto finish_up;
        }
        case VALUE_TYPE_FLT: {
            irange_t val = {(int)node->_value._flt, (int)node->_value._flt};
            node->_value._irange = val;
            goto finish_up;
        }
        default: return false;
        }
    case VALUE_TYPE_STR:
        switch(src_type) {
        case VALUE_TYPE_STR: goto finish_up;
        default: return false;
        }
    default: return false;
    }
finish_up:
    node->type = dst_type;
    node->return_type = dst_type;
    return true;
}

static void fix_precedence(ast_node_t** node) {
    // We parse from left to right thus unbalancing the tree in its right child node _child[1]
    // Therefore we only need to fix potential precedence issues down this path
    ASSERT(node);
    ast_node_t* parent = *node;
    ASSERT(parent && parent->_child[1]);
    ast_node_t* child = dec_rel_ptr(&parent->_child[1]);
    ASSERT(child->_child[0]);
    ast_node_t* grand_child = dec_rel_ptr(&child->_child[0]);

    if (child->type > parent->type) { // type is sorted on precedence
        enc_rel_ptr(&child->_child[0], parent);
        enc_rel_ptr(&parent->_child[1], grand_child);
        *node = child;
    }
}

static void* encode_args(mem_arena_t* arena, uint32_t arg_count, ast_node_t** arg_nodes) {
    if (arg_count == 1) {
        // No need to allocate and encode an array of relative pointers to the nodes, just encode the one node
        return arg_nodes[0];
    }
    int16_t* mem = arena_alloc(arena, arg_count * sizeof(int16_t));
    for (uint32_t i = 0; i < arg_count; ++i) {
        enc_rel_ptr(&mem[i], arg_nodes[i]);
    }
    return mem;
}

static void decode_args(const ast_node_t** arg_nodes, uint32_t arg_count, const void* mem) {
    if (arg_count == 1) {
        arg_nodes[0] = (const ast_node_t*)mem;
    }
    else {
        const int16_t* rel_ptr = (const int16_t*)mem;  
        for (uint32_t i = 0; i < arg_count; ++i) {
            arg_nodes[i] = (const ast_node_t*)dec_rel_ptr(&rel_ptr[i]);
        }
    }
}

static ast_node_t* parse_expression(parse_context_t*, lexer_t* lexer);
static ast_node_t* parse_identifier(parse_context_t*, lexer_t* lexer);

static ast_node_t* parse_identifier(parse_context_t* ctx, lexer_t* lexer) {
    token_t token = next_token(lexer);
    ASSERT(token.type == TOKEN_IDENT);

    str_slice_t ident_str = token.str;
    token = peek_token(lexer);

    ast_node_t* nodes[64] = {0};
    uint32_t    node_count = 0;
    uint32_t    arg_lengths[MAX_FUNC_ARGS] = {0};
    uint32_t    arg_count = 0;

    if (token.type == '(') { // Function call with arguments
        next_token(lexer); // (
        while(true) {
            token = peek_token(lexer);
            if (token.type == ')') {
                break;
            }
            else if (token.type == ',') {
                next_token(lexer); // ,
                ++arg_count;
                if (arg_count > MAX_FUNC_ARGS) {
                    log_error_with_context("Too many arguments for any function", ctx->expr_str, ctx->expr_len, token.str);
                    return NULL;
                }
            }
            else {
                ctx->cur_node = NULL;
                if (!(nodes[node_count++] = parse_expression(ctx, lexer))) return NULL;
                ++arg_lengths[arg_count];
            }
        }
        next_token(lexer); // )
        ++arg_count;
    }

    if (arg_count == 0) { // Perhaps function call with no arguments. i.e. 'all', 'none' etc.
        const uint32_t ident_idx = match_ident(ctx->expr_str + ident_str.offset, ident_str.length, 0, NULL);
        if (ident_idx == 0) {
            log_error("Could not match identifier");
            return NULL;
        }
        ident_t ident = ident_table[ident_idx];
        ast_node_t* node = parser_allocate_node(ctx, ident.type);
        if (!node) return NULL;
        node->return_type = VALUE_TYPE_BOOL;
        node->_func.ident = (uint16_t)ident_idx;
        return node;
    }
    else if (arg_count == 1) {
        // One arg is a special case since all args are implicitly 'OR'-ed
        ast_node_t* node = NULL;
        for (uint32_t i = 0; i < node_count; ++i) {
            uint32_t ident_idx = match_ident(ctx->expr_str + ident_str.offset, ident_str.length, 1, &nodes[i]->return_type);
            if (!ident_idx) {
                log_error_with_context("Could not match identifier", ctx->expr_str, ctx->expr_len, ident_str);
                return NULL;
            }

            ast_node_t* func_node = parser_allocate_node(ctx, NODE_TYPE_FUNC);
            if (!func_node) return NULL;
            func_node->return_type = VALUE_TYPE_BOOL;
            func_node->_func.ident = (uint16_t)ident_idx;
            enc_rel_ptr(&func_node->_func.args, encode_args(ctx->arena, 1, &nodes[i]));

            if (node) {
                ast_node_t* or_node = parser_allocate_node(ctx, NODE_TYPE_OR);
                if (!or_node) return NULL;
                or_node->return_type = VALUE_TYPE_BOOL;
                enc_rel_ptr(&or_node->_child[0], node);
                enc_rel_ptr(&or_node->_child[1], func_node);
                node = or_node;
            } else {
                node = func_node;
            }
        }
        return node;
    }
    else {
        value_type_t arg_types[64];
        for (uint32_t i = 0; i < arg_count; ++i) {
            if (arg_lengths[i] != 1) {
                log_error_with_context("Only one argument is allowed per slot in a multi-argument function call", ctx->expr_str, ctx->expr_len, ident_str);
                return NULL;
            }
            arg_types[i] = nodes[i]->return_type;
        }
        uint32_t ident_idx = match_ident(ctx->expr_str + ident_str.offset, ident_str.length, arg_count, arg_types);
        if (!ident_idx) {
            log_error_with_context("Could not match identifier", ctx->expr_str, ctx->expr_len, ident_str);
            return NULL;
        }

        ast_node_t* node = parser_allocate_node(ctx, NODE_TYPE_FUNC);
        if (!node) return NULL;
        node->return_type = VALUE_TYPE_BOOL;
        node->_func.ident = (uint16_t)ident_idx;
        enc_rel_ptr(&node->_func.args, encode_args(ctx->arena, arg_count, nodes));
        return node;
    }
}

static ast_node_t* parse_number(parse_context_t* ctx, lexer_t* lexer) {
    token_t token = {0};
    if (require_token_type(lexer, TOKEN_NUM, &token)) {
        ast_node_t* node = parser_allocate_node(ctx, NODE_TYPE_VALUE);
        str_slice_t str = token.str;
        token_t next = peek_token(lexer);
        if (next.type == '.') {
            str.length += next.str.length;
            next_token(lexer); // '.'
            next = peek_token(lexer);
            if (next.type == TOKEN_NUM) {
                next_token(lexer); // [0-9]
                str.length += next.str.length;
            }
            node->return_type = VALUE_TYPE_FLT;
            node->_value.type = VALUE_TYPE_FLT;
            node->_value._flt = parse_float(ctx->expr_str + str.offset, str.length);
        } else {
            node->return_type = VALUE_TYPE_INT;
            node->_value.type = VALUE_TYPE_INT;
            node->_value._int = parse_int(ctx->expr_str + str.offset, str.length);
        }
        return node;
    }
    log_error_with_context("Could not parse numeric expression", ctx->expr_str, ctx->expr_len, token.str);
    return NULL;
}

static ast_node_t* parse_expression(parse_context_t* ctx, lexer_t* lexer) {
    token_t token = {0};

    do {
        token = peek_token(lexer);

        if (token.type == TOKEN_UNDEFINED || token.type == TOKEN_END) {
            break;
        }

        switch (token.type) {
        case '(':
        {
            next_token(lexer); // (
            ast_node_t* node = parse_expression(ctx, lexer);
            if (!node) return NULL;
            if (!require_token_type(lexer, ')', &token)) {
                log_error_with_context("Expected ')' here", ctx->expr_str, ctx->expr_len, token.str);
                return NULL;
            }
            ctx->cur_node = node;
            break;
        }
        case ')':
            return ctx->cur_node;
        case '@':
        {
            next_token(lexer); // @
            ast_node_t* str_node = parse_expression(ctx, lexer);
            if (!str_node) return NULL;
            if (str_node->type != NODE_TYPE_VALUE || str_node->_value.type != VALUE_TYPE_STR) {
                log_error_with_context("Could expression following @ did not resolve to string", ctx->expr_str, ctx->expr_len, str_node->str);
                return NULL;
            }

            ast_node_t* ref_node = parser_allocate_node(ctx, NODE_TYPE_REF);
            if (!ref_node) return NULL;

            ref_node->return_type = VALUE_TYPE_BOOL;
            enc_rel_ptr(&ref_node->_child[0], str_node);
            ctx->cur_node = ref_node;
            break;
        }
        case TOKEN_IDENT:
        {
            ctx->cur_node = parse_identifier(ctx, lexer);
            if (!ctx->cur_node) return NULL;
            break;
        }
        case TOKEN_AND:
        case TOKEN_OR:
        {
            token_t t = next_token(lexer); // and / or
            node_type_t node_type = token.type == TOKEN_AND ? NODE_TYPE_AND : NODE_TYPE_OR;
            ast_node_t* node = parser_allocate_node(ctx, node_type);
            if (!node) return NULL;
            
            ast_node_t* prv_node = ctx->cur_node;
            ctx->cur_node = node;
            ast_node_t* arg_node[2] = {prv_node, parse_expression(ctx, lexer)};

            if (!arg_node[0] || arg_node[0]->return_type != VALUE_TYPE_BOOL) {
                log_error_with_context("Left hand side expression for logic operation did not evaluate to bool", ctx->expr_str, ctx->expr_len, t.str);
                return NULL;
            }
            if (!arg_node[1] || arg_node[1]->return_type != VALUE_TYPE_BOOL) {
                log_error_with_context("Right hand side expression for logic operation did not evaluate to bool", ctx->expr_str, ctx->expr_len, t.str);
                return NULL;
            }

            node->return_type = VALUE_TYPE_BOOL;
            enc_rel_ptr(&node->_child[0], arg_node[0]);
            enc_rel_ptr(&node->_child[1], arg_node[1]);
            fix_precedence(&node);
            ctx->cur_node = node;
            break;
        }
        case TOKEN_NOT:
        {
            token_t t = next_token(lexer); // not
            ast_node_t* node = parser_allocate_node(ctx, NODE_TYPE_NOT);
            if (!node) return NULL;

            node->type = NODE_TYPE_NOT;
            node->return_type = VALUE_TYPE_BOOL;

            token_t tok = peek_token(lexer);
            ast_node_t* arg_node = parse_expression(ctx, lexer);
            ctx->cur_node = node;

            if (!arg_node || arg_node->return_type != VALUE_TYPE_BOOL) {
                log_error_with_context("Expression following 'not' did not evaluate to bool", ctx->expr_str, ctx->expr_len, t.str);
                return NULL;
            }

            enc_rel_ptr(&node->_child[0], arg_node);            
            break;
        }
        case '-': // Unary operator '-'
        {
            next_token(lexer); // -
            ast_node_t* node = parse_expression(ctx, lexer);
            if (!node) return NULL;

            if (node->type == NODE_TYPE_VALUE && node->_value.type == VALUE_TYPE_FLT) {
                node->_value._flt = -node->_value._flt;
            } else if (node->type == NODE_TYPE_VALUE && node->_value.type == VALUE_TYPE_INT) {
                node->_value._int = -node->_value._int;
            } else {
                log_error_with_context("Cannot apply unary operator '-' to expression", ctx->expr_str, ctx->expr_len, node->str);
                return NULL;
            }
            return node;
            break;
        }
        case '+': // Unary operator '+'
        {
            next_token(lexer); // +
            ast_node_t* node = parse_expression(ctx, lexer);
            break;
        }
        case ':':
        {
            token_t delim_tok = next_token(lexer); // :
            ast_node_t* prev_node = ctx->cur_node;
            ast_node_t* next_node;
            if (!prev_node) {
                log_error("Missing token before range delimiter ':'");
                return NULL;
            }
            if (prev_node->type == NODE_TYPE_VALUE && prev_node->_value.type == VALUE_TYPE_FLT) {
                next_node = parse_expression(ctx, lexer);
                if (!next_node || !convert_node(next_node, VALUE_TYPE_FLT)) {
                    log_error_with_context("Token following ':' could not be interpreted as float", ctx->expr_str, ctx->expr_len, delim_tok.str);
                    return NULL;
                }
            }
            else if (prev_node->type == NODE_TYPE_VALUE && prev_node->_value.type == VALUE_TYPE_INT) {
                next_node = parse_expression(ctx, lexer);
                if (!next_node || !convert_node(next_node, VALUE_TYPE_INT)) {
                    log_error_with_context("Token following ':' could not be interpreted as integer", ctx->expr_str, ctx->expr_len, delim_tok.str);
                    return NULL;
                }
            }
            else {
                log_error_with_context("Token preceeding ':' is not valid within a range expression", ctx->expr_str, ctx->expr_len, prev_node->str);
                return NULL;
            }
            
            ast_node_t* node = parser_allocate_node(ctx, NODE_TYPE_VALUE);
            if (!node) return NULL;

            if (prev_node->_value.type == VALUE_TYPE_FLT) {
                ASSERT(next_node->_value.type == VALUE_TYPE_FLT);
                node->return_type = VALUE_TYPE_FLT_RNG;
                node->_value.type = VALUE_TYPE_FLT_RNG;
                node->_value._frange.min = prev_node->_value._flt;
                node->_value._frange.max = next_node->_value._flt;
            }
            else {
                ASSERT(prev_node->_value.type == VALUE_TYPE_INT);
                ASSERT(next_node->_value.type == VALUE_TYPE_INT);
                node->return_type = VALUE_TYPE_INT_RNG;
                node->_value.type = VALUE_TYPE_INT_RNG;
                node->_value._irange.min = prev_node->_value._int;
                node->_value._irange.max = next_node->_value._int;
            }
            ctx->cur_node = node;
            break;
        }
        case TOKEN_NUM:
        {
            ast_node_t* node = parse_number(ctx, lexer);
            ctx->cur_node = node;
            if (peek_token(lexer).type != ':') return node;
            break;
        }
        case TOKEN_STR:
        {
            token = next_token(lexer);
            ast_node_t* node = parser_allocate_node(ctx, NODE_TYPE_VALUE);
            if (!node) return NULL;

            node->return_type = VALUE_TYPE_STR;
            node->_value.type = VALUE_TYPE_STR;
            node->_value._str = token.str;
            node->str = token.str;
            return node;
        }
        default:
            log_error_with_context("Unexpected token", ctx->expr_str, ctx->expr_len, token.str);
            return NULL;
        }
    } while (token.type != TOKEN_END);

    return ctx->cur_node;
}

static void indent(FILE* file, int amount) {
    for (int i = 0; i < amount; ++i) {
        fprintf(file, "\t");
    }
}

static const char* node_type_to_str(node_type_t type) {
    switch (type) {
    case NODE_TYPE_UNDEFINED: return "undefined";
    case NODE_TYPE_AND:       return "and";
    case NODE_TYPE_OR:        return "or";
    case NODE_TYPE_NOT:       return "not";
    case NODE_TYPE_FUNC:      return "function";
    case NODE_TYPE_VALUE:     return "value";
    case NODE_TYPE_REF:       return "reference";
    default:                  return "unknown";
    }
}

static const char* value_type_to_str(value_type_t type) {
    switch (type) {
    case VALUE_TYPE_UNDEFINED: return "undefined";
    case VALUE_TYPE_BOOL:      return "bool";
    case VALUE_TYPE_INT:       return "int";
    case VALUE_TYPE_FLT:       return "float";
    case VALUE_TYPE_INT_RNG:   return "irange";
    case VALUE_TYPE_FLT_RNG:   return "frange";
    case VALUE_TYPE_STR:       return "string";
    default:                   return "unknown";
    }
}

static void print_label(FILE* file, const ast_node_t* node, const char* expr_str) {
    switch(node->type) {
    case NODE_TYPE_VALUE:
        switch(node->_value.type) {
        case VALUE_TYPE_INT:
            fprintf(file, "%i", node->_value._int);
            break;
        case VALUE_TYPE_FLT:
            fprintf(file, "%f", node->_value._flt);
            break;
        case VALUE_TYPE_INT_RNG:
            fprintf(file, "%i:%i", node->_value._irange.min, node->_value._irange.max);
            break;
        case VALUE_TYPE_FLT_RNG:
            fprintf(file, "%f:%f", node->_value._frange.min, node->_value._frange.max);
            break;
        case VALUE_TYPE_STR:
            fprintf(file, "%.*s", node->_value._str.length, expr_str + node->_value._str.offset);
            break;
        }
        break;
    case NODE_TYPE_FUNC:
        fprintf(file, "%s", ident_table[node->_func.ident].str);
        break;
    default:
        fprintf(file, "%s", node_type_to_str(node->type));
        break;
    }
    fprintf(file, " (%s)", value_type_to_str(node->return_type));
}

static void print_node(const ast_node_t* node, FILE* file, int depth, const char* expr_str) {
    if (!node || !file) return;

    indent(file, depth);
    fprintf(file, "{\"name\": \"");
    print_label(file, node, expr_str);
    fprintf(file, "\",\n");
    switch (node->type) {
    case NODE_TYPE_AND:
    case NODE_TYPE_OR:
    case NODE_TYPE_NOT:
    {
        if (node->_child[0]) {
            indent(file, depth);
            fprintf(file, "\"children\": [\n");
            print_node((const ast_node_t*)dec_rel_ptr(&node->_child[0]), file, depth + 1, expr_str);
            if (node->_child[1]) {
                print_node((const ast_node_t*)dec_rel_ptr(&node->_child[1]), file, depth + 1, expr_str);
            }
            indent(file, depth);
            fprintf(file, "]\n");
        }
        break;
    }
    case NODE_TYPE_FUNC:
    {
        ident_t ident = ident_table[node->_func.ident];
        const ast_node_t* arg_nodes[MAX_FUNC_ARGS];
        decode_args(arg_nodes, ident.arg_count, dec_rel_ptr(&node->_func.args));
        indent(file, depth);
        fprintf(file, "\"children\": [\n");
        for (uint32_t i = 0; i < ident.arg_count; ++i) {
            print_node(arg_nodes[i], file, depth + 1, expr_str);
        }
        indent(file, depth);
        fprintf(file, "]\n");
    }
    default:
        break;
    }
    indent(file, depth);
    fprintf(file, "},\n");
}

static void save_tree_to_json(const ast_node_t* tree, const char* filename, const char* expr_str) {
    FILE* file = fopen(filename, "w");

    if (file) {
        fprintf(file, "var treeData = [\n");
        print_node(tree, file, 0, expr_str);
        fprintf(file, "];");
        fclose(file);
    }
}

typedef struct eval_context_t eval_context_t;
struct eval_context_t {
    const char* expr_str;
    uint32_t    expr_len;
    uint64_t    bit_count;
    uint64_t*   bit_buf[2];
    const mold_filter_context* filt_ctx;
};

static inline void swap_bit_buffers(eval_context_t* ctx) {
    uint64_t* tmp = ctx->bit_buf[0];
    ctx->bit_buf[0] = ctx->bit_buf[1];
    ctx->bit_buf[1] = tmp;
}

static value_t call_func(ident_t ident, const value_t* args, eval_context_t* ctx) {
    value_t res;
    switch (ident.func_type)
    {
    case FUNC_TYPE_VOID:
    {
        void (*func)(uint64_t*, const mold_molecule*) = ident.func_addr;
        func(ctx->bit_buf[0], ctx->filt_ctx->mol);
        break;
    }
    case FUNC_TYPE_STR:
    {
        ASSERT(args && args[0].type == VALUE_TYPE_STR);
        void (*func)(const char*, uint64_t*, const mold_molecule*) = ident.func_addr;
        str_slice_t str = args[0]._str;
        ASSERT(str.length < 127);
        char buf[128] = {0};
        strncpy(buf, ctx->expr_str + str.offset, str.length);
        func(buf, ctx->bit_buf[0], ctx->filt_ctx->mol);
        break;
    }
    case FUNC_TYPE_FLT:
    {
        ASSERT(args && args[0].type == VALUE_TYPE_FLT);
        float val = args[0]._flt;
        void (*func)(float, uint64_t*, const mold_molecule*) = ident.func_addr;
        func(val, ctx->bit_buf[0], ctx->filt_ctx->mol);
        break;
    }
    case FUNC_TYPE_INT:
    {
        ASSERT(args && args[0].type == VALUE_TYPE_INT);
        int val = args[0]._int;
        void (*func)(int, uint64_t*, const mold_molecule*) = ident.func_addr;
        func(val, ctx->bit_buf[0], ctx->filt_ctx->mol);
        break;
    }
    case FUNC_TYPE_FLT_RNG:
    {
        ASSERT(args && args[0].type == VALUE_TYPE_FLT_RNG);
        frange_t range = args[0]._frange;
        void (*func)(float, float, uint64_t*, const mold_molecule*) = ident.func_addr;
        func(range.min, range.max, ctx->bit_buf[0], ctx->filt_ctx->mol);
        break;
    }
    case FUNC_TYPE_INT_RNG:
    {
        ASSERT(args && args[0].type == VALUE_TYPE_INT_RNG);
        irange_t range = args[0]._irange;
        void (*func)(int, int, uint64_t*, const mold_molecule*) = ident.func_addr;
        func(range.min, range.max, ctx->bit_buf[0], ctx->filt_ctx->mol);
        break;
    }
    case FUNC_TYPE_FLT_BOOL:
    {
        ASSERT(args && args[0].type == VALUE_TYPE_FLT && args[1].type == VALUE_TYPE_BOOL);
        float val = args[0]._flt;
        uint64_t* src_bits = args[1]._bool;
        void (*func)(float, const uint64_t*, uint64_t*, const mold_molecule*) = ident.func_addr;
        func(val, src_bits, ctx->bit_buf[0], ctx->filt_ctx->mol);
        break;
    }
    case FUNC_TYPE_FLT_RNG_BOOL:
    {
        ASSERT(args && args[0].type == VALUE_TYPE_FLT_RNG && args[1].type == VALUE_TYPE_BOOL);
        frange_t range = args[0]._frange;
        uint64_t* src_bits = args[1]._bool;
        void (*func)(float, float, const uint64_t*, uint64_t*, const mold_molecule*) = ident.func_addr;
        func(range.min, range.max, src_bits, ctx->bit_buf[0], ctx->filt_ctx->mol);
        break;
    }
    default:
        ASSERT(false);
        break;
    }
    res.type = VALUE_TYPE_BOOL;
    res._bool = ctx->bit_buf[0];
    return res;
}

static value_t evaluate(const ast_node_t* node, eval_context_t* ctx) {
    switch (node->type)
    {
    case NODE_TYPE_AND:
    {
        const ast_node_t* child[2] = {dec_rel_ptr(&node->_child[0]), dec_rel_ptr(&node->_child[1])};
        evaluate(child[0], ctx);
        swap_bit_buffers(ctx);
        evaluate(child[1], ctx);
        swap_bit_buffers(ctx);
        bit_and(ctx->bit_buf[0], ctx->bit_buf[0], ctx->bit_buf[1], 0, ctx->bit_count);
        value_t res;
        res.type = VALUE_TYPE_BOOL;
        res._bool = ctx->bit_buf[0];
        return res;
    }
    case NODE_TYPE_OR:
    {
        const ast_node_t* child[2] = {dec_rel_ptr(&node->_child[0]), dec_rel_ptr(&node->_child[1])};
        evaluate(child[0], ctx);
        swap_bit_buffers(ctx);
        evaluate(child[1], ctx);
        swap_bit_buffers(ctx);
        bit_or(ctx->bit_buf[0], ctx->bit_buf[0], ctx->bit_buf[1], 0, ctx->bit_count);
        value_t res;
        res.type = VALUE_TYPE_BOOL;
        res._bool = ctx->bit_buf[0];
        return res;
    }
    case NODE_TYPE_NOT:
    {
        const ast_node_t* child = (ast_node_t*)dec_rel_ptr(&node->_child[0]);
        evaluate(child, ctx);
        bit_invert(ctx->bit_buf[0], 0, ctx->bit_count);
        value_t res;
        res.type = VALUE_TYPE_BOOL;
        res._bool = ctx->bit_buf[0];
        return res;
    }
    case NODE_TYPE_FUNC:
    {
        ident_t ident = ident_table[node->_func.ident];
        const ast_node_t* arg_nodes[MAX_FUNC_ARGS];
        value_t           args[MAX_FUNC_ARGS];
        decode_args(arg_nodes, ident.arg_count, dec_rel_ptr(&node->_func.args));
        for (uint32_t i = 0; i < ident.arg_count; ++i) {
            args[i] = evaluate(arg_nodes[i], ctx);
        }
        value_t res = call_func(ident, args, ctx);
        return res;
    }
    case NODE_TYPE_REF:
    {
        const ast_node_t* str_node = dec_rel_ptr(&node->_child[0]);
        ASSERT(str_node->type == NODE_TYPE_VALUE && str_node->_value.type == VALUE_TYPE_STR);
        str_slice_t str = str_node->_value._str;
        if (str.length > 255) {
            log_error_with_context("stored selection identifier exceeds 255 characters", ctx->expr_str, ctx->expr_len, str);
            break;
        }
        char buf[256] = {0};
        strncpy(buf, ctx->expr_str + str.offset, str.length);

        uint32_t idx = -1;
        for (uint32_t i = 0; i < ctx->filt_ctx->sel_count; ++i) {
            if (strncmp(buf, ctx->filt_ctx->sel[i].ident, str.length) == 0) {
                idx = i;
                break;
            }
        }

        if (idx == -1) {
            log_error_with_context("could not find matching identifier for stored selection", ctx->expr_str, ctx->expr_len, str);
            break;
        }

        const uint64_t block_count = (ctx->bit_count + 63) / 64;
        memcpy(ctx->bit_buf[0], ctx->filt_ctx->sel[idx].bits, block_count * sizeof(uint64_t));
        value_t res = {VALUE_TYPE_BOOL};
        res._bool = ctx->bit_buf[0];
        return res;
    }
    case NODE_TYPE_VALUE:
        switch(node->_value.type) {
        case VALUE_TYPE_FLT:
        {
            value_t res;
            res.type = VALUE_TYPE_FLT;
            res._flt = node->_value._flt;
            return res;
            break;
        }
        case VALUE_TYPE_INT:
        {
            value_t res;
            res.type = VALUE_TYPE_INT;
            res._int = node->_value._int;
            return res;
            break;
        }
        case VALUE_TYPE_STR:
        {
            value_t res;
            res.type = VALUE_TYPE_STR;
            res._str = node->_value._str;
            return res;
            break;
        }
        case VALUE_TYPE_FLT_RNG:
        {
            value_t res;
            res.type = VALUE_TYPE_FLT_RNG;
            res._frange = node->_value._frange;
            return res;
            break;
        }
        case VALUE_TYPE_INT_RNG:
        {
            value_t res;
            res.type = VALUE_TYPE_INT_RNG;
            res._irange = node->_value._irange;
            return res;
            break;
        }
        default:
            break;
        }
    }

    value_t res = {0};
    return res;
}

/*
bool mold_filter_apply(uint64_t* bits, uint64_t bit_count, const mold_filter* filter, const mold_filter_context* filt_ctx) {
    if (bits && filter && filt_ctx) {
        const uint64_t block_count = (bit_count + 63) / 64;
        uint64_t* tmp_bits = (uint64_t*)calloc(block_count, sizeof(uint64_t));
        const ast_node_t* root = (ast_node_t*)((uint8_t*)filter + filter->root_offset);

        bit_clear(bits, 0, bit_count);
        bit_clear(tmp_bits, 0, bit_count);

        eval_context_t eval_ctx = {0};
        eval_ctx.expr_str = filter->expr_str;
        eval_ctx.expr_len = filter->expr_len;
        eval_ctx.bit_buf[0] = bits;
        eval_ctx.bit_buf[1] = tmp_bits;
        eval_ctx.bit_count = bit_count;
        eval_ctx.filt_ctx = filt_ctx;

        value_t res = evaluate(root, &eval_ctx);
        free(tmp_bits);
        return res.type != VALUE_TYPE_UNDEFINED;
    }

    return false;
}
*/

bool mold_filter_parse(const char* expression, const mold_filter_context* context) {
    int balance = 0;
    for (const char* c = expression; *c; ++c) {
        if (*c == '(') ++balance;
        else if (*c == ')') --balance;
    }
    if (balance != 0) {
        log_error("Unbalanced parentheses in expression");    
        return false;
    }

    const char* expr_str = expression;
    uint32_t    expr_len = strlen(expression);

    lexer_t lexer = {0};
    lexer.buff = expr_str;
    lexer.buff_len = expr_len;

    char ast_mem[1 << 12]; // enough for 256 nodes
    mem_arena_t ast_arena = {0};
    arena_init(&ast_arena, ast_mem, ARRAY_SIZE(ast_mem));

    parse_context_t parse_ctx = {0};
    parse_ctx.arena = &ast_arena;
    parse_ctx.expr_str = expr_str;
    parse_ctx.expr_len = expr_len;
    parse_ctx.lexer = &lexer;

    ast_node_t* root = parse_expression(&parse_ctx, &lexer);
    if (root) {
#if 1
        // @NOTE: For debugging
        save_tree_to_json(root, "tree.json", expr_str);
#endif
        if (context && context->bits && context->bit_count && context->mol) {
            const uint64_t block_count = (context->bit_count + 63) / 64;
            uint64_t* tmp_bits = (uint64_t*)calloc(block_count, sizeof(uint64_t));

            bit_clear(context->bits, 0, context->bit_count);
            bit_clear(tmp_bits, 0, context->bit_count);

            eval_context_t eval_ctx = {0};
            eval_ctx.expr_str = expr_str;
            eval_ctx.expr_len = expr_len;
            eval_ctx.bit_buf[0] = context->bits;
            eval_ctx.bit_buf[1] = tmp_bits;
            eval_ctx.bit_count = context->bit_count;
            eval_ctx.filt_ctx = context;

            value_t res = evaluate(root, &eval_ctx);
            free(tmp_bits);
            return res.type != VALUE_TYPE_UNDEFINED;
        }
    }

    return false;
}

enum {
    Unknown = 0,
    H, He, Li, Be, B, C, N, O, F, Ne, Na, Mg, Al, Si, P, S, Cl, Ar, K, Ca, Sc, Ti, V,
    Cr, Mn, Fe, Co, Ni, Cu, Zn, Ga, Ge, As, Se, Br, Kr, Rb, Sr, Y, Zr, Nb, Mo, Tc, Ru,
    Rh, Pd, Ag, Cd, In, Sn, Sb, Te, I, Xe, Cs, Ba, La, Ce, Pr, Nd, Pm, Sm, Eu, Gd, Tb,
    Dy, Ho, Er, Tm, Yb, Lu, Hf, Ta, W, Re, Os, Ir, Pt, Au, Hg, Tl, Pb, Bi, Po, At, Rn,
    Fr, Ra, Ac, Th, Pa, U, Np, Pu, Am, Cm, Bk, Cf, Es, Fm, Md, No, Lr, Rf, Db, Sg, Bh,
    Hs, Mt, Ds, Rg, Cn, Uut, Fl, Mc, Lv, Ts, Og
};

static const char* element_symbols[] = {
    "Xx", "H",  "He", "Li", "Be", "B",  "C",  "N",  "O",  "F",  "Ne", "Na", "Mg", "Al", "Si", "P",  "S",  "Cl", "Ar", "K",  "Ca", "Sc", "Ti", "V",
    "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br", "Kr", "Rb", "Sr", "Y",  "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag",
    "Cd", "In", "Sn", "Sb", "Te", "I",  "Xe", "Cs", "Ba", "La", "Ce", "Pr", "Nd", "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb", "Lu",
    "Hf", "Ta", "W",  "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi", "Po", "At", "Rn", "Fr", "Ra", "Ac", "Th", "Pa", "U",  "Np", "Pu", "Am",
    "Cm", "Bk", "Cf", "Es", "Fm", "Md", "No", "Lr", "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Ds", "Rg", "Cn", "Nh", "Fl", "Mc", "Lv", "Ts", "Og" };

static const char* amino_acids[] = {
    "ALA", "ARG", "ASN", "ASP", "CYS", "CYX", "GLN", "GLU",
    "GLY", "HIS", "ILE", "LEU", "LYS", "MET", "PHE", "PRO", "SER",
    "THR", "TRP", "TYR", "VAL", "SEC", "PYL", "ASC", "GLX", "XLE",
};

static const char* dna[] = {"DA", "DA3", "DA5", "DC", "DC3", "DC5", "DG", "DG3", "DG5", "DT", "DT3", "DT5"};

static const char* acidic[] = {
    "ASP", "GLU"
};

static const char* basic[] = {
    "ARG", "HIS", "LYS"
};

static const char* neutral[] = {
    "VAL", "PHE", "GLN", "TYR", "HIS", "CYS", "MET", "TRP", "ASX", "GLX", "PCA", "HYP"
};

static const char* water[] = {
    "H2O", "HHO", "OHH", "HOH", "OH2", "SOL", "WAT", "TIP", "TIP2", "TIP3", "TIP4"
};

static const char* hydrophobic[] = {
    "ALA", "VAL", "ILE", "LEU", "MET", "PHE", "TYR", "TRP", "CYX"
};

static inline uint8_t element_symbol_to_index(const char* str) {
    for (uint8_t i = 0; i < ARRAY_SIZE(element_symbols); ++i) {
        if (strcmp(str, element_symbols[i]) == 0) return i;
    }
    return 0;
}

void filter_func_all(uint64_t* bits, const mold_molecule* mol) {
    bit_set(bits, 0, mol->atom.count);
}

void filter_func_none(uint64_t* bits, const mold_molecule* mol) {
    bit_clear(bits, 0, mol->atom.count);
}

void filter_func_protein(uint64_t* bits, const mold_molecule* mol) {
    const char** name = mol->atom.name;
    for (uint32_t ri = 0; ri < mol->residue.count; ++ri) {
        const mold_range range = mol->residue.atom_range[ri];
        const uint32_t range_ext = (range.end - range.beg);
        if (4 <= range_ext && range_ext <= 30) {
            uint32_t mask = 0;
            for (uint32_t i = range.beg; i < range.end; ++i) {
                if (!(mask & 1) && name[i][0] == 'N')                         { mask |= 1; continue; }
                if (!(mask & 2) && name[i][0] == 'C' && name[i][1] == 'A')    { mask |= 2; continue; }
                if (!(mask & 4) && name[i][0] == 'C')                         { mask |= 4; continue; }
                if (!(mask & 8) && name[i][0] == 'O')                         { mask |= 8; continue; }
                if (!(mask & 8) && i == (range.end - 1) && name[i][0] == 'O') { mask |= 8; continue; }
                if (mask == 15) break;
            }
            if (mask == 15) {
                bit_set(bits, range.beg, range_ext);
            }
        }
    }
}

void filter_func_water(uint64_t* bits, const mold_molecule* mol) {
    for (uint32_t i = 0; i < mol->residue.count; ++i) {
        const mold_range range = mol->residue.atom_range[i];
        const uint32_t range_ext = (range.end - range.beg);
        if (range_ext == 3) {
            const uint8_t* elem = mol->atom.element;
            uint32_t j = mol->residue.atom_range->beg;

            if (elem[j] == O && elem[j+1] == H && elem[j+2] == H ||
                elem[j] == H && elem[j+1] == O && elem[j+2] == H ||
                elem[j] == H && elem[j+1] == H && elem[j+2] == O) {
                bit_set(bits, range.beg, range_ext);
            }
        }
    }
}

void filter_func_name(const char* str, uint64_t* bits, const mold_molecule* mol) {
    const uint64_t blk_count = DIV_UP(mol->atom.count);
    for (uint64_t blk_idx = 0; blk_idx < blk_count; ++blk_idx) {
        const uint64_t bit_count = (blk_idx != (blk_count - 1)) ? 64 : (mol->atom.count & 63);
        uint64_t mask = 0;
        for (uint64_t bit = 0; bit < bit_count; ++bit) {
            const uint64_t i = blk_idx * 64 + bit;
            if (strcmp(str, mol->atom.name[i]) == 0) {
                mask |= 1ULL << bit;
            }
        }
        if (mask) bits[blk_idx] |= mask;
    }
}

void filter_func_element(const char* str, uint64_t* bits, const mold_molecule* mol) {
    uint8_t elem_idx = element_symbol_to_index(str);
    if (!elem_idx) return;

    const uint64_t blk_count = DIV_UP(mol->atom.count);
    for (uint64_t blk_idx = 0; blk_idx < blk_count; ++blk_idx) {
        const uint64_t bit_count = (blk_idx != (blk_count - 1)) ? 64 : (mol->atom.count & 63);
        uint64_t mask = 0;
        for (uint64_t bit = 0; bit < bit_count; ++bit) {
            const uint64_t i = blk_idx * 64 + bit;
            if (mol->atom.element[i] == elem_idx) {
                mask |= 1ULL << bit;
            }
        }
        if (mask) bits[blk_idx] |= mask;
    }
}

void filter_func_resname(const char* str, uint64_t* bits, const mold_molecule* mol) {
    for (uint32_t i = 0; i < mol->residue.count; ++i) {
        if (strcmp(str, mol->residue.name[i]) == 0) {
            const mold_range range = mol->residue.atom_range[i];
            const uint32_t range_ext = (range.end - range.beg);
            bit_set(bits, range.beg, range_ext);
        }
    }
}

void filter_func_resid(int min_range, int max_range, uint64_t* bits, const mold_molecule* mol) {
    for (uint32_t i = 0; i < mol->residue.count; ++i) {
        const int32_t id = mol->residue.id[i];
        if (min_range <= id && id <= max_range) {
            const mold_range range = mol->residue.atom_range[i];
            const uint32_t range_ext = (range.end - range.beg);
            bit_set(bits, range.beg, range_ext);
        }
    }
}

void filter_func_residue(int min_range, int max_range, uint64_t* bits, const mold_molecule* mol) {
    min_range = MAX(min_range, 0);
    max_range = MIN(max_range, (int)mol->residue.count - 1);
    for (int i = min_range; i <= max_range; ++i) {
        const mold_range range = mol->residue.atom_range[i];
        const uint32_t range_ext = (range.end - range.beg);
        bit_set(bits, range.beg, range_ext);
    }
}

void filter_func_chain(const char* str, uint64_t* bits, const mold_molecule* mol) {
    for (uint32_t i = 0; i < mol->chain.count; ++i) {
        if (strcmp(str, mol->chain.id[i]) == 0) {
            const mold_range range = mol->chain.atom_range[i];
            const uint32_t range_ext = range.end - range.beg;
            bit_set(bits, range.beg, range.end);
        }
    }
}

#define SPATIAL_GRID_MIN_SIZE 16
#define GRID_RES 32
#define INIT_CELL_COORD(x,y,z) {((x) - aabb_min[0]) * inv_aabb_ext[0], ((y) - aabb_min[1]) * inv_aabb_ext[1], ((z) - aabb_min[2]) * inv_aabb_ext[2]}

inline uint32_t compute_cell_idx(const uint32_t cc[3]) {
    return cc[2] * GRID_RES * GRID_RES + cc[1] * GRID_RES + cc[0];
}

typedef struct grid_cell_t grid_cell_t;
struct grid_cell_t {
    uint32_t offset : 22;
    uint32_t count  : 9;
    uint32_t mark   : 1;
};

static_assert(sizeof(grid_cell_t) == 4, "sizeof grid_cell should be 4");

void filter_func_within(float min_range, float max_range, const uint64_t* in_bits, uint64_t* out_bits, const mold_molecule* mol) {
    // Create a coarse uniform grid where we mark the cells which are candidate locations based on atomic positions of in_bits.
    // As a second step we go over the entire set of atoms and bin them to the uniform grid, if they end up in a candidate voxel, we do additional distance tests

    const float* x = mol->atom.x;
    const float* y = mol->atom.y;
    const float* z = mol->atom.z;

    const float min_d2 = min_range > 0 ? min_range * min_range : 0;
    const float max_d2 = max_range > 0 ? max_range * max_range : 0;

    if (max_d2 - min_d2 < FLT_EPSILON) return;

    const uint64_t N = bit_count(in_bits, 0, mol->atom.count);
    const uint64_t M = GRID_RES * GRID_RES * GRID_RES;

    if (N > SPATIAL_GRID_MIN_SIZE) {
        const size_t req_mem = M * sizeof(grid_cell_t) + N * (sizeof(uint32_t) * 2 + sizeof(float) * 6);
        void* mem = calloc(req_mem, 1); // Need 0 for cells, g_idx and l_idx

        grid_cell_t* cells = (grid_cell_t*)(mem);
        uint32_t* g_idx = (uint32_t*)(cells + M) + N * 0;
        uint32_t* l_idx = (uint32_t*)(cells + M) + N * 1;
        float* ref_x =       (float*)(cells + M) + N * 2;
        float* ref_y =       (float*)(cells + M) + N * 3;
        float* ref_z =       (float*)(cells + M) + N * 4;
        float* tmp_x =       (float*)(cells + M) + N * 5;
        float* tmp_y =       (float*)(cells + M) + N * 6;
        float* tmp_z =       (float*)(cells + M) + N * 7;

        float aabb_min[3] = {+FLT_MAX, +FLT_MAX, +FLT_MAX};
        float aabb_max[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        uint64_t i = 0;
        uint64_t dst_i = 0;
        while (find_next_bit_set(&i, in_bits, 0, mol->atom.count)) {
            aabb_min[0] = (x[i] < aabb_min[0]) ? x[i] : aabb_min[0];
            aabb_min[1] = (y[i] < aabb_min[1]) ? y[i] : aabb_min[1];
            aabb_min[2] = (z[i] < aabb_min[2]) ? z[i] : aabb_min[2];

            aabb_max[0] = (x[i] > aabb_max[0]) ? x[i] : aabb_max[0];
            aabb_max[1] = (y[i] > aabb_max[1]) ? y[i] : aabb_max[1];
            aabb_max[2] = (z[i] > aabb_max[2]) ? z[i] : aabb_max[2];

            tmp_x[dst_i] = x[i];
            tmp_y[dst_i] = y[i];
            tmp_z[dst_i] = z[i];

            ++dst_i;
            ++i;
        }

        const float inv_aabb_ext[3] = {
            1.0f / (aabb_max[0]-aabb_min[0]),
            1.0f / (aabb_max[1]-aabb_min[1]),
            1.0f / (aabb_max[2]-aabb_min[2])
        };
        for (i = 0; i < N; ++i) {
            const uint32_t cell_coord[3] = {
                (uint32_t)((tmp_x[i] - aabb_min[0]) * inv_aabb_ext[0]),
                (uint32_t)((tmp_y[i] - aabb_min[1]) * inv_aabb_ext[1]),
                (uint32_t)((tmp_z[i] - aabb_min[2]) * inv_aabb_ext[2])
            };

            const uint32_t min_cell_coord[3] = {
                MAX((uint32_t)((tmp_x[i] - max_range - aabb_min[0]) * inv_aabb_ext[0]), 0),
                MAX((uint32_t)((tmp_y[i] - max_range - aabb_min[1]) * inv_aabb_ext[1]), 0),
                MAX((uint32_t)((tmp_z[i] - max_range - aabb_min[2]) * inv_aabb_ext[2]), 0)
            };

            const uint32_t max_cell_coord[3] = {
                MIN((uint32_t)((tmp_x[i] + max_range - aabb_min[0]) * inv_aabb_ext[0]), GRID_RES - 1),
                MIN((uint32_t)((tmp_y[i] + max_range - aabb_min[1]) * inv_aabb_ext[1]), GRID_RES - 1),
                MIN((uint32_t)((tmp_z[i] + max_range - aabb_min[2]) * inv_aabb_ext[2]), GRID_RES - 1)
            };

            const uint32_t cell_idx = compute_cell_idx(cell_coord);
            l_idx[i] = cells[cell_idx].count++;
            g_idx[i] = cell_idx;

            uint32_t cc[3];
            for (cc[2] = min_cell_coord[2]; cc[2] <= max_cell_coord[2]; ++cc[2]) {
                for (cc[1] = min_cell_coord[1]; cc[1] < max_cell_coord[1]; ++cc[1]) {
                    for (cc[0] = min_cell_coord[0]; cc[0] < max_cell_coord[0]; ++cc[0]) {
                        const uint32_t idx = compute_cell_idx(cc);
                        cells[idx].mark = 1;
                    }
                }
            }
        }

        for (i = 1; i < M; ++i) {
            cells[i].offset = cells[i - 1].offset + cells[i - 1].count;
        }

        for (i = 0; i < N; ++i) {
            dst_i = g_idx[i] + l_idx[i];
            ref_x[dst_i] = tmp_x[i];
            ref_y[dst_i] = tmp_y[i]; 
            ref_z[dst_i] = tmp_z[i]; 
        }

        const uint64_t blk_count = mol->atom.count / 64;
        for (uint64_t blk_idx = 0; blk_idx < blk_count; ++blk_idx) {
            const uint64_t bit_count = (blk_idx != (blk_count - 1)) ? 64 : (mol->atom.count & 63);
            uint64_t mask = 0;
            for (uint64_t bit = 0; bit < bit_count; ++bit) {
                const uint64_t i = blk_idx * 64 + bit;
                const uint32_t min_cell_coord[3] = {
                    MAX((uint32_t)((x[i] - max_range - aabb_min[0]) * inv_aabb_ext[0]), 0),
                    MAX((uint32_t)((y[i] - max_range - aabb_min[1]) * inv_aabb_ext[1]), 0),
                    MAX((uint32_t)((z[i] - max_range - aabb_min[2]) * inv_aabb_ext[2]), 0)
                };

                const uint32_t max_cell_coord[3] = {
                    MIN((uint32_t)((x[i] + max_range - aabb_min[0]) * inv_aabb_ext[0]), GRID_RES - 1),
                    MIN((uint32_t)((y[i] + max_range - aabb_min[1]) * inv_aabb_ext[1]), GRID_RES - 1),
                    MIN((uint32_t)((z[i] + max_range - aabb_min[2]) * inv_aabb_ext[2]), GRID_RES - 1)
                };

                uint32_t cc[3];
                for (cc[2] = min_cell_coord[2]; cc[2] <= max_cell_coord[2]; ++cc[2]) {
                    for (cc[1] = min_cell_coord[1]; cc[1] < max_cell_coord[1]; ++cc[1]) {
                        for (cc[0] = min_cell_coord[0]; cc[0] < max_cell_coord[0]; ++cc[0]) {
                            const uint32_t idx = compute_cell_idx(cc);
                            if (cells[idx].mark) {
                                const uint32_t beg = cells[idx].offset;
                                const uint32_t end = cells[idx].count;
                                for (uint32_t j = beg; j < end; ++j) {
                                    const float dx = x[i] - ref_x[j];
                                    const float dy = y[i] - ref_y[j];
                                    const float dz = z[i] - ref_z[j];
                                    const float r2 = dx * dx + dy * dy + dz * dz;
                                    if (min_d2 < r2 && r2 < max_d2) {
                                        mask |= 1ULL << bit;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (mask) out_bits[blk_idx] = mask;
        }

        free(mem);
    }
    else if (N > 0) {
        const uint64_t ref_count = N;
        float ref_x[SPATIAL_GRID_MIN_SIZE] = {0};
        float ref_y[SPATIAL_GRID_MIN_SIZE] = {0};
        float ref_z[SPATIAL_GRID_MIN_SIZE] = {0};

        uint64_t dst_i = 0;
        uint64_t src_i = 0;
        while (find_next_bit_set(&src_i, in_bits, src_i, mol->atom.count - src_i)) {
            ref_x[dst_i] = x[src_i];
            ref_y[dst_i] = y[src_i];
            ref_z[dst_i] = z[src_i];
            ++dst_i;
            ++src_i;
        }

        // test all atoms against the reference set
        const uint64_t blk_count = DIV_UP(mol->atom.count);
        for (uint64_t blk_idx = 0; blk_idx < blk_count; ++blk_idx) {
            const uint64_t bit_count = (blk_idx != (blk_count - 1)) ? 64 : (mol->atom.count & 63);
            uint64_t mask = 0;
            for (uint64_t bit = 0; bit < bit_count; ++bit) {
                const uint64_t i = blk_idx * 64 + bit;
                for (uint64_t j = 0; j < ref_count; ++j) {
                    const float dx = x[i] - ref_x[j];
                    const float dy = y[i] - ref_y[j];
                    const float dz = z[i] - ref_z[j];
                    const float r2 = dx * dx + dy * dy + dz * dz;
                    if (min_d2 <= r2 && r2 <= max_d2) {
                        mask |= 1ULL << bit;
                    }
                }
            }
            if(mask) out_bits[blk_idx] = mask;
        }
    }
}

void filter_func_resid_int(int val, uint64_t* bits, const mold_molecule* mol) {
    filter_func_resid(val, val, bits, mol);
}

void filter_func_residue_int(int val, uint64_t* bits, const mold_molecule* mol) {
    filter_func_residue(val, val, bits, mol);
}

void filter_func_within_flt(float range, const uint64_t* in_bits, uint64_t* out_bits, const mold_molecule* mol) {
    filter_func_within(0, range, in_bits, out_bits, mol);
}

#ifdef __cplusplus
}
#endif
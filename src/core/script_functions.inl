#ifndef __MD_SCRIPT_FUNCTIONS_INL__
#define __MD_SCRIPT_FUNCTIONS_INL__


#define as_float(arg) (*((float*)((arg).ptr)))
#define as_float_arr(arg) ((float*)((arg).ptr))

#define as_int(arg) (*((int*)((arg).ptr)))
#define as_int_arr(arg) ((int*)((arg).ptr))

#define as_frange(arg) (*((frange_t*)((arg).ptr)))
#define as_frange_arr(arg) ((frange_t*)((arg).ptr))

#define as_irange(arg) (*((irange_t*)((arg).ptr)))
#define as_irange_arr(arg) ((irange_t*)((arg).ptr))

#define as_string(arg) (*((str_t*)((arg).ptr)))
#define as_string_arr(arg) ((str_t*)((arg).ptr))

#define as_bitrange(arg) (*((bitrange_t*)((arg).ptr)))
#define as_bitrange_arr(arg) ((bitrange_t*)((arg).ptr))

#define as_bitfield(arg) (*((md_bitfield_t*)((arg).ptr)))

#define as_vec3(arg) (*((vec3_t*)((arg).ptr)))
#define as_vec3_arr(arg) (((vec3_t*)((arg).ptr)))

// Macros to bake standard functions into something callable through our interface
#define BAKE_FUNC_F__F(prefix, func) \
    static int prefix##func(data_t* dst, data_t arg[], eval_context_t* ctx) { \
        (void)ctx; \
        as_float(*dst) = func(as_float(arg[0])); \
        return 0; \
    }

#define BAKE_FUNC_F__F_F(prefix, func) \
    static int prefix##func(data_t* dst, data_t arg[], eval_context_t* ctx) { \
        (void)ctx; \
        as_float(*dst) = func(as_float(arg[0]), as_float(arg[1])); \
        return 0; \
    }

#define BAKE_FUNC_FARR__FARR(prefix, func) \
    static int prefix##func(data_t* dst, data_t arg[], eval_context_t* ctx) { \
        (void)ctx; \
        if (dst) { \
            for (int64_t i = 0; i < element_count(*dst); ++i) { \
                as_float_arr(*dst)[i] = func(as_float_arr(arg[0])[i]); \
            } \
            return 0; \
        } \
        else { \
            return (int)element_count(arg[0]); \
        } \
    }

// float func(float)
BAKE_FUNC_F__F(_, sqrtf)
BAKE_FUNC_F__F(_, cbrtf)
BAKE_FUNC_F__F(_, fabsf)
BAKE_FUNC_F__F(_, floorf)
BAKE_FUNC_F__F(_, ceilf)
BAKE_FUNC_F__F(_, cosf)
BAKE_FUNC_F__F(_, sinf)
BAKE_FUNC_F__F(_, tanf)
BAKE_FUNC_F__F(_, acosf)
BAKE_FUNC_F__F(_, asinf)
BAKE_FUNC_F__F(_, atanf)
BAKE_FUNC_F__F(_, logf)
BAKE_FUNC_F__F(_, expf)
BAKE_FUNC_F__F(_, log2f)
BAKE_FUNC_F__F(_, exp2f)
BAKE_FUNC_F__F(_, log10f)

// float func(float, float)
BAKE_FUNC_F__F_F(_, atan2f)
BAKE_FUNC_F__F_F(_, powf)

// array versions
// @TODO: Fill in these when needed. It's a bit uncelar up front what should be supported and exposed as array operations?
BAKE_FUNC_FARR__FARR(_arr_, fabsf)
BAKE_FUNC_FARR__FARR(_arr_, floorf)
BAKE_FUNC_FARR__FARR(_arr_, ceilf)

// MACROS TO BAKE OPERATORS INTO CALLABLE FUNCTIONS
#define BAKE_OP_UNARY_S(name, op, base_type) \
    static int name(data_t* dst, data_t arg[], eval_context_t* ctx) { \
        (void)ctx; \
        *((base_type*)dst->ptr) = op *((base_type*)arg[0].ptr); \
        return 0; \
    }

#define BAKE_OP_UNARY_M(name, op, base_type) \
    static int name(data_t* dst, data_t arg[], eval_context_t* ctx) { \
        (void)ctx; \
        for (int64_t i = 0; i < element_count(*dst); ++i) { \
            ((base_type*)dst->ptr)[i] = op ((base_type*)arg[0].ptr)[i]; \
        } \
        return 0; \
    }

#define BAKE_OP_S_S(name, op, base_type) \
    static int name(data_t* dst, data_t arg[], eval_context_t* ctx) { \
        (void)ctx; \
        *((base_type*)dst->ptr) = *((base_type*)arg[0].ptr) op *((base_type*)arg[1].ptr); \
        return 0; \
    }

#define BAKE_OP_M_S(name, op, base_type) \
    static int name(data_t* dst, data_t arg[], eval_context_t* ctx) { \
        (void)ctx; \
        for (int64_t i = 0; i < element_count(*dst); ++i) { \
            ((base_type*)dst->ptr)[i] = ((base_type*)arg[0].ptr)[i] op *((base_type*)arg[1].ptr); \
        } \
        return 0; \
    }

#define BAKE_OP_M_M(name, op, base_type) \
    static int name(data_t* dst, data_t arg[], eval_context_t* ctx) { \
        (void)ctx; \
        for (int64_t i = 0; i < element_count(*dst); ++i) { \
            ((base_type*)dst->ptr)[i] = ((base_type*)arg[0].ptr)[i] op ((base_type*)arg[1].ptr)[i]; \
        } \
        return 0; \
    }

BAKE_OP_S_S(_op_and_b_b, &&,  bool)
BAKE_OP_S_S(_op_or_b_b,  ||,  bool)
BAKE_OP_UNARY_S(_op_not_b, !, bool)

BAKE_OP_S_S(_op_add_f_f, +, float)
BAKE_OP_S_S(_op_sub_f_f, -, float)
BAKE_OP_S_S(_op_mul_f_f, *, float)
BAKE_OP_S_S(_op_div_f_f, /, float)
BAKE_OP_UNARY_S(_op_neg_f, -, float)
BAKE_OP_UNARY_M(_op_neg_farr, -, float)

BAKE_OP_M_S(_op_add_farr_f, +, float)
BAKE_OP_M_S(_op_sub_farr_f, -, float)
BAKE_OP_M_S(_op_mul_farr_f, *, float)
BAKE_OP_M_S(_op_div_farr_f, /, float)

BAKE_OP_M_M(_op_add_farr_farr, +, float)
BAKE_OP_M_M(_op_sub_farr_farr, -, float)
BAKE_OP_M_M(_op_mul_farr_farr, *, float)
BAKE_OP_M_M(_op_div_farr_farr, /, float)

BAKE_OP_S_S(_op_add_i_i, +, int)
BAKE_OP_S_S(_op_sub_i_i, -, int)
BAKE_OP_S_S(_op_mul_i_i, *, int)
BAKE_OP_S_S(_op_div_i_i, /, int)
BAKE_OP_UNARY_S(_op_neg_i, -, int)
BAKE_OP_UNARY_M(_op_neg_iarr, -, int)

BAKE_OP_M_S(_op_add_iarr_i, +, int)
BAKE_OP_M_S(_op_sub_iarr_i, -, int)
BAKE_OP_M_S(_op_mul_iarr_i, *, int)
BAKE_OP_M_S(_op_div_iarr_i, /, int)

BAKE_OP_M_M(_op_add_iarr_iarr, +, int)
BAKE_OP_M_M(_op_sub_iarr_iarr, -, int)
BAKE_OP_M_M(_op_mul_iarr_iarr, *, int)
BAKE_OP_M_M(_op_div_iarr_iarr, /, int)

// Forward declarations of functions
// @TODO: Add your declarations here

// Casts (Implicit ones)

static int _cast_int_to_flt             (data_t*, data_t[], eval_context_t*);
static int _cast_int_to_irng            (data_t*, data_t[], eval_context_t*);
static int _cast_irng_to_frng           (data_t*, data_t[], eval_context_t*);
static int _cast_int_arr_to_flt_arr     (data_t*, data_t[], eval_context_t*);
static int _cast_irng_arr_to_frng_arr   (data_t*, data_t[], eval_context_t*);

static int _cast_bf_to_atom     (data_t*, data_t[], eval_context_t*);
static int _cast_bf_to_residue  (data_t*, data_t[], eval_context_t*);
static int _cast_bf_to_chain    (data_t*, data_t[], eval_context_t*);

// Logical operators for custom types
static int _not  (data_t*, data_t[], eval_context_t*); // -> bitfield
static int _and  (data_t*, data_t[], eval_context_t*); // -> bitfield
static int _or   (data_t*, data_t[], eval_context_t*); // -> bitfield

// Selectors
// Atomic level selectors
static int _all     (data_t*, data_t[], eval_context_t*); // -> bitfield
static int _x       (data_t*, data_t[], eval_context_t*); // -> bitfield
static int _y       (data_t*, data_t[], eval_context_t*); // -> bitfield
static int _z       (data_t*, data_t[], eval_context_t*); // -> bitfield
static int _within_flt  (data_t*, data_t[], eval_context_t*); // -> bitfield
static int _within_frng (data_t*, data_t[], eval_context_t*); // -> bitfield
static int _name    (data_t*, data_t[], eval_context_t*); // -> bitfield
static int _element_str (data_t*, data_t[], eval_context_t*); // -> bitfield
static int _element_irng(data_t*, data_t[], eval_context_t*); // -> bitfield
static int _atom    (data_t*, data_t[], eval_context_t*);   // -> bitfield

// Residue level selectors
static int _water   (data_t*, data_t[], eval_context_t*); // -> bitfield
static int _protein   (data_t*, data_t[], eval_context_t*); // -> bitfield
static int _ion     (data_t*, data_t[], eval_context_t*); // -> bitfield
static int _resname (data_t*, data_t[], eval_context_t*); // (str[]) -> bitfield
static int _resid   (data_t*, data_t[], eval_context_t*); // (irange[]) -> bitfield   // irange also covers integers, since integers are implicitly convertible to irange
static int _residue (data_t*, data_t[], eval_context_t*); // (irange[]) -> bitfield

// Chain level selectors
static int _chain_irng  (data_t*, data_t[], eval_context_t*); // (irange[]) -> bitfield
static int _chain_str   (data_t*, data_t[], eval_context_t*); // (str[]) -> bitfield

// Property Compute
static int _distance        (data_t*, data_t[], eval_context_t*); // (float[3], float[3])   -> float
static int _distance_min    (data_t*, data_t[], eval_context_t*); // (float[3][], float[3][]) -> float
static int _distance_max    (data_t*, data_t[], eval_context_t*); // (float[3][], float[3][]) -> float
static int _distance_pair   (data_t*, data_t[], eval_context_t*); // (float[3][], float[3][]) -> float

static int _angle   (data_t*, data_t[], eval_context_t*); // (float[3], float[3], float[3]) -> float
static int _dihedral(data_t*, data_t[], eval_context_t*); // (float[3], float[3], float[3], float[3]) -> float

static int _rmsd    (data_t*, data_t[], eval_context_t*); // (float[3][]) -> float

static int _rdf     (data_t*, data_t[], eval_context_t*); // (bitfield, bitfield, float) -> float[128] (Histogram). The idea is that we use a fixed amount of bins, then we let the user choose some kernel to smooth it.
static int _sdf     (data_t*, data_t[], eval_context_t*); // (bitfield, bitfield, float) -> float[128][128][128]. This one cannot be stored explicitly as one copy per frame, but is rather accumulated.

// Geometric operations
static int _com     (data_t*, data_t[], eval_context_t*); // (float[3][]) -> float
static int _plane   (data_t*, data_t[], eval_context_t*);  // (float[3][]) -> float[4]

static int _position_int    (data_t*, data_t[], eval_context_t*);   // (int)      -> float[3]
static int _position_irng   (data_t*, data_t[], eval_context_t*);   // (irange)   -> float[3][]
static int _position_bf     (data_t*, data_t[], eval_context_t*);   // (bitfield) -> float[3][]

static int _ref_frame_bf    (data_t*, data_t[], eval_context_t*);   // (bitfield) -> float[4][4][]

// Linear algebra
static int _dot           (data_t*, data_t[], eval_context_t*); // (float[], float[]) -> float
static int _cross         (data_t*, data_t[], eval_context_t*); // (float[3], float[3]) -> float[3]
static int _mat4_mul_mat4 (data_t*, data_t[], eval_context_t*); // (float[4][4], float[4][4]) -> float[4][4]
static int _mat4_mul_vec4 (data_t*, data_t[], eval_context_t*); // (float[4][4], float[4]) -> float[4]

static int _vec2 (data_t*, data_t[], eval_context_t*); // (float, float) -> float[2]
static int _vec3 (data_t*, data_t[], eval_context_t*); // (float, float, float) -> float[4]
static int _vec4 (data_t*, data_t[], eval_context_t*); // (float, float, float, float) -> float[4]


// This is to mark that the procedure supports a varying length
#define ANY_LENGTH -1
#define ANY_LEVEL -1

#define DIST_BINS 128
#define VOL_DIM 128

// Type info declaration helpers
#define TI_BOOL         {TYPE_BOOL, {1}, 0}

#define TI_FLOAT        {TYPE_FLOAT, {1}, 0}
#define TI_FLOAT_ARR    {TYPE_FLOAT, {ANY_LENGTH}, 0}
#define TI_FLOAT2       {TYPE_FLOAT, {2,1}, 1}
#define TI_FLOAT2_ARR   {TYPE_FLOAT, {2,ANY_LENGTH}, 1}
#define TI_FLOAT3       {TYPE_FLOAT, {3,1}, 1}
#define TI_FLOAT3_ARR   {TYPE_FLOAT, {3,ANY_LENGTH}, 1}
#define TI_FLOAT4       {TYPE_FLOAT, {4,1}, 1}
#define TI_FLOAT4_ARR   {TYPE_FLOAT, {4,ANY_LENGTH}, 1}
#define TI_FLOAT44      {TYPE_FLOAT, {4,4,1}, 2}
#define TI_FLOAT44_ARR  {TYPE_FLOAT, {4,4,ANY_LENGTH}, 2}

#define TI_DISTRIBUTION {TYPE_FLOAT, {DIST_BINS,1}, 1}
#define TI_VOLUME       {TYPE_FLOAT, {VOL_DIM,VOL_DIM,VOL_DIM,1}, 3}

#define TI_INT          {TYPE_INT, {1}, 0}
#define TI_INT_ARR      {TYPE_INT, {ANY_LENGTH}, 0}

#define TI_FRANGE       {TYPE_FRANGE, {1}, 0}
#define TI_FRANGE_ARR   {TYPE_FRANGE, {ANY_LENGTH}, 0}

#define TI_IRANGE       {TYPE_IRANGE, {1}, 0}
#define TI_IRANGE_ARR   {TYPE_IRANGE, {ANY_LENGTH}, 0}

#define TI_STRING       {TYPE_STRING, {1}, 0}
#define TI_STRING_ARR   {TYPE_STRING, {ANY_LENGTH}, 0}

#define TI_BITRANGE     {TYPE_BITRANGE, {1}, 0}
#define TI_BITRANGE_ARR {TYPE_BITRANGE, {ANY_LENGTH}, 0}

#define TI_BITFIELD         {TYPE_BITFIELD, {1}, 0, ANY_LEVEL}
#define TI_BITFIELD_ATOM    {TYPE_BITFIELD, {1}, 0, LEVEL_ATOM}
#define TI_BITFIELD_RESIDUE {TYPE_BITFIELD, {1}, 0, LEVEL_RESIDUE}
#define TI_BITFIELD_CHAIN   {TYPE_BITFIELD, {1}, 0, LEVEL_CHAIN}

// Predefined constants
static const float _PI  = 3.14159265358f;
static const float _TAU = 6.28318530718f;
static const float _E   = 2.71828182845f;

#define cstr(string) {.ptr = (string ""), .len = (sizeof(string)-1)}

// @TODO: Add your values here
static identifier_t constants[] = {
    {cstr("PI"),     {TI_FLOAT, (void*)(&_PI),    sizeof(float)}},
    {cstr("TAU"),    {TI_FLOAT, (void*)(&_TAU),   sizeof(float)}},
    {cstr("E"),      {TI_FLOAT, (void*)(&_E),     sizeof(float)}},
};

// IMPLICIT CASTS/CONVERSIONS
static procedure_t casts[] = {
    {cstr("cast"),    TI_FLOAT,              1,  {TI_INT},           _cast_int_to_flt},
    {cstr("cast"),    TI_IRANGE,             1,  {TI_INT},           _cast_int_to_irng},
    {cstr("cast"),    TI_FRANGE,             1,  {TI_IRANGE},        _cast_irng_to_frng},
    {cstr("cast"),    TI_FLOAT_ARR,          1,  {TI_INT_ARR},       _cast_int_arr_to_flt_arr,   FLAG_RET_AND_ARG_EQUAL_LENGTH},
    {cstr("cast"),    TI_FRANGE_ARR,         1,  {TI_IRANGE_ARR},    _cast_irng_arr_to_frng_arr, FLAG_RET_AND_ARG_EQUAL_LENGTH},

    {cstr("cast"),    TI_BITFIELD_ATOM,      1,  {TI_BITFIELD},          _cast_bf_to_atom},
    {cstr("cast"),    TI_BITFIELD_RESIDUE,   1,  {TI_BITFIELD_CHAIN},    _cast_bf_to_residue},

    // This does the heavy lifting for implicitly converting every compatible argument into a position (vec3) if the procedure is marked with FLAG_POSITION
    {cstr("extract pos"),   TI_FLOAT3_ARR,  1,  {TI_INT_ARR},       _position_int,  FLAG_DYNAMIC | FLAG_POSITION | FLAG_RET_AND_ARG_EQUAL_LENGTH},
    {cstr("extract pos"),   TI_FLOAT3_ARR,  1,  {TI_IRANGE_ARR},    _position_irng, FLAG_DYNAMIC | FLAG_POSITION | FLAG_QUERYABLE_LENGTH},
    {cstr("extract pos"),   TI_FLOAT3_ARR,  1,  {TI_BITFIELD},      _position_bf,   FLAG_DYNAMIC | FLAG_POSITION | FLAG_QUERYABLE_LENGTH},
};

static procedure_t operators[] = {
    {cstr("not"),    TI_BOOL,            1,  {TI_BOOL},              _op_not_b},
    {cstr("or"),     TI_BOOL,            2,  {TI_BOOL,   TI_BOOL},   _op_or_b_b},
    {cstr("and"),    TI_BOOL,            2,  {TI_BOOL,   TI_BOOL},   _op_and_b_b},

    // BITFIELD NOT
    {cstr("not"),    TI_BITFIELD_ATOM,   1,  {TI_BITFIELD_ATOM},     _not},
    {cstr("not"),    TI_BITFIELD_RESIDUE,1,  {TI_BITFIELD_RESIDUE},  _not},
    {cstr("not"),    TI_BITFIELD_CHAIN,  1,  {TI_BITFIELD_CHAIN},    _not},

    // BITFIELD AND -> MAINTAIN THE HIGHEST LEVEL OF CONTEXT AMONG OPERANDS
    {cstr("and"),    TI_BITFIELD_ATOM,   2,  {TI_BITFIELD_ATOM,      TI_BITFIELD_ATOM},      _and},
    {cstr("and"),    TI_BITFIELD_RESIDUE,2,  {TI_BITFIELD_RESIDUE,   TI_BITFIELD_RESIDUE},   _and},
    {cstr("and"),    TI_BITFIELD_CHAIN,  2,  {TI_BITFIELD_CHAIN,     TI_BITFIELD_CHAIN},     _and},

    {cstr("and"),    TI_BITFIELD_RESIDUE,2,  {TI_BITFIELD_ATOM,      TI_BITFIELD_RESIDUE},   _and,   FLAG_SYMMETRIC_ARGS},
    {cstr("and"),    TI_BITFIELD_CHAIN,  2,  {TI_BITFIELD_ATOM,      TI_BITFIELD_CHAIN},     _and,   FLAG_SYMMETRIC_ARGS},
    {cstr("and"),    TI_BITFIELD_CHAIN,  2,  {TI_BITFIELD_RESIDUE,   TI_BITFIELD_CHAIN},     _and,   FLAG_SYMMETRIC_ARGS},

    {cstr("or"),     TI_BITFIELD_ATOM,   2,  {TI_BITFIELD_ATOM,      TI_BITFIELD_ATOM},      _or},
    {cstr("or"),     TI_BITFIELD_RESIDUE,2,  {TI_BITFIELD_RESIDUE,   TI_BITFIELD_RESIDUE},   _or},
    {cstr("or"),     TI_BITFIELD_CHAIN,  2,  {TI_BITFIELD_CHAIN,     TI_BITFIELD_CHAIN},     _or},

    {cstr("or"),     TI_BITFIELD_ATOM,   2,  {TI_BITFIELD_ATOM,      TI_BITFIELD_RESIDUE},   _or,    FLAG_SYMMETRIC_ARGS},
    {cstr("or"),     TI_BITFIELD_ATOM,   2,  {TI_BITFIELD_ATOM,      TI_BITFIELD_CHAIN},     _or,    FLAG_SYMMETRIC_ARGS},
    {cstr("or"),     TI_BITFIELD_RESIDUE,2,  {TI_BITFIELD_RESIDUE,   TI_BITFIELD_CHAIN},     _or,    FLAG_SYMMETRIC_ARGS},

    // Binary add
    {cstr("+"),      TI_FLOAT,       2,  {TI_FLOAT,      TI_FLOAT},      _op_add_f_f},
    {cstr("+"),      TI_FLOAT_ARR,   2,  {TI_FLOAT_ARR,  TI_FLOAT},      _op_add_farr_f,     FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_SYMMETRIC_ARGS},
    {cstr("+"),      TI_FLOAT_ARR,   2,  {TI_FLOAT_ARR,  TI_FLOAT_ARR},  _op_add_farr_farr,  FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_ARGS_EQUAL_LENGTH},

    {cstr("+"),      TI_INT,         2,  {TI_INT,        TI_INT},        _op_add_i_i},
    {cstr("+"),      TI_INT_ARR,     2,  {TI_INT_ARR,    TI_INT},        _op_add_iarr_i,     FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_SYMMETRIC_ARGS},
    {cstr("+"),      TI_INT_ARR,     2,  {TI_INT_ARR,    TI_INT_ARR},    _op_add_iarr_iarr,  FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_ARGS_EQUAL_LENGTH},

    // Unary negation
    {cstr("-"),      TI_FLOAT,       1,  {TI_FLOAT},                     _op_neg_f},
    {cstr("-"),      TI_FLOAT_ARR,   1,  {TI_FLOAT_ARR},                 _op_neg_farr,       FLAG_RET_AND_ARG_EQUAL_LENGTH},

    {cstr("-"),      TI_INT,         1,  {TI_INT},                       _op_neg_i},
    {cstr("-"),      TI_INT_ARR,     1,  {TI_INT_ARR},                   _op_neg_iarr,       FLAG_RET_AND_ARG_EQUAL_LENGTH},

    // Binary sub
    {cstr("-"),      TI_FLOAT,       2,  {TI_FLOAT,      TI_FLOAT},      _op_sub_f_f},
    {cstr("-"),      TI_FLOAT_ARR,   2,  {TI_FLOAT_ARR,  TI_FLOAT},      _op_sub_farr_f,     FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_SYMMETRIC_ARGS},
    {cstr("-"),      TI_FLOAT_ARR,   2,  {TI_FLOAT_ARR,  TI_FLOAT_ARR},  _op_sub_farr_farr,  FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_ARGS_EQUAL_LENGTH},

    {cstr("-"),      TI_INT,         2,  {TI_INT,        TI_INT},        _op_sub_i_i},
    {cstr("-"),      TI_INT_ARR,     2,  {TI_INT_ARR,    TI_INT},        _op_sub_iarr_i,     FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_SYMMETRIC_ARGS},
    {cstr("-"),      TI_INT_ARR,     2,  {TI_INT_ARR,    TI_INT_ARR},    _op_sub_iarr_iarr,  FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_ARGS_EQUAL_LENGTH},

    // Binary mul
    {cstr("*"),      TI_FLOAT,       2,  {TI_FLOAT,      TI_FLOAT},      _op_mul_f_f},
    {cstr("*"),      TI_FLOAT_ARR,   2,  {TI_FLOAT_ARR,  TI_FLOAT},      _op_mul_farr_f,     FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_SYMMETRIC_ARGS},
    {cstr("*"),      TI_FLOAT_ARR,   2,  {TI_FLOAT_ARR,  TI_FLOAT_ARR},  _op_mul_farr_farr,  FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_ARGS_EQUAL_LENGTH},

    {cstr("*"),      TI_INT,         2,  {TI_INT,        TI_INT},        _op_mul_i_i},
    {cstr("*"),      TI_INT_ARR,     2,  {TI_INT_ARR,    TI_INT},        _op_mul_iarr_i,     FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_SYMMETRIC_ARGS},
    {cstr("*"),      TI_INT_ARR,     2,  {TI_INT_ARR,    TI_INT_ARR},    _op_mul_iarr_iarr,  FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_ARGS_EQUAL_LENGTH},

    // Binary div
    {cstr("/"),      TI_FLOAT,       2,  {TI_FLOAT,      TI_FLOAT},      _op_div_f_f},
    {cstr("/"),      TI_FLOAT_ARR,   2,  {TI_FLOAT_ARR,  TI_FLOAT},      _op_div_farr_f,     FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_SYMMETRIC_ARGS},
    {cstr("/"),      TI_FLOAT_ARR,   2,  {TI_FLOAT_ARR,  TI_FLOAT_ARR},  _op_div_farr_farr,  FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_ARGS_EQUAL_LENGTH},

    {cstr("/"),      TI_INT,         2,  {TI_INT,        TI_INT},        _op_div_i_i},
    {cstr("/"),      TI_INT_ARR,     2,  {TI_INT_ARR,    TI_INT},        _op_div_iarr_i,     FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_SYMMETRIC_ARGS},
    {cstr("/"),      TI_INT_ARR,     2,  {TI_INT_ARR,    TI_INT_ARR},    _op_div_iarr_iarr,  FLAG_RET_AND_ARG_EQUAL_LENGTH | FLAG_ARGS_EQUAL_LENGTH},
};

static procedure_t procedures[] = {
    // NATIVE FUNCS
    {cstr("sqrt"),   TI_FLOAT, 1, {TI_FLOAT}, _sqrtf},
    {cstr("cbrt"),   TI_FLOAT, 1, {TI_FLOAT}, _cbrtf},
    {cstr("abs"),    TI_FLOAT, 1, {TI_FLOAT}, _fabsf},
    {cstr("floor"),  TI_FLOAT, 1, {TI_FLOAT}, _floorf},
    {cstr("ceil"),   TI_FLOAT, 1, {TI_FLOAT}, _ceilf},
    {cstr("cos"),    TI_FLOAT, 1, {TI_FLOAT}, _cosf},
    {cstr("sin"),    TI_FLOAT, 1, {TI_FLOAT}, _sinf},
    {cstr("asin"),   TI_FLOAT, 1, {TI_FLOAT}, _asinf},
    {cstr("acos"),   TI_FLOAT, 1, {TI_FLOAT}, _acosf},
    {cstr("atan"),   TI_FLOAT, 1, {TI_FLOAT}, _atanf},
    {cstr("log"),    TI_FLOAT, 1, {TI_FLOAT}, _logf},
    {cstr("exp"),    TI_FLOAT, 1, {TI_FLOAT}, _expf},
    {cstr("log2"),   TI_FLOAT, 1, {TI_FLOAT}, _log2f},
    {cstr("exp2"),   TI_FLOAT, 1, {TI_FLOAT}, _exp2f},
    {cstr("log10"),  TI_FLOAT, 1, {TI_FLOAT}, _log10f},

    {cstr("atan"),   TI_FLOAT, 2, {TI_FLOAT, TI_FLOAT}, _atan2f},
    {cstr("atan2"),  TI_FLOAT, 2, {TI_FLOAT, TI_FLOAT}, _atan2f},
    {cstr("pow"),    TI_FLOAT, 2, {TI_FLOAT, TI_FLOAT}, _powf},

    // ARRAY VERSIONS OF NATIVE FUNCS
    {cstr("abs"),    TI_FLOAT_ARR, 1, {TI_FLOAT_ARR}, _arr_fabsf,    FLAG_RET_AND_ARG_EQUAL_LENGTH},
    {cstr("floor"),  TI_FLOAT_ARR, 1, {TI_FLOAT_ARR}, _arr_floorf,   FLAG_RET_AND_ARG_EQUAL_LENGTH},
    {cstr("ceil"),   TI_FLOAT_ARR, 1, {TI_FLOAT_ARR}, _arr_ceilf,    FLAG_RET_AND_ARG_EQUAL_LENGTH},

    // LINEAR ALGEBRA
    {cstr("dot"),    TI_FLOAT,   2, {TI_FLOAT_ARR,   TI_FLOAT_ARR},  _dot},
    {cstr("cross"),  TI_FLOAT3,  2, {TI_FLOAT3,      TI_FLOAT3},     _cross},
    {cstr("mul"),    TI_FLOAT44, 2, {TI_FLOAT44,     TI_FLOAT44},    _mat4_mul_mat4},
    {cstr("mul"),    TI_FLOAT4,  2, {TI_FLOAT44,     TI_FLOAT4},     _mat4_mul_vec4},

    // CONSTRUCTORS
    {cstr("vec2"),   TI_FLOAT2,  2, {TI_FLOAT, TI_FLOAT},                       _vec2},
    {cstr("vec3"),   TI_FLOAT3,  3, {TI_FLOAT, TI_FLOAT, TI_FLOAT},             _vec3},
    {cstr("vec4"),   TI_FLOAT4,  4, {TI_FLOAT, TI_FLOAT, TI_FLOAT, TI_FLOAT},   _vec4},

    // BITFIELD CONVERSION (Explicit casts of bitfield types)
    {cstr("atoms"),      TI_BITFIELD_ATOM, 1, {TI_BITFIELD}, _cast_bf_to_atom},
    {cstr("residues"),   TI_BITFIELD_ATOM, 1, {TI_BITFIELD}, _cast_bf_to_residue},
    {cstr("chains"),     TI_BITFIELD_ATOM, 1, {TI_BITFIELD}, _cast_bf_to_chain},

    // --- SELECTION ---

    // Atom level
    {cstr("all"),       TI_BITFIELD_ATOM, 0, {0},               _all},
    {cstr("type"),      TI_BITFIELD_ATOM, 1, {TI_STRING_ARR},   _name,              FLAG_QUERYABLE_LENGTH | FLAG_VALIDATABLE_ARGS},
    {cstr("name"),      TI_BITFIELD_ATOM, 1, {TI_STRING_ARR},   _name,              FLAG_QUERYABLE_LENGTH | FLAG_VALIDATABLE_ARGS},
    {cstr("label"),     TI_BITFIELD_ATOM, 1, {TI_STRING_ARR},   _name,              FLAG_QUERYABLE_LENGTH | FLAG_VALIDATABLE_ARGS},
    {cstr("element"),   TI_BITFIELD_ATOM, 1, {TI_STRING_ARR},   _element_str,       FLAG_QUERYABLE_LENGTH | FLAG_VALIDATABLE_ARGS},
    {cstr("element"),   TI_BITFIELD_ATOM, 1, {TI_IRANGE_ARR},   _element_irng,      FLAG_QUERYABLE_LENGTH | FLAG_VALIDATABLE_ARGS},
    {cstr("atom"),      TI_BITFIELD_ATOM, 1, {TI_IRANGE_ARR},   _atom,              FLAG_VALIDATABLE_ARGS},

    // Residue level
    {cstr("protein"),   TI_BITFIELD_RESIDUE, 0, {0},                _protein},
    {cstr("water"),     TI_BITFIELD_RESIDUE, 0, {0},                _water},
    {cstr("ion"),       TI_BITFIELD_RESIDUE, 0, {0},                _ion},
    {cstr("resname"),   TI_BITFIELD_RESIDUE, 1, {TI_STRING_ARR},    _resname,       FLAG_VALIDATABLE_ARGS},
    {cstr("residue"),   TI_BITFIELD_RESIDUE, 1, {TI_STRING_ARR},    _resname,       FLAG_VALIDATABLE_ARGS},
    {cstr("resid"),     TI_BITFIELD_RESIDUE, 1, {TI_IRANGE_ARR},    _resid,         FLAG_VALIDATABLE_ARGS},
    {cstr("residue"),   TI_BITFIELD_RESIDUE, 1, {TI_IRANGE_ARR},    _residue,       FLAG_VALIDATABLE_ARGS},

    // Chain level
    {cstr("chain"),     TI_BITFIELD_CHAIN,  1,  {TI_STRING_ARR},    _chain_str,     FLAG_VALIDATABLE_ARGS},
    {cstr("chain"),     TI_BITFIELD_CHAIN,  1,  {TI_IRANGE_ARR},    _chain_irng,    FLAG_VALIDATABLE_ARGS},

    // Dynamic selectors (depend on atomic position, therefore marked as dynamic which means the values cannot be determined at compile-time)
    // Also have variable result (well its a single bitfield, but the number of atoms within is not fixed)
    {cstr("x"),         TI_BITFIELD_ATOM, 1, {TI_FRANGE},                   _x,             FLAG_DYNAMIC | FLAG_RET_VARYING_LENGTH},
    {cstr("y"),         TI_BITFIELD_ATOM, 1, {TI_FRANGE},                   _y,             FLAG_DYNAMIC | FLAG_RET_VARYING_LENGTH},
    {cstr("z"),         TI_BITFIELD_ATOM, 1, {TI_FRANGE},                   _z,             FLAG_DYNAMIC | FLAG_RET_VARYING_LENGTH},
    {cstr("within"),    TI_BITFIELD_ATOM, 2, {TI_FLOAT,  TI_FLOAT3_ARR},    _within_flt,    FLAG_DYNAMIC | FLAG_RET_VARYING_LENGTH | FLAG_POSITION},
    {cstr("within"),    TI_BITFIELD_ATOM, 2, {TI_FRANGE, TI_FLOAT3_ARR},    _within_frng,   FLAG_DYNAMIC | FLAG_RET_VARYING_LENGTH | FLAG_POSITION},

    // --- PROPERTY COMPUTE ---
    {cstr("distance"),      TI_FLOAT,       2,  {TI_FLOAT3_ARR, TI_FLOAT3_ARR}, _distance,      FLAG_DYNAMIC | FLAG_POSITION},
    {cstr("distance_min"),  TI_FLOAT,       2,  {TI_FLOAT3_ARR, TI_FLOAT3_ARR}, _distance_min,  FLAG_DYNAMIC | FLAG_POSITION},
    {cstr("distance_max"),  TI_FLOAT,       2,  {TI_FLOAT3_ARR, TI_FLOAT3_ARR}, _distance_max,  FLAG_DYNAMIC | FLAG_POSITION},
    {cstr("distance_pair"), TI_FLOAT_ARR,   2,  {TI_FLOAT3_ARR, TI_FLOAT3_ARR}, _distance_pair, FLAG_QUERYABLE_LENGTH | FLAG_DYNAMIC | FLAG_POSITION},

    {cstr("angle"),     TI_FLOAT,   3,  {TI_FLOAT3, TI_FLOAT3, TI_FLOAT3},              _angle,     FLAG_DYNAMIC | FLAG_POSITION},
    {cstr("dihedral"),  TI_FLOAT,   4,  {TI_FLOAT3, TI_FLOAT3, TI_FLOAT3, TI_FLOAT3},   _dihedral,  FLAG_DYNAMIC | FLAG_POSITION},

    {cstr("rmsd"),      TI_FLOAT,   1,  {TI_FLOAT3_ARR},    _rmsd,     FLAG_DYNAMIC | FLAG_POSITION},

    {cstr("rdf"),       TI_DISTRIBUTION,    3,  {TI_FLOAT3_ARR, TI_FLOAT3_ARR, TI_FLOAT},   _rdf,  FLAG_DYNAMIC | FLAG_POSITION},
    {cstr("sdf"),       TI_VOLUME,          3,  {TI_BITFIELD_RESIDUE, TI_FLOAT3_ARR, TI_FLOAT},  _sdf,  FLAG_DYNAMIC | FLAG_POSITION},

    // --- GEOMETRICAL OPERATIONS ---
    {cstr("com"),       TI_FLOAT3,  1,  {TI_FLOAT3_ARR},    _com,           FLAG_DYNAMIC | FLAG_POSITION},
    {cstr("plane"),     TI_FLOAT4,  1,  {TI_FLOAT3_ARR},    _plane,         FLAG_DYNAMIC | FLAG_POSITION},
    //{{"referenceframe", 14}, TI_FLOAT44, 1, {TI_FLOAT3_ARR}, _ref_frame_bf, FLAG_DYNAMIC | FLAG_POSITION},
};

#undef cstr

static inline bool in_range(irange_t range, int idx) {
    // I think a range should be inclusive in this context... Since we should be 1 based and not 0 based on indices
    return range.beg <= idx && idx <= range.end;
}

static inline bool range_in_range(irange_t small_range, irange_t big_range) {
    return big_range.beg <= small_range.beg && small_range.end <= big_range.end;
}

static inline float dihedral_angle(vec3_t p0, vec3_t p1, vec3_t p2, vec3_t p3) {
    const vec3_t b1 = vec3_normalize(vec3_sub(p1, p0));
    const vec3_t b2 = vec3_normalize(vec3_sub(p2, p1));
    const vec3_t b3 = vec3_normalize(vec3_sub(p3, p2));
    const vec3_t c1 = vec3_cross(b1, b2);
    const vec3_t c2 = vec3_cross(b2, b3);
    return atan2f(vec3_dot(vec3_cross(c1, c2), b2), vec3_dot(c1, c2));
}

// IMPLEMENTATIONS
// @TODO: Add more here

static int _not  (data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && is_type_directly_compatible(dst->type, (md_type_info_t)TI_BITFIELD));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_BITFIELD));
    (void)ctx;

    md_bitfield_t* bf_dst = dst->ptr;
    md_bitfield_t* bf_src = arg[0].ptr;

    ASSERT(bf_dst->num_bits == bf_src->num_bits);

    bit_not(bf_dst->bits, bf_src->bits, 0, bf_dst->num_bits);
    return 0;
}

static int _and  (data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && is_type_directly_compatible(dst->type, (md_type_info_t)TI_BITFIELD));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_BITFIELD));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_BITFIELD));
    (void)ctx;

    md_bitfield_t* bf_dst = dst->ptr;
    md_bitfield_t* bf_src[2] = {arg[0].ptr, arg[1].ptr};

    ASSERT(bf_dst->num_bits == bf_src[0]->num_bits);
    ASSERT(bf_dst->num_bits == bf_src[1]->num_bits);

    bit_and(bf_dst->bits, bf_src[0]->bits, bf_src[1]->bits, 0, bf_dst->num_bits);
    return 0;
}

static int _or   (data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && is_type_directly_compatible(dst->type, (md_type_info_t)TI_BITFIELD));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_BITFIELD));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_BITFIELD));
    (void)ctx;

    md_bitfield_t* bf_dst = dst->ptr;
    md_bitfield_t* bf_src[2] = {arg[0].ptr, arg[1].ptr};

    ASSERT(bf_dst->num_bits == bf_src[0]->num_bits);
    ASSERT(bf_dst->num_bits == bf_src[1]->num_bits);

    bit_or(bf_dst->bits, bf_src[0]->bits, bf_src[1]->bits, 0, bf_dst->num_bits);
    return 0;
}

static int _dot(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT_ARR));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_FLOAT_ARR));
    (void)ctx;
    float* a = (float*)arg[0].ptr;
    float* b = (float*)arg[1].ptr;
    double res = 0; // Accumulate in double, then cast to float
    for (int64_t i = 0; i < element_count(arg[0]); ++i) {
        res += (double)a[i] * (double)b[i]; 
    }
    as_float(*dst) = (float)res;
    return 0;
}

static int _cross(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_FLOAT3));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT3));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_FLOAT3));
    (void)ctx;

    float* a = (float*)arg[0].ptr;
    float* b = (float*)arg[1].ptr;
    float* c = (float*)dst->ptr;
    c[0] = a[1]*b[2] - b[1]*a[2];
    c[1] = a[2]*b[0] - b[2]*a[0];
    c[2] = a[0]*b[1] - b[0]*a[1];

    return 0;
}

static int _mat4_mul_mat4(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_FLOAT44));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT44));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_FLOAT44));
    (void)ctx;

    // The type system should already have covered this, we are only reading data we know exists.
    mat4_t* A = (mat4_t*) arg[0].ptr;
    mat4_t* B = (mat4_t*) arg[1].ptr;
    mat4_t* C = (mat4_t*) dst->ptr;

    *C = mat4_mul(*A, *B);
    return 0;
}

static int _mat4_mul_vec4(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && is_type_equivalent(dst->type,   (md_type_info_t)TI_FLOAT4));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT44));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_FLOAT4));
    (void)ctx;

    // The type system should already have covered this, we are only reading data we know exists.
    mat4_t* M = (mat4_t*) arg[0].ptr;
    vec4_t* v = (vec4_t*) arg[1].ptr;
    vec4_t* r = (vec4_t*) dst->ptr;

    *r = mat4_mul_vec4(*M, *v);
    return 0;
}

static int _vec2(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_FLOAT2));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_FLOAT));

    (void)ctx;
    float (*res) = (float*)dst->ptr;
    res[0] = as_float(arg[0]);
    res[1] = as_float(arg[1]);
    return 0;
}

static int _vec3(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_FLOAT3));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_directly_compatible(arg[2].type, (md_type_info_t)TI_FLOAT));

    (void)ctx;
    float (*res) = (float*)dst->ptr;
    res[0] = as_float(arg[0]);
    res[1] = as_float(arg[1]);
    res[2] = as_float(arg[2]);
    return 0;
}

static int _vec4(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_FLOAT4));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_directly_compatible(arg[2].type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_directly_compatible(arg[3].type, (md_type_info_t)TI_FLOAT));

    (void)ctx;
    float (*res) = (float*)dst->ptr;
    res[0] = as_float(arg[0]);
    res[1] = as_float(arg[1]);
    res[2] = as_float(arg[2]);
    res[3] = as_float(arg[3]);
    return 0;
}

static int _all(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_ATOM));
    (void)arg;
    (void)ctx;
    md_bitfield_t result = *((md_bitfield_t*)dst->ptr);
    //bit_clear(result.bits, 0, result.num_bits);
    bit_set(result.bits, (uint64_t)ctx->mol_ctx.atom.beg, (uint64_t)ctx->mol_ctx.atom.end - (uint64_t)ctx->mol_ctx.atom.beg);
    return 0;
}

static int _name(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_STRING_ARR));
    ASSERT(ctx && ctx->mol && ctx->mol->atom.name);

    const str_t* str = as_string_arr(arg[0]);
    const int64_t num_str = element_count(arg[0]);

    if (dst) {
        ASSERT(is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_ATOM));
        md_bitfield_t result = *((md_bitfield_t*)dst->ptr);

        for (int64_t i = ctx->mol_ctx.atom.beg; i < ctx->mol_ctx.atom.end; ++i) {
            const char* atom_str = ctx->mol->atom.name[i];
            for (int64_t j = 0; j < num_str; ++j) {
                if (compare_str_cstr(str[j], atom_str)) {
                    bit_set_idx(result.bits, i);
                }
            }
        }
    } else {
        int count = 0;
        for (int64_t j = 0; j < num_str; ++j) {
            bool match = false;
            for (int64_t i = ctx->mol_ctx.atom.beg; i < ctx->mol_ctx.atom.end; ++i) {
                if (compare_str_cstr(str[j], ctx->mol->atom.name[i])) {
                    ++count;
                    match = true;
                    break;
                }
            }
            if (!match) {
                create_error(ctx->ir, ctx->arg_tokens[0], "The string '%.*s' did not match any atom label within the structure", str[j].len, str[j].ptr);
                return -1;
            }
        }
        return count;
    }

    return 0;
}

static int _element_str(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_STRING_ARR));
    ASSERT(ctx && ctx->mol && ctx->mol->atom.element);

    uint8_t* elem_idx = 0;

    const uint64_t num_str = (uint64_t)element_count(arg[0]);
    const str_t* str = as_string_arr(arg[0]);

    for (uint64_t i = 0; i < num_str; ++i) {
        md_element elem = md_util_lookup_element(str[i]);
        if (elem)
            md_array_push(elem_idx, elem, ctx->temp_alloc);
        else {
            create_error(ctx->ir, ctx->arg_tokens[0], "Failed to map '%.*s' into any Element.", str[i].len, str[i].ptr);
            return -1;
        }
    }

    if (dst) {
        ASSERT(is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_ATOM));
        md_bitfield_t result = as_bitfield(*dst);

        const uint64_t num_elem = md_array_size(elem_idx);
        for (uint64_t i = ctx->mol_ctx.atom.beg; i < ctx->mol_ctx.atom.end; ++i) {
            for (uint64_t j = 0; j < num_elem; ++j) {
                if (elem_idx[j] == ctx->mol->atom.element[i]) {
                    bit_set_idx(result.bits, i);
                    break;
                }
            }
        }
    
    } else {
        int count = 0;
        const uint64_t num_elem = md_array_size(elem_idx);
        for (uint64_t j = 0; j < num_elem; ++j) {
            bool found = false;
            for (uint64_t i = ctx->mol_ctx.atom.beg; i < ctx->mol_ctx.atom.end; ++i) {
                if (elem_idx[j] == ctx->mol->atom.element[i]) {
                    ++count;
                    found = true;
                    break;
                }
            }
            if (!found) {
                create_error(ctx->ir, ctx->arg_tokens[0], "Element '%.*s' was not found within structure.", str[j].len, str[j].ptr);
                return -1;
            }
        }
        return count;
    }

    return 0;
}

static int _element_irng(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_IRANGE_ARR));
    ASSERT(ctx && ctx->mol && ctx->mol->atom.element);

    md_bitfield_t result = as_bitfield(*dst);
    irange_t* ranges = as_irange_arr(arg[0]);
    const uint64_t num_ranges = element_count(arg[0]);

    if (dst) {
    ASSERT(dst->ptr && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_ATOM));
        for (uint64_t i = ctx->mol_ctx.atom.beg; i < ctx->mol_ctx.atom.end; ++i) {
            for (uint64_t j = 0; j < num_ranges; ++j) {
                if (in_range(ranges[j], ctx->mol->atom.element[i])) {
                    bit_set_idx(result.bits, i);
                    break;
                }
            }
        }
    }
    else {
        for (uint64_t j = 0; j < num_ranges; ++j) {
            bool match = false;
            for (uint64_t i = ctx->mol_ctx.atom.beg; i < ctx->mol_ctx.atom.end; ++i) {
                if (in_range(ranges[j], ctx->mol->atom.element[i])) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                create_error(ctx->ir, ctx->arg_tokens[0], "No element within range (%i:%i) was found within structure.", ranges[j].beg, ranges[j].end);
                return -1;
            }
        }
    }

    return 0;
}

static int _x(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->atom.x);
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FRANGE));
    const frange_t range = as_frange(arg[0]);

    if (dst) {
        ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_ATOM));
        md_bitfield_t result = as_bitfield(*dst);
        for (uint64_t i = ctx->mol_ctx.atom.beg; i < ctx->mol_ctx.atom.end; ++i) {
            if (range.beg <= ctx->mol->atom.x[i] && ctx->mol->atom.x[i] <= range.end) {
                bit_set_idx(result.bits, i);
            }
        }
    }
    else {
        int count = 0;
        for (uint64_t i = ctx->mol_ctx.atom.beg; i < ctx->mol_ctx.atom.end; ++i) {
            if (range.beg <= ctx->mol->atom.x[i] && ctx->mol->atom.x[i] <= range.end) {
                count += 1;
            }
        }
        return count;
    }

    return 0;
}

static int _y(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->atom.y);
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FRANGE));
    const frange_t range = as_frange(arg[0]);

    if (dst) {
        ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_ATOM));
        md_bitfield_t result = as_bitfield(*dst);
        for (uint64_t i = ctx->mol_ctx.atom.beg; i < ctx->mol_ctx.atom.end; ++i) {
            if (range.beg <= ctx->mol->atom.y[i] && ctx->mol->atom.y[i] <= range.end) {
                bit_set_idx(result.bits, i);
            }
        }
    } else {
        int count = 0;
        for (uint64_t i = ctx->mol_ctx.atom.beg; i < ctx->mol_ctx.atom.end; ++i) {
            if (range.beg <= ctx->mol->atom.y[i] && ctx->mol->atom.y[i] <= range.end) {
                count += 1;
            }
        }
        return count;
    }

    return 0;
}

static int _z(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->atom.z);
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FRANGE));
    const frange_t range = as_frange(arg[0]);

    if (dst) {
        ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_ATOM));
        md_bitfield_t result = as_bitfield(*dst);
        for (uint64_t i = ctx->mol_ctx.atom.beg; i < ctx->mol_ctx.atom.end; ++i) {
            if (range.beg <= ctx->mol->atom.z[i] && ctx->mol->atom.z[i] <= range.end) {
                bit_set_idx(result.bits, i);
            }
        }
    } else {
        int count = 0;
        for (uint64_t i = ctx->mol_ctx.atom.beg; i < ctx->mol_ctx.atom.end; ++i) {
            if (range.beg <= ctx->mol->atom.z[i] && ctx->mol->atom.z[i] <= range.end) {
                count += 1;
            }
        }
        return count;
    }

    return 0;
}

// @TODO: Implement spatial hashing for these
static int _within_flt(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->atom.z);
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_ATOM));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_BITFIELD));

    ASSERT(false);

    return 0;
}

static int _within_frng(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->atom.z);
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_ATOM));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FRANGE));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_BITFIELD));

    ASSERT(false);

    return 0;
}


// @TODO: All these have poor names

static inline irange_t remap_range_to_context(irange_t range, irange_t context) {
    if (range.beg == INT32_MIN)
        range.beg = 0;
    else 
        range.beg = range.beg - 1;

    if (range.end == INT32_MAX)
        range.end = context.end - context.beg;
    else 
        range.end = range.end;

    range.beg = context.beg + range.beg;
    range.end = context.beg + range.end;

    return range;
}

static inline irange_t clamp_range(irange_t range, irange_t context) {
    range.beg = CLAMP(range.beg, context.beg, context.end);
    range.end = CLAMP(range.end, context.beg, context.end);
    return range;
}

static int _atom(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_IRANGE_ARR));
    ASSERT(ctx && ctx->mol);

    const int64_t num_ranges = element_count(arg[0]);
    const irange_t* ranges = as_irange_arr(arg[0]);
    const irange_t ctx_range = ctx->mol_ctx.atom;
    const int32_t ctx_size = ctx_range.end - ctx_range.beg;
    if (dst) {
        ASSERT(dst->ptr && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_ATOM));
        md_bitfield_t result = as_bitfield(*dst);

        for (int64_t i = 0; i < num_ranges; ++i) {
            irange_t range = remap_range_to_context(ranges[i], ctx_range);
            range = clamp_range(range, ctx_range);
            bit_set(result.bits, range.beg, range.end-range.beg);
        }
    }
    else {
        ASSERT(ctx->arg_tokens);
        for (int64_t i = 0; i < num_ranges; ++i) {
            irange_t range = ranges[i];
            if (!range_in_range(remap_range_to_context(range, ctx_range), ctx_range)) {
                create_error(ctx->ir, ctx->arg_tokens[0], "supplied range (%i:%i) is not within expected range (%i:%i)",
                    range.beg, range.end, 1, ctx_size);
                return -1;
            }
        }
    }

    return 0;
}

static int _water(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->residue.atom_range && ctx->mol->atom.element);
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_RESIDUE));
    (void)arg;
    md_bitfield_t result = *((md_bitfield_t*)dst->ptr);

    md_element* elem = ctx->mol->atom.element;
    for (int32_t i = ctx->mol_ctx.residue.beg; i < ctx->mol_ctx.residue.end; ++i) {
        md_range range = ctx->mol->residue.atom_range[i];
        if (range.end - range.beg == 3) {
            int32_t j = range.beg;
            // Square each element index in the composition and sum up.
            // Water should be: 1*1 + 1*1 + 8*8 = 66
            int magic = elem[j] * elem[j] + elem[j+1] * elem[j+1] + elem[j+2] * elem[j+2];
            if (magic == 66) {
                bit_set(result.bits, (uint64_t)range.beg, (uint64_t)range.end - (uint64_t)range.beg);
            }
        }
    }
    
    return 0;
}

static int _protein(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->residue.name && ctx->mol->residue.atom_range);
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_RESIDUE));
    (void)arg;
    md_bitfield_t result = *((md_bitfield_t*)dst->ptr);

    for (int32_t i = ctx->mol_ctx.residue.beg; i < ctx->mol_ctx.residue.end; ++i) {
        str_t str = {.ptr = ctx->mol->residue.name[i], .len = strlen(ctx->mol->residue.name[i])};
        if (md_util_is_resname_amino_acid(str)) {
            md_range range = ctx->mol->residue.atom_range[i];
            bit_set(result.bits, (uint64_t)range.beg, (uint64_t)range.end - (uint64_t)range.beg);
        }
    }

    return 0;
}

static int _ion(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->residue.atom_range && ctx->mol->atom.element);
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_RESIDUE));
    (void)arg;
    md_bitfield_t result = *((md_bitfield_t*)dst->ptr);

    md_element* elem = ctx->mol->atom.element;
    for (int32_t i = ctx->mol_ctx.residue.beg; i < ctx->mol_ctx.residue.end; ++i) {
        md_range range = ctx->mol->residue.atom_range[i];
        // Currently only check for monatomic ions
        // We don't know the charges.
        // We make the assumption that if it is a residue of size 1 and of the correct element then its an ion
        if (range.end - range.beg == 1) {
            md_element elem = ctx->mol->atom.element[range.beg];
            switch (elem) {
            case 1:  goto set_bit;  // H+ / H-
            case 3:  goto set_bit;  // Li+
            case 4:  goto set_bit;  // Be2+
            case 8:  goto set_bit;  // O2-
            case 9:  goto set_bit;  // F-
            case 11: goto set_bit;  // Na+
            case 12: goto set_bit;  // Mg2+
            case 13: goto set_bit;  // Al3+
            case 16: goto set_bit;  // S2-
            case 17: goto set_bit;  // Cl-
            case 19: goto set_bit;  // K+
            case 20: goto set_bit;  // Ca2+
            case 35: goto set_bit;  // Br-
            case 47: goto set_bit;  // Ag+
            case 53: goto set_bit;  // I-
            case 55: goto set_bit;  // Cs+
            case 56: goto set_bit;  // Ba2+
            default: continue;
            }
        set_bit:
            bit_set_idx(result.bits, range.beg);
        }
    }

    return 0;
}

static int _residue(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->residue.name);
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_IRANGE_ARR));

    const int64_t num_ranges = element_count(arg[0]);
    const irange_t* ranges = as_irange_arr(arg[0]);
    const irange_t ctx_range = ctx->mol_ctx.residue;
    const int32_t ctx_size = ctx_range.end - ctx_range.beg;

    if (dst) {
        md_bitfield_t result = as_bitfield(*dst);
        ASSERT(dst->ptr && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_RESIDUE));
        for (int64_t i = 0; i < num_ranges; ++i) {
            irange_t range = remap_range_to_context(ranges[i], ctx_range);
            range = clamp_range(range, ctx_range);
            for (int64_t j = range.beg; j < range.end; ++j) {
                const uint64_t offset = ctx->mol->residue.atom_range[j].beg;
                const uint64_t length = ctx->mol->residue.atom_range[j].end - ctx->mol->residue.atom_range[j].beg;
                bit_set(result.bits, offset, length);
            }
        }
    }
    else {
        for (int64_t i = 0; i < num_ranges; ++i) {
            irange_t range = ranges[i];
            if (!range_in_range(remap_range_to_context(range, ctx_range), ctx_range)) {
                create_error(ctx->ir, ctx->arg_tokens[0], "supplied range (%i:%i) is not within expected range (%i:%i)",
                    range.beg, range.end, 1, ctx_size);
                return -1;
            }
        }
    }

    return 0;
}

static int _resname(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->residue.name);
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_STRING_ARR));

    md_bitfield_t result = as_bitfield(*dst);
    const uint64_t num_str = element_count(arg[0]);
    const str_t* str = as_string_arr(arg[0]);

    if (dst) {
        ASSERT(dst->ptr && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_RESIDUE));
        for (uint64_t i = ctx->mol_ctx.residue.beg; i < ctx->mol_ctx.residue.end; ++i) {
            for (uint64_t j = 0; j < num_str; ++j) {
                if (compare_str_cstr(str[j], ctx->mol->residue.name[i])) {
                    uint64_t offset = ctx->mol->residue.atom_range[i].beg;
                    uint64_t length = ctx->mol->residue.atom_range[i].end - ctx->mol->residue.atom_range[i].beg;
                    bit_set(result.bits, offset, length);
                    break;
                }
            }
        }
    }
    else {
        for (uint64_t j = 0; j < num_str; ++j) {
            bool match = false;
            for (uint64_t i = ctx->mol_ctx.residue.beg; i < ctx->mol_ctx.residue.end; ++i) {
                if (compare_str_cstr(str[j], ctx->mol->residue.name[i])) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                create_error(ctx->ir, ctx->arg_tokens[0], "The string '%.*s' did not match any residue within the structure", str[j].len, str[j].ptr);
                return -1;
            }
        }
    }

    return 0;
}

static int _resid(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->residue.name);
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_IRANGE_ARR));
    md_bitfield_t* bf = dst->ptr;

    const uint64_t  num_rid = element_count(arg[0]);
    const irange_t*     rid = arg[0].ptr;

    if (dst) {
        ASSERT(dst->ptr && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_RESIDUE));
        for (uint64_t i = ctx->mol_ctx.residue.beg; i < ctx->mol_ctx.residue.end; ++i) {
            for (uint64_t j = 0; j < num_rid; ++j) {
                if (in_range(rid[j], (int)ctx->mol->residue.id[i])) {
                    uint64_t offset = ctx->mol->residue.atom_range[i].beg;
                    uint64_t length = ctx->mol->residue.atom_range[i].end - ctx->mol->residue.atom_range[i].beg;
                    bit_set(bf->bits, offset, length);
                    break;
                }
            }
        }
    }
    else {
        for (uint64_t j = 0; j < num_rid; ++j) {
            bool match = false;
            for (uint64_t i = ctx->mol_ctx.residue.beg; i < ctx->mol_ctx.residue.end; ++i) {
                if (in_range(rid[j], (int)ctx->mol->residue.id[i])) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                create_error(ctx->ir, ctx->arg_tokens[0], "No matching residue id was found within the range (%i:%i)", rid[j].beg, rid[j].end);
                return -1;
            }
        }
    }

    return 0;
}

static int _chain_irng(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->chain.atom_range);
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_IRANGE_ARR));

    md_bitfield_t result = as_bitfield(*dst);
    const int64_t num_ranges = element_count(arg[0]);
    const irange_t* ranges = as_irange_arr(arg[0]);
    const irange_t ctx_range = ctx->mol_ctx.chain;
    const int32_t ctx_size = ctx_range.end - ctx_range.beg;

    if (dst) {
        ASSERT(dst->ptr && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_CHAIN));
        for (int64_t i = 0; i < num_ranges; ++i) {
            irange_t range = remap_range_to_context(ranges[i], ctx_range);
            for (int64_t j = range.beg; j < range.end; ++j) {
                const uint64_t offset = ctx->mol->chain.atom_range[j].beg;
                const uint64_t length = ctx->mol->chain.atom_range[j].end - ctx->mol->chain.atom_range[j].beg;
                bit_set(result.bits, offset, length);
            }
        }
    }
    else {
        for (int64_t i = 0; i < num_ranges; ++i) {
            irange_t range = ranges[i];
            if (!range_in_range(remap_range_to_context(range, ctx_range), ctx_range)) {
                create_error(ctx->ir, ctx->arg_tokens[0], "supplied range (%i:%i) is not within expected range (%i:%i)",
                    range.beg, range.end, 1, ctx_size);
                return -1;
            }
        }
    }


    return 0;
}

static int _chain_str(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->chain.id && ctx->mol->chain.atom_range);
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_STRING_ARR));
    md_bitfield_t* bf = dst->ptr;

    const uint64_t num_str = element_count(arg[0]);
    const str_t*   str     = arg[0].ptr;

    if (dst) {
        ASSERT(dst->ptr && is_type_equivalent(dst->type, (md_type_info_t)TI_BITFIELD_CHAIN));
        for (uint64_t i = ctx->mol_ctx.residue.beg; i < ctx->mol_ctx.residue.end; ++i) {
            for (uint64_t j = 0; j < num_str; ++j) {
                if (compare_str_cstr(str[j], ctx->mol->chain.id[i])) {
                    uint64_t offset = ctx->mol->chain.atom_range[i].beg;
                    uint64_t length = ctx->mol->chain.atom_range[i].end - ctx->mol->chain.atom_range[i].beg;
                    bit_set(bf->bits, offset, length);
                    break;
                }
            }
        }
    }
    else {
        for (uint64_t j = 0; j < num_str; ++j) {
            bool match = false;
            for (uint64_t i = ctx->mol_ctx.chain.beg; i < ctx->mol_ctx.chain.end; ++i) {
                if (compare_str_cstr(str[j], ctx->mol->chain.id[i])) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                create_error(ctx->ir, ctx->arg_tokens[0], "The string '%.*s' did not match any chain within the structure", str[j].len, str[j].ptr);
                return -1;
            }
        }
    }

    return 0;
}

// Property Compute

static int _distance(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->atom.x && ctx->mol->atom.y && ctx->mol->atom.z);
    ASSERT(dst);
    ASSERT(is_type_equivalent(dst->type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT3_ARR));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_FLOAT3_ARR));

    const vec3_t* a = as_vec3_arr(arg[0]);
    const vec3_t* b = as_vec3_arr(arg[1]);
    const int64_t a_len = element_count(arg[0]);
    const int64_t b_len = element_count(arg[1]);

    float d = 0;

    if (a_len > 0 && b_len > 0) {
        vec3_t com_a = {0};
        for (int64_t i = 0; i < a_len; ++i) {
            com_a = vec3_add(com_a, a[i]);
        }
        com_a = vec3_div_f(com_a, (float)a_len);

        vec3_t com_b = {0};
        for (int64_t i = 0; i < b_len; ++i) {
            com_b = vec3_add(com_b, b[i]);
        }
        com_b = vec3_div_f(com_b, (float)b_len);
        d = vec3_dist(com_a, com_b);
    }

    as_float(*dst) = d;
    dst->unit = MD_SCRIPT_UNIT_ANGSTROM;
    dst->min_range = 0.0f;
    dst->max_range = FLT_MAX;

    return 0;
}

static int _distance_min(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->atom.x && ctx->mol->atom.y && ctx->mol->atom.z);
    ASSERT(dst);
    ASSERT(is_type_equivalent(dst->type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT3_ARR));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_FLOAT3_ARR));

    const vec3_t* a = as_vec3_arr(arg[0]);
    const vec3_t* b = as_vec3_arr(arg[1]);
    const int64_t a_len = element_count(arg[0]);
    const int64_t b_len = element_count(arg[1]);

    float min_dist = a_len > 0 ? FLT_MAX : 0.0f;
    for (int64_t i = 0; i < a_len; ++i) {
        for (int64_t j = 0; j < b_len; ++j) {
            const float dist = vec3_dist(a[i], b[j]);
            min_dist = MIN(min_dist, dist);
        }
    }

    as_float(*dst) = min_dist;
    dst->unit = MD_SCRIPT_UNIT_ANGSTROM;
    dst->min_range = 0.0f;
    dst->max_range = FLT_MAX;

    return 0;
}

static int _distance_max(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->atom.x && ctx->mol->atom.y && ctx->mol->atom.z);
    ASSERT(dst);
    ASSERT(is_type_equivalent(dst->type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT3_ARR));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_FLOAT3_ARR));

    const vec3_t* a = as_vec3_arr(arg[0]);
    const vec3_t* b = as_vec3_arr(arg[1]);
    const int64_t a_len = element_count(arg[0]);
    const int64_t b_len = element_count(arg[1]);

    float max_dist = 0;
    for (int64_t i = 0; i < a_len; ++i) {
        for (int64_t j = 0; j < b_len; ++j) {
            const float dist = vec3_dist(a[i], b[j]);
            max_dist = MAX(max_dist, dist);
        }
    }

    as_float(*dst) = max_dist;
    dst->unit = MD_SCRIPT_UNIT_ANGSTROM;
    dst->min_range = 0.0f;
    dst->max_range = FLT_MAX;

    return 0;
}

static int _distance_pair(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT3_ARR));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_FLOAT3_ARR));

    const int64_t a_len = element_count(arg[0]);
    const int64_t b_len = element_count(arg[1]);
    if (dst) {
        ASSERT(ctx && ctx->mol && ctx->mol->atom.x && ctx->mol->atom.y && ctx->mol->atom.z);
        ASSERT(is_type_directly_compatible(dst->type, (md_type_info_t)TI_FLOAT_ARR));
        const vec3_t* a = as_vec3_arr(arg[0]);
        const vec3_t* b = as_vec3_arr(arg[1]);
        float* dst_arr = as_float_arr(*dst);
        const int64_t dst_len = element_count(*dst);
        ASSERT(dst_len == a_len * b_len);

        for (int64_t i = 0; i < a_len; ++i) {
            for (int64_t j = 0; j < b_len; ++j) {
                const float dist = vec3_dist(a[i], b[j]);
                dst_arr[i * b_len + j] = dist;
            }
        }

        dst->unit = MD_SCRIPT_UNIT_ANGSTROM;
        dst->min_range = 0.0f;
        dst->max_range = FLT_MAX;
        return 0;
    } else {
        return (int)(a_len * b_len);
    }
}

static int _angle(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->atom.x && ctx->mol->atom.y && ctx->mol->atom.z);
    ASSERT(dst);
    ASSERT(is_type_equivalent(dst->type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_equivalent(arg[0].type, (md_type_info_t)TI_FLOAT3));
    ASSERT(is_type_equivalent(arg[1].type, (md_type_info_t)TI_FLOAT3));
    ASSERT(is_type_equivalent(arg[2].type, (md_type_info_t)TI_FLOAT3));
    
    const vec3_t a = as_vec3(arg[0]);
    const vec3_t b = as_vec3(arg[1]);
    const vec3_t c = as_vec3(arg[2]);
    const vec3_t v0 = vec3_normalize(vec3_sub(a, b));
    const vec3_t v1 = vec3_normalize(vec3_sub(c, b));
    
    as_float(*dst) = (float)RAD_TO_DEG(acosf(vec3_dot(v0, v1)));
    dst->unit = MD_SCRIPT_UNIT_DEGREES;
    dst->min_range = 0.0f;
    dst->max_range = 180.0f;
    return 0;
}

static int _dihedral(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->atom.x && ctx->mol->atom.y && ctx->mol->atom.z);
    ASSERT(dst);
    ASSERT(is_type_equivalent(dst->type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_equivalent(arg[0].type, (md_type_info_t)TI_FLOAT3));
    ASSERT(is_type_equivalent(arg[1].type, (md_type_info_t)TI_FLOAT3));
    ASSERT(is_type_equivalent(arg[2].type, (md_type_info_t)TI_FLOAT3));
    ASSERT(is_type_equivalent(arg[3].type, (md_type_info_t)TI_FLOAT3));

    const vec3_t a = as_vec3(arg[0]);
    const vec3_t b = as_vec3(arg[1]);
    const vec3_t c = as_vec3(arg[2]);
    const vec3_t d = as_vec3(arg[3]);

    as_float(*dst) = (float)RAD_TO_DEG(dihedral_angle(a,b,c,d));
    dst->unit = MD_SCRIPT_UNIT_DEGREES;
    dst->min_range = -180.0f;
    dst->max_range = 180.0f;

    return 0;
}

static int _rmsd(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(ctx && ctx->mol && ctx->mol->atom.x && ctx->mol->atom.y && ctx->mol->atom.z);
    ASSERT(dst);
    ASSERT(is_type_equivalent(dst->type, (md_type_info_t)TI_FLOAT));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT3_ARR));

    ASSERT(false);

    return 0;
}


// #################
// ###   CASTS   ###
// #################

static int _cast_int_to_flt(data_t* dst, data_t arg[], eval_context_t* ctx){
    ASSERT(dst && compare_type_info(dst->type, (md_type_info_t)TI_FLOAT));
    ASSERT(compare_type_info(arg[0].type, (md_type_info_t)TI_INT));
    (void)ctx;
    as_int(*dst) = (int)as_float(arg[0]);
    return 0;
}

static int _cast_int_to_irng(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && compare_type_info(dst->type, (md_type_info_t)TI_IRANGE));
    ASSERT(compare_type_info(arg[0].type, (md_type_info_t)TI_INT));
    (void)ctx;
    as_irange(*dst) = (irange_t){as_int(arg[0]), as_int(arg[0])};
    return 0;
}

static int _cast_irng_to_frng(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && compare_type_info(dst->type, (md_type_info_t)TI_FRANGE));
    ASSERT(compare_type_info(arg[0].type, (md_type_info_t)TI_IRANGE));
    (void)ctx;
    as_frange(*dst) = (frange_t){(float)as_irange(arg[0]).beg, (float)as_irange(arg[0]).end};
    return 0;
}

static int _cast_int_arr_to_flt_arr(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && is_type_directly_compatible(dst->type, (md_type_info_t)TI_FLOAT_ARR));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_INT_ARR));
    (void)ctx;
    const uint64_t dst_len = element_count(*dst);
    const uint64_t arg_len = element_count(arg[0]);
    ASSERT(dst_len == arg_len);

    float* d = dst->ptr;
    int*   s = arg[0].ptr;

    for (uint64_t i = 0; i < dst_len; ++i) {
        d[i] = (float)s[i]; 
    }
    return 0;
}
static int _cast_irng_arr_to_frng_arr(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && is_type_directly_compatible(dst->type, (md_type_info_t)TI_FRANGE_ARR));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_IRANGE_ARR));
    (void)ctx;
    const uint64_t dst_len = element_count(*dst);
    const uint64_t arg_len = element_count(arg[0]);
    ASSERT(dst_len == arg_len);

    frange_t* d = dst->ptr;
    irange_t* s = arg[0].ptr;

    for (uint64_t i = 0; i < dst_len; ++i) {
        d[i].beg = (float)s[i].beg;
        d[i].end = (float)s[i].end; 
    }
    return 0;
}



static int _cast_bf_to_atom(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && compare_type_info(dst->type, (md_type_info_t)TI_BITFIELD_ATOM));
    ASSERT(compare_type_info(arg[0].type, (md_type_info_t)TI_BITFIELD));
    (void)dst;
    (void)arg;
    (void)ctx;
    return 0;
}

static int _cast_bf_to_residue(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && compare_type_info(dst->type, (md_type_info_t)TI_BITFIELD_RESIDUE));
    ASSERT(compare_type_info(arg[0].type, (md_type_info_t)TI_BITFIELD));
    (void)dst;
    (void)arg;
    (void)ctx;
    return 0;
}

static int _cast_bf_to_chain(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && compare_type_info(dst->type, (md_type_info_t)TI_BITFIELD_CHAIN));
    ASSERT(compare_type_info(arg[0].type, (md_type_info_t)TI_BITFIELD));
    (void)dst;
    (void)arg;
    (void)ctx;
    return 0;
}

static int _com(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && compare_type_info(dst->type, (md_type_info_t)TI_FLOAT3));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT3_ARR));
    (void)dst;
    (void)arg;
    (void)ctx;

    const vec3_t* pos = as_vec3_arr(arg[0]);
    const int64_t pos_size = element_count(arg[0]);

    vec3_t com = {0};
    if (pos_size > 0) {
        for (int64_t i = 0; i < pos_size; ++i) {
            com = vec3_add(com, pos[i]);
        }
        const double scl = 1.0 / (double)pos_size;
        com.x = (float)(com.x * scl);
        com.y = (float)(com.y * scl);
        com.z = (float)(com.z * scl);
    }

    as_vec3(*dst) = com;

    return 0;
}

static int _plane(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(dst && compare_type_info(dst->type, (md_type_info_t)TI_FLOAT4));
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT3_ARR));
    (void)dst;
    (void)arg;
    (void)ctx;

    ASSERT(false);
    return 0;
}

static int _position_int(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_INT_ARR));
    ASSERT(arg[0].ptr);
    const int* indices = as_int_arr(arg[0]);
    const uint64_t num_idx = element_count(arg[0]);

    if (dst) {
        ASSERT(element_count(*dst) == element_count(arg[0]));
        ASSERT(dst && is_type_directly_compatible(dst->type, (md_type_info_t)TI_FLOAT3_ARR));
        vec3_t* position = as_vec3_arr(*dst);

        for (uint64_t i = 0; i < num_idx; ++i) {
            // Shift here since we use 1 based indices for atoms
            const uint64_t idx = ctx->mol_ctx.atom.beg + indices[i] - 1;
            ASSERT(0 <= idx && idx < ctx->mol_ctx.atom.end);
            position[i] = (vec3_t){ctx->mol->atom.x[idx], ctx->mol->atom.y[idx], ctx->mol->atom.z[idx]};
        }
    } else {
        for (uint64_t i = 0; i < num_idx; ++i) {
            if (indices[i] < 1 || (int)ctx->mol_ctx.atom.end < indices[i]) {
                return -1;
            }
        }
        return (int)num_idx;
    }

    return 0;
}

static int _position_irng(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_IRANGE_ARR));
    ASSERT(arg[0].ptr);

    const int64_t num_ranges = element_count(arg[0]);
    const irange_t* ranges = as_irange_arr(arg[0]);
    const irange_t ctx_range = ctx->mol_ctx.atom;
    const int32_t ctx_size = ctx_range.end - ctx_range.beg;

    if (dst) {
        ASSERT(dst && is_type_directly_compatible(dst->type, (md_type_info_t)TI_FLOAT3_ARR));
        vec3_t* position = as_vec3_arr(*dst);

        uint64_t dst_idx = 0;
        for (int64_t i = 0; i < num_ranges; ++i) {
            irange_t range = remap_range_to_context(ranges[i], ctx_range);
            range = clamp_range(range, ctx_range);
            for (int64_t j = range.beg; j < range.end; ++j) {
                const int64_t src_idx = ctx->mol_ctx.atom.beg + j;
                position[dst_idx++] = (vec3_t){ctx->mol->atom.x[src_idx], ctx->mol->atom.y[src_idx], ctx->mol->atom.z[src_idx]};
            }
        }
    } else {
        int count = 0;
        for (int64_t i = 0; i < num_ranges; ++i) {
            irange_t range = remap_range_to_context(ranges[i], ctx_range);
            if (range_in_range(range, ctx_range)) {
                count += range.end - range.beg;
            } else {
                create_error(ctx->ir, ctx->arg_tokens[0], "supplied range (%i:%i) is not within expected range (%i:%i)",
                    ranges[i].beg, ranges[i].end, 1, ctx_size);
                return -1;
            }
        }
        return count;
    }

    return 0;
}

static int _position_bf(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_BITFIELD));
    ASSERT(arg[0].ptr);
    const md_bitfield_t bf = as_bitfield(arg[0]);
    const uint64_t offset = ctx->mol_ctx.atom.beg;
    const uint64_t length = ctx->mol_ctx.atom.end - ctx->mol_ctx.atom.beg;

    if (dst) {
        //const int64_t num_bits = bit_count(bf.bits, offset, length);
        ASSERT(dst && is_type_directly_compatible(dst->type, (md_type_info_t)TI_FLOAT3_ARR));
        //ASSERT(element_count(*dst) == num_bits);
        vec3_t* position = as_vec3_arr(*dst);

        const int64_t max_count = element_count(*dst);

        int64_t dst_idx = 0;
        uint64_t bit_idx = 0;
        int64_t num_bits = length;
        while ((bit_idx = bit_scan(bf.bits, offset + bit_idx, num_bits)) != 0) {
            ASSERT(dst_idx < max_count);
            uint64_t src_idx = bit_idx - 1;
            position[dst_idx++] = (vec3_t){ctx->mol->atom.x[src_idx], ctx->mol->atom.y[src_idx], ctx->mol->atom.z[src_idx]};
            num_bits = length - bit_idx;
            if (num_bits <= 0) break;
        }
    } else {
        return (int)bit_count(bf.bits, offset, length);
    }

    return 0;
}

static int _rdf(data_t* dst, data_t arg[], eval_context_t* ctx) {
    (void)ctx;
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_FLOAT3_ARR));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_FLOAT3_ARR));
    ASSERT(is_type_directly_compatible(arg[2].type, (md_type_info_t)TI_FLOAT));
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_DISTRIBUTION));
    ASSERT(arg[0].ptr);
    ASSERT(arg[1].ptr);
    ASSERT(arg[2].ptr);
    ASSERT(dst->ptr);

    const vec3_t* ref_pos = as_vec3_arr(arg[0]);
    const int64_t ref_size = element_count(arg[0]);

    const vec3_t* target_pos = as_vec3_arr(arg[1]);
    const int64_t target_size = element_count(arg[1]);

    const float cutoff = as_float(arg[2]);
    const int32_t num_bins = DIST_BINS;
    float* bins = as_float_arr(*dst);

    memset(bins, 0, num_bins * sizeof(float));

    for (int64_t i = 0; i < ref_size; ++i) {
        for (int64_t j = 0; j < target_size; ++j) {
            const float d = vec3_dist(ref_pos[i], target_pos[j]);
            if (d < cutoff) {
                const int32_t bin_idx = (int32_t)((d / cutoff) * num_bins);
                bins[bin_idx] += 1.0f;
            }
        }
    }

    // Normalize the distribution
    const float scl = 1.0f / num_bins;
    for (int64_t i = 0; i < num_bins; ++i) {
        bins[i] *= scl;
    }

    dst->min_range = 0.0f;
    dst->max_range = cutoff;

    return 0;
}

static int _sdf(data_t* dst, data_t arg[], eval_context_t* ctx) {
    ASSERT(is_type_directly_compatible(arg[0].type, (md_type_info_t)TI_BITFIELD_RESIDUE));
    ASSERT(is_type_directly_compatible(arg[1].type, (md_type_info_t)TI_FLOAT3_ARR));
    ASSERT(is_type_directly_compatible(arg[2].type, (md_type_info_t)TI_FLOAT));
    ASSERT(dst && is_type_equivalent(dst->type, (md_type_info_t)TI_VOLUME));
    ASSERT(arg[0].ptr);
    ASSERT(arg[1].ptr);
    ASSERT(arg[2].ptr);
    ASSERT(dst->ptr);

    if (dst) {
        
    } else {
        
    }

    return 0;
}

#undef ANY_LENGTH
#undef ANY_LEVEL

#endif
#include "filter.h"
#include "utest.h"
#include "molecule.h"

extern void filter_func_all     (uint64_t* bits, const molecule_t* mol);
extern void filter_func_none    (uint64_t* bits, const molecule_t* mol);
extern void filter_func_protein (uint64_t* bits, const molecule_t* mol);
extern void filter_func_water   (uint64_t* bits, const molecule_t* mol);
extern void filter_func_name    (const char* str,                uint64_t* bits, const molecule_t* mol);
extern void filter_func_resname (const char* str,                uint64_t* bits, const molecule_t* mol);
extern void filter_func_resid   (int min_range, int max_range,   uint64_t* bits, const molecule_t* mol);
extern void filter_func_residue (int min_range, int max_range,   uint64_t* bits, const molecule_t* mol);
extern void filter_func_chain   (const char* str,                uint64_t* bits, const molecule_t* mol);
extern void filter_func_within  (float min_range, float max_range, const uint64_t* in_bits, uint64_t* out_bits, const molecule_t* mol);

uint32_t str_len(const char* cstr) {
    if (!cstr) return 0;
    uint32_t len = 0;
    while(cstr[len] != '\0') len++;
    return len;
}

uint64_t make_bits(const char* str) {
    uint64_t bits = {0};
    uint64_t len = str_len(str) < 64 ? str_len(str) : 64;
    for (uint64_t i = 0; i < len; i++) {
        if (str[i] != '0') bits |= (1LLU << i);
    }
    return bits;
}

UTEST_MAIN();

UTEST(parser, test) {
    const uint32_t atom_count = 8;
    const float x[] = {1,2,3,4,5,6,7,8};
    const float y[] = {1,1,1,1,1,1,1,1};
    const float z[] = {2,2,2,2,2,2,2,2};
    const float r[] = {1,2,3,4,4,4,5,1};
    const float m[] = {1,2,2,2,2,4,4,4};
    const uint8_t e[] = {1,8,1,8,5,8,8,8};
    const char* n[] = {"H", "O", "H", "He", "C", "N", "CA", "O"};
    const float b[] = {0,0,0,0,0,1,0,0};
    const float o[] = {1,1,1,1,1,2,2,2};
    const uint32_t r_idx[] = {0,0,0,1,1,1,1,1};
    const uint32_t c_idx[] = {0,0,0,0,0,0,0,0};

    const uint32_t res_count = 2;
    const char* r_name[] = {"SOL", "LYS"};
    const uint32_t r_id[] = {1, 2};
    const atom_range_t r_range[] = {{0, 3}, {3, 8}};

    const uint32_t chain_count = 1;
    const char* c_id[] = {"A"};
    const atom_range_t c_range[] = {0,8};

    molecule_t mol = {0};

    mol.atom.count = atom_count;
    mol.atom.x = x;
    mol.atom.y = y;
    mol.atom.z = z;
    mol.atom.radius = r;
    mol.atom.mass = m;
    mol.atom.element = e;
    mol.atom.name = n;
    mol.atom.bfactor = b;
    mol.atom.occupancy = o;
    mol.atom.residue_idx = r_idx;
    mol.atom.chain_idx = c_idx;

    mol.residue.count = res_count;
    mol.residue.resname = r_name;
    mol.residue.resid = r_id;
    mol.residue.atom_range = r_range;

    mol.chain.count = chain_count;
    mol.chain.id = c_id;
    mol.chain.atom_range = c_range;

    uint64_t bits = 0;
    uint64_t ref = 0;
    {
        bits = 0;
        filter_func_all(&bits, &mol);
        ref = make_bits("11111111");
        EXPECT_EQ(bits, ref);
    }

    {
        bits = 0;
        filter_func_none(&bits, &mol);
        ref = make_bits("00000000");
        EXPECT_EQ(bits, ref);
    }

    {
        bits = 0;
        filter_func_protein(&bits, &mol);
        ref = make_bits("00011111");
        EXPECT_EQ(bits, ref);
    }

    {
        bits = 0;
        filter_func_water(&bits, &mol);
        ref = make_bits("11100000");
        EXPECT_EQ(bits, ref);
    }

    {
        bits = 0;
        filter_func_name("H", &bits, &mol);
        ref = make_bits("10100000");
        EXPECT_EQ(bits, ref);

        bits = 0;
        filter_func_name("CA", &bits, &mol);
        ref = make_bits("00000010");
        EXPECT_EQ(bits, ref);
    }

    {
        bits = 0;
        filter_func_resname("SOL", &bits, &mol);
        ref = make_bits("11100000");
        EXPECT_EQ(bits, ref);

        bits = 0;
        filter_func_resname("LYS", &bits, &mol);
        ref = make_bits("00011111");
        EXPECT_EQ(bits, ref);
    }

    {
        bits = 0;
        filter_func_resid(1, 1, &bits, &mol);
        ref = make_bits("11100000");
        EXPECT_EQ(bits, ref);

        bits = 0;
        filter_func_resid(2, 2, &bits, &mol);
        ref = make_bits("00011111");
        EXPECT_EQ(bits, ref);
    }

    {
        bits = 0;
        filter_func_residue(0, 0, &bits, &mol);
        ref = make_bits("11100000");
        EXPECT_EQ(bits, ref);

        bits = 0;
        filter_func_residue(1, 1, &bits, &mol);
        ref = make_bits("00011111");
        EXPECT_EQ(bits, ref);
    }

    {
        bits = 0;
        filter_func_chain("A", &bits, &mol);
        ref = make_bits("11111111");
        EXPECT_EQ(bits, ref);
    }

    {
        uint64_t mask = make_bits("10000000");
        bits = 0;
        filter_func_within(0, 3, &mask, &bits, &mol);
        ref = make_bits("11110000");
        EXPECT_EQ(bits, ref);
    }

    {
        const char* expr = "resname(ALA LYS) and within(2.5, name(C H O) or residue(1:18)) and protein or @cool";
        filter_t* filt = filter_create(expr, str_len(expr));
        EXPECT_TRUE(filt);

        uint64_t stored_bits = make_bits("00000001");
        filter_stored_selection_t stored;
        stored.identifier = "cool";
        stored.bits = &stored_bits;
        filter_context_t ctx;
        ctx.mol = &mol;
        ctx.sel = &stored;
        ctx.sel_count = 1;
        uint64_t bits = 0;
        bool res = filter_apply(&bits, atom_count, filt, &ctx);
        EXPECT_TRUE(res);


    }
    {
#if 0
        const char* expr = "all and residue(1:48) or not name(H C O)";
        EXPECT_TRUE(filter_create(expr, str_len(expr)));
#endif
    }
    {
        const char* expr = "all and not resjizzle(1 2 3 4)";
        EXPECT_FALSE(filter_create(expr, str_len(expr)));
    }
    {
        const char* expr = "residue(1 ALA 8 1:8)";
        EXPECT_FALSE(filter_create(expr, str_len(expr)));
    }
}
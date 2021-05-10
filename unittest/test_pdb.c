#include "utest.h"
#include <string.h>

#include <md_pdb.h>
#include <md_trajectory.h>
#include <core/md_allocator.h>
#include <core/md_file.h>

UTEST(pdb, parse_ordinary) {
    const str_t path = make_cstr(MD_UNITTEST_DATA_DIR"/1k4r.pdb");
    struct md_pdb_data_t pdb_data = {0};
    bool result = md_pdb_data_parse_file(path, &pdb_data, default_allocator);
    EXPECT_TRUE(result);
    EXPECT_EQ(pdb_data.num_models, 0);
    EXPECT_EQ(pdb_data.num_atom_coordinates, 9084);
    EXPECT_EQ(pdb_data.num_connections, 36);
    EXPECT_EQ(pdb_data.num_cryst1, 1);
    EXPECT_EQ(pdb_data.num_helices, 24);
    EXPECT_EQ(pdb_data.num_sheets, 102);

    md_pdb_data_free(&pdb_data, default_allocator);
}

UTEST(pdb, parse_trajectory) {
    const str_t path = make_cstr(MD_UNITTEST_DATA_DIR "/1ALA-560ns.pdb");
    struct md_pdb_data_t pdb_data = {0};
    bool result = md_pdb_data_parse_file(path, &pdb_data, default_allocator);
    EXPECT_TRUE(result);
    EXPECT_EQ(pdb_data.num_models, 38);
    EXPECT_EQ(pdb_data.num_atom_coordinates, 5814);
    EXPECT_EQ(pdb_data.num_cryst1, 1);
    EXPECT_EQ(pdb_data.num_connections, 0);
    EXPECT_EQ(pdb_data.num_helices, 0);
    EXPECT_EQ(pdb_data.num_sheets, 0);

    md_file_o* file = md_file_open(path, MD_FILE_READ | MD_FILE_BINARY);
    ASSERT_NE(file, NULL);
    for (int64_t i = 0; i < pdb_data.num_models; ++i) {
        char data[6] = {0};
        md_file_seek(file, pdb_data.models[i].byte_offset, MD_FILE_BEG);
        md_file_read(file, data, 5);
        EXPECT_EQ(strncmp(data, "MODEL", 5), 0);
    }
    md_file_close(file);

    md_pdb_data_free(&pdb_data, default_allocator);
}

UTEST(pdb, trajectory_i) {
    const str_t path = make_cstr(MD_UNITTEST_DATA_DIR "/1ALA-560ns.pdb");
    struct md_trajectory_i* traj = md_pdb_trajectory_open(path, default_allocator);
    EXPECT_NE(traj, NULL);

    md_trajectory_header_t header;
    EXPECT_TRUE(md_trajectory_extract_header(traj, &header));

    EXPECT_EQ(header.num_atoms, 153);
    EXPECT_EQ(header.num_frames, 38);

    const int64_t mem_size = header.num_atoms * 3 * sizeof(float);
    void* mem_ptr = md_alloc(default_temp_allocator, mem_size);
    float *x = (float*)mem_ptr;
    float *y = (float*)mem_ptr + header.num_atoms * 1;
    float *z = (float*)mem_ptr + header.num_atoms * 2;
    float box[3][3] = {0};
    double timestamp = 0;

    md_trajectory_data_t write_target = {
        .num_atoms = header.num_atoms,
        .x = x,
        .y = y,
        .z = z,
        .box = box,
        .timestamp = &timestamp
    };

    for (int64_t i = 0; i < header.num_frames; ++i) {
        EXPECT_TRUE(md_trajectory_load_frame(traj, i, MD_TRAJ_FIELD_XYZ | MD_TRAJ_FIELD_BOX | MD_TRAJ_FIELD_TIMESTAMP, &write_target));
    }

    md_free(default_temp_allocator, mem_ptr, mem_size);

    md_pdb_trajectory_close(traj);
}

UTEST(pdb, create_molecule) {
    const md_allocator_i* alloc = default_allocator;
    const char* raw_path = MD_UNITTEST_DATA_DIR "/1k4r.pdb";
    const str_t path = {raw_path, strlen(raw_path)};

    struct md_pdb_data_t pdb_data = {0};
    EXPECT_TRUE(md_pdb_data_parse_file(path, &pdb_data, alloc));

    md_pdb_molecule_t pdb_mol = {0};
    EXPECT_TRUE(md_pdb_molecule_init(&pdb_mol, &pdb_data, alloc));

    ASSERT_EQ(pdb_mol.mol.atom.count, pdb_data.num_atom_coordinates);

    for (int64_t i = 0; i < pdb_mol.mol.atom.count; ++i) {
        EXPECT_EQ(pdb_mol.mol.atom.x[i], pdb_data.atom_coordinates[i].x);
        EXPECT_EQ(pdb_mol.mol.atom.y[i], pdb_data.atom_coordinates[i].y);
        EXPECT_EQ(pdb_mol.mol.atom.z[i], pdb_data.atom_coordinates[i].z);
    }

    md_pdb_molecule_free(&pdb_mol, alloc);
    md_pdb_data_free(&pdb_data, alloc);
}
#include "md_pdb.h"

#include <core/md_common.h>
#include <core/md_array.inl>
#include <core/md_str.h>
#include <core/md_file.h>
#include <core/md_arena_allocator.h>
#include <core/md_allocator.h>
#include <core/md_log.h>
#include <md_molecule.h>
#include <md_trajectory.h>
#include <md_util.h>

#include <string.h> // memcpy

#ifdef __cplusplus
extern "C" {
#endif

#define MD_PDB_MOL_MAGIC  0x285ada29078a9bc8
#define MD_PDB_TRAJ_MAGIC 0x2312ad7b78a9bc78

typedef struct label {
    char str[8];
} label_t;

// The opaque blob
typedef struct pdb_molecule {
    uint64_t magic;
    label_t* labels;
    md_allocator_i* allocator;
} pdb_molecule_t;

typedef struct pdb_trajectory {
    uint64_t magic;
    md_file_o* file;
    uint64_t filesize;
    int64_t* frame_offsets;
    float box[3][3];
    md_allocator_i* allocator;
} pdb_trajectory_t;

static inline bool compare_n(str_t str, const char* cstr, int64_t len) {
    if (str.len < len) return false;
    for (int64_t i = 0; i < len; ++i) {
        if (str.ptr[i] != cstr[i]) return false;
    }
    return true;
}

static inline void copy_str_field(char* dst, int64_t dst_size, str_t line, int64_t beg, int64_t end) {
    if (line.len < end) {
        ASSERT(dst_size > 0);
        dst[0] = '\0';
    }
    str_t src = trim_whitespace(substr(line, beg - 1, end-beg + 1));
    ASSERT(dst_size >= src.len);
    memcpy(dst, src.ptr, src.len);
}

// We massage the beg and end indices here to correspond the pdb specification
// This makes our life easier when specifying all the different ranges
static inline int32_t extract_int(str_t line, int64_t beg, int64_t end) {
    if (line.len < end) return 0;
    return (int32_t)parse_int(trim_whitespace(substr(line, beg - 1, end-beg + 1)));
}

static inline float extract_float(str_t line, int64_t beg, int64_t end) {
    if (line.len < end) return 0.0f;
    return (float)parse_float(trim_whitespace(substr(line, beg - 1, end-beg + 1)));
}

static inline char extract_char(str_t line, int32_t idx) {
    if (line.len < idx) return 0;
    return line.ptr[idx - 1];
}

static inline struct md_pdb_coordinate extract_coord(str_t line) {
    struct md_pdb_coordinate coord = {
        .atom_serial = extract_int(line, 7, 11),
        .atom_name = {0},
        .alt_loc = extract_char(line, 17),
        .res_name = {0},
        .chain_id = extract_char(line, 22),
        .res_seq = extract_int(line, 23, 26),
        .icode = extract_char(line, 27),
        .x = extract_float(line, 31, 38),
        .y = extract_float(line, 39, 46),
        .z = extract_float(line, 47, 54),
        .occupancy = extract_float(line, 55, 60),
        .temp_factor = extract_float(line, 61, 66),
        .element = {0},
        .charge = {0},
        .is_hetatm = compare_n(line, "HETATM", 6),
    };
    copy_str_field(coord.atom_name, ARRAY_SIZE(coord.atom_name),    line, 13, 16);
    copy_str_field(coord.res_name,  ARRAY_SIZE(coord.res_name),     line, 18, 20);
    copy_str_field(coord.element,   ARRAY_SIZE(coord.element),      line, 77, 78);
    copy_str_field(coord.charge,    ARRAY_SIZE(coord.charge),       line, 79, 80);
    return coord;
}

static inline struct md_pdb_helix extract_helix(str_t line) {
    struct md_pdb_helix helix = {
        .serial_number = extract_int(line, 8, 10),
        .id = {0},
        .init_res_name = {0},
        .init_chain_id = extract_char(line, 20),
        .init_res_seq = extract_int(line, 22, 25),
        .init_res_i_code = extract_char(line, 26),
        .end_res_name = {0},
        .end_chain_id = extract_char(line, 32),
        .end_res_seq = extract_int(line, 34, 37),
        .end_res_i_code = extract_char(line, 38),
        .helix_class = extract_int(line, 39, 40),
        .comment = {0},
        .length = extract_int(line, 72, 76),
    };
    copy_str_field(helix.id,            ARRAY_SIZE(helix.id),               line, 12, 14);
    copy_str_field(helix.init_res_name, ARRAY_SIZE(helix.init_res_name),    line, 16, 18);
    copy_str_field(helix.end_res_name,  ARRAY_SIZE(helix.end_res_name),     line, 28, 30);
    copy_str_field(helix.comment,       ARRAY_SIZE(helix.comment),          line, 41, 70);
    return helix;
}

static inline struct md_pdb_sheet extract_sheet(str_t line) {
    struct md_pdb_sheet sheet = {
        .strand = extract_int(line, 8, 10),
        .id = {0},
        .num_strands = extract_int(line, 15, 16),
        .init_res_name = {0},
        .init_chain_id = extract_char(line, 20),
        .init_res_seq = extract_int(line, 22, 25),
        .init_res_i_code = extract_char(line, 26),
        .end_res_name = {0},
        .end_chain_id = extract_char(line, 32),
        .end_res_seq = extract_int(line, 34, 37),
        .end_res_i_code = extract_char(line, 38),
        .sense = extract_int(line, 39, 40),
        .cur_atom_name = {0},
        .cur_res_name = {0},
        .cur_chain_id = extract_char(line, 50),
        .cur_res_seq = extract_int(line, 51, 54),
        .cur_res_i_code = extract_char(line, 55),
        .prev_atom_name = {0},
        .prev_res_name = {0},
        .prev_chain_id = extract_char(line, 65),
        .prev_res_seq = extract_int(line, 66, 69),
        .prev_res_i_code = extract_char(line, 70),
    };
    copy_str_field(sheet.id,            ARRAY_SIZE(sheet.id),               line, 12, 14);
    copy_str_field(sheet.init_res_name, ARRAY_SIZE(sheet.init_res_name),    line, 18, 20);
    copy_str_field(sheet.end_res_name,  ARRAY_SIZE(sheet.end_res_name),     line, 29, 31);
    copy_str_field(sheet.cur_atom_name, ARRAY_SIZE(sheet.cur_atom_name),    line, 42, 45);
    copy_str_field(sheet.cur_res_name,  ARRAY_SIZE(sheet.cur_res_name),     line, 46, 48);
    copy_str_field(sheet.prev_atom_name,ARRAY_SIZE(sheet.prev_atom_name),   line, 57, 60);
    copy_str_field(sheet.prev_res_name, ARRAY_SIZE(sheet.prev_res_name),    line, 61, 63);
    return sheet;
}

static inline struct md_pdb_connect extract_connect(str_t line) {
    line = trim_whitespace(line);
    struct md_pdb_connect c = {
        .atom_serial = {
            [0] = extract_int(line, 7, 11),
            [1] = extract_int(line, 12, 16),
            [2] = (line.len > 20) ? extract_int(line, 17, 21) : 0,
            [3] = (line.len > 25) ? extract_int(line, 22, 26) : 0,
            [4] = (line.len > 30) ? extract_int(line, 27, 31) : 0,
        },
    };
    return c;
}

static inline struct md_pdb_cryst1 extract_cryst1(str_t line) {
    line = trim_whitespace(line);
    struct md_pdb_cryst1 c = {
        .a = extract_float(line, 7,  15),
        .b = extract_float(line, 16, 24),
        .c = extract_float(line, 25, 33),
        .alpha = extract_float(line, 34, 40),
        .beta  = extract_float(line, 41, 47),
        .gamma = extract_float(line, 48, 54),
        .space_group = {0},
        .z = extract_int(line, 67, 70),
    };
    copy_str_field(c.space_group, ARRAY_SIZE(c.space_group), line, 56, 66);
    return c;
}

static inline void append_connect(struct md_pdb_connect* connect, const struct md_pdb_connect* other) {
    memcpy(&connect->atom_serial[5], &other->atom_serial[1], 3 * sizeof(int32_t));
}

// This is lowlevel cruft for enabling parallel loading and decoding of frames
// Returns size in bytes of frame, frame_data_ptr is optional and is the destination to write the frame data to.
int64_t pdb_extract_frame(struct md_trajectory_o* inst, int64_t frame_idx, void* frame_data_ptr) {
    pdb_trajectory_t* pdb = (pdb_trajectory_t*)inst;
    ASSERT(pdb);
    ASSERT(pdb->magic == MD_PDB_TRAJ_MAGIC);

    if (!pdb->filesize) {
        md_print(MD_LOG_TYPE_ERROR, "File size is zero");
        return 0;
    }

    if (!pdb->file) {
        md_print(MD_LOG_TYPE_ERROR, "File handle is NULL");
        return 0;
    }

    if (!pdb->frame_offsets) {
        md_print(MD_LOG_TYPE_ERROR, "Frame offsets is empty");
        return 0;
    }

    if (frame_idx < 0 || (int64_t)md_array_size(pdb->frame_offsets) <= frame_idx) {
        md_print(MD_LOG_TYPE_ERROR, "Frame index is out of range");
        return 0;
    }

    const int64_t beg = pdb->frame_offsets[frame_idx];
    const int64_t end = frame_idx + 1 < (int64_t)md_array_size(pdb->frame_offsets) ? pdb->frame_offsets[frame_idx + 1] : (int64_t)pdb->filesize;
    const uint64_t frame_size = end - beg;

    if (frame_data_ptr) {
        md_file_seek(pdb->file, beg, MD_FILE_BEG);
        const uint64_t bytes_read = md_file_read(pdb->file, frame_data_ptr, frame_size);
        ASSERT(frame_size == bytes_read);
    }

    return frame_size;
}

bool pdb_decode_frame(struct md_trajectory_o* inst, const void* frame_data_ptr, int64_t frame_data_size, md_trajectory_field_flags_t requested_fields, md_trajectory_data_t* write_target) {
    ASSERT(inst);
    ASSERT(frame_data_ptr);
    ASSERT(frame_data_size);
    ASSERT(write_target);

    pdb_trajectory_t* pdb = (pdb_trajectory_t*)inst;
    if (pdb->magic != MD_PDB_TRAJ_MAGIC) {
        md_print(MD_LOG_TYPE_ERROR, "Error when decoding frame data, pdb magic did not match");
        return false;
    }

    if (requested_fields & MD_TRAJ_FIELD_BOX) {
        ASSERT(write_target->box);
        memcpy(write_target->box, pdb->box, sizeof(md_trajectory_box_t));
    }

    if (requested_fields & MD_TRAJ_FIELD_XYZ) {
        struct md_pdb_data data = {0};
        md_pdb_data_parse_str((str_t){frame_data_ptr, frame_data_size}, &data, default_allocator);

        if (write_target->num_atoms != data.num_atom_coordinates) {
            md_printf(MD_LOG_TYPE_ERROR, "number of atoms (%lli) in parsed pdb frame did not match supplied value (%lli)", data.num_atom_coordinates, write_target->num_atoms);
            md_pdb_data_free(&data, default_allocator);
            return false;
        }

        ASSERT(write_target->x);
        ASSERT(write_target->y);
        ASSERT(write_target->z);
        for (int64_t i = 0; i < write_target->num_atoms; ++i) {
            write_target->x[i] = data.atom_coordinates[i].x;
            write_target->y[i] = data.atom_coordinates[i].y;
            write_target->z[i] = data.atom_coordinates[i].z;
        }
        md_pdb_data_free(&data, default_allocator);

        write_target->written_fields |= MD_TRAJ_FIELD_XYZ;
    }

    return true;
}

// PUBLIC PROCEDURES

bool md_pdb_data_parse_str(str_t str, struct md_pdb_data* data, struct md_allocator* alloc) {
    str_t line;
    const char* base_offset = str.ptr;
    while (extract_line(&line, &str)) {
        if (line.len < 6) continue;
        if (compare_n(line, "ATOM", 4) || compare_n(line, "HETATM", 6)) {
            if (md_array_size(data->atom_coordinates) > 5815) {
                while(0);
            }
            struct md_pdb_coordinate coord = extract_coord(line);
            md_array_push(data->atom_coordinates, coord, alloc);
        }
        else if (compare_n(line, "HELIX", 5)) {
            struct md_pdb_helix helix = extract_helix(line);
            md_array_push(data->helices, helix, alloc);
        }
        else if (compare_n(line, "SHEET", 5)) {
            struct md_pdb_sheet sheet = extract_sheet(line);
            md_array_push(data->sheets, sheet, alloc);
        }
        else if (compare_n(line, "CONECT", 6)) {
            struct md_pdb_connect connect = extract_connect(line);
            struct md_pdb_connect* last = md_array_last(data->connections);
            if (last && last->atom_serial[0] == connect.atom_serial[0]) {
                append_connect(last, &connect);
            }
            else {
                md_array_push(data->connections, connect, alloc);
            }
        }
        else if (compare_n(line, "MODEL", 5)) {
            const int32_t num_coords = (int32_t)md_array_size(data->atom_coordinates);
            struct md_pdb_model model = {
                .serial = (int32_t)parse_int(substr(line, 10, 4)),
                .beg_atom_serial = num_coords > 0 ? data->atom_coordinates[num_coords-1].atom_serial : 1,
                .beg_atom_index = num_coords,
                .byte_offset = line.ptr - base_offset,
            };
            md_array_push(data->models, model, alloc);
        }
        else if (compare_n(line, "ENDMDL", 6)) {
            const int32_t num_coords = (int32_t)md_array_size(data->atom_coordinates);
            struct md_pdb_model* model = md_array_last(data->models);
            ASSERT(model);
            model->end_atom_serial = num_coords > 0 ? data->atom_coordinates[num_coords-1].atom_serial : 1;
            model->end_atom_index = num_coords;
        }
        else if (compare_n(line, "CRYST1", 6)) {
            struct md_pdb_cryst1 cryst1 = extract_cryst1(line);
            md_array_push(data->cryst1, cryst1, alloc);
        }
    }

    data->num_models = md_array_size(data->models);
    data->num_atom_coordinates = md_array_size(data->atom_coordinates);
    data->num_helices = md_array_size(data->helices);
    data->num_sheets = md_array_size(data->sheets);
    data->num_connections = md_array_size(data->connections);
    data->num_cryst1 = md_array_size(data->cryst1);

    return data;
}

bool md_pdb_data_parse_file(str_t filename, struct md_pdb_data* data, struct md_allocator* alloc) {
    md_file_o* file = md_file_open(filename, MD_FILE_READ | MD_FILE_BINARY);
    if (file) {
        const uint64_t file_size = md_file_size(file);
        const uint64_t buf_size = KILOBYTES(64ULL);
        char* buf_ptr = md_alloc(default_temp_allocator, buf_size);
        uint64_t file_offset = 0;
        uint64_t bytes_read = 0;
        uint64_t read_offset = 0;
        uint64_t read_size = buf_size;
        while ((bytes_read = md_file_read(file, buf_ptr + read_offset, read_size)) == read_size) {
            str_t str = {.ptr = buf_ptr, .len = read_offset + bytes_read};

            bool done = (file_offset + read_offset + bytes_read == file_size);

            if (!done) {
                // We want to make sure we send complete lines for parsing so we locate the last '\n'
                const int64_t last_new_line = rfind_char(str, '\n');
                ASSERT(str.ptr[last_new_line] == '\n');
                if (last_new_line == -1) {
                    md_print(MD_LOG_TYPE_ERROR, "Unexpected structure within pdb file, missing new lines???");
                    md_file_close(file);
                    return false;
                }
                str.len = last_new_line + 1;
            }

            const int64_t pre_num_models = data->num_models;
            if (!md_pdb_data_parse_str(str, data, alloc)) {
                md_file_close(file);
                return false;
            }

            for (int64_t i = pre_num_models; i < data->num_models; ++i) {
                data->models[i].byte_offset += file_offset;
            }

            // Copy remainder to beginning of buffer
            read_offset = buf_size - str.len;
            memcpy(buf_ptr, str.ptr + str.len, read_offset);
            read_size = buf_size - read_offset;
            file_offset += read_size;
        }

        const int64_t pre_num_models = data->num_models;
        const str_t str = {.ptr = buf_ptr, .len = bytes_read + read_offset}; // add read_offset here since we also have some portion which was copied
        if (!md_pdb_data_parse_str(str, data, alloc)) {
            md_file_close(file);
            return false;
        }

        for (int64_t i = pre_num_models; i < data->num_models; ++i) {
            data->models[i].byte_offset += file_offset;
        }

        md_file_close(file);
        return true;
    }
    md_printf(MD_LOG_TYPE_ERROR, "Could not open file '%.*s'", filename.len, filename.ptr);
    return false;
}

void md_pdb_data_free(struct md_pdb_data* data, struct md_allocator* alloc) {
    ASSERT(data);
    if (data->models)               md_array_free(data->models, alloc);
    if (data->helices)              md_array_free(data->helices, alloc);
    if (data->sheets)               md_array_free(data->sheets, alloc);
    if (data->atom_coordinates)     md_array_free(data->atom_coordinates, alloc);
    if (data->connections)          md_array_free(data->connections, alloc);
}

static inline str_t add_label(str_t str, pdb_molecule_t* inst) {
    ASSERT(inst);
    for (int64_t i = 0; i < md_array_size(inst->labels); ++i) {
        str_t label = { .ptr = inst->labels[i].str, .len = strnlen(inst->labels[i].str, ARRAY_SIZE(inst->labels[i].str)) };
        if (compare_str(str, label)) {
            return label;
        }
    }
    label_t label = {0};
    memcpy(label.str, str.ptr, MIN(str.len, ARRAY_SIZE(label.str) - 1));
    return (str_t){md_array_push(inst->labels, label, inst->allocator)->str, str.len};
}

bool md_pdb_molecule_init(md_molecule* mol, const struct md_pdb_data* data, struct md_allocator* alloc) {
    ASSERT(mol);
    ASSERT(data);
    ASSERT(alloc);

    int64_t beg_atom_index = 0;
    int64_t end_atom_index = data->num_atom_coordinates;

    // if we have models and they have the same size, interperet it as a trajectory and only load the first model
    if (data->num_models > 0) {
        bool is_trajectory = true;
        const int64_t ref_size = data->models[0].end_atom_index - data->models[0].beg_atom_index;
        for (int64_t i = 1; i < data->num_models; ++i) {
            if ((data->models[i].end_atom_index - data->models[i].beg_atom_index) != ref_size) {
                is_trajectory = false;
                break;
            }
        }

        if (is_trajectory) {
            // Limit the scope of atom coordinate entries if we have a trajectory (only consider first model)
            beg_atom_index = data->models[0].beg_atom_index;
            end_atom_index = data->models[0].end_atom_index;
        }
    }

    if (mol->inst) {
        md_print(MD_LOG_TYPE_DEBUG, "molecule inst object is not zero, potentially leaking memory when clearing");
    }

    memset(mol, 0, sizeof(md_molecule));

    mol->inst = md_alloc(alloc, sizeof(pdb_molecule_t));
    pdb_molecule_t* inst = (pdb_molecule_t*)mol->inst;
    memset(inst, 0, sizeof(pdb_molecule_t));
    
    inst->magic = MD_PDB_MOL_MAGIC;
    inst->allocator = md_arena_allocator_create(alloc, KILOBYTES(16));
    inst->labels = 0;

    const int64_t num_atoms = end_atom_index - beg_atom_index;

    // Change allocator to arena so we can very easily free it later.
    alloc = inst->allocator;

    md_array_ensure(mol->atom.x, num_atoms, alloc);
    md_array_ensure(mol->atom.y, num_atoms, alloc);
    md_array_ensure(mol->atom.z, num_atoms, alloc);
    md_array_ensure(mol->atom.element, num_atoms, alloc);
    md_array_ensure(mol->atom.name, num_atoms, alloc);
    md_array_ensure(mol->atom.radius, num_atoms, alloc);
    md_array_ensure(mol->atom.mass, num_atoms, alloc);
    md_array_ensure(mol->atom.flags, num_atoms, alloc);

    int32_t cur_res_id = -1;
    char cur_chain_id = -1;

    for (int64_t i = beg_atom_index; i < end_atom_index; ++i) {
        float x = data->atom_coordinates[i].x;
        float y = data->atom_coordinates[i].y;
        float z = data->atom_coordinates[i].z;
        str_t atom_name = (str_t){data->atom_coordinates[i].atom_name, strnlen(data->atom_coordinates[i].atom_name, ARRAY_SIZE(data->atom_coordinates[i].atom_name))};
        atom_name = add_label(atom_name, inst);

        str_t res_name = (str_t){data->atom_coordinates[i].res_name, strnlen(data->atom_coordinates[i].res_name, ARRAY_SIZE(data->atom_coordinates[i].res_name))};

        md_element element = 0;
        if (data->atom_coordinates[i].element[0]) {
            // If the element is available, we directly try to use that to lookup the element
            str_t element_str = { data->atom_coordinates[i].element, strnlen(data->atom_coordinates[i].element, ARRAY_SIZE(data->atom_coordinates[i].element)) };
            element = md_util_lookup_element(element_str);
            if (element == 0) {
                md_printf(MD_LOG_TYPE_INFO, "Failed to lookup element with string '%s'", data->atom_coordinates[i].element);
            }
        }
        else {
            element = md_util_decode_element(atom_name, res_name);
            if (element == 0) {
                md_printf(MD_LOG_TYPE_INFO, "Failed to decode element with atom name '%s' and residue name '%s'", data->atom_coordinates[i].atom_name, data->atom_coordinates[i].res_name);
            }
        }
        float radius = md_util_element_vdw_radius(element);
        float mass = md_util_element_atomic_mass(element);

        char chain_id = data->atom_coordinates[i].chain_id;
        if (chain_id != cur_chain_id) {
            cur_chain_id = chain_id;
            
            str_t chain_label = { &data->atom_coordinates[i].chain_id, 1 };
            chain_label = add_label(chain_label, inst);
            md_range residue_range = {(uint32_t)mol->residue.count, (uint32_t)mol->residue.count};
            md_range atom_range = {(uint32_t)mol->atom.count, (uint32_t)mol->atom.count};
            
            mol->chain.count += 1;
            md_array_push(mol->chain.id, chain_label.ptr, alloc);
            md_array_push(mol->chain.residue_range, residue_range, alloc);
            md_array_push(mol->chain.atom_range, atom_range, alloc);

            if (mol->chain.residue_range) md_array_last(mol->chain.residue_range)->end += 1;
        }

        int32_t res_id = data->atom_coordinates[i].res_seq;
        if (res_id != cur_res_id) {
            cur_res_id = res_id;

            res_name = add_label(res_name, inst);
            md_residue_id id = res_id;
            md_range atom_range = {(uint32_t)mol->atom.count, (uint32_t)mol->atom.count};

            mol->residue.count += 1;
            md_array_push(mol->residue.name, res_name.ptr, alloc);
            md_array_push(mol->residue.id, id, alloc);
            md_array_push(mol->residue.atom_range, atom_range, alloc);
        }

        if (mol->residue.atom_range) md_array_last(mol->residue.atom_range)->end += 1;
        if (mol->chain.atom_range) md_array_last(mol->chain.atom_range)->end += 1;

        mol->atom.count += 1;
        md_array_push(mol->atom.x, x, alloc);
        md_array_push(mol->atom.y, y, alloc);
        md_array_push(mol->atom.z, z, alloc);
        md_array_push(mol->atom.element, element, alloc);
        md_array_push(mol->atom.name, atom_name.ptr, alloc);
        md_array_push(mol->atom.radius, radius, alloc);
        md_array_push(mol->atom.mass, mass, alloc);
        md_array_push(mol->atom.flags, 0, alloc);
        
        if (mol->residue.count) md_array_push(mol->atom.residue_idx, (md_residue_idx)(mol->residue.count - 1), alloc);
        if (mol->chain.count)   md_array_push(mol->atom.chain_idx, (md_chain_idx)(mol->chain.count - 1), alloc);
    }

    {
        // Compute backbone data
        int64_t bb_offset = 0;
        for (int64_t i = 0; i < mol->chain.count; ++i) {
            const int64_t res_count = mol->chain.residue_range[i].end - mol->chain.residue_range[i].beg;
            md_range bb_range = {bb_offset, bb_offset + res_count};
            md_array_push(mol->chain.backbone_range, bb_range, alloc);
            bb_offset += res_count;
        }

        mol->backbone.count = bb_offset;
        md_array_resize(mol->backbone.atoms, mol->backbone.count, alloc);
        md_array_resize(mol->backbone.angle, mol->backbone.count, alloc);
        md_array_resize(mol->backbone.secondary_structure, mol->backbone.count, alloc);
        md_array_resize(mol->backbone.residue_idx, mol->backbone.count, alloc);

        int64_t backbone_idx = 0;
        for (int64_t i = 0; i < mol->chain.count; ++i) {
            const int64_t res_count = mol->chain.residue_range[i].end - mol->chain.residue_range[i].beg;
            const int64_t res_offset = mol->chain.residue_range[i].beg;

            for (int64_t j = 0; j < res_count; ++j) {
                mol->backbone.residue_idx[backbone_idx + j] = res_offset + j;
            }

            md_util_backbone_args_t bb_args = {
                .atom = {
                    .count = mol->atom.count,
                    .name = mol->atom.name,
            },
            .residue = {
                    .count = res_count,
                    .atom_range = mol->residue.atom_range + res_offset,
            },
            };
            md_util_extract_backbone_atoms(mol->backbone.atoms + backbone_idx, mol->backbone.count, &bb_args);

            backbone_idx += res_count;
        }

        md_util_secondary_structure_args_t ss_args = {
            .atom = {
                .count = mol->atom.count,
                .x = mol->atom.x,
                .y = mol->atom.y,
                .z = mol->atom.z,
        },
        .backbone = {
                .count = mol->backbone.count,
                .atoms = mol->backbone.atoms,
        },
        .chain = {
                .count = mol->chain.count,
                .backbone_range = mol->chain.backbone_range,
        }
        };
        md_util_compute_secondary_structure(mol->backbone.secondary_structure, mol->backbone.count, &ss_args);
    }

    {
#if 0
        // Compute/Extract covalent bonds
        if (data->num_connections > 0) {
            // Convert given connections
            ASSERT(data->connections);
            for (int64_t i = 0; i < data->num_connections; ++i) {
                data->connections[i];
            }
        }
        else {
        }
#endif
        md_array_ensure(mol->residue.internal_covalent_bond_range, mol->residue.count, alloc);
        md_array_ensure(mol->residue.complete_covalent_bond_range, mol->residue.count, alloc);

        // Use heuristical method of finding covalent bonds
        md_util_covalent_bond_args_t args = {
            .atom = {
                .count = mol->atom.count,
                .x = mol->atom.x,
                .y = mol->atom.y,
                .z = mol->atom.z,
                .element = mol->atom.element
            },
            .residue = {
                .count = mol->residue.count,
                .atom_range = mol->residue.atom_range,
                .internal_bond_range = mol->residue.internal_covalent_bond_range,
                .complete_bond_range = mol->residue.complete_covalent_bond_range,
            },
        };
        mol->covalent_bond.bond = md_util_extract_covalent_bonds(&args, alloc);
        mol->covalent_bond.count = md_array_size(mol->covalent_bond.bond);
    }
    return true;
}

bool md_pdb_molecule_free(struct md_molecule* mol, struct md_allocator* alloc) {
    ASSERT(mol);
    ASSERT(mol->inst);
    ASSERT(alloc);

    pdb_molecule_t* inst = (pdb_molecule_t*)mol->inst;
    if (inst->magic != MD_PDB_MOL_MAGIC) {
        md_print(MD_LOG_TYPE_ERROR, "PDB magic did not match!");
        return false;
    }

    md_arena_allocator_destroy(inst->allocator);
    md_free(alloc, inst, sizeof(pdb_molecule_t));
    memset(mol, 0, sizeof(md_molecule));

    return true;
}

bool md_pdb_trajectory_open(struct md_trajectory* traj, str_t filename, struct md_allocator* alloc) {
    int64_t* frame_offsets = 0;
    int64_t filesize = 0;
    float box[3][3] = {0};
    int64_t num_atoms = 0;

    // @TODO: try to read cache-file if it exists.

    // Otherwise we fall back to this
    {
        md_file_o* file = md_file_open(filename, MD_FILE_READ | MD_FILE_BINARY);
        if (!file) {
            md_print(MD_LOG_TYPE_ERROR, "Failed to open file for PDB trajectory");
            return false;
        }
        filesize = md_file_size(file);
        md_file_close(file);

        struct md_pdb_data data = {0};
        if (!md_pdb_data_parse_file(filename, &data, default_allocator)) {
            return false;
        }

        if (data.num_models == 0) {
            md_print(MD_LOG_TYPE_ERROR, "The PDB file did not contain any model entries and cannot be read as a trajectory");
            md_pdb_data_free(&data, default_allocator);
            return false;
        }

        {
            // Validate the models
            const int64_t ref_length = data.models[0].end_atom_index - data.models[0].beg_atom_index;
            for (int64_t i = 1; i < data.num_models; ++i) {
                const int64_t length = data.models[i].end_atom_index - data.models[i].beg_atom_index;
                if (length != ref_length) {
                    md_print(MD_LOG_TYPE_ERROR, "The PDB file models are not of equal length and cannot be read as a trajectory");
                    md_pdb_data_free(&data, default_allocator);
                    return false;
                }
            }
            num_atoms = ref_length;
        }

        for (int64_t i = 0; i < data.num_models; ++i) {
            md_array_push(frame_offsets, data.models[i].byte_offset, alloc);
        }

        if (data.num_cryst1 > 0) {
            if (data.num_cryst1 > 1) {
                md_print(MD_LOG_TYPE_INFO, "The PDB file contains multiple CRYST1 entries, will pick the first one for determining the simulation box");
            }
            if (data.cryst1[0].alpha != 90.0f || data.cryst1[0].beta != 90.0f || data.cryst1[0].gamma != 90.0f) {
                md_print(MD_LOG_TYPE_INFO, "The PDB file CRYST1 entry has one or more non-orthogonal axes, these are ignored");
            }
            box[0][0] = data.cryst1[0].a;
            box[1][1] = data.cryst1[0].b;
            box[2][2] = data.cryst1[0].c;
        }

        // @TODO: Write data to cache
    }

    pdb_trajectory_t* pdb = md_alloc(alloc, sizeof(pdb_trajectory_t));
    ASSERT(pdb);
    memset(pdb, 0, sizeof(pdb_trajectory_t));

    pdb->magic = MD_PDB_TRAJ_MAGIC;
    pdb->file = md_file_open(filename, MD_FILE_READ | MD_FILE_BINARY);
    pdb->filesize = filesize;
    pdb->frame_offsets = frame_offsets;
    memcpy(pdb->box, box, sizeof(box));
    pdb->allocator = alloc;

    traj->inst = (struct md_trajectory_o*)pdb;
    traj->num_atoms = num_atoms;
    traj->num_frames = md_array_size(pdb->frame_offsets);
    traj->extract_frame = pdb_extract_frame;
    traj->decode_frame = pdb_decode_frame;

    return true;
}

bool md_pdb_trajectory_close(struct md_trajectory* traj) {
    ASSERT(traj);
    ASSERT(traj->inst);
    pdb_trajectory_t* pdb = (pdb_trajectory_t*)traj->inst;
    if (pdb->magic != MD_PDB_TRAJ_MAGIC) {
        md_printf(MD_LOG_TYPE_ERROR, "Trajectory is not a valid PDB trajectory.");
        return false;
    }

    if (pdb->file) md_file_close(pdb->file);
    if (pdb->frame_offsets) md_array_free(pdb->frame_offsets, pdb->allocator);
    md_free(pdb->allocator, traj->inst, sizeof(pdb_trajectory_t));

    memset(traj, 0, sizeof(*traj));

    return true;
}

#ifdef __cplusplus
}
#endif
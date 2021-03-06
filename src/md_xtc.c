#include "md_xtc.h"

#include <core/md_common.h>
#include <core/md_array.inl>
#include <core/md_file.h>
#include <core/md_allocator.h>
#include <core/md_log.h>
#include <md_trajectory.h>
#include <ext/xtc/xdrfile.h>

#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MD_XTC_TRAJ_MAGIC 0x162365dac721995

#define XTC_MAGIC 1995

/* XTC small header size (natoms<=9).
*  > int(4) magic
*  > int(4) natoms
*  > int(4) step
*  > float(4) time
*  > 9xfloat(4) box
*  > int(4) natoms (again)
*/
#define XTC_SMALL_HEADER_SIZE 56

/* Size of uncompressed coordinates for one atom.
* 3xfloat(4) x
*/
#define XTC_SMALL_COORDS_SIZE 12

/* XTC header size (natoms>=10).
* Compressed trajectories contain some additional values:
*  > float(4) preccission
*  > 3xint(4) minint
*  > 3xint(4) maxint
*  > int(4) smallidx
* See `xdrfile_compress_coord_double()`.
*/
#define XTC_HEADER_SIZE (XTC_SMALL_HEADER_SIZE + 32)

typedef struct xtc_t {
    uint64_t magic;
    XDRFILE* file;
    int64_t filesize;
    int64_t* frame_offsets;
    md_allocator_i* allocator;
} xtc_t;

typedef struct xtc_header_t {
    bool success;
    int num_atoms;
    int64_t* frame_offsets;
} xtc_header_t;

static bool xtc_header(XDRFILE* xd, int* natoms, int* step, float* time) {
    int result, magic;

    if ((result = xdrfile_read_int(&magic, 1, xd)) != 1) {
        md_print(MD_LOG_TYPE_ERROR, "Failed to read magic number");
        return false;
    }
    if (magic != XTC_MAGIC) {
        md_print(MD_LOG_TYPE_ERROR, "Magic number did not match");
        return false;
    }
    if ((result = xdrfile_read_int(natoms, 1, xd)) != 1) {
        md_print(MD_LOG_TYPE_ERROR, "Failed to read number of atoms");
        return false;
    }
    if ((result = xdrfile_read_int(step, 1, xd)) != 1) {
        md_print(MD_LOG_TYPE_ERROR, "Failed to read step");
        return false;
    }
    if ((result = xdrfile_read_int((int*)time, 1, xd)) != 1) {
        md_print(MD_LOG_TYPE_ERROR, "Failed to read timestamp");
        return false;
    }

    return true;
}

static bool xtc_coord(XDRFILE* xd, int natoms, matrix box, rvec* pos) {
    int result;
    float prec;

    /* box */
    result = xdrfile_read_float(box[0], DIM * DIM, xd);
    if (DIM * DIM != result) {
        md_print(MD_LOG_TYPE_ERROR, "Failed to read box dimensions");
        return false;
    } else {
        result = xdrfile_decompress_coord_float(pos[0], &natoms, &prec, xd);
        if (result != natoms) {
            md_print(MD_LOG_TYPE_ERROR, "Failed to read coordinates");
            return false;
        }
    }
    return true;
}

static bool xtc_offsets(XDRFILE* xd, int64_t** offsets, md_allocator_i* alloc) {
    int i, result, est_nframes, framebytes, step, natoms, nframes;
    float time;

    int64_t* arr = 0;

    /* Go to file beg */
    if (xdr_seek(xd, 0L, SEEK_SET) != exdrOK) {
        return false;
    }

    if (!xtc_header(xd, &natoms, &step, &time)) {
        return false;
    }

    /* Go to file end */
    if (xdr_seek(xd, 0L, SEEK_END) != exdrOK) {
        return false;
    }
    /* Cursor position is equivalent to file size */
    int64_t filesize = xdr_tell(xd);

    /* Dont bother with compression for nine atoms or less */
    if (natoms <= 9) {
        xdrfile_close(xd);
        framebytes = XTC_SMALL_HEADER_SIZE + XTC_SMALL_COORDS_SIZE * natoms;
        nframes = (int)(filesize / framebytes); /* Should we complain if framesize doesn't divide filesize? */
        for (i = 0; i < nframes; i++) {
            md_array_push(arr, i * framebytes, alloc);
        }
    } else {
        /* Go back to the beginning of the file */
        if (xdr_seek(xd, (int64_t)XTC_HEADER_SIZE, SEEK_SET) != exdrOK) {
            return false;
        }

        if (xdrfile_read_int(&framebytes, 1, xd) == 0) {
            md_print(MD_LOG_TYPE_ERROR, "Failed to read framebytes");
            return false;
        }
        framebytes = (framebytes + 3) & ~0x03; /* Rounding to the next 32-bit boundary */
        est_nframes = (int)(filesize / ((int64_t)(framebytes + XTC_HEADER_SIZE)) +
            1); /* must be at least 1 for successful growth */
                /* First `framebytes` might be larger than average, so we would underestimate `est_nframes`
                */
        est_nframes += est_nframes / 5;
        md_array_ensure(arr, est_nframes, alloc);

        while (1) {
            /* Skip `framebytes` and next header */
            result = xdr_seek(xd, (int64_t)(framebytes + XTC_HEADER_SIZE), SEEK_CUR);
            if (result != exdrOK) {
                md_print(MD_LOG_TYPE_ERROR, "Failed to skip framebytes");
                md_array_free(arr, alloc);
                return false;
            }

            /* Store position in `offsets`, adjust for header */
            md_array_push(arr, xdr_tell(xd) - (int64_t)(XTC_HEADER_SIZE), alloc);

            /* Read how much to skip next time */
            if (xdrfile_read_int(&framebytes, 1, xd) == 0) {
                *offsets = arr;
                return true;
            }
            framebytes = (framebytes + 3) & ~0x03; /* Rounding to the next 32-bit boundary */
        }
    }

    return false;
}

static bool xtc_decode(XDRFILE* xd, md_trajectory_field_flags_t fields, md_trajectory_data_t* write_target) {
    ASSERT(write_target);

    write_target->written_fields = 0;

    if (fields & MD_TRAJ_FIELD_XYZ | fields & MD_TRAJ_FIELD_BOX | fields & MD_TRAJ_FIELD_TIMESTAMP) {
        int natoms, step;
        float time;

        if (!xtc_header(xd, &natoms, &step, &time)) {
            return false;
        }
        if (fields & MD_TRAJ_FIELD_XYZ | fields & MD_TRAJ_FIELD_BOX) {

            uint64_t size = natoms * sizeof(rvec);
            rvec* pos = md_alloc(default_temp_allocator, size);
            float box[3][3] = {0};
            if (xtc_coord(xd, natoms, box, pos)) {
                if (fields & MD_TRAJ_FIELD_XYZ) {
                    ASSERT(write_target->num_atoms >= natoms);
                    ASSERT(write_target->x);
                    ASSERT(write_target->y);
                    ASSERT(write_target->z);
                    for (int64_t i = 0; i < natoms; ++i) {
                        write_target->x[i] = pos[i][0] * 10.0f;
                        write_target->y[i] = pos[i][1] * 10.0f;
                        write_target->z[i] = pos[i][2] * 10.0f;
                    }
                    write_target->written_fields |= MD_TRAJ_FIELD_XYZ;
                }
                if (fields & MD_TRAJ_FIELD_BOX) {
                    ASSERT(write_target->box);

                    box[0][0] *= 10.0f;
                    box[0][1] *= 10.0f;
                    box[0][2] *= 10.0f;
                    
                    box[1][0] *= 10.0f;
                    box[1][1] *= 10.0f;
                    box[1][2] *= 10.0f;

                    box[2][0] *= 10.0f;
                    box[2][1] *= 10.0f;
                    box[2][2] *= 10.0f;

                    memcpy(write_target->box, box, sizeof(box));
                    write_target->written_fields |= MD_TRAJ_FIELD_BOX;
                }
            }
            md_free(default_temp_allocator, pos, size);
        }
        if (fields & MD_TRAJ_FIELD_TIMESTAMP) {
            ASSERT(write_target->timestamp);
            *(write_target->timestamp) = time;
            write_target->written_fields |= MD_TRAJ_FIELD_TIMESTAMP;
        }
    }

    return true;
}

// This is lowlevel cruft for enabling parallel loading and decoding of frames
static int64_t xtc_extract_frame(struct md_trajectory_o* inst, int64_t frame_idx, void* frame_data_ptr) {
    xtc_t* xtc = (xtc_t*)inst;
    ASSERT(xtc);
    ASSERT(xtc->magic == MD_XTC_TRAJ_MAGIC);

    const int64_t beg = xtc->frame_offsets[frame_idx];
    const int64_t end = frame_idx + 1 < (int64_t)md_array_size(xtc->frame_offsets) ? xtc->frame_offsets[frame_idx + 1] : (int64_t)xtc->filesize;
    const int64_t frame_size = end - beg;

    if (!xtc->filesize) {
        md_print(MD_LOG_TYPE_ERROR, "File size is zero");
        return 0;
    }

    if (!xtc->file) {
        md_print(MD_LOG_TYPE_ERROR, "File handle is NULL");
        return 0;
    }

    if (!xtc->frame_offsets) {
        md_print(MD_LOG_TYPE_ERROR, "Frame offsets is empty");
        return 0;
    }

    if (frame_idx < 0 || (int64_t)md_array_size(xtc->frame_offsets) <= frame_idx) {
        md_print(MD_LOG_TYPE_ERROR, "Frame index is out of range");
        return 0;
    }

    if (frame_data_ptr) {
        ASSERT(xtc->file);
        xdr_seek(xtc->file, beg, SEEK_SET);
        const int64_t bytes_read = xdr_read(xtc->file, frame_data_ptr, frame_size);
        ASSERT(frame_size == bytes_read);
    }
    return frame_size;
}

static bool xtc_decode_frame(struct md_trajectory_o* inst, const void* frame_data_ptr, int64_t frame_data_size, md_trajectory_field_flags_t requested_fields, md_trajectory_data_t* write_target) {
    ASSERT(inst);
    ASSERT(frame_data_ptr);
    ASSERT(frame_data_size);
    ASSERT(write_target);

    xtc_t* xtc = (xtc_t*)inst;
    if (xtc->magic != MD_XTC_TRAJ_MAGIC) {
        md_print(MD_LOG_TYPE_ERROR, "Error when decoding frame data, xtc magic did not match");
        return false;
    }
    
    // There is a warning for ignoring const qualifier for frame_data_ptr, but it is ok since we only perform read operations "r" with the data.
    XDRFILE* file = xdrfile_mem((void*)frame_data_ptr, frame_data_size, "r");
    ASSERT(file);

    bool result = xtc_decode(file, requested_fields, write_target);
    xdrfile_close(file);

    return result;
}

bool md_xtc_trajectory_open(struct md_trajectory* traj, str_t filename, struct md_allocator* alloc) {
    ASSERT(traj);
    ASSERT(alloc);

    char z_filename[512] = {0};
    ASSERT(filename.len < ARRAY_SIZE(z_filename));
    memcpy(z_filename, filename.ptr, filename.len);

    XDRFILE* file = xdrfile_open(z_filename, "r");

    if (traj->inst) {
        md_print(MD_LOG_TYPE_DEBUG, "Trajectory instance data was zero, potentially leeking memory here");
    }

    if (file) {
        int num_atoms, step;
        float time;
        if (!xtc_header(file, &num_atoms, &step, &time)) {
            xdrfile_close(file);
            return false;
        }

        int64_t* offsets = 0;
        if (!xtc_offsets(file, &offsets, alloc)) {
            xdrfile_close(file);
            return false;
        }

        xdr_seek(file, 0, SEEK_END);
        const int64_t filesize = xdr_tell(file);
        xdr_seek(file, 0, SEEK_SET);
        
        ASSERT(offsets);
        ASSERT(num_atoms > 0);

        xtc_t* xtc = md_alloc(alloc, sizeof(xtc_t));
        ASSERT(xtc);
        memset(xtc, 0, sizeof(xtc_t));

        xtc->magic = MD_XTC_TRAJ_MAGIC;
        xtc->allocator = alloc;
        xtc->file = file;
        xtc->filesize = filesize;
        xtc->frame_offsets = offsets;

        traj->inst = (struct md_trajectory_o*)xtc;
        traj->num_atoms = num_atoms;
        traj->num_frames = md_array_size(offsets);
        traj->extract_frame = xtc_extract_frame;
        traj->decode_frame = xtc_decode_frame;
        return true;
    }
    
    return false;
}

bool md_xtc_trajectory_close(struct md_trajectory* traj) {
    ASSERT(traj);
    ASSERT(traj->inst);
    xtc_t* xtc = (xtc_t*)traj->inst;
    if (xtc->magic != MD_XTC_TRAJ_MAGIC) {
        md_printf(MD_LOG_TYPE_ERROR, "Trajectory is not a valid XTC trajectory.");
        return false;
    }

    if (xtc->file) xdrfile_close(xtc->file);
    if (xtc->frame_offsets) md_array_free(xtc->frame_offsets, xtc->allocator);
    md_free(xtc->allocator, xtc, sizeof(xtc_t));

    memset(traj, 0, sizeof(struct md_trajectory));

    return true;
}

#ifdef __cplusplus
}
#endif
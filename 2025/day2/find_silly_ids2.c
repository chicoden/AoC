#include <stdio.h>
#include <stdint.h>

void write_u64(uint64_t value, FILE* stream) {
    char digits[32];
    char* stack_pointer = digits;
    do {
        *(stack_pointer++) = value % 10 + '0';
        value /= 10;
    } while (value != 0);

    while (stack_pointer != digits) {
        fputc(*(--stack_pointer), stream);
    }
}

int read_u64(FILE* stream, uint64_t* value) {
    *value = 0;
    int found_digits = 0;
    while (1) {
        int ch = fgetc(stream);
        if (ch < '0' || ch > '9') {
            ungetc(ch, stream);
            return found_digits;
        }

        found_digits = 1;
        *value = (*value) * 10 + (ch - '0');
    }
}

int count_digits(uint64_t value) {
    int count = 0;
    do {
        count++;
        value /= 10;
    } while (value != 0);
    return count;
}

int has_repeating_subsequence(uint64_t id) {
    int id_length = count_digits(id);
    int max_subseq_len = id_length / 2;

    uint64_t subseq_mask = 10;
    for (int subseq_len = 1; subseq_len <= max_subseq_len; subseq_len++) {
        uint64_t subseq = id % subseq_mask;
        //

        uint64_t remaining_seq = id / subseq_mask;
        int is_repeating = 1;
        while (remaining_seq != 0) {
            if (remaining_seq % subseq_mask != subseq) {
                is_repeating = 0;
                break;
            }

            remaining_seq /= subseq_mask;
        }

        if (is_repeating) {
            return 1;
        }

        subseq_mask *= 10;
    }

    return 0;
}

int main() {
    FILE* file = fopen("input.txt", "rb");
    if (file == NULL) {
        puts("failed to open input file");
        return -1;
    }

    uint64_t total = 0;
    int expecting_range = 1;
    while (expecting_range) {
        uint64_t start, end;
        if (
            !read_u64(file, &start) ||
            fgetc(file) != '-' ||
            !read_u64(file, &end)
        ) {
            puts("failed to read range");
            break;
        }

        int delimiter = fgetc(file);
        expecting_range = delimiter == ',';
        if (!expecting_range && delimiter != '\n' && delimiter != '\r') {
            puts("expected newline");
            break;
        }

        for (uint64_t id = start; id <= end; id++) {
            if (has_repeating_subsequence(id)) {
                printf("%llu\n", id);///
                total += id;
            }
        }
    }

    fputs("Sum of invalid IDs: ", stdout);
    write_u64(total, stdout);
    fputc('\n', stdout);

    fclose(file);
    return 0;
}
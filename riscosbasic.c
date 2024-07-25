#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "keywords.c"

const char *name = "";

typedef enum {
    None,
    Decode,
    Encode
} Mode;

uint8_t line_buf[256];
uint8_t line_buf_decoded[256];

void print_usage() {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s <--encode | --decode> <FILE>\n", name);
    fprintf(stderr, "%s <-e | -d> <FILE>\n", name);
}

void decode_line() {
    uint8_t head = 0;
    uint8_t head_decoded = 0;

    while (line_buf[head] != '\0') {
        size_t kw_len;
        size_t kwenc_len;
        const char *kw = kw_decode(line_buf + head, &kw_len, &kwenc_len);

        if (kw != NULL) {
            sprintf(line_buf_decoded + head_decoded, "%s ", kw);
            head += kwenc_len;
            head_decoded += kw_len + 1;
            continue;
        }

        /*
        if (!strncmp(line_buf + head, "\xF1\x20", 2)) {
            sprintf(line_buf_decoded + head_decoded, "PRINT ");
            head += 2;
            head_decoded += 6;
            continue;
        }
        */

        line_buf_decoded[head_decoded++] = line_buf[head++];
    }

    line_buf_decoded[head_decoded] = '\0';
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    name = argv[0];

    Mode mode = None;

    if (!strcmp(argv[1], "--decode") || !strcmp(argv[1], "-d")) {
        mode = Decode;
    } else if (!strcmp(argv[1], "--encode") || !strcmp(argv[1], "-e")) {
        mode = Encode;
    }

    if (mode == None) {
        print_usage();
        return 1;
    }

    FILE *f = fopen(argv[2], "r");

    if (!f) {
        fprintf(stderr, "Unable to open file %s\n", argv[1]);
        return 2;
    }

    while (!feof(f)) {
        uint8_t c = fgetc(f);

        if (c != '\r') {
            fprintf(stderr, "Line began with %X (expected %X)\n", c, '\r');
            return 3;
        }

        if (feof(f)) {
            break;
        }

        uint8_t line_high = fgetc(f);

        if (feof(f)) {
            break;
        }

        uint8_t line_low = fgetc(f);

        if (feof(f)) {
            break;
        }

        uint16_t line_number = (line_high << 8) + line_low;

        uint8_t line_len = fgetc(f) - 3;

        fgets(line_buf, line_len, f);
        line_buf[line_len] = '\0';

        decode_line();

        printf("%d %s\n", line_number, line_buf_decoded);
    }

    return 0;
}

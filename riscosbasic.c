#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "keywords.c"

#define MAX_LINE_LEN 255
#define MAX_LINE_NUMBER_LEN 15

const char *name = "";

typedef enum {
    None,
    Decode,
    DecodeNoNumber,
    Encode
} Mode;

Mode mode = None;
uint8_t line_buf[MAX_LINE_LEN + 1];
uint8_t line_buf_decoded[MAX_LINE_LEN + 1];
uint8_t line_buf_encoded[MAX_LINE_LEN + 5];

typedef enum {
    ERR_NONE,
    ERR_USAGE,
    ERR_FOPEN,
    ERR_FORMAT,
    ERR_MISSINGNUMBER,
    ERR_LONGLINE
} Error;

void print_usage() {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s <--encode | --decode | --decode-nonumber> <FILE>\n", name);
    fprintf(stderr, "%s <-e | -d | -D> <FILE>\n", name);
}

void decode_line() {
    uint8_t head = 0;
    uint8_t head_decoded = 0;

    while (line_buf[head] != '\0') {
        size_t kw_len;
        size_t kwenc_len;
        const char *kw = kw_decode(line_buf + head, &kw_len, &kwenc_len);

        if (kw != NULL) {
            sprintf(line_buf_decoded + head_decoded, "%s", kw);
            head += kwenc_len;
            head_decoded += kw_len;
            continue;
        }

        line_buf_decoded[head_decoded++] = line_buf[head++];
    }

    line_buf_decoded[head_decoded] = '\0';
}

int decode(const char* filename) {
    FILE *f = fopen(filename, "r");

    if (!f) {
        fprintf(stderr, "Unable to open file %s\n", filename);
        return ERR_FOPEN;
    }

    uint32_t line_number_expected = 10;

    while (!feof(f)) {
        uint8_t c = fgetc(f);

        if (c != '\r') {
            fprintf(stderr, "Line began with %X (expected %X)\n", c, '\r');
            return ERR_FORMAT;
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

        if (mode == DecodeNoNumber && line_number != line_number_expected) {
            fprintf(stderr, "--decode-nonumber only works if lines are sequential 10s\n");
            return(4);
        }

        line_number_expected += 10;

        uint8_t line_len = fgetc(f) - 3;

        fgets(line_buf, line_len, f);
        line_buf[line_len] = '\0';

        decode_line();

        if (!(mode == DecodeNoNumber)) {
            printf("%d ", line_number);
        }

        printf("%s\n", line_buf_decoded);
    }

    fclose(f);

    return 0;
}

size_t encode_line(uint32_t line_num) {
    line_buf_encoded[0] = '\r';
    line_buf_encoded[1] = (uint8_t)(line_num >> 8);
    line_buf_encoded[2] = (uint8_t)(line_num & 0xFF);
    size_t len = 0;
    uint8_t kw[16];
    for (size_t i = 0; line_buf[i] != '\0';) {
        size_t kwlen = 0;
        size_t off = 0;
        uint8_t kw_c = line_buf[i+off];
        while (kw_c >= 'A' && kw_c <= 'Z') {
            kw[kwlen++] = kw_c;
            ++off;
            kw_c = line_buf[i+off];
        }

        kw[kwlen] = '\0';

        if (kwlen > 0) {
            const char* enc = kw_encode(kw);
            if (enc != NULL) {
                size_t enclen = strlen(enc);
                size_t chars = snprintf(&line_buf_encoded[4+len], enclen+1, enc);
                len += chars;
                i += kwlen;
            } else {
                size_t chars = snprintf(&line_buf_encoded[4+len], kwlen+1, kw);
                len += chars;
                i += chars;
            }
            continue;
        }

        line_buf_encoded[4+len++] = line_buf[i++];

        if (4+len >= MAX_LINE_LEN) {
            fprintf(stderr, "Line too long: %d\n", line_num);
        }
    }

    size_t encoded_line_len = 4 + len;
    line_buf_encoded[3] = encoded_line_len;
    return encoded_line_len;
}

int encode(const char* filename) {
    FILE *f = fopen(filename, "r");

    if (!f) {
        fprintf(stderr, "Unable to open file %s\n", filename);
        return ERR_FOPEN;
    }

    char line_number[MAX_LINE_NUMBER_LEN + 1];
    bool autonum = false;
    uint32_t line_number_in = 0;
    size_t line_head;

    while (!feof(f)) {
        line_head = 0;
        ++line_number_in;

        uint8_t line_number_head = 0;

        uint8_t c = fgetc(f);

        if (feof(f)) {
            break;
        }

        while (c == '\r' || c == '\n') {
            c = fgetc(f);
        }

        if (!autonum) {
            while (c >= '0' && c <= '9') {
                line_number[line_number_head++] = c;
                if (line_number_head >= MAX_LINE_NUMBER_LEN) {
                    line_number[line_number_head] = '\0';
                    fprintf(stderr, "Line number too long (%s...)\n", line_number);
                }
                c = fgetc(f);
            }
        }

        line_number[line_number_head] = '\0';

        if (!autonum && line_number_head == 0) {
            if (line_number_in == 1) {
                autonum = true;
            } else {
                if (feof(f)) {
                    break;
                }
                fprintf(stderr, "No number at line %d\n", line_number_in);
                return ERR_MISSINGNUMBER;
            }
        }

        if (autonum) {
            snprintf(line_number, MAX_LINE_NUMBER_LEN, "%d", line_number_in * 10);
        }

        // Don't consider the first whitespace after the line number, if present
        if (!autonum && (c == ' ' || c == '\t')) {
            c = fgetc(f);
        }

        while (c != '\r' && c != '\n' && !feof(f)) {
            line_buf[line_head++] = c;
            if (line_head > MAX_LINE_LEN) {
                fprintf(stderr, "Line too long at %d", line_number_in);
                return ERR_LONGLINE;
            }
            c = fgetc(f);
        }

        line_buf[line_head] = '\0';

        uint32_t line_number_int = atoi(line_number);
        size_t len = encode_line(line_number_int);
        for (size_t i = 0; i < len; ++i) {
            fputc(line_buf_encoded[i], stdout);
        }
    }

    fputc('\x0D', stdout);
    fputc('\xFF', stdout);
    fclose(f);

    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return ERR_USAGE;
    }

    name = argv[0];

    if (!strcmp(argv[1], "--decode") || !strcmp(argv[1], "-d")) {
        mode = Decode;
    } else if (!strcmp(argv[1], "--decode-nonumber") || !strcmp(argv[1], "-D")) {
        mode = DecodeNoNumber;
    } else if (!strcmp(argv[1], "--encode") || !strcmp(argv[1], "-e")) {
        mode = Encode;
    }

    switch (mode) {
        case Decode:
        case DecodeNoNumber:
            return decode(argv[2]);
            break;
        case Encode:
            return encode(argv[2]);
            break;
        case None:
            print_usage();
            return ERR_USAGE;
    }

    fflush(stdout);
    return 0;
}

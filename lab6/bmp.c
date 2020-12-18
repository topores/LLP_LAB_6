#include <stdint.h>
#include <stdio.h>
#include <mm_malloc.h>
#include <math.h>
#include "bmp.h"


void bmp_header_print(struct bmp_header header) {
    printf("bfType: %#x\n", header.bfType);
    printf("bfileSize: %u\n", header.bfileSize);
    printf("bfReserved: %u\n", header.bfReserved);
    printf("bOffBits: %u\n", header.bOffBits);
    printf("biSize: %u\n", header.biSize);
    printf("biWidth: %u\n", header.biWidth);
    printf("biHeight: %u\n", header.biHeight);
    printf("biPlanes: %u\n", header.biPlanes);
    printf("biBitCount: %u\n", header.biBitCount);
    printf("biCompression: %u\n", header.biCompression);
    printf("biSizeImage: %u\n", header.biSizeImage);
    printf("biXPelsPerMeter: %u\n", header.biXPelsPerMeter);
    printf("biYPelsPerMeter: %u\n", header.biYPelsPerMeter);
    printf("biClrUsed: %u\n", header.biClrUsed);
    printf("biClrImportant: %u\n", header.biClrImportant);
}


struct image create_image(uint32_t width, uint32_t height) {
    struct image img;
    img.width = width;
    img.height = height;
    img.pixels = malloc(img.height * img.width * sizeof(struct pixel));
    return img;
}


struct bmp_header create_header(struct image *img) {
    struct bmp_header header;
    uint64_t remainder;

    header.bfType = HEADER_TYPE;
    header.bfReserved = HEADER_RESERVED;
    header.bOffBits =  HEADER_OFFBITS;
    header.biSize = HEADER_SIZE;
    header.biPlanes = HEADER_PLANES;
    header.biBitCount = HEADER_BITCOUNT;
    header.biCompression = HEADER_COMPRESSION;
    header.biXPelsPerMeter = HEADER_XMETER;
    header.biYPelsPerMeter = HEADER_YMETER;
    header.biClrUsed = HEADER_CLRUSED;
    header.biClrImportant = HEADER_CLRIMPORTANT;

    header.biHeight = img->height;
    header.biWidth = img->width;

    remainder = (img->width * 3l) % 4l;
    remainder = (remainder == 0) ? 0 : (4l - remainder);
    header.biSizeImage = (img->width * 3l + remainder) * img->height;
    header.bfileSize = header.biSizeImage + header.bOffBits;

    return header;
}


enum read_status from_bmp(FILE *in_bmp, struct image *in_image) {

    struct bmp_header header;

    if (!fread(&header, sizeof(header), 1, in_bmp)) return READ_IO_ERROR;

    bmp_header_print(header);
    uint8_t spare[4];
    int64_t remainder, row;

    *in_image = create_image(header.biWidth, header.biHeight);


    /* how many bytes to skip to get on the next row */
    remainder = (in_image->width * 3l) % 4l;
    remainder = (remainder == 0) ? 0 : (4l - remainder);

    for (row = in_image->height - 1; row >= 0; row--) {

        uint64_t row_bits = fread(&in_image->pixels[row * in_image->width], sizeof(struct pixel), in_image->width,
                                  in_bmp);
        uint64_t rem_bits = fread(spare, sizeof(uint8_t), remainder, in_bmp);

        if (!row_bits || (remainder && !rem_bits)) {
            free(in_image->pixels);
            return READ_IO_ERROR;
        }

    }


    return READ_OK;
}


enum write_status to_bmp(FILE *out_bmp, struct image *out_image) {
    int64_t remainder, row;
    const uint8_t spare[4] = {0};
    struct bmp_header header;

    header = create_header(out_image);
    if (!fwrite(&header, sizeof(header), 1, out_bmp)) return WRITE_IO_ERROR;

    /* how many bytes to skip to get on the next row */
    remainder = (out_image->width * 3l) % 4l;
    remainder = (remainder == 0) ? 0 : (4l - remainder);

    for (row = out_image->height - 1; row >= 0; row--) {
        uint64_t row_bits = fwrite(&out_image->pixels[row * out_image->width], sizeof(struct pixel), out_image->width,
                                   out_bmp);
        uint64_t rem_bits = fwrite(spare, sizeof(uint8_t), remainder, out_bmp);
        if (!row_bits || (remainder && !rem_bits)) return WRITE_IO_ERROR;
    }

    return WRITE_OK;
}

struct image rotate90(struct image const source) {
    struct image new_image = create_image(source.height, source.width);
    for (uint32_t x = 0; x < new_image.width; x++)
        for (uint32_t y = 0; y < new_image.height; y++)
            new_image.pixels[y * new_image.width + x] = source.pixels[y + new_image.height*(new_image.width - x -1)];
    return new_image;
}

void print_error_read(enum read_status status) {
    if (status == READ_OK) return;
    char *err_msg;

    switch (status) {

        case READ_INVALID_SIGNATURE:
            err_msg = "Invalid signature";
            break;
        case READ_INVALID_BITS:
            err_msg = "Invalid bit count";
            break;
        case READ_INVALID_HEADER:
            err_msg = "Invalid header";
            break;
        case READ_IO_ERROR:
            err_msg = "Read IO error";
            break;
        default:
            err_msg = "Read error";
            break;
    }

    fprintf(stderr, "%s\n", err_msg);
}


void print_error_write(enum write_status status) {
    if (status == WRITE_OK) return;
    char *err_msg;

    switch (status) {
        case WRITE_IO_ERROR:
            err_msg = "Write IO error";
            break;
        default:
            err_msg = "Write error";
            break;
    }

    fprintf(stderr, "%s\n", err_msg);
}


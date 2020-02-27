#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>

uint32_t
hex2int(const char *hex)
{
    uint32_t res = 0;
    if (*hex++ != '0') return 0;
    if (*hex++ != 'x') return 0;
    do {
        res *= 16;
        if (*hex >= '0' && *hex <= '9') res += *hex - '0';
        if (*hex >= 'a' && *hex <= 'f') res += *hex - 'a' + 10;
        if (*hex >= 'A' && *hex <= 'F') res += *hex - 'A' + 10;
        hex++;
    } while (*hex);
    return res;
}

void
printusage(void)
{
    puts("USAGE: pixmap2ppm [-o <output file>] <pixmap xid>");
}

int
main(int argc, const char *argv[])
{
    xcb_drawable_t drawable = 0;
    xcb_connection_t *connection = xcb_connect(NULL, NULL);

    if (argc < 2) {
        printusage();
        exit(1);
    }

    FILE *img = stdout;

    for (int i=1; i < argc; i++) {
        const char *arg = argv[i];
        if (arg[0] == '-') {
            if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0 || strcmp(arg, "-?") == 0) {
                printusage();
                exit(0);
            } else if (strcmp(arg, "-o") == 0) {
                if (++i == argc) {
                    printusage();
                    exit(1);
                }
                arg = argv[i];
                setoutput:
                img = fopen(arg, "wb");
                if (!img) {
                    perror("pixmap2ppm");
                }
            } else if (arg[0] == '-' && arg[1] == 'o') {
                arg += 2;
                goto setoutput;
            } else {
                printf("Unknown option: '%s'\n", arg);
                exit(1);
            }
        } else if (!drawable) {
            drawable = hex2int(arg);
            if (!drawable) {
                printf("'%s' is not a valid pixmap xid\n", arg);
                exit(1);
            }
        }
    }

    if (!drawable) {
        printusage();
        exit(1);
    }

    xcb_generic_error_t *error = NULL;
    xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(connection,
        xcb_get_geometry(connection, drawable), &error);
    if (!geometry) {
        if (error && error->error_code == XCB_DRAWABLE) {
            printf("'0x%08x' is not a pixmap or drawable", drawable);
            exit(1);
        }
        printf("Failed to get geometry for pixmap '0x%08x'", drawable);
        exit(1);
    }
    int width = geometry->width;
    int height = geometry->height;
    xcb_get_image_reply_t *image = xcb_get_image_reply(connection,
        xcb_get_image(connection, XCB_IMAGE_FORMAT_Z_PIXMAP, drawable, 0, 0, width, height, 0xffffff), NULL);
    if (!image) {
        printf("Failed to get image data for pixmap '0x%08x'", drawable);
        exit(1);
    }

    uint8_t *data = xcb_get_image_data(image);
    int len = xcb_get_image_data_length(image);
    fprintf(img, "P6\n");
    fprintf(img, "%d %d\n", width, height);
    fprintf(img, "255\n");
    for (int idx = 0; idx < len; idx += 4) {
        // Assume TrueColor little-endian ARGB
        fprintf(img, "%c%c%c", data[idx + 2], data[idx + 1], data[idx + 0]);
    }
    free(image);
    free(geometry);

    xcb_disconnect(connection);

    if (img != stdout) fclose(img);

    return 0;
}

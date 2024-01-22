#include <stdio.h>
#include <string.h>
#include <pcap.h>
#include <setjmp.h>
#include <signal.h>
#include <malloc.h>
#include <unistd.h>

#define MAC_MICROSOFT 0x98, 0x5F, 0xD3, 0xFE, 0x35, 0x0F

static unsigned char ep[] = {
        0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x80, 0x00, 0x00, 0x00,
        
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        MAC_MICROSOFT,
        MAC_MICROSOFT,
        
        0x40, 0xdf,
        
        0x03, 0xc5, 0x16, 0xdf, 0x67, 0x00, 0x00, 0x00,
        0x64, 0x00,
        0x21, 0x14,
        
        0x00
};

static unsigned char ed[] = {
        0x01,
        0x04, 0x82, 0x84, 0x8b, 0x96, 0x03, 0x01, 0x01, 0x05, 0x04, 0x00, 0x02, 0x00, 0x00, 0x07, 0x06,
        0x4b, 0x52, 0x04, 0x01, 0x0d, 0x17, 0x2a, 0x01, 0x00, 0x32, 0x08, 0x0c, 0x12, 0x18, 0x24, 0x30,
        0x48, 0x60, 0x6c, 0x3b, 0x06, 0x51, 0x53, 0x54, 0x7d, 0x80, 0x81, 0x2d, 0x1a, 0xad, 0x09, 0x17,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x3d, 0x16, 0x01, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f,
        0x08, 0x04, 0x00, 0x00, 0x82, 0x00, 0x00, 0x00, 0x40, 0xbf, 0x0c, 0x92, 0x79, 0x81, 0x33, 0xfa,
        0xff, 0x0c, 0x03, 0xfa, 0xff, 0x0c, 0x03, 0xc0, 0x05, 0x00, 0x00, 0x00, 0xfa, 0xff, 0xc3, 0x02,
        0x00, 0x17, 0xff, 0x1a, 0x23, 0x01, 0x01, 0x00, 0x82, 0x40, 0x00, 0x00, 0x33, 0x4c, 0x89, 0x0d,
        0x01, 0x80, 0x08, 0x00, 0x0c, 0x00, 0xfa, 0xff, 0xfa, 0xff, 0x19, 0x1c, 0xc7, 0x71, 0xff, 0x07,
        0x24, 0xf4, 0x3f, 0x00, 0x0f, 0xfc, 0xff, 0xdd, 0x18, 0x00, 0x50, 0xf2, 0x02, 0x01, 0x01, 0x81,
        0x00, 0x03, 0xa4, 0x00, 0x00, 0x27, 0xa4, 0x00, 0x00, 0x42, 0x43, 0x5e, 0x00, 0x62, 0x32, 0x2f,
        0x00, 0xdd, 0x0b, 0x8c, 0xfd, 0xf0, 0x01, 0x01, 0x02, 0x01, 0x00, 0x02, 0x01, 0x01, 0x6b, 0x01,
        0x02, 0x6c, 0x02, 0x7f, 0x00, 0xdd, 0x05, 0x00, 0x16, 0x32, 0x80, 0x00, 0xdd, 0x08, 0x00, 0x50,
        0xf2, 0x11, 0x02, 0x00, 0x00, 0x00, /* 0xd6, 0xdd,   0x91, 0xbe, */
};


static size_t ssid(char* dst, const char* src)
{
    char len = (char)strnlen(src, 32);
    dst[0] = len;
    memcpy(dst + 1, src, len);
    return len + 1;
}

static size_t beacon(char* dst, const char* name)
{
    size_t ofs = 0;
    
    memcpy(dst, ep, sizeof ep);
    ofs += sizeof ep;
    
    dst[21] ^= name[0];
    dst[22] ^= name[1];
    dst[23] ^= name[2];
    
    dst[27] ^= name[0];
    dst[28] ^= name[1];
    dst[29] ^= name[2];
    
    ofs += ssid(dst + ofs, name);
    
    memcpy(dst + ofs, ed, sizeof ed);
    ofs += sizeof ed;
    
    return ofs;
}


static jmp_buf ehf;

void sigh(int __attribute__((unused)) _)
{
    longjmp(ehf, 1);
}


int main(int argc, char* argv[])
{
    static char err[PCAP_ERRBUF_SIZE];
    
    size_t* size;
    char  * buf;
    pcap_t* dev;
    
    int r,i;
    
    if (argc < 3)
    {
        fprintf(stderr, "invalid argument count: %d\n", argc);
        return 1;
    }
    
    dev = pcap_open_live(argv[1], PCAP_BUF_SIZE, 1, 1, err);
    if (!dev)
    {
        fprintf(stderr, "pcap_open_live(): %s\n", err);
        return 1;
    }
    
    signal(SIGABRT, sigh);
    signal(SIGKILL, sigh);
    signal(SIGINT, sigh);
    
    buf  = malloc(BUFSIZ * (argc - 2));
    size = malloc(sizeof *size * (argc - 2));
    for (i = 0; i < argc - 2; ++i)
        size[i] = beacon(buf + i * BUFSIZ, argv[i + 2]);
    
    if (setjmp(ehf))
        goto EXIT;
    
    for (i = 0; pcap_inject(dev, buf + i * BUFSIZ, size[i]) >= 0; i = (i + 1) % (argc - 2))
    {
    }
    fprintf(stderr, "pcap_inject(): %s\n", pcap_geterr(dev));

EXIT:
    free(buf);
    free(size);
    pcap_close(dev);
    return 0;
}

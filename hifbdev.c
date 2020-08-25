/**
 * @file hifbdev.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "hifbdev.h"

#if USE_FBDEV || USE_BSD_FBDEV

#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#if USE_BSD_FBDEV
#include <sys/fcntl.h>
#include <sys/time.h>
#include <sys/consio.h>
#include <sys/fbio.h>
#else  /* USE_BSD_FBDEV */
#include <linux/fb.h>
#endif /* USE_BSD_FBDEV */

/*********************
 *      DEFINES
 *********************/
#ifndef FBDEV_PATH
#define FBDEV_PATH  "/dev/fb0"
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      STRUCTURES
 **********************/

struct bsd_fb_var_info{
    uint32_t xoffset;
    uint32_t yoffset;
    uint32_t xres;
    uint32_t yres;
    int bits_per_pixel;
 };

struct bsd_fb_fix_info{
    long int line_length;
    long int smem_len;
};

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
#if USE_BSD_FBDEV
static struct bsd_fb_var_info vinfo;
static struct bsd_fb_fix_info finfo;
#else
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
#endif /* USE_BSD_FBDEV */
static char *fbp = 0;
static long int screensize = 0;
static int fbfd = 0;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

#if 0
static struct fb_bitfield s_r16 = {10, 5, 0};
static struct fb_bitfield s_g16 = {5, 5, 0};
static struct fb_bitfield s_b16 = {0, 5, 0};
static struct fb_bitfield s_a16 = {15, 1, 0};
#else
static struct fb_bitfield s_r32 = {16, 8, 0};
static struct fb_bitfield s_g32 = {8, 8, 0};
static struct fb_bitfield s_b32 = {0, 8, 0};
static struct fb_bitfield s_a32 = {24, 8, 0};
#endif

typedef enum {
    HI_FALSE = 0,
    HI_TRUE  = 1,
} HI_BOOL;

typedef unsigned int            HI_U32;

typedef enum
{        
    HIFB_FMT_RGB565 = 0,        
    HIFB_FMT_RGB888,		      /**<  RGB888 24bpp */
	
    HIFB_FMT_KRGB444,       /**<  RGB444 16bpp */
    HIFB_FMT_KRGB555,       /**<  RGB555 16bpp */
    HIFB_FMT_KRGB888,       /**<  RGB888 32bpp */
    
    HIFB_FMT_ARGB4444,      /**< ARGB4444 */     
    HIFB_FMT_ARGB1555,      /**< ARGB1555 */  
    HIFB_FMT_ARGB8888,      /**< ARGB8888 */   
    HIFB_FMT_ARGB8565,      /**< ARGB8565 */
	
    HIFB_FMT_RGBA4444,      /**< ARGB4444 */
    HIFB_FMT_RGBA5551,      /**< RGBA5551 */
    HIFB_FMT_RGBA5658,      /**< RGBA5658 */
    HIFB_FMT_RGBA8888,      /**< RGBA8888 */
    
    HIFB_FMT_BGR565,        /**< BGR565 */   
    HIFB_FMT_BGR888,        /**< BGR888 */   
    HIFB_FMT_ABGR4444,      /**< ABGR4444 */   
    HIFB_FMT_ABGR1555,      /**< ABGR1555 */   
    HIFB_FMT_ABGR8888,      /**< ABGR8888 */   
    HIFB_FMT_ABGR8565,      /**< ABGR8565 */      
    HIFB_FMT_KBGR444,       /**< BGR444 16bpp */
    HIFB_FMT_KBGR555,       /**< BGR555 16bpp */
    HIFB_FMT_KBGR888,       /**< BGR888 32bpp */
    
    HIFB_FMT_1BPP,          /**<  clut1 */
    HIFB_FMT_2BPP,          /**<  clut2 */    
    HIFB_FMT_4BPP,          /**<  clut4 */ 
    HIFB_FMT_8BPP,          /**< clut8 */
    HIFB_FMT_ACLUT44,       /**< AClUT44*/
    HIFB_FMT_ACLUT88,         /**< ACLUT88 */
    HIFB_FMT_PUYVY,         /**< UYVY */
    HIFB_FMT_PYUYV,         /**< YUYV */
    HIFB_FMT_PYVYU,         /**< YVYU */
    HIFB_FMT_YUV888,        /**< YUV888 */
    HIFB_FMT_AYUV8888,      /**< AYUV8888 */
    HIFB_FMT_YUVA8888,      /**< YUVA8888 */
    HIFB_FMT_BUTT
}HIFB_COLOR_FMT_E;

typedef struct
{
    HI_BOOL bKeyRgb;
    HI_BOOL bKeyAlpha;      /**<  whether support colorkey alpha */
    HI_BOOL bGlobalAlpha;   /**<  whether support global alpha */
    HI_BOOL bCmap;          /**<  whether support color map */
    HI_BOOL bHasCmapReg;    /**<  whether has color map register*/
    HI_BOOL bColFmt[HIFB_FMT_BUTT]; /**<  support which color format */
    HI_BOOL bVoScale;       /**< support vo scale*/
    HI_BOOL bLayerSupported;    /**< whether support a certain layer, for example:x5 HD support HIFB_SD_0 not support HIFB_SD_1*/
    HI_U32  u32MaxWidth;    /**<  the max pixels per line */
    HI_U32  u32MaxHeight;   /**<  the max lines */
    HI_U32  u32MinWidth;    /**<  the min pixels per line */
    HI_U32  u32MinHeight;   /**<  the min lines */ 
    HI_U32  u32VDefLevel;   /**<  vertical deflicker level, 0 means vertical deflicker is unsupported */
    HI_U32  u32HDefLevel;   /**<  horizontal deflicker level, 0 means horizontal deflicker is unsupported  */
    HI_BOOL  bDcmp;
    HI_BOOL  bPreMul;
}HIFB_CAPABILITY_S;

#define IOC_TYPE_HIFB       'F'
/** To set the display state of an overlay layer */
#define FBIOPUT_SHOW_HIFB           _IOW(IOC_TYPE_HIFB, 101, int)
/** to obtain the capability of an overlay layer */
#define FBIOGET_CAPABILITY_HIFB    _IOR(IOC_TYPE_HIFB, 103, HIFB_CAPABILITY_S)

static void get_color_format()
{
    HIFB_CAPABILITY_S cap;
    int r, i;
    
    r = ioctl (fbfd,
        FBIOGET_CAPABILITY_HIFB,
        &cap);
    if(r < 0)
    {
        perror("Error: FBIOGET_CAPABILITY_HIFB");
        return;
    }

    for(i = 0; i < HIFB_FMT_BUTT; i++)
    {
        if(cap.bColFmt[i])
        {
            printf("capability->ColFmt[%d] is supported \n", i);
        }
    }
}

int hifbdev_init(void)
{
    // Open the file for reading and writing
    fbfd = open(FBDEV_PATH, O_RDWR);
    if(fbfd == -1) {
        fprintf(stderr, "open:%s ", FBDEV_PATH);
        perror("Error: cannot open framebuffer device");
        return -1;
    }
    printf("The framebuffer device was opened successfully.\n");

#if USE_BSD_FBDEV
    struct fbtype fb;
    unsigned line_length;

    //Get fb type
    if (ioctl(fbfd, FBIOGTYPE, &fb) != 0) {
        perror("ioctl(FBIOGTYPE)");
        return -1;
    }

    //Get screen width
    if (ioctl(fbfd, FBIO_GETLINEWIDTH, &line_length) != 0) {
        perror("ioctl(FBIO_GETLINEWIDTH)");
        return -1;
    }

    vinfo.xres = (unsigned) fb.fb_width;
    vinfo.yres = (unsigned) fb.fb_height;
    vinfo.bits_per_pixel = fb.fb_depth;
    vinfo.xoffset = 0;
    vinfo.yoffset = 0;
    finfo.line_length = line_length;
    finfo.smem_len = finfo.line_length * vinfo.yres;
#else /* USE_BSD_FBDEV */

#if 1
    {
        get_color_format();
        
        // Get variable screen information
        if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
            perror("Error reading variable information");
            return -1;
        }
        // Set variable screen information
#if LV_COLOR_DEPTH == 32
        vinfo.transp= s_a32;
        vinfo.red = s_r32;
        vinfo.green = s_g32;
        vinfo.blue = s_b32;
#else
        vinfo.transp= s_a16;
        vinfo.red = s_r16;
        vinfo.green = s_g16;
        vinfo.blue = s_b16;
#endif
        vinfo.bits_per_pixel = LV_COLOR_DEPTH;
        vinfo.activate = FB_ACTIVATE_NOW;
        
        vinfo.xres_virtual = LV_HOR_RES_MAX;
        vinfo.yres_virtual = LV_VER_RES_MAX*2;
        vinfo.xres = LV_HOR_RES_MAX;
        vinfo.yres = LV_VER_RES_MAX;
        
        if(ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo) == -1) {
            perror("Error reading variable information");
            return -1;
        }
    }
#endif
    
    // Get fixed screen information
    if(ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        return -1;
    }

#endif /* USE_BSD_FBDEV */

    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    // Figure out the size of the screen in bytes
    screensize =  finfo.smem_len; //finfo.line_length * vinfo.yres;    

    // Map the device to memory
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if((intptr_t)fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        return -1;
    }
    memset(fbp, 0, screensize);

    printf("The framebuffer device was mapped to memory successfully.\n");

#if 0
    uint16_t *ptemp = (uint16_t*)(fbp);
    int x,y;
    for (y = 0; y < LV_VER_RES_MAX/2; y++)
    {
        for (x = 0; x < LV_HOR_RES_MAX/2; x++)
        {
            *(ptemp + y * vinfo.xres + x) = 0xFC00;
        }
    }
    printf("paint red triangle.\n");
    sleep(3);
#endif
    return 0;
}

void hifbdev_exit(void)
{
    close(fbfd);
}

/**
 * Flush a buffer to the marked area
 * @param drv pointer to driver where this function belongs
 * @param area an area where to copy `color_p`
 * @param color_p an array of pixel to copy to the `area` part of the screen
 */
void hifbdev_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
    if(fbp == NULL ||
            area->x2 < 0 ||
            area->y2 < 0 ||
            area->x1 > (int32_t)vinfo.xres - 1 ||
            area->y1 > (int32_t)vinfo.yres - 1) {
        lv_disp_flush_ready(drv);
        return;
    }

    /*Truncate the area to the screen*/
    int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
    int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
    int32_t act_x2 = area->x2 > (int32_t)vinfo.xres - 1 ? (int32_t)vinfo.xres - 1 : area->x2;
    int32_t act_y2 = area->y2 > (int32_t)vinfo.yres - 1 ? (int32_t)vinfo.yres - 1 : area->y2;


    lv_coord_t w = (act_x2 - act_x1 + 1);
    long int location = 0;
    long int byte_location = 0;
    unsigned char bit_location = 0;

    /*32 or 24 bit per pixel*/
    if(vinfo.bits_per_pixel == 32 || vinfo.bits_per_pixel == 24) {
        uint32_t * fbp32 = (uint32_t *)fbp;
        int32_t y;
        for(y = act_y1; y <= act_y2; y++) {
            location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length / 4;
            memcpy(&fbp32[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1) * 4);
            color_p += w;
        }
    }
    /*16 bit per pixel*/
    else if(vinfo.bits_per_pixel == 16) {
        uint16_t * fbp16 = (uint16_t *)fbp;
        int32_t y;
        for(y = act_y1; y <= act_y2; y++) {
            location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length / 2;
            memcpy(&fbp16[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1) * 2);
            color_p += w;
        }
    }
    /*8 bit per pixel*/
    else if(vinfo.bits_per_pixel == 8) {
        uint8_t * fbp8 = (uint8_t *)fbp;
        int32_t y;
        for(y = act_y1; y <= act_y2; y++) {
            location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length;
            memcpy(&fbp8[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1));
            color_p += w;
        }
    }
    /*1 bit per pixel*/
    else if(vinfo.bits_per_pixel == 1) {
        uint8_t * fbp8 = (uint8_t *)fbp;
        int32_t x;
        int32_t y;
        for(y = act_y1; y <= act_y2; y++) {
            for(x = act_x1; x <= act_x2; x++) {
                location = (x + vinfo.xoffset) + (y + vinfo.yoffset) * vinfo.xres;
                byte_location = location / 8; /* find the byte we need to change */
                bit_location = location % 8; /* inside the byte found, find the bit we need to change */
                fbp8[byte_location] &= ~(((uint8_t)(1)) << bit_location);
                fbp8[byte_location] |= ((uint8_t)(color_p->full)) << bit_location;
                color_p++;
            }

            color_p += area->x2 - act_x2;
        }
    } else {
        /*Not supported bit per pixel*/
    }

    //May be some direct update command is required
    //ret = ioctl(state->fd, FBIO_UPDATE, (unsigned long)((uintptr_t)rect));

    lv_disp_flush_ready(drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif


/*******************************************************************
 *
 * main.c - LVGL simulator for GNU/Linux
 *
 * Based on the original file from the repository
 *
 * @note eventually this file won't contain a main function and will
 * become a library supporting all major operating systems
 *
 * To see how each driver is initialized check the
 * 'src/lib/display_backends' directory
 *
 * - Clean up
 * - Support for multiple backends at once
 *   2025 EDGEMTech Ltd.
 *
 * Author: EDGEMTech Ltd, Erik Tagirov (erik.tagirov@edgemtech.ch)
 *
 ******************************************************************/
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lvgl/src/core/lv_global.h"
#include "lvgl/src/core/lv_group.h"
#include "lvgl/src/misc/lv_types.h"
#include "lvgl/src/indev/lv_indev_private.h"
#include "lvgl/src/misc/lv_event_private.h"
#include "lvgl/src/misc/lv_event.h"

#include "src/lib/driver_backends.h"
#include "src/lib/simulator_util.h"
#include "src/lib/simulator_settings.h"
#include "src/lib/rtc_util.h"


/* Display dimensions */
#define DISPLAY_WIDTH  400
#define DISPLAY_HEIGHT 300

/* Box dimensions */
#define TOP_BAR_WIDTH  400
#define TOP_BAR_HEIGHT 60

#define BODY_WIDTH  250
#define BODY_HEIGHT 240

#define IMAGE_BOX_WIDTH  150
#define IMAGE_BOX_HEIGHT 150

#define IMAGE_BOX2_WIDTH  150
#define IMAGE_BOX2_HEIGHT 90

#define L2_IMAGE_BOX_WIDTH  150
#define L2_IMAGE_BOX_HEIGHT 120

#define QR_BOX_WIDTH  150
#define QR_BOX_HEIGHT 120


#define BADGE_LAYOUT1 1
#define BADGE_LAYOUT2 2

/* Internal functions */
static void configure_simulator(int argc, char **argv);
static void print_lvgl_version(void);
static void print_usage(void);

/* contains the name of the selected backend if user
 * has specified one on the command line */
static char *selected_backend;

/* Global simulator settings, defined in lv_linux_backend.c */
extern simulator_settings_t settings;

static char *user_name;
static char *user_designation;
static char *user_email;
static char *user_qrlink;


/**
 * @brief Print LVGL version
 */
static void print_lvgl_version(void)
{
    fprintf(stdout, "%d.%d.%d-%s\n",
            LVGL_VERSION_MAJOR,
            LVGL_VERSION_MINOR,
            LVGL_VERSION_PATCH,
            LVGL_VERSION_INFO);
}

/**
 * @brief Print usage information
 */
static void print_usage(void)
{
    fprintf(stdout, "\nlvglsim [-V] [-B] [-b backend_name] [-W window_width] [-H window_height]\n\n");
    fprintf(stdout, "-V print LVGL version\n");
    fprintf(stdout, "-B list supported backends\n");
}

/**
 * @brief Configure simulator
 * @description process arguments recieved by the program to select
 * appropriate options
 * @param argc the count of arguments in argv
 * @param argv The arguments
 */
static void configure_simulator(int argc, char **argv)
{
    int opt = 0;

    selected_backend = NULL;
    user_name = NULL;
    user_designation = NULL;
    user_email = NULL;
    user_qrlink = NULL;

    driver_backends_register();

    const char *env_w = getenv("LV_SIM_WINDOW_WIDTH");
    const char *env_h = getenv("LV_SIM_WINDOW_HEIGHT");
    /* Default values */
    settings.window_width = atoi(env_w ? env_w : "1280");
    settings.window_height = atoi(env_h ? env_h : "1000");

    /* Parse the command-line options. */
    while ((opt = getopt (argc, argv, "b:fmW:H:BVhn:d:e:q:")) != -1) {
        switch (opt) {
        case 'h':
            print_usage();
            exit(EXIT_SUCCESS);
            break;
        case 'V':
            print_lvgl_version();
            exit(EXIT_SUCCESS);
            break;
        case 'B':
            driver_backends_print_supported();
            exit(EXIT_SUCCESS);
            break;
        case 'b':
            if (driver_backends_is_supported(optarg) == 0) {
                die("error no such backend: %s\n", optarg);
            }
            selected_backend = strdup(optarg);
            break;
        case 'W':
            settings.window_width = atoi(optarg);
            break;
        case 'H':
            settings.window_height = atoi(optarg);
            break;
        case 'n':
            user_name = strdup(optarg);
            break;
        case 'd':
            user_designation = strdup(optarg);
            break;
        case 'e':
            user_email = strdup(optarg);
            break;
        case 'q':
            user_qrlink = strdup(optarg);
            break;
        case ':':
            print_usage();
            die("Option -%c requires an argument.\n", optopt);
            break;
        case '?':
            print_usage();
            die("Unknown option -%c.\n", optopt);
        }
    }
}

LV_IMG_DECLARE(txn_logo)
LV_IMG_DECLARE(ces_logo)
LV_IMG_DECLARE(chip_logo)
LV_IMG_DECLARE(user_image)

uint32_t existing_layout = 0;

lv_obj_t *scr;
/* Create containers for the screen */
lv_obj_t *top_bar_cnt;
lv_obj_t *bottom_bar_cnt;
lv_obj_t *qr_cnt;
lv_obj_t *image_cnt;
lv_obj_t *body_cnt;

void layout_badge() 
{  
    /* Get the active screen */
    scr = lv_scr_act();
    
    /* Clear the screen */
    lv_obj_clean(scr);

    /* Set white background for eInk */
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    



    /* Check Inputs */
    if(user_name[0] == '\0' || user_designation[0] == '\0' || user_email[0] == '\0' || user_qrlink[0] == '\0') {
        LV_LOG_USER("User args variables are empty! Using defaults");
        return;
    }

    /* ==================== TOP BAR (400x60) ==================== */
    top_bar_cnt = lv_obj_create(scr);
    lv_obj_set_size(top_bar_cnt, TOP_BAR_WIDTH, TOP_BAR_HEIGHT);
    lv_obj_set_pos(top_bar_cnt, 0, 0);
    lv_obj_set_style_bg_color(top_bar_cnt, lv_color_white(), 0);
    lv_obj_set_style_border_color(top_bar_cnt, lv_color_black(), 0);
    lv_obj_set_style_border_width(top_bar_cnt, 2, 0);
    lv_obj_set_style_pad_all(top_bar_cnt, 5, 0);
    lv_obj_clear_flag(top_bar_cnt, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Top bar image */
    lv_obj_t *top_bar_cnt_img = lv_img_create(top_bar_cnt);
    lv_obj_set_size(top_bar_cnt_img, 373, 48);
    lv_obj_center(top_bar_cnt_img);
    lv_img_set_src(top_bar_cnt_img, &txn_logo);
      
    /* ==================== BODY (250x220) ==================== */
    body_cnt = lv_obj_create(scr);
    lv_obj_set_size(body_cnt, BODY_WIDTH, BODY_HEIGHT);
    lv_obj_set_pos(body_cnt, 0, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_color(body_cnt, lv_color_white(), 0);
    lv_obj_set_style_border_color(body_cnt, lv_color_black(), 0);
    lv_obj_set_style_border_width(body_cnt, 0, 0);
    lv_obj_set_style_pad_all(body_cnt, 10, 0);
    lv_obj_clear_flag(body_cnt, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Body text 1 */
    lv_obj_t *bottom_bar_cnt_text1 = lv_label_create(body_cnt);
    lv_label_set_text(bottom_bar_cnt_text1, user_name);
    lv_obj_set_width(bottom_bar_cnt_text1, BODY_WIDTH - 20);
    lv_label_set_long_mode(bottom_bar_cnt_text1, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(bottom_bar_cnt_text1, lv_color_black(), 0);
    lv_obj_set_style_text_font(bottom_bar_cnt_text1, &lv_font_montserrat_32, 0);
    lv_obj_align(bottom_bar_cnt_text1, LV_ALIGN_TOP_LEFT, 0, 20);
    
    /* Body text 2 */
    lv_obj_t *bottom_bar_cnt_text2 = lv_label_create(body_cnt);
    lv_label_set_text(bottom_bar_cnt_text2, user_designation);
    lv_obj_set_width(bottom_bar_cnt_text2, BODY_WIDTH - 20);
    lv_label_set_long_mode(bottom_bar_cnt_text2, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(bottom_bar_cnt_text2, lv_color_black(), 0);
    lv_obj_set_style_text_font(bottom_bar_cnt_text2, &lv_font_montserrat_24, 0);
    lv_obj_align(bottom_bar_cnt_text2, LV_ALIGN_TOP_LEFT, 0, 110);

    /* Body text 3 */
    lv_obj_t *bottom_bar_cnt_text3 = lv_label_create(body_cnt);
    lv_label_set_text(bottom_bar_cnt_text3, user_email);
    lv_obj_set_width(bottom_bar_cnt_text3, BODY_WIDTH - 20);
    lv_label_set_long_mode(bottom_bar_cnt_text3, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(bottom_bar_cnt_text3, lv_color_black(), 0);
    lv_obj_set_style_text_font(bottom_bar_cnt_text3, &lv_font_montserrat_18, 0);
    lv_obj_align(bottom_bar_cnt_text3, LV_ALIGN_TOP_LEFT, 0, 190);

    /* ==================== IMAGE BOX (150x110) ==================== */
    image_cnt = lv_obj_create(scr);
    lv_obj_set_size(image_cnt, IMAGE_BOX_WIDTH, IMAGE_BOX_HEIGHT);
    lv_obj_set_pos(image_cnt, BODY_WIDTH, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_color(image_cnt, lv_color_white(), 0);
    lv_obj_set_style_border_color(image_cnt, lv_color_black(), 0);
    lv_obj_set_style_border_width(image_cnt, 0, 0);
    lv_obj_clear_flag(image_cnt, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Image placeholder (100x100) - centered */
    lv_obj_t *image = lv_img_create(image_cnt);
    lv_obj_set_size(image, 122, 140);
    lv_obj_center(image);
    lv_img_set_src(image, &user_image);
    //lv_img_set_zoom(image, 50);

    /* ==================== IMAGE BOX (150x110) ==================== */
    qr_cnt = lv_obj_create(scr);
    lv_obj_set_size(qr_cnt, IMAGE_BOX2_WIDTH, IMAGE_BOX2_HEIGHT);
    lv_obj_set_pos(qr_cnt, BODY_WIDTH, TOP_BAR_HEIGHT + IMAGE_BOX_HEIGHT);
    lv_obj_set_style_bg_color(qr_cnt, lv_color_white(), 0);
    lv_obj_set_style_border_color(qr_cnt, lv_color_black(), 0);
    lv_obj_set_style_border_width(qr_cnt, 0, 0);
    lv_obj_clear_flag(qr_cnt, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Image placeholder (100x100) - centered */
    lv_obj_t *image2 = lv_img_create(qr_cnt);
    lv_obj_set_size(image2, 100, 28);
    lv_obj_center(image2);
    lv_img_set_src(image2, &ces_logo);
}


void layout_am62l() 
{   
    /* Get the active screen */
    scr = lv_scr_act();
    
    /* Clear the screen */
    lv_obj_clean(scr);

    /* Set white background for eInk */
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    
    /* Check Inputs */
    if(user_name[0] == '\0' || user_designation[0] == '\0' || user_email[0] == '\0' || user_qrlink[0] == '\0') {
        LV_LOG_USER("User args variables are empty! Using defaults");
        return;
    }

    /* ==================== TOP BAR (400x60) ==================== */
    top_bar_cnt = lv_obj_create(scr);
    lv_obj_set_size(top_bar_cnt, TOP_BAR_WIDTH, TOP_BAR_HEIGHT);
    lv_obj_set_pos(top_bar_cnt, 0, 0);
    lv_obj_set_style_bg_color(top_bar_cnt, lv_color_white(), 0);
    lv_obj_set_style_border_color(top_bar_cnt, lv_color_black(), 0);
    lv_obj_set_style_border_width(top_bar_cnt, 2, 0);
    lv_obj_set_style_pad_all(top_bar_cnt, 5, 0);
    lv_obj_clear_flag(top_bar_cnt, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Top bar image */
    lv_obj_t *top_bar_cnt_img = lv_img_create(top_bar_cnt);
    lv_obj_set_size(top_bar_cnt_img, 373, 48);
    lv_obj_center(top_bar_cnt_img);
    lv_img_set_src(top_bar_cnt_img, &txn_logo);
      
    /* ==================== BODY (250x220) ==================== */
    body_cnt = lv_obj_create(scr);
    lv_obj_set_size(body_cnt, BODY_WIDTH, BODY_HEIGHT);
    lv_obj_set_pos(body_cnt, 0, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_color(body_cnt, lv_color_white(), 0);
    lv_obj_set_style_border_color(body_cnt, lv_color_black(), 0);
    lv_obj_set_style_border_width(body_cnt, 0, 0);
    lv_obj_set_style_pad_all(body_cnt, 10, 0);
    lv_obj_clear_flag(body_cnt, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Body text 1 */
    lv_obj_t *bottom_bar_cnt_text1 = lv_label_create(body_cnt);
    lv_label_set_text(bottom_bar_cnt_text1, "AM62L");
    lv_obj_set_width(bottom_bar_cnt_text1, BODY_WIDTH - 20);
    lv_label_set_long_mode(bottom_bar_cnt_text1, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(bottom_bar_cnt_text1, lv_color_black(), 0);
    lv_obj_set_style_text_font(bottom_bar_cnt_text1, &lv_font_montserrat_32, 0);
    lv_obj_align(bottom_bar_cnt_text1, LV_ALIGN_TOP_MID, 0, 0);
    
    /* Body text 2 */
    lv_obj_t *bottom_bar_cnt_text2 = lv_label_create(body_cnt);
    lv_label_set_text(bottom_bar_cnt_text2, 
        LV_SYMBOL_BULLET " Dual A53 cores @1.25GHz\n"
        LV_SYMBOL_BULLET " SoC Power consumption:\n\t< 1 W: Active mode\n\t< 1 mW: RTC+DDR LP mode\n"
        LV_SYMBOL_BULLET " Full HD display [DPI/DSI]\n"
        LV_SYMBOL_BULLET " 10-bit ADC\n"
        LV_SYMBOL_BULLET " LPDDR4 support\n"
        LV_SYMBOL_BULLET " Secure boot\n"
        LV_SYMBOL_BULLET " Cost Effective"
    );
    lv_obj_set_style_text_line_space(bottom_bar_cnt_text2, 3, 0);
    lv_obj_set_width(bottom_bar_cnt_text2, BODY_WIDTH - 20);
    lv_label_set_long_mode(bottom_bar_cnt_text2, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(bottom_bar_cnt_text2, lv_color_black(), 0);
    lv_obj_set_style_text_font(bottom_bar_cnt_text2, &lv_font_montserrat_16, 0);
    lv_obj_align(bottom_bar_cnt_text2, LV_ALIGN_TOP_LEFT, 0, 40);

    /* ==================== IMAGE BOX (150x110) ==================== */
    image_cnt = lv_obj_create(scr);
    lv_obj_set_size(image_cnt, L2_IMAGE_BOX_WIDTH, L2_IMAGE_BOX_HEIGHT);
    lv_obj_set_pos(image_cnt, BODY_WIDTH, TOP_BAR_HEIGHT);
    lv_obj_set_style_bg_color(image_cnt, lv_color_white(), 0);
    lv_obj_set_style_border_color(image_cnt, lv_color_black(), 0);
    lv_obj_set_style_border_width(image_cnt, 0, 0);
    lv_obj_clear_flag(image_cnt, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Image placeholder (100x100) - centered */
    lv_obj_t *image = lv_img_create(image_cnt);
    lv_obj_set_size(image, 110, 110);
    lv_obj_center(image);
    lv_img_set_src(image, &chip_logo);

    /* ==================== QR BOX (150x110) ==================== */
    qr_cnt = lv_obj_create(scr);
    lv_obj_set_size(qr_cnt, QR_BOX_WIDTH, QR_BOX_HEIGHT);
    lv_obj_set_pos(qr_cnt, BODY_WIDTH, TOP_BAR_HEIGHT + L2_IMAGE_BOX_HEIGHT);
    lv_obj_set_style_bg_color(qr_cnt, lv_color_white(), 0);
    lv_obj_set_style_border_color(qr_cnt, lv_color_black(), 0);
    lv_obj_set_style_border_width(qr_cnt, 0, 0);
    lv_obj_clear_flag(qr_cnt, LV_OBJ_FLAG_SCROLLABLE);
    
    /* QR code placeholder (100x100) - centered */
#if LV_USE_QRCODE
    lv_obj_t *qr_code = lv_qrcode_create(qr_cnt);
    lv_qrcode_set_size(qr_code, 100);
    lv_qrcode_set_dark_color(qr_code, lv_color_black());
    lv_qrcode_set_light_color(qr_code, lv_color_white());
    
    /* Set QR code data */
    const char *qr_data = "https://www.ti.com/tool/AM62L-PROCESSOR-SDK";
    lv_qrcode_update(qr_code, qr_data, strlen(qr_data));
    lv_obj_center(qr_code);
#else
    /* Fallback if QR code not enabled */
    lv_obj_t *qr_code = lv_obj_create(qr_cnt);
    lv_obj_set_size(qr_code, 100, 100);
    lv_obj_set_style_bg_color(qr_code, lv_color_make(200, 200, 200), 0);
    lv_obj_center(qr_code);
    
    lv_obj_t *qr_label = lv_label_create(qr_code);
    lv_label_set_text(qr_label, "QR");
    lv_obj_set_style_text_color(qr_label, lv_color_black(), 0);
    lv_obj_center(qr_label);
#endif
}

void change_layout(void) 
{
    if(BADGE_LAYOUT1 == existing_layout) {
        layout_am62l();
        existing_layout = BADGE_LAYOUT2;
    } else {
        layout_badge();
        existing_layout = BADGE_LAYOUT1;
    }   

}

void indev_keys_cb(lv_event_t *e)
{
    if(!e->param) {
        LV_LOG_USER("No user data found");
        return; 
    }
    int *pressed_key = (int *)e->param;

    LV_LOG_USER("Key = %d", *pressed_key);

    switch(*pressed_key) {
        case LV_KEY_RIGHT:
            change_layout();
            break;
        case LV_KEY_LEFT:
            change_layout();
            break;
        case LV_KEY_UP:
            change_layout();
            break;
        case LV_KEY_DOWN:
            change_layout();
            break;
        case LV_KEY_ENTER:
            rtc_suspend();
            break;
        case LV_KEY_HOME:
        default:
            layout_badge();
            break;
    }
}



void lv_demo_eink(void)
{
    int32_t display_width = lv_display_get_horizontal_resolution(NULL);
    int32_t display_height = lv_display_get_vertical_resolution(NULL);

    /* Default layout */
    layout_badge();
    existing_layout = BADGE_LAYOUT1;
    printf("eInk demo initialized: %dx%d @ %d-bit\n", 
           display_width, display_height, LV_COLOR_DEPTH);
}

/**
 * @brief entry point
 * @description start a demo
 * @param argc the count of arguments in argv
 * @param argv The arguments
 */
int main(int argc, char **argv)
{

    configure_simulator(argc, argv);

    /* Initialize LVGL. */
    lv_init();

    /* Initialize the configured backend */
    if (driver_backends_init_backend(selected_backend) == -1) {
        die("Failed to initialize display backend");
    }

    /* Enable for EVDEV support */
#if LV_USE_EVDEV
    if (driver_backends_init_backend("EVDEV") == -1) {
        die("Failed to initialize evdev");
    }
#endif

    /*Create a Demo*/
    // lv_demo_widgets();
    lv_demo_eink();
    // lv_demo_widgets_start_slideshow();
    // lv_demo_high_res_api_example("/usr/share/ti-lvgl-demo/assets/", "/usr/share/ti-lvgl-demo/assets/img_lv_demo_high_res_ti_logo.png", "/usr/share/ti-lvgl-demo/slides");

    /* Enter the run loop of the selected backend */
    driver_backends_run_loop();

    return 0;
}

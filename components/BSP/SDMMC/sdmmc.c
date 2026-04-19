/**
 ******************************************************************************
 * @file        sdmmc.c
 * @version     V1.0
 * @brief       TF驱动代码
 ******************************************************************************
 * @attention   Waiken-Smart 慧勤智远
 * 
 * 实验平台:     慧勤智远 ESP32-P4 开发板
 ******************************************************************************
 */

#include "sdmmc.h"
#include "esp_idf_version.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "ff.h"
#include "diskio_impl.h"
#include "diskio_sdmmc.h"
#include <stdlib.h>

#if defined(CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE) && CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE \
    && (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0))
#define SDMMC_HOSTED_SDIO_INIT_WORKAROUND 1
#else
#define SDMMC_HOSTED_SDIO_INIT_WORKAROUND 0
#endif

const char* sdmmc_tag = "sdmmc";
sdmmc_card_t *card = NULL;
const char mount_point[] = MOUNT_POINT;
uint8_t sdmmc_mount_flag = 0x00;
static esp_ldo_channel_handle_t s_ldo_sd_phy = NULL;
static SemaphoreHandle_t s_sdmmc_mutex = NULL;
static FATFS s_direct_fatfs;
static uint8_t s_direct_fatfs_mounted = 0;

#define SDMMC_FATFS_DRIVE_NUM 0

static bool sdmmc_is_fatfs_ready(void)
{
    FF_DIR dir;
    FRESULT fr = f_opendir(&dir, "0:/");
    if (fr == FR_OK)
    {
        f_closedir(&dir);
        return true;
    }
    return false;
}

static esp_err_t sdmmc_mount_direct_fatfs(const sdmmc_host_t *host_cfg,
                                          const sdmmc_slot_config_t *slot_cfg,
                                          const esp_vfs_fat_sdmmc_mount_config_t *mount_cfg)
{
    esp_err_t ret;
    const BYTE pdrv = SDMMC_FATFS_DRIVE_NUM;
    char drv[3] = {(char)('0' + pdrv), ':', 0};

    if (s_direct_fatfs_mounted && sdmmc_is_fatfs_ready())
    {
        return ESP_OK;
    }

    if (card == NULL)
    {
        card = (sdmmc_card_t *)calloc(1, sizeof(sdmmc_card_t));
        if (card == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    ret = host_cfg->init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(sdmmc_tag, "Direct mount host init failed: %s", esp_err_to_name(ret));
        goto err;
    }

    ret = sdmmc_host_init_slot(host_cfg->slot, slot_cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(sdmmc_tag, "Direct mount slot init failed: %s", esp_err_to_name(ret));
        goto err;
    }

    ret = sdmmc_card_init(host_cfg, card);
    if (ret != ESP_OK)
    {
        ESP_LOGE(sdmmc_tag, "Direct mount card init failed: %s", esp_err_to_name(ret));
        goto err;
    }

    ff_diskio_register_sdmmc(pdrv, card);
    ff_sdmmc_set_disk_status_check(pdrv, mount_cfg->disk_status_check_enable);

    FRESULT fr = f_mount(&s_direct_fatfs, drv, 1);
    if (fr != FR_OK)
    {
        ff_diskio_unregister(pdrv);
        ESP_LOGE(sdmmc_tag, "Direct FatFs mount failed, f_mount=%d", fr);
        ret = ESP_FAIL;
        goto err;
    }

    s_direct_fatfs_mounted = 1;
    ESP_LOGW(sdmmc_tag, "Mounted by direct FatFs fallback on %s", drv);
    return ESP_OK;

err:
    if (card)
    {
        free(card);
        card = NULL;
    }
    return ret;
}

#if SDMMC_HOSTED_SDIO_INIT_WORKAROUND
static esp_err_t sdmmc_host_init_dummy(void)
{
    return ESP_OK;
}

static esp_err_t sdmmc_host_deinit_dummy(void)
{
    return ESP_OK;
}
#endif


/**
 * @brief       SD卡初始化并挂载SD卡
 * @param       无
 * @retval      ESP_OK:初始化成功
 */
esp_err_t sdmmc_init(void)
{
    ESP_LOGI(sdmmc_tag, "Init SDMMC in Slot0 mode");

    if (s_sdmmc_mutex == NULL)
    {
        s_sdmmc_mutex = xSemaphoreCreateMutex();
        if (s_sdmmc_mutex == NULL)
        {
            ESP_LOGE(sdmmc_tag, "Create sdmmc mutex failed");
            return ESP_ERR_NO_MEM;
        }
    }

    if (xSemaphoreTake(s_sdmmc_mutex, pdMS_TO_TICKS(3000)) != pdTRUE)
    {
        ESP_LOGE(sdmmc_tag, "Take sdmmc mutex timeout");
        return ESP_ERR_TIMEOUT;
    }

	sd_dev_bsp_enable_phy_power();    /* 配置SDIO接口电压3.3V */

    esp_err_t ret = ESP_OK;

    /* 已挂载则直接复用，避免并发场景下重复挂载占满 FATFS/VFS 资源 */
    if (sdmmc_mount_flag == 0x01 && card != NULL)
    {
        xSemaphoreGive(s_sdmmc_mutex);
        ESP_LOGI(sdmmc_tag, "SDMMC already mounted, reuse existing mount");
        return ESP_OK;
    }
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = SDMMC_FORMAT_IF_MOUNT_FAILED, /* 挂载失败时是否自动格式化 */
        .max_files              = SDMMC_MAX_FILES,          /* 打开分文件最大数量 */
        .allocation_unit_size   = SDMMC_ALLOCATION_UNIT_SIZE/* 分配单位大小 */
    };

    sdmmc_host_t sdmmc_host = SDMMC_HOST_DEFAULT();     /* SDMMC控制器默认设置,20MHz通信频率 */
    sdmmc_host.slot = SDMMC_HOST_SLOT_0;                /* 强制本地TF卡使用 Slot0，避免与 Hosted Slot1 冲突 */
    sdmmc_host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;     /* 设置速度最大值为40MHz */

#if SDMMC_HOSTED_SDIO_INIT_WORKAROUND
    /*
     * Hosted-SDIO 可能已完成 SDMMC host 初始化。
     * 避免再次 init/deinit 触发冲突，参照官方 hosted+sdcard 示例处理。
     */
    sdmmc_host.init = sdmmc_host_init_dummy;
    sdmmc_host.deinit = sdmmc_host_deinit_dummy;
#endif

    sdmmc_slot_config_t sdmmc_config = SDMMC_SLOT_CONFIG_DEFAULT(); /* SDMMC控制器硬件相关设置,总线宽度为4位 */
    sdmmc_config.width = 4;                 /* SDMMC总线宽度为4 */
    sdmmc_config.clk   = SDMMC_PIN_CLK;     /* SDMMC的时钟引脚 */
    sdmmc_config.cmd   = SDMMC_PIN_CMD;     /* SDMMC的命令引脚 */
    sdmmc_config.d0    = SDMMC_PIN_D0;      /* SDMMC的数据0引脚 */
    sdmmc_config.d1    = SDMMC_PIN_D1;      /* SDMMC的数据1引脚 */
    sdmmc_config.d2    = SDMMC_PIN_D2;      /* SDMMC的数据2引脚 */
    sdmmc_config.d3    = SDMMC_PIN_D3;      /* SDMMC的数据3引脚 */
    sdmmc_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;  /* 内部弱上拉 */

    card = NULL;
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &sdmmc_host, &sdmmc_config, &mount_config, &card);   /* 挂载文件系统(内部会调用sdmmc_host_init_slot初始化SDMMC) */

    if (ret == ESP_ERR_NO_MEM)
    {
        /*
         * 兜底恢复: 某些并发/异常路径下可能遗留 VFS FAT context, 导致 register 阶段返回 NO_MEM。
         * 先尝试清理挂载点，再以更小占用参数重试一次。
         */
        esp_err_t unreg_ret = esp_vfs_fat_unregister_path(mount_point);
        if (unreg_ret == ESP_OK)
        {
            ESP_LOGW(sdmmc_tag, "Recovered stale FATFS context on %s, retry mount", mount_point);
        }

        mount_config.max_files = 1;
        mount_config.allocation_unit_size = 0;
        vTaskDelay(pdMS_TO_TICKS(20));

        card = NULL;
        ret = esp_vfs_fat_sdmmc_mount(mount_point, &sdmmc_host, &sdmmc_config, &mount_config, &card);
    }

    if (ret != ESP_OK)
    {
        if (ret == ESP_ERR_NO_MEM)
        {
            ESP_LOGW(sdmmc_tag, "VFS mount returned NO_MEM, fallback to direct FatFs mount");
            ret = sdmmc_mount_direct_fatfs(&sdmmc_host, &sdmmc_config, &mount_config);
            if (ret == ESP_OK)
            {
                sdmmc_card_print_info(stdout, card);
                sdmmc_mount_flag = 0x01;
                xSemaphoreGive(s_sdmmc_mutex);
                return ESP_OK;
            }
        }

        if (ret == ESP_ERR_NO_MEM && sdmmc_is_fatfs_ready())
        {
            /* 已存在可用 FATFS 时，避免重复挂载继续消耗上下文。 */
            sdmmc_mount_flag = 0x01;
            xSemaphoreGive(s_sdmmc_mutex);
            ESP_LOGW(sdmmc_tag, "FATFS already ready, reuse existing drive without remount");
            return ESP_OK;
        }

        if (ret == ESP_FAIL)
        {
            ESP_LOGE(sdmmc_tag, "Failed to mount filesystem. "
                     "Card may be unformatted/corrupted or using unsupported FS (recommend FAT32). "
                     "format_if_mount_failed=%d", SDMMC_FORMAT_IF_MOUNT_FAILED);

            if (!SDMMC_FORMAT_IF_MOUNT_FAILED)
            {
                ESP_LOGW(sdmmc_tag, "You can set SDMMC_FORMAT_IF_MOUNT_FAILED to 1 in sdmmc.h to auto-format (THIS ERASES ALL DATA).");
            }
        }
        else
        {
            if (ret == ESP_ERR_NO_MEM)
            {
                size_t free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
                size_t largest_8bit = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
                size_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                ESP_LOGE(sdmmc_tag, "No memory while mounting SD: free_8bit=%u, largest_8bit=%u, free_spiram=%u",
                         (unsigned)free_8bit, (unsigned)largest_8bit, (unsigned)free_spiram);
            }
            ESP_LOGE(sdmmc_tag, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        sdmmc_mount_flag = 0x00;
        xSemaphoreGive(s_sdmmc_mutex);
        return ret;
    }

    /* 打印当前TF卡相关信息 */
    sdmmc_card_print_info(stdout, card);
    sdmmc_mount_flag = 0x01;
    xSemaphoreGive(s_sdmmc_mutex);
    return ESP_OK;
}

/**
 * @brief       取消挂载SD卡
 * @param       无
 * @retval      ESP_OK:初始化成功
 */
esp_err_t sdmmc_unmount(void)
{
    if (s_sdmmc_mutex && xSemaphoreTake(s_sdmmc_mutex, pdMS_TO_TICKS(3000)) != pdTRUE)
    {
        ESP_LOGE(sdmmc_tag, "Take sdmmc mutex timeout in unmount");
        return ESP_ERR_TIMEOUT;
    }

    if (sdmmc_mount_flag == 0x01)
    {
        if (s_direct_fatfs_mounted)
        {
            char drv[3] = {(char)('0' + SDMMC_FATFS_DRIVE_NUM), ':', 0};
            f_mount(NULL, drv, 0);
            ff_diskio_unregister(SDMMC_FATFS_DRIVE_NUM);
            s_direct_fatfs_mounted = 0;
            if (card)
            {
                free(card);
                card = NULL;
            }
        }
        else if (card != NULL)
        {
            ESP_ERROR_CHECK(esp_vfs_fat_sdcard_unmount(mount_point, card));
            card = NULL;
        }
    }

    sdmmc_mount_flag = 0x00;

    if (s_sdmmc_mutex)
    {
        xSemaphoreGive(s_sdmmc_mutex);
    }

    return ESP_OK;
}

/**
 * @brief       配置sd phy电压
 * @param       无
 * @retval      无
 */
void sd_dev_bsp_enable_phy_power(void)
{
#ifdef SD_PHY_PWR_LDO_CHAN
#if defined(CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE) && CONFIG_ESP_HOSTED_SDIO_HOST_INTERFACE
    /* Hosted-SDIO 已管理 SD 电源域，避免重复申请同一 LDO 通道。 */
    ESP_LOGI(sdmmc_tag, "Hosted SDIO enabled: skip local SD LDO acquire");
    return;
#endif

    if (s_ldo_sd_phy != NULL)
    {
        return;
    }

    esp_ldo_channel_config_t ldo_sd_phy_config = {
        .chan_id    = SD_PHY_PWR_LDO_CHAN,        /* 选择内存LDO */
        .voltage_mv = SD_PHY_PWR_LDO_VOLTAGE_MV,  /* 输出标准电压提供VDD_SD_DPHY */
    };
    esp_err_t ret = esp_ldo_acquire_channel(&ldo_sd_phy_config, &s_ldo_sd_phy);
    if (ret != ESP_OK)
    {
        /*
         * 资源已被其他模块占用（例如 ESP-Hosted SDIO）时，不要触发 abort。
         * 后续挂载流程会返回错误，业务层可根据错误码降级处理。
         */
        ESP_LOGW(sdmmc_tag, "SD PHY LDO acquire failed: %s", esp_err_to_name(ret));
        s_ldo_sd_phy = NULL;
        return;
    }

    ESP_LOGI(sdmmc_tag, "SD PHY Powered on");
#endif
}

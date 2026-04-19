set(src_dirs
            LED
            LCD
            MYIIC
            TOUCH
            MYI2S
            MYES8311
            INMP441
            SDMMC
            DHT22
            SMOKE)

set(include_dirs
            LED
            LCD
            MYIIC
            TOUCH
            MYI2S
            MYES8311
            INMP441
            SDMMC
            DHT22
            SMOKE)

set(requires
            driver
            esp_lcd
            esp_common
            esp_timer
            esp_codec_dev
            fatfs
            es8311
            esp_adc)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format -Wno-format)
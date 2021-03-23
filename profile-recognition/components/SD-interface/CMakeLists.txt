idf_component_register(
    SRCS "SD-interface.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos driver log fatfs)
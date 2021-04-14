idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES "prof-recog" "integ-things"
    PRIV_REQUIRES "CFAL1602" "Keypad-interface"
)

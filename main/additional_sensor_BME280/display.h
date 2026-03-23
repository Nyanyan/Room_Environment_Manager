#include <ST7032_asukiaaa.h>
#include <float.h>

ST7032_asukiaaa lcd;

void init_lcd();
void display_print_info(float temperature_c, float humidity_pct, float pressure_hpa, float co2_concentration_ppm = FLT_MAX);
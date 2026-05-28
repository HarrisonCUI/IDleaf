#pragma once

#include <lvgl.h>
#include <stdint.h>

lv_obj_t* smartPlantCreate(lv_obj_t* parent);
void smartPlantUpdate(int soilPercent, float temperatureC, float humidityPct, uint8_t phonesOnline);
void smartPlantReceiveHeart(const char* sender);
void smartPlantStartDemo();
bool smartPlantConsumeBacklightRequest();

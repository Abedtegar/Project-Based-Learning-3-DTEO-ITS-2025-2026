#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include <Arduino.h>
#include <Preferences.h>

void DataStorage_INIT();
float ReadData(String name, String data, float &value);
void StoreData(String name, String data, float value);
void PrintStoredData();

#endif
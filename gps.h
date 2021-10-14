#include <Arduino.h>
#ifndef __GPS_H__
#define __GPS_H__

extern String N_date, N_time,N_lat,N_lng,N_satellites,N_meters;
extern String P_date, P_time,P_lat,P_lng,P_satellites,P_meters;
extern double Lat,Lng,Meters,Satellites;
void GpsSerialInit();
void GetGpsInfoPolling();
void GpsstopListening();
void listen();
void UpdateGpsInfo();
int UpdateGpsData(char* destination);

#endif

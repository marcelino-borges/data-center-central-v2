/*
 * HttpResponse.h - Library created to centralize all error messages
 * Created by Marcelino Borges, in 04/09/2021
 */

#ifndef HttpResponse_h
#define HttpResponse_h

#include "Arduino.h"
#include <ArduinoJson.h>

class HttpResponse
{

public:
  //Attributes
  String data;
  String error;
  int code;

public:
  HttpResponse(int temp);
  HttpResponse(String msg);
  HttpResponse(String err, int codeRes);
  HttpResponse();
};

#endif

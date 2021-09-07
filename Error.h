/*
 * Errors.h - Library created to centralize all error messages
 * Created by Marcelino Borges, in 04/09/2021
 */

#ifndef Error_h
#define Error_h

#include "Arduino.h"

class Error
{
public:
  static const String ERROR_MSG_READ_TEMP;
  static const String ERROR_MSG_INVALID_HTTP_METHOD_TYPE;
  static const String ERROR_MSG_UNKNOWN_ROUTE;
  static const String ERROR_MSG_UNKNOWN_SUBROUTE;
  static const String ERROR_MSG_TURNON_UNKNOWN_AIR;
};
#endif
#include "HttpResponse.h"

HttpResponse::HttpResponse(int temp)
{
  HttpResponse::data = String("{ \"temperature\": ") + String(temp) + String(" }");
  HttpResponse::error = "";
  HttpResponse::code = 200;
}

HttpResponse::HttpResponse(String msg)
{
  HttpResponse::data = String("{ \"message\": ") + String(msg) + String(" }");
  HttpResponse::error = "";
  HttpResponse::code = 200;
}

HttpResponse::HttpResponse(String err, int codeRes)
{
  HttpResponse::data = "";
  HttpResponse::error = err;
  HttpResponse::code = codeRes;
}

HttpResponse::HttpResponse()
{
  HttpResponse::data = "";
  HttpResponse::error = "";
  HttpResponse::code = 0;
}

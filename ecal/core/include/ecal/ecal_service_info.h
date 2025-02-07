/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2019 Continental Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================= eCAL LICENSE =================================
*/

/**
 * @file   ecal_service_info.h
 * @brief  eCAL service info
**/

#pragma once

/**
 * @brief  Service call state.
**/
enum eCallState
{
  call_state_none = 0,    //!< undefined
  call_state_executed,    //!< executed (successfully)
  call_state_failed       //!< failed
};

#ifdef __cplusplus

#include <functional>
#include <string>

namespace eCAL
{
  /**
   * @brief eCAL service info struct.
  **/
  struct SServiceInfo
  {
    SServiceInfo()
    {
      ret_state  = 0;
      call_state = call_state_none;
    };
    std::string  host_name;
    std::string  service_name;
    std::string  method_name;
    std::string  error_msg;
    int          ret_state;
    eCallState   call_state;
  };

  typedef std::function<int(const std::string& method_, const std::string& req_type_, const std::string& resp_type_, const std::string& request_, std::string& response_)> MethodCallbackT;
  typedef std::function<void(const struct SServiceInfo& service_info_, const std::string& response_)> ResponseCallbackT;
};

#endif /* __cplusplus */

#ifdef _MSC_VER
#pragma pack(push, 8)
#endif

/**
 * @brief eCAL service info struct returned as service response (C API).
**/
struct SServiceInfoC
{
  const char*      host_name;      //!< service host
  const char*      service_name;   //!< service name
  const char*      method_name;    //!< method name
  const char*      error_msg;      //!< error message in case of failure
  int              ret_state;      //!< return state from method callback
  enum eCallState  call_state;     //!< service call state
};

#ifdef _MSC_VER
#pragma pack(pop)
#endif

/**
 * @brief eCAL service method callback function (server side)
 *
 * @param method_             Method name.
 * @param req_type_           Type of the request message.
 * @param resp_type_          Type of the response message.
 * @param request_            Request payload.
 * @param request_len_        Request payload length.
 * @param [out] reponse_      Method response payload.
 * @param [out] reponse_len_  Method response payload length.
 * @param par_                Forwarded user defined parameter.
**/
typedef int(*MethodCallbackCT)(const char* method_, const char* req_type_, const char* resp_type_, const char* request_, int request_len_, void** response_, int* response_len_, void* par_);

/**
 * @brief eCAL service response callback function (client side)
 *
 * @param service_info_       Service info struct.
 * @param reponse_            Method response payload.
 * @param reponse_len_        Method response payload length.
 * @param par_                Forwarded user defined parameter.
**/
typedef void (*ResponseCallbackCT)(const struct SServiceInfoC* service_info_, const char* response_, int response_len_, void* par_);

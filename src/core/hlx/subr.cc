//: ----------------------------------------------------------------------------
//: Copyright (C) 2015 Verizon.  All Rights Reserved.
//: All Rights Reserved
//:
//: \file:    subr.cc
//: \details: TODO
//: \author:  Reed P. Morrison
//: \date:    07/20/2015
//:
//:   Licensed under the Apache License, Version 2.0 (the "License");
//:   you may not use this file except in compliance with the License.
//:   You may obtain a copy of the License at
//:
//:       http://www.apache.org/licenses/LICENSE-2.0
//:
//:   Unless required by applicable law or agreed to in writing, software
//:   distributed under the License is distributed on an "AS IS" BASIS,
//:   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//:   See the License for the specific language governing permissions and
//:   limitations under the License.
//:
//: ----------------------------------------------------------------------------

//: ----------------------------------------------------------------------------
//: Includes
//: ----------------------------------------------------------------------------
#include "hlx/hlx.h"
#include "ndebug.h"
#include "string_util.h"
#include "http_parser.h"

#include <string.h>

namespace ns_hlx {

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
subr::subr(void):
        m_type(SUBR_TYPE_NONE),
        m_scheme(SCHEME_NONE),
        m_host(),
        m_port(0),
        m_server_label(),
        m_save(true),
        m_connect_only(false),
        m_is_multipath(false),
        m_timeout_s(10),
        m_path(),
        m_query(),
        m_fragment(),
        m_userinfo(),
        m_hostname(),
        m_verb("GET"),
        m_keepalive(false),
        m_id(),
        m_where(),
        m_headers(),
        m_body_data(NULL),
        m_body_data_len(0),
        m_num_to_request(1),
        m_num_requested(0),
        m_num_completed(0),
        m_error_cb(NULL),
        m_completion_cb(NULL),
        m_create_req_cb(create_request),
        m_data(NULL),
        m_detach_resp(false),
        m_uid(0),
        m_requester_hconn(NULL),
        m_host_info(NULL),
        m_t_hlx(NULL)
{
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
subr::subr(const subr &a_subr):
        m_type(a_subr.m_type),
        m_scheme(a_subr.m_scheme),
        m_host(a_subr.m_host),
        m_port(a_subr.m_port),
        m_server_label(a_subr.m_server_label),
        m_save(a_subr.m_save),
        m_connect_only(a_subr.m_connect_only),
        m_is_multipath(a_subr.m_is_multipath),
        m_timeout_s(a_subr.m_timeout_s),
        m_path(a_subr.m_path),
        m_query(a_subr.m_query),
        m_fragment(a_subr.m_fragment),
        m_userinfo(a_subr.m_userinfo),
        m_hostname(a_subr.m_hostname),
        m_verb(a_subr.m_verb),
        m_keepalive(a_subr.m_keepalive),
        m_id(a_subr.m_id),
        m_where(a_subr.m_where),
        m_headers(a_subr.m_headers),
        m_body_data(a_subr.m_body_data),
        m_body_data_len(a_subr.m_body_data_len),
        m_num_to_request(a_subr.m_num_to_request),
        m_num_requested(a_subr.m_num_requested),
        m_num_completed(a_subr.m_num_completed),
        m_error_cb(a_subr.m_error_cb),
        m_completion_cb(a_subr.m_completion_cb),
        m_create_req_cb(a_subr.m_create_req_cb),
        m_data(a_subr.m_data),
        m_detach_resp(a_subr.m_detach_resp),
        m_uid(a_subr.m_uid),
        m_requester_hconn(a_subr.m_requester_hconn),
        m_host_info(a_subr.m_host_info),
        m_t_hlx(a_subr.m_t_hlx)
{
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
subr::~subr(void)
{
}

//: ----------------------------------------------------------------------------
//:                                  Getters
//: ----------------------------------------------------------------------------
//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
subr_type_t subr::get_type(void)
{
        return m_type;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
scheme_t subr::get_scheme(void)
{
        return m_scheme;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
bool subr::get_save(void)
{
        return m_save;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
bool subr::get_connect_only(void)
{
        return m_connect_only;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
const std::string &subr::get_host(void)
{
        return m_host;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
const std::string &subr::get_hostname(void)
{
        return m_hostname;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
const std::string &subr::get_id(void)
{
        return m_id;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
const std::string &subr::get_where(void)
{
        return m_where;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
uint16_t subr::get_port(void)
{
        return m_port;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t subr::get_num_to_request(void)
{
        return m_num_to_request;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
uint32_t subr::get_num_requested(void)
{
        return m_num_requested;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
uint32_t subr::get_num_completed(void)
{
        return m_num_completed;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
bool subr::get_keepalive(void)
{
        return m_keepalive;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
bool subr::get_is_done(void)
{
        if((m_num_to_request < 0) || m_num_completed < (uint32_t)m_num_to_request)
        {
                return false;
        }
        else
        {
                return true;
        }
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
bool subr::get_is_pending_done(void)
{
        if((m_num_to_request < 0) || m_num_requested < (uint32_t)m_num_to_request)
        {
                return false;
        }
        else
        {
                return true;
        }
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
bool subr::get_is_multipath(void)
{
        return m_is_multipath;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
subr::error_cb_t subr::get_error_cb(void)
{
        return m_error_cb;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
subr::completion_cb_t subr::get_completion_cb(void)
{
        return m_completion_cb;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
subr::create_req_cb_t subr::get_create_req_cb(void)
{
        return m_create_req_cb;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t subr::get_timeout_s(void)
{
        return m_timeout_s;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void *subr::get_data(void)
{
        return m_data;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
bool subr::get_detach_resp(void)
{
        return m_detach_resp;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
uint64_t subr::get_uid(void)
{
        return m_uid;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
hconn *subr::get_requester_hconn(void)
{
        return m_requester_hconn;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
const host_info_s *subr::get_host_info(void)
{
        return m_host_info;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
t_hlx *subr::get_t_hlx(void)
{
        return m_t_hlx;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::reset_label(void)
{
        switch(m_scheme)
        {
        case SCHEME_NONE:
        {
                m_server_label += "none://";
                break;
        }
        case SCHEME_TCP:
        {
                m_server_label += "http://";
                break;
        }
        case SCHEME_TLS:
        {
                m_server_label += "https://";
                break;
        }
        default:
        {
                m_server_label += "default://";
                break;
        }
        }
        m_server_label += m_host;
        char l_port_str[16];
        snprintf(l_port_str, 16, ":%u", m_port);
        m_server_label += l_port_str;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
const std::string &subr::get_label(void)
{
        if(m_server_label.empty())
        {
                reset_label();
        }
        return m_server_label;
}

//: ----------------------------------------------------------------------------
//:                                  Setters
//: ----------------------------------------------------------------------------
//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_scheme(scheme_t a_scheme)
{
        m_scheme = a_scheme;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_save(bool a_val)
{
        m_save = a_val;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_connect_only(bool a_val)
{
        m_connect_only = a_val;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_is_multipath(bool a_val)
{
        m_is_multipath = a_val;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_num_to_request(int32_t a_val)
{
        m_num_to_request = a_val;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_keepalive(bool a_val)
{
        m_keepalive = a_val;
        if(m_keepalive)
        {
                set_header("Connection", "keep-alive");
                //if(m_num_reqs_per_conn == 1)
                //{
                //        set_num_reqs_per_conn(-1);
                //}
        }
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_error_cb(error_cb_t a_cb)
{
        m_error_cb = a_cb;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_completion_cb(completion_cb_t a_cb)
{
        m_completion_cb = a_cb;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_create_req_cb(create_req_cb_t a_cb)
{
        m_create_req_cb = a_cb;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_type(subr_type_t a_type)
{
        m_type = a_type;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_timeout_s(int32_t a_val)
{
        m_timeout_s = a_val;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_host(std::string a_val)
{
        m_host = a_val;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_hostname(std::string a_val)
{
        m_hostname = a_val;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_id(std::string a_val)
{
        m_id = a_val;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_where(std::string a_val)
{
        m_where = a_val;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_port(uint16_t a_val)
{
        m_port = a_val;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_data(void *a_data)
{
        m_data = a_data;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_detach_resp(bool a_val)
{
        m_detach_resp = a_val;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_uid(uint64_t a_uid)
{
        m_uid = a_uid;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_requester_hconn(hconn *a_hconn)
{
        m_requester_hconn = a_hconn;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_host_info(const host_info_s *a_host_info)
{
        m_host_info = a_host_info;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_t_hlx(t_hlx *a_t_hlx)
{
        m_t_hlx = a_t_hlx;
}

//: ----------------------------------------------------------------------------
//:                             Request Getters
//: ----------------------------------------------------------------------------
//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
const std::string &subr::get_query(void)
{
        return m_query;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
const std::string &subr::get_path(void)
{
        return m_path;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
const std::string &subr::get_verb(void)
{
        return m_verb;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
const kv_map_list_t &subr::get_headers(void)
{
        return m_headers;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
const char *subr::get_body_data(void)
{
        return m_body_data;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
uint32_t subr::get_body_len(void)
{
        return m_body_data_len;
}

//: ----------------------------------------------------------------------------
//:                             Request Setters
//: ----------------------------------------------------------------------------
//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_body_data(const char *a_ptr, uint32_t a_len)
{
        m_body_data = a_ptr;
        m_body_data_len = a_len;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::set_verb(const std::string &a_verb)
{
        m_verb = a_verb;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int subr::set_header(const std::string &a_header)
{
        int32_t l_status;
        std::string l_header_key;
        std::string l_header_val;
        l_status = break_header_string(a_header, l_header_key, l_header_val);
        if(l_status != 0)
        {
                // If verbose???
                NDBG_PRINT("Error header string[%s] is malformed\n", a_header.c_str());
                return STATUS_ERROR;
        }
        l_status = set_header(l_header_key, l_header_val);
        if(l_status != STATUS_OK)
        {
                // If verbose???
                NDBG_PRINT("Error performing set header with key: %s value: %s\n", l_header_key.c_str(), l_header_val.c_str());
                return STATUS_ERROR;
        }
        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int subr::set_header(const std::string &a_key, const std::string &a_val)
{
        kv_map_list_t::iterator i_obj = m_headers.find(a_key);
        if(i_obj != m_headers.end())
        {
                i_obj->second.push_back(a_val);
        }
        else
        {
                str_list_t l_list;
                l_list.push_back(a_val);
                m_headers[a_key] = l_list;
        }
        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::clear_headers(void)
{
        m_headers.clear();
}

//: ----------------------------------------------------------------------------
//:                                 Stats
//: ----------------------------------------------------------------------------
//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::bump_num_requested(void)
{
        ++m_num_requested;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void subr::bump_num_completed(void)
{
        ++m_num_completed;
}

//: ----------------------------------------------------------------------------
//:                              Initialize
//: ----------------------------------------------------------------------------
//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t subr::init_with_url(const std::string &a_url)
{
        std::string l_url_fixed = a_url;
        // Find scheme prefix "://"
        if(a_url.find("://", 0) == std::string::npos)
        {
                l_url_fixed = "http://" + a_url;
        }

        //NDBG_PRINT("Parse url:           %s\n", a_url.c_str());
        //NDBG_PRINT("Parse a_wildcarding: %d\n", a_wildcarding);
        http_parser_url l_url;
        // silence bleating memory sanitizers...
        memset(&l_url, 0, sizeof(l_url));
        int l_status;
        l_status = http_parser_parse_url(l_url_fixed.c_str(), l_url_fixed.length(), 0, &l_url);
        if(l_status != 0)
        {
                NDBG_PRINT("Error parsing url: %s\n", l_url_fixed.c_str());
                // TODO get error msg from http_parser
                return STATUS_ERROR;
        }

        // Set no port
        m_port = 0;

        for(uint32_t i_part = 0; i_part < UF_MAX; ++i_part)
        {
                //NDBG_PRINT("i_part: %d offset: %d len: %d\n", i_part, l_url.field_data[i_part].off, l_url.field_data[i_part].len);
                //NDBG_PRINT("len+off: %d\n",       l_url.field_data[i_part].len + l_url.field_data[i_part].off);
                //NDBG_PRINT("a_url.length(): %d\n", (int)a_url.length());
                if(l_url.field_data[i_part].len &&
                  // TODO Some bug with parser -parsing urls like "http://127.0.0.1" sans paths
                  ((l_url.field_data[i_part].len + l_url.field_data[i_part].off) <= l_url_fixed.length()))
                {
                        switch(i_part)
                        {
                        case UF_SCHEMA:
                        {
                                std::string l_part = l_url_fixed.substr(l_url.field_data[i_part].off, l_url.field_data[i_part].len);
                                //NDBG_PRINT("l_part: %s\n", l_part.c_str());
                                if(l_part == "http")
                                {
                                        m_scheme = SCHEME_TCP;
                                }
                                else if(l_part == "https")
                                {
                                        m_scheme = SCHEME_TLS;
                                }
                                else
                                {
                                        NDBG_PRINT("Error schema[%s] is unsupported\n", l_part.c_str());
                                        return STATUS_ERROR;
                                }
                                break;
                        }
                        case UF_HOST:
                        {
                                std::string l_part = l_url_fixed.substr(l_url.field_data[i_part].off, l_url.field_data[i_part].len);
                                //NDBG_PRINT("l_part[UF_HOST]: %s\n", l_part.c_str());
                                m_host = l_part;
                                break;
                        }
                        case UF_PORT:
                        {
                                std::string l_part = l_url_fixed.substr(l_url.field_data[i_part].off, l_url.field_data[i_part].len);
                                //NDBG_PRINT("l_part[UF_PORT]: %s\n", l_part.c_str());
                                m_port = (uint16_t)strtoul(l_part.c_str(), NULL, 10);
                                break;
                        }
                        case UF_PATH:
                        {
                                std::string l_part = l_url_fixed.substr(l_url.field_data[i_part].off, l_url.field_data[i_part].len);
                                //NDBG_PRINT("l_part[UF_PATH]: %s\n", l_part.c_str());
                                m_path = l_part;
                                break;
                        }
                        case UF_QUERY:
                        {
                                std::string l_part = l_url_fixed.substr(l_url.field_data[i_part].off, l_url.field_data[i_part].len);
                                //NDBG_PRINT("l_part[UF_QUERY]: %s\n", l_part.c_str());
                                m_query = l_part;
                                break;
                        }
                        case UF_FRAGMENT:
                        {
                                std::string l_part = l_url_fixed.substr(l_url.field_data[i_part].off, l_url.field_data[i_part].len);
                                //NDBG_PRINT("l_part[UF_FRAGMENT]: %s\n", l_part.c_str());
                                m_fragment = l_part;
                                break;
                        }
                        case UF_USERINFO:
                        {
                                std::string l_part = l_url_fixed.substr(l_url.field_data[i_part].off, l_url.field_data[i_part].len);
                                //sNDBG_PRINT("l_part[UF_USERINFO]: %s\n", l_part.c_str());
                                m_userinfo = l_part;
                                break;
                        }
                        default:
                        {
                                break;
                        }
                        }
                }
        }

        // Default ports
        if(!m_port)
        {
                switch(m_scheme)
                {
                case SCHEME_TCP:
                {
                        m_port = 80;
                        break;
                }
                case SCHEME_TLS:
                {
                        m_port = 443;
                        break;
                }
                default:
                {
                        m_port = 80;
                        break;
                }
                }
        }
        //m_num_to_req = m_path_vector.size();
        //NDBG_PRINT("Showing parsed url.\n");
        //m_url.show();
        if (STATUS_OK != l_status)
        {
                // Failure
                NDBG_PRINT("Error parsing url: %s.\n", l_url_fixed.c_str());
                return STATUS_ERROR;
        }
        //NDBG_PRINT("Parsed url: %s\n", l_url_fixed.c_str());
        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t subr::create_request(hlx &a_hlx, subr &a_subr, nbq &ao_q)
{
        std::string l_path_ref = a_subr.get_path();

        char l_buf[2048];
        int32_t l_len = 0;
        if(l_path_ref.empty())
        {
                l_path_ref = "/";
        }
        if(!(a_subr.get_query().empty()))
        {
                l_path_ref += "?";
                l_path_ref += a_subr.get_query();
        }
        //NDBG_PRINT("HOST: %s PATH: %s\n", a_reqlet.m_url.m_host.c_str(), l_path_ref.c_str());
        l_len = snprintf(l_buf, sizeof(l_buf),
                        "%s %.500s HTTP/1.1", a_subr.get_verb().c_str(), l_path_ref.c_str());

        nbq_write_request_line(ao_q, l_buf, l_len);

        // -------------------------------------------
        // Add repo headers
        // -------------------------------------------
        bool l_specd_host = false;

        // Loop over reqlet map
        for(kv_map_list_t::const_iterator i_hl = a_subr.get_headers().begin();
            i_hl != a_subr.get_headers().end();
            ++i_hl)
        {
                if(i_hl->first.empty() || i_hl->second.empty())
                {
                        continue;
                }
                for(str_list_t::const_iterator i_v = i_hl->second.begin();
                    i_v != i_hl->second.end();
                    ++i_v)
                {
                        nbq_write_header(ao_q, i_hl->first.c_str(), i_hl->first.length(), i_v->c_str(), i_v->length());
                        if (strcasecmp(i_hl->first.c_str(), "host") == 0)
                        {
                                l_specd_host = true;
                        }
                }
        }

        // -------------------------------------------
        // Default Host if unspecified
        // -------------------------------------------
        if (!l_specd_host)
        {
                nbq_write_header(ao_q, "Host", strlen("Host"), a_subr.get_host().c_str(), a_subr.get_host().length());
        }

        // -------------------------------------------
        // body
        // -------------------------------------------
        if(a_subr.get_body_data() && a_subr.get_body_len())
        {
                //NDBG_PRINT("Write: buf: %p len: %d\n", l_buf, l_len);
                nbq_write_body(ao_q, a_subr.get_body_data(), a_subr.get_body_len());
        }
        else
        {
                nbq_write_body(ao_q, NULL, 0);
        }

        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
#if 0
void subr::set_uri(const std::string &a_uri)
{
        m_uri = a_uri;

        // -------------------------------------------------
        // TODO Zero copy with something like substring...
        // This is pretty awful performance wise
        // -------------------------------------------------
        // Read uri up to first '?'
        size_t l_query_pos = 0;
        if((l_query_pos = m_uri.find('?', 0)) == std::string::npos)
        {
                // No query string -path == uri
                m_path = m_uri;
                return;
        }

        m_path = m_uri.substr(0, l_query_pos);

        // TODO Url decode???

        std::string l_query = m_uri.substr(l_query_pos + 1, m_uri.length() - l_query_pos + 1);

        // Split the query by '&'
        if(!l_query.empty())
        {

                //NDBG_PRINT("%s__QUERY__%s: l_query: %s\n", ANSI_COLOR_BG_WHITE, ANSI_COLOR_OFF, l_query.c_str());

                size_t l_qi_begin = 0;
                size_t l_qi_end = 0;
                bool l_last = false;
                while (!l_last)
                {
                        l_qi_end = l_query.find('&', l_qi_begin);
                        if(l_qi_end == std::string::npos)
                        {
                                l_last = true;
                                l_qi_end = l_query.length();
                        }

                        std::string l_query_item = l_query.substr(l_qi_begin, l_qi_end - l_qi_begin);

                        // Search for '='
                        size_t l_qi_val_pos = 0;
                        l_qi_val_pos = l_query_item.find('=', 0);
                        std::string l_q_k;
                        std::string l_q_v;
                        if(l_qi_val_pos != std::string::npos)
                        {
                                l_q_k = l_query_item.substr(0, l_qi_val_pos);
                                l_q_v = l_query_item.substr(l_qi_val_pos + 1, l_query_item.length());
                        }
                        else
                        {
                                l_q_k = l_query_item;
                        }

                        //NDBG_PRINT("%s__QUERY__%s: k[%s]: %s\n",
                        //                ANSI_COLOR_BG_WHITE, ANSI_COLOR_OFF, l_q_k.c_str(), l_q_v.c_str());

                        // Add to list
                        kv_list_map_t::iterator i_key = m_query.find(l_q_k);
                        if(i_key == m_query.end())
                        {
                                value_list_t l_list;
                                l_list.push_back(l_q_v);
                                m_query[l_q_k] = l_list;
                        }
                        else
                        {
                                i_key->second.push_back(l_q_v);
                        }

                        // Move fwd
                        l_qi_begin = l_qi_end + 1;

                }
        }

}
#endif

#if 0
//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
const kv_list_map_t &subr::get_uri_decoded_query(void)
{

        if(m_query_uri_decoded.empty() && !m_query.empty())
        {

                // Decode the arguments for now
                for(kv_list_map_t::const_iterator i_kv = m_query.begin();
                    i_kv != m_query.end();
                    ++i_kv)
                {
                        value_list_t l_value_list;
                        for(value_list_t::const_iterator i_v = i_kv->second.begin();
                            i_v != i_kv->second.end();
                            ++i_v)
                        {
                                std::string l_v = uri_decode(*i_v);
                                l_value_list.push_back(l_v);
                        }

                        std::string l_k = uri_decode(i_kv->first);
                        m_query_uri_decoded[l_k] = l_value_list;
                }
        }

        return m_query_uri_decoded;

}
#endif

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
#if 0
void subr::show(bool a_color)
{
        std::string l_host_color = "";
        std::string l_query_color = "";
        std::string l_header_color = "";
        std::string l_body_color = "";
        std::string l_off_color = "";
        if(a_color)
        {
                l_host_color = ANSI_COLOR_FG_BLUE;
                l_query_color = ANSI_COLOR_FG_MAGENTA;
                l_header_color = ANSI_COLOR_FG_GREEN;
                l_body_color = ANSI_COLOR_FG_YELLOW;
                l_off_color = ANSI_COLOR_OFF;
        }

        // Host
        NDBG_OUTPUT("%sUri%s:  %s\n", l_host_color.c_str(), l_off_color.c_str(), m_uri.c_str());
        NDBG_OUTPUT("%sPath%s: %s\n", l_host_color.c_str(), l_off_color.c_str(), m_path.c_str());

        // Query
        for(kv_list_map_t::iterator i_key = m_query.begin();
                        i_key != m_query.end();
            ++i_key)
        {
                NDBG_OUTPUT("%s%s%s: %s\n",
                                l_query_color.c_str(), i_key->first.c_str(), l_off_color.c_str(),
                                i_key->second.begin()->c_str());
        }


        // Headers
        for(kv_list_map_t::iterator i_key = m_headers.begin();
            i_key != m_headers.end();
            ++i_key)
        {
                NDBG_OUTPUT("%s%s%s: %s\n",
                                l_header_color.c_str(), i_key->first.c_str(), l_off_color.c_str(),
                                i_key->second.begin()->c_str());
        }

        // Body
        NDBG_OUTPUT("%sBody%s: %s\n", l_body_color.c_str(), l_off_color.c_str(), m_body.c_str());

}
#endif

} //namespace ns_hlx {
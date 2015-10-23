//: ----------------------------------------------------------------------------
//: Copyright (C) 2014 Verizon.  All Rights Reserved.
//: All Rights Reserved
//:
//: \file:    main.cc
//: \details: TODO
//: \author:  Reed P. Morrison
//: \date:    02/07/2014
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
#include "rapidjson/document.h"

#include <string.h>

// getrlimit
#include <sys/time.h>
#include <sys/resource.h>

// signal
#include <signal.h>

// Shared pointer
//#include <tr1/memory>

#include <list>
#include <algorithm>
#include <unordered_set>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h> // For getopt_long
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

// Profiler
#define ENABLE_PROFILER 1
#ifdef ENABLE_PROFILER
#include <google/profiler.h>
#endif

//: ----------------------------------------------------------------------------
//: Constants
//: ----------------------------------------------------------------------------
#define NB_ENABLE  1
#define NB_DISABLE 0

#define MAX_READLINE_SIZE 1024

// Version
#define HLE_VERSION_MAJOR 0
#define HLE_VERSION_MINOR 0
#define HLE_VERSION_MACRO 1
#define HLE_VERSION_PATCH "alpha"

//: ----------------------------------------------------------------------------
//: Status
//: ----------------------------------------------------------------------------
#ifndef STATUS_ERROR
#define STATUS_ERROR -1
#endif

#ifndef STATUS_OK
#define STATUS_OK 0
#endif

//: ----------------------------------------------------------------------------
//: ANSI Color Code Strings
//:
//: Taken from:
//: http://pueblo.sourceforge.net/doc/manual/ansi_color_codes.html
//: ----------------------------------------------------------------------------
#define ANSI_COLOR_OFF          "\033[0m"
#define ANSI_COLOR_FG_BLACK     "\033[01;30m"
#define ANSI_COLOR_FG_RED       "\033[01;31m"
#define ANSI_COLOR_FG_GREEN     "\033[01;32m"
#define ANSI_COLOR_FG_YELLOW    "\033[01;33m"
#define ANSI_COLOR_FG_BLUE      "\033[01;34m"
#define ANSI_COLOR_FG_MAGENTA   "\033[01;35m"
#define ANSI_COLOR_FG_CYAN      "\033[01;36m"
#define ANSI_COLOR_FG_WHITE     "\033[01;37m"
#define ANSI_COLOR_FG_DEFAULT   "\033[01;39m"
#define ANSI_COLOR_BG_BLACK     "\033[01;40m"
#define ANSI_COLOR_BG_RED       "\033[01;41m"
#define ANSI_COLOR_BG_GREEN     "\033[01;42m"
#define ANSI_COLOR_BG_YELLOW    "\033[01;43m"
#define ANSI_COLOR_BG_BLUE      "\033[01;44m"
#define ANSI_COLOR_BG_MAGENTA   "\033[01;45m"
#define ANSI_COLOR_BG_CYAN      "\033[01;46m"
#define ANSI_COLOR_BG_WHITE     "\033[01;47m"
#define ANSI_COLOR_BG_DEFAULT   "\033[01;49m"

//: ----------------------------------------------------------------------------
//: Macros
//: ----------------------------------------------------------------------------
#define UNUSED(x) ( (void)(x) )

//: ----------------------------------------------------------------------------
//: Settings
//: ----------------------------------------------------------------------------
typedef struct settings_struct
{
        bool m_verbose;
        bool m_color;
        bool m_quiet;
        bool m_show_stats;
        bool m_show_summary;
        bool m_cli;
        ns_hlx::hlx *m_hlx;
        ns_hlx::subreq *m_subreq;
        uint32_t m_total_reqs;

        // ---------------------------------
        // Defaults...
        // ---------------------------------
        settings_struct(void) :
                m_verbose(false),
                m_color(false),
                m_quiet(false),
                m_show_stats(false),
                m_show_summary(false),
                m_cli(false),
                m_hlx(NULL),
                m_subreq(NULL),
                m_total_reqs(0)
        {}

private:
        // Disallow copy/assign
        settings_struct& operator=(const settings_struct &);
        settings_struct(const settings_struct &);

} settings_struct_t;

//: ----------------------------------------------------------------------------
//: Globals
//: ----------------------------------------------------------------------------
bool g_test_finished = false;
bool g_cancelled = false;
settings_struct_t *g_settings = NULL;

//: ----------------------------------------------------------------------------
//: Prototypes
//: ----------------------------------------------------------------------------
void display_status_line(settings_struct_t &a_settings);
void display_summary(settings_struct_t &a_settings, uint32_t a_num_hosts);
int32_t read_file(const char *a_file, char **a_buf, uint32_t *a_len);

//: ----------------------------------------------------------------------------
//: \details: Completion callback
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
static int32_t s_completion_cb(ns_hlx::nconn &a_nconn,
                               ns_hlx::subreq &a_subreq,
                               ns_hlx::http_resp &a_resp)
{
        g_test_finished = true;
        return 0;
}

//: ----------------------------------------------------------------------------
//: \details: Signal handler
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void sig_handler(int signo)
{
        if (signo == SIGINT)
        {
                // Kill program
                g_test_finished = true;
                g_cancelled = true;
                g_settings->m_hlx->stop();
        }
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int kbhit()
{
        struct timeval tv;
        fd_set fds;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        //STDIN_FILENO is 0
        select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
        return FD_ISSET(STDIN_FILENO, &fds);
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void nonblock(int state)
{
        struct termios ttystate;
        //get the terminal state
        tcgetattr(STDIN_FILENO, &ttystate);
        if (state == NB_ENABLE)
        {
                //turn off canonical mode
                ttystate.c_lflag &= ~ICANON;
                //minimum of number input read.
                ttystate.c_cc[VMIN] = 1;
        } else if (state == NB_DISABLE)
        {
                //turn on canonical mode
                ttystate.c_lflag |= ICANON;
        }
        //set the terminal attributes.
        tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int command_exec(settings_struct_t &a_settings, bool a_send_stop)
{
        int i = 0;
        char l_cmd = ' ';
        //bool l_sent_stop = false;
        //bool l_first_time = true;
        nonblock(NB_ENABLE);
        int l_status;

        //printf("Adding subreq\n\n");
        l_status = a_settings.m_hlx->add_subreq(*(a_settings.m_subreq));
        if(l_status != HLX_SERVER_STATUS_OK)
        {
                printf("Error: performing add_subreq.\n");
                return -1;
        }
        g_test_finished = false;
        // ---------------------------------------
        //   Loop forever until user quits
        // ---------------------------------------
        while (!g_test_finished)
        {
                i = kbhit();
                if (i != 0)
                {
                        l_cmd = fgetc(stdin);

                        switch (l_cmd)
                        {

                        // -------------------------------------------
                        // Quit
                        // -only works when not reading from stdin
                        // -------------------------------------------
                        case 'q':
                        {
                                g_test_finished = true;
                                a_settings.m_hlx->stop();
                                //l_sent_stop = true;
                                break;
                        }

                        // -------------------------------------------
                        // Default
                        // -------------------------------------------
                        default:
                        {
                                break;
                        }
                        }
                }

                // TODO add define...
                usleep(500000);

                if(a_settings.m_show_stats)
                {
                        display_status_line(a_settings);
                }
                //if (!a_settings.m_hlx->is_running())
                //{
                //        //printf("IS NOT RUNNING.\n");
                //        g_test_finished = true;
                //}
        }
        // Send stop -if unsent
        //if(!l_sent_stop && a_send_stop)
        //{
        //        a_settings.m_hlx->stop();
        //        l_sent_stop = true;
        //}
        // wait for completion...
        //a_settings.m_hlx->wait_till_stopped();
        // One more status for the lovers
        if(a_settings.m_show_stats)
        {
                display_status_line(a_settings);
        }
        nonblock(NB_DISABLE);
        return 0;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void show_help(void)
{
        printf(" hle commands: \n");
        printf("  h    Help or ?\n");
        printf("  r    Run\n");
        printf("  q    Quit\n");
}

#define MAX_CMD_SIZE 64
int command_exec_cli(settings_struct_t &a_settings)
{
        a_settings.m_hlx->set_use_persistent_pool(true);

        bool l_done = false;
        // -------------------------------------------
        // Interactive mode banner
        // -------------------------------------------
        if(a_settings.m_color)
        {
                printf("%shle interactive mode%s: (%stype h for command help%s)\n",
                                ANSI_COLOR_FG_YELLOW, ANSI_COLOR_OFF,
                                ANSI_COLOR_FG_CYAN, ANSI_COLOR_OFF);
        }
        else
        {
                printf("hle interactive mode: (type h for command help)\n");
        }

        // ---------------------------------------
        //   Loop forever until user quits
        // ---------------------------------------
        while (!l_done && !g_cancelled)
        {
                // -------------------------------------------
                // Interactive mode prompt
                // -------------------------------------------
                if(a_settings.m_color)
                {
                        printf("%shle>>%s", ANSI_COLOR_FG_GREEN, ANSI_COLOR_OFF);
                }
                else
                {
                        printf("hle>>");
                }
                fflush(stdout);

                char l_cmd[MAX_CMD_SIZE] = {' '};
                char *l_status;
                l_status = fgets(l_cmd, MAX_CMD_SIZE, stdin);
                if(!l_status)
                {
                        printf("Error reading cmd from stdin\n");
                        return -1;
                }

                switch (l_cmd[0])
                {
                // -------------------------------------------
                // Quit
                // -only works when not reading from stdin
                // -------------------------------------------
                case 'q':
                {
                        l_done = true;
                        break;
                }
                // -------------------------------------------
                // run
                // -------------------------------------------
                case 'r':
                {
                        int l_status;
                        l_status = command_exec(a_settings, false);
                        if(l_status != 0)
                        {
                                return -1;
                        }
                        break;
                }
                // -------------------------------------------
                // Help
                // -------------------------------------------
                case 'h':
                case '?':
                {
                        show_help();
                        break;
                }
                // -------------------------------------------
                // Default
                // -------------------------------------------
                default:
                {
                        break;
                }
                }
        }
        return 0;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t add_line(FILE *a_file_ptr, ns_hlx::host_list_t &a_host_list)
{
        char l_readline[MAX_READLINE_SIZE];
        while(fgets(l_readline, sizeof(l_readline), a_file_ptr))
        {
                size_t l_readline_len = strnlen(l_readline, MAX_READLINE_SIZE);
                if(MAX_READLINE_SIZE == l_readline_len)
                {
                        // line was truncated
                        // Bail out -reject lines longer than limit
                        // -host names ought not be too long
                        printf("Error: hostnames must be shorter than %d chars\n", MAX_READLINE_SIZE);
                        return STATUS_ERROR;
                }
                // read full line
                // Nuke endline
                l_readline[l_readline_len - 1] = '\0';
                std::string l_string(l_readline);
                l_string.erase( std::remove_if( l_string.begin(), l_string.end(), ::isspace ), l_string.end() );
                ns_hlx::host_t l_host;
                l_host.m_host = l_string;
                if(!l_string.empty())
                        a_host_list.push_back(l_host);
                //printf("READLINE: %s\n", l_readline);
        }
        return STATUS_OK;
}

//: ----------------------------------------------------------------------------
//: \details: Print the version.
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void print_version(FILE* a_stream, int a_exit_code)
{

        // print out the version information
        fprintf(a_stream, "hle HTTP Parallel Curl.\n");
        fprintf(a_stream, "Copyright (C) 2015 Verizon Digital Media.\n");
        fprintf(a_stream, "               Version: %d.%d.%d.%s\n",
                        HLE_VERSION_MAJOR,
                        HLE_VERSION_MINOR,
                        HLE_VERSION_MACRO,
                        HLE_VERSION_PATCH);
        exit(a_exit_code);

}

//: ----------------------------------------------------------------------------
//: \details: Print the command line help.
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void print_usage(FILE* a_stream, int a_exit_code)
{
        fprintf(a_stream, "Usage: hle -u [http[s]://]hostname[:port]/path [options]\n");
        fprintf(a_stream, "Options are:\n");
        fprintf(a_stream, "  -h, --help           Display this help and exit.\n");
        fprintf(a_stream, "  -V, --version        Display the version number and exit.\n");
        fprintf(a_stream, "  \n");
        fprintf(a_stream, "URL Options -or without parameter\n");
        fprintf(a_stream, "  -u, --url            URL -REQUIRED (unless running cli: see --cli option).\n");
        fprintf(a_stream, "  -d, --data           HTTP body data -supports bodies up to 8k.\n");
        fprintf(a_stream, "  \n");
        fprintf(a_stream, "Hostname Input Options -also STDIN:\n");
        fprintf(a_stream, "  -f, --host_file      Host name file.\n");
        fprintf(a_stream, "  -x, --execute        Script to execute to get host names.\n");
        fprintf(a_stream, "  \n");
        fprintf(a_stream, "Settings:\n");
        fprintf(a_stream, "  -p, --parallel       Num parallel.\n");
        fprintf(a_stream, "  -t, --threads        Number of parallel threads.\n");
        fprintf(a_stream, "  -H, --header         Request headers -can add multiple ie -H<> -H<>...\n");
        fprintf(a_stream, "  -X, --verb           Request command -HTTP verb to use -GET/PUT/etc\n");
        fprintf(a_stream, "  -T, --timeout        Timeout (seconds).\n");
        fprintf(a_stream, "  -R, --recv_buffer    Socket receive buffer size.\n");
        fprintf(a_stream, "  -S, --send_buffer    Socket send buffer size.\n");
        fprintf(a_stream, "  -D, --no_delay       Socket TCP no-delay.\n");
        fprintf(a_stream, "  -A, --ai_cache       Path to Address Info Cache (DNS lookup cache).\n");
        fprintf(a_stream, "  -C, --connect_only   Only connect -do not send request.\n");
        fprintf(a_stream, "  \n");
        fprintf(a_stream, "SSL Settings:\n");
        fprintf(a_stream, "  -y, --cipher         Cipher --see \"openssl ciphers\" for list.\n");
        fprintf(a_stream, "  -O, --ssl_options    SSL Options string.\n");
        // TODO
#if 0
        fprintf(a_stream, "  -K, --ssl_verify     Verify server certificate.\n");
        fprintf(a_stream, "  -N, --ssl_sni        Use SSL SNI.\n");
#endif
        fprintf(a_stream, "  -F, --ssl_ca_file    SSL CA File.\n");
        fprintf(a_stream, "  -L, --ssl_ca_path    SSL CA Path.\n");
        fprintf(a_stream, "  \n");
        fprintf(a_stream, "Command Line Client:\n");
        fprintf(a_stream, "  -I, --cli            Start interactive command line -URL not required.\n");
        fprintf(a_stream, "  \n");
        fprintf(a_stream, "Print Options:\n");
        fprintf(a_stream, "  -v, --verbose        Verbose logging\n");
        fprintf(a_stream, "  -c, --color          Color\n");
        fprintf(a_stream, "  -q, --quiet          Suppress output\n");
        fprintf(a_stream, "  -s, --show_progress  Show progress\n");
        fprintf(a_stream, "  -m, --show_summary   Show summary output\n");
        fprintf(a_stream, "  \n");
        fprintf(a_stream, "Output Options: -defaults to line delimited\n");
        fprintf(a_stream, "  -o, --output         File to write output to. Defaults to stdout\n");
        fprintf(a_stream, "  -l, --line_delimited Output <HOST> <RESPONSE BODY> per line\n");
        fprintf(a_stream, "  -j, --json           JSON { <HOST>: \"body\": <RESPONSE> ...\n");
        fprintf(a_stream, "  -P, --pretty         Pretty output\n");
        fprintf(a_stream, "  \n");
#ifdef ENABLE_PROFILER
        fprintf(a_stream, "Debug Options:\n");
        fprintf(a_stream, "  -G, --gprofile       Google profiler output file\n");
#endif
        fprintf(a_stream, "  \n");
        fprintf(a_stream, "Note: If running large jobs consider enabling tcp_tw_reuse -eg:\n");
        fprintf(a_stream, "echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse\n");
        fprintf(a_stream, "\n");
        exit(a_exit_code);
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int main(int argc, char** argv)
{
        settings_struct_t l_settings;
        ns_hlx::hlx *l_hlx = new ns_hlx::hlx();
        l_settings.m_hlx = l_hlx;

        // For sighandler
        g_settings = &l_settings;

        l_hlx->set_split_requests_by_thread(true);
        l_hlx->set_collect_stats(false);
        l_hlx->set_use_ai_cache(true);
        l_hlx->set_use_persistent_pool(false);

        // -------------------------------------------------
        // Subrequest settings
        // -------------------------------------------------
        ns_hlx::subreq *l_subreq = new ns_hlx::subreq("MY_COOL_ID");
        l_settings.m_subreq = l_subreq;

        // Turn on wildcarding by default
        l_subreq->m_multipath = false;
        l_subreq->m_type = ns_hlx::subreq::SUBREQ_TYPE_FANOUT;
        l_subreq->set_save_response(true);

        // Setup default headers before the user
        l_subreq->set_header("User-Agent", "EdgeCast Parallel Curl hle ");
        l_subreq->set_header("Accept", "*/*");
        //l_hlx->set_header("User-Agent", "ONGA_BONGA (╯°□°）╯︵ ┻━┻)");
        //l_hlx->set_header("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/33.0.1750.117 Safari/537.36");
        //l_hlx->set_header("x-select-backend", "self");
        //l_hlx->set_header("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
        //l_hlx->set_header("Accept-Encoding", "gzip,deflate");
        l_subreq->set_header("Connection", "keep-alive");
        l_subreq->set_completion_cb(s_completion_cb);
        l_subreq->set_keepalive(true);

        // -------------------------------------------
        // Get args...
        // -------------------------------------------
        char l_opt;
        std::string l_argument;
        int l_option_index = 0;
        struct option l_long_options[] =
                {
                { "help",           0, 0, 'h' },
                { "version",        0, 0, 'V' },
                { "url",            1, 0, 'u' },
                { "data",           1, 0, 'd' },
                { "host_file",      1, 0, 'f' },
                { "host_file_json", 1, 0, 'J' },
                { "execute",        1, 0, 'x' },
                { "parallel",       1, 0, 'p' },
                { "threads",        1, 0, 't' },
                { "header",         1, 0, 'H' },
                { "verb",           1, 0, 'X' },
                { "timeout",        1, 0, 'T' },
                { "recv_buffer",    1, 0, 'R' },
                { "send_buffer",    1, 0, 'S' },
                { "no_delay",       1, 0, 'D' },
                { "ai_cache",       1, 0, 'A' },
                { "connect_only",   0, 0, 'C' },
                { "cipher",         1, 0, 'y' },
                { "ssl_options",    1, 0, 'O' },
                { "ssl_verify",     0, 0, 'K' },
                { "ssl_sni",        0, 0, 'N' },
                { "ssl_ca_file",    1, 0, 'F' },
                { "ssl_ca_path",    1, 0, 'L' },
                { "cli",            0, 0, 'I' },
                { "verbose",        0, 0, 'v' },
                { "color",          0, 0, 'c' },
                { "quiet",          0, 0, 'q' },
                { "show_progress",  0, 0, 's' },
                { "show_summary",   0, 0, 'm' },
                { "output",         1, 0, 'o' },
                { "line_delimited", 0, 0, 'l' },
                { "json",           0, 0, 'j' },
                { "pretty",         0, 0, 'P' },
#ifdef ENABLE_PROFILER
                { "gprofile",       1, 0, 'G' },
#endif
                // list sentinel
                { 0, 0, 0, 0 }
        };
#ifdef ENABLE_PROFILER
        std::string l_gprof_file;
#endif
        std::string l_execute_line;
        std::string l_host_file_str;
        std::string l_host_file_json_str;
        std::string l_url;
        std::string l_ai_cache;
        std::string l_output_file = "";
        bool l_cli = false;

        // Defaults
        ns_hlx::subreq::output_type_t l_output_mode = ns_hlx::subreq::OUTPUT_JSON;
        int l_output_part =   ns_hlx::subreq::PART_HOST
                            | ns_hlx::subreq::PART_SERVER
                            | ns_hlx::subreq::PART_STATUS_CODE
                            | ns_hlx::subreq::PART_HEADERS
                            | ns_hlx::subreq::PART_BODY
                            ;
        bool l_output_pretty = false;

        // -------------------------------------------
        // Assume unspecified arg url...
        // TODO Unsure if good way to allow unspecified
        // arg...
        // -------------------------------------------
        bool is_opt = false;
        for(int i_arg = 1; i_arg < argc; ++i_arg) {

                if(argv[i_arg][0] == '-') {
                        // next arg is for the option
                        is_opt = true;
                }
                else if(argv[i_arg][0] != '-' && is_opt == false) {
                        // Stuff in url field...
                        l_url = std::string(argv[i_arg]);
                        //if(l_settings.m_verbose)
                        //{
                        //      printf("Found unspecified argument: %s --assuming url...\n", l_url.c_str());
                        //}
                        break;
                } else {
                        // reset option flag
                        is_opt = false;
                }
        }

        // -------------------------------------------------
        // Args...
        // -------------------------------------------------
#ifdef ENABLE_PROFILER
        char l_short_arg_list[] = "hVvu:d:f:J:x:y:O:KNF:L:Ip:t:H:X:T:R:S:DA:CRcqsmo:ljPG:";
#else
        char l_short_arg_list[] = "hVvu:d:f:J:x:y:O:KNF:L:Ip:t:H:X:T:R:S:DA:CRcqsmo:ljP";
#endif
        while ((l_opt = getopt_long_only(argc, argv, l_short_arg_list, l_long_options, &l_option_index)) != -1)
        {

                if (optarg)
                {
                        l_argument = std::string(optarg);
                }
                else
                {
                        l_argument.clear();
                }
                //printf("arg[%c=%d]: %s\n", l_opt, l_option_index, l_argument.c_str());

                switch (l_opt)
                {
                // ---------------------------------------
                // Help
                // ---------------------------------------
                case 'h':
                {
                        print_usage(stdout, 0);
                        break;
                }
                // ---------------------------------------
                // Version
                // ---------------------------------------
                case 'V':
                {
                        print_version(stdout, 0);
                        break;
                }
                // ---------------------------------------
                // URL
                // ---------------------------------------
                case 'u':
                {
                        l_url = l_argument;
                        break;
                }
                // ---------------------------------------
                // Data
                // ---------------------------------------
                case 'd':
                {
                        // TODO Size limits???
                        int32_t l_status;
                        // If a_data starts with @ assume file
                        if(l_argument[0] == '@')
                        {
                                l_status = read_file(l_argument.data() + 1, &(l_subreq->m_body_data), &(l_subreq->m_body_data_len));
                                if(l_status != 0)
                                {
                                        printf("Error reading body data from file: %s\n", l_argument.c_str() + 1);
                                        return -1;
                                }
                        }
                        else
                        {
                                l_subreq->m_body_data_len = l_argument.length() + 1;
                                l_subreq->m_body_data = (char *)malloc(sizeof(char)*l_subreq->m_body_data_len);
                                memcpy(l_subreq->m_body_data,l_argument.c_str(), l_subreq->m_body_data_len);
                        }

                        // Add content length
                        char l_len_str[64];
                        sprintf(l_len_str, "%u", l_subreq->m_body_data_len);
                        l_subreq->set_header("Content-Length", l_len_str);
                        break;
                }
                // ---------------------------------------
                // Host file
                // ---------------------------------------
                case 'f':
                {
                        l_host_file_str = l_argument;
                        break;
                }
                // ---------------------------------------
                // Host file JSON
                // ---------------------------------------
                case 'J':
                {
                        l_host_file_json_str = l_argument;
                        break;
                }
                // ---------------------------------------
                // Execute line
                // ---------------------------------------
                case 'x':
                {
                        l_execute_line = l_argument;
                        break;
                }
                // ---------------------------------------
                // cipher
                // ---------------------------------------
                case 'y':
                {
                        l_hlx->set_ssl_cipher_list(l_argument);
                        break;
                }
                // ---------------------------------------
                // ssl options
                // ---------------------------------------
                case 'O':
                {
                        int32_t l_status;
                        l_status = l_hlx->set_ssl_options(l_argument);
                        if(l_status != STATUS_OK)
                        {
                                return STATUS_ERROR;
                        }

                        break;
                }
                // ---------------------------------------
                // ssl verify
                // ---------------------------------------
                case 'K':
                {
                        // TODO
                        //l_hlx->set_ssl_verify(true);
                        break;
                }
                // ---------------------------------------
                // ssl sni
                // ---------------------------------------
                case 'N':
                {
                        // TODO
                        //l_hlx->set_ssl_sni_verify(true);
                        break;
                }
                // ---------------------------------------
                // ssl ca file
                // ---------------------------------------
                case 'F':
                {
                        l_hlx->set_ssl_ca_file(l_argument);
                        break;
                }
                // ---------------------------------------
                // ssl ca path
                // ---------------------------------------
                case 'L':
                {
                        l_hlx->set_ssl_ca_path(l_argument);
                        break;
                }
                // ---------------------------------------
                // cli
                // ---------------------------------------
                case 'I':
                {
                        l_settings.m_cli = true;
                        break;
                }
                // ---------------------------------------
                // parallel
                // ---------------------------------------
                case 'p':
                {
                        int l_num_parallel = 1;
                        //printf("arg: --parallel: %s\n", optarg);
                        //l_settings.m_start_type = START_PARALLEL;
                        l_num_parallel = atoi(optarg);
                        if (l_num_parallel < 1)
                        {
                                printf("Error parallel must be at least 1\n");
                                return -1;
                        }

                        l_hlx->set_num_parallel(l_num_parallel);
                        break;
                }
                // ---------------------------------------
                // num threads
                // ---------------------------------------
                case 't':
                {
                        int l_max_threads = 1;
                        //printf("arg: --threads: %s\n", l_argument.c_str());
                        l_max_threads = atoi(optarg);
                        if (l_max_threads < 0)
                        {
                                printf("Error num-threads must be 0 or greater\n");
                                return -1;
                        }
                        l_hlx->set_num_threads(l_max_threads);
                        break;
                }
                // ---------------------------------------
                // Header
                // ---------------------------------------
                case 'H':
                {
                        int32_t l_status;
                        l_status = l_subreq->set_header(l_argument);
                        if(l_status != HLX_SERVER_STATUS_OK)
                        {
                                printf("Error header string[%s] is malformed\n", l_argument.c_str());
                                return -1;
                        }
                        break;
                }
                // ---------------------------------------
                // Verb
                // ---------------------------------------
                case 'X':
                {
                        if(l_argument.length() > 64)
                        {
                                printf("Error verb string: %s too large try < 64 chars\n", l_argument.c_str());
                                return -1;
                        }
                        l_subreq->set_verb(l_argument);
                        break;
                }
                // ---------------------------------------
                // Timeout
                // ---------------------------------------
                case 'T':
                {
                        int l_timeout_s = -1;
                        //printf("arg: --threads: %s\n", l_argument.c_str());
                        l_timeout_s = atoi(optarg);
                        if (l_timeout_s < 1)
                        {
                                printf("Error connection timeout must be > 0\n");
                                return -1;
                        }
                        l_subreq->set_timeout_s(l_timeout_s);
                        break;
                }
                // ---------------------------------------
                // sock_opt_recv_buf_size
                // ---------------------------------------
                case 'R':
                {
                        int l_sock_opt_recv_buf_size = atoi(optarg);
                        // TODO Check value...
                        l_hlx->set_sock_opt_recv_buf_size(l_sock_opt_recv_buf_size);
                        break;
                }
                // ---------------------------------------
                // sock_opt_send_buf_size
                // ---------------------------------------
                case 'S':
                {
                        int l_sock_opt_send_buf_size = atoi(optarg);
                        // TODO Check value...
                        l_hlx->set_sock_opt_send_buf_size(l_sock_opt_send_buf_size);
                        break;
                }
                // ---------------------------------------
                // No delay
                // ---------------------------------------
                case 'D':
                {
                        l_hlx->set_sock_opt_no_delay(true);
                        break;
                }
                // ---------------------------------------
                // Address Info cache
                // ---------------------------------------
                case 'A':
                {
                        l_hlx->set_ai_cache(l_argument);
                        break;
                }
                // ---------------------------------------
                // connect only
                // ---------------------------------------
                case 'C':
                {
                        l_subreq->set_connect_only(true);
                        break;
                }
                // ---------------------------------------
                // verbose
                // ---------------------------------------
                case 'v':
                {
                        l_settings.m_verbose = true;
                        l_hlx->set_verbose(true);
                        break;
                }
                // ---------------------------------------
                // color
                // ---------------------------------------
                case 'c':
                {
                        l_settings.m_color = true;
                        l_hlx->set_color(true);
                        break;
                }
                // ---------------------------------------
                // quiet
                // ---------------------------------------
                case 'q':
                {
                        l_settings.m_quiet = true;
                        break;
                }
                // ---------------------------------------
                // show progress
                // ---------------------------------------
                case 's':
                {
                        l_settings.m_show_stats = true;
                        break;
                }
                // ---------------------------------------
                // show progress
                // ---------------------------------------
                case 'm':
                {
                        l_hlx->set_show_summary(true);
                        l_settings.m_show_summary = true;
                        break;
                }
                // ---------------------------------------
                // output file
                // ---------------------------------------
                case 'o':
                {
                        l_output_file = l_argument;
                        break;
                }
                // ---------------------------------------
                // line delimited
                // ---------------------------------------
                case 'l':
                {
                        l_output_mode = ns_hlx::subreq::OUTPUT_LINE_DELIMITED;
                        break;
                }
                // ---------------------------------------
                // json output
                // ---------------------------------------
                case 'j':
                {
                        l_output_mode = ns_hlx::subreq::OUTPUT_JSON;
                        break;
                }
                // ---------------------------------------
                // pretty output
                // ---------------------------------------
                case 'P':
                {
                        l_output_pretty = true;
                        break;
                }
#ifdef ENABLE_PROFILER
                // ---------------------------------------
                // Google Profiler Output File
                // ---------------------------------------
                case 'G':
                {
                        l_gprof_file = l_argument;
                        break;
                }
#endif
                // What???
                case '?':
                {
                        // Required argument was missing
                        // '?' is provided when the 3rd arg to getopt_long does not begin with a ':', and is preceeded
                        // by an automatic error message.
                        fprintf(stdout, "  Exiting.\n");
                        print_usage(stdout, -1);
                        break;
                }
                // Huh???
                default:
                {
                        fprintf(stdout, "Unrecognized option.\n");
                        print_usage(stdout, -1);
                        break;
                }
                }
        }

        // Check for required url argument
        if(l_url.empty() && !l_cli)
        {
                fprintf(stdout, "No URL specified.\n");
                print_usage(stdout, -1);
        }
        // else set url
        if(!l_url.empty())
        {
                l_subreq->init_with_url(l_url);
        }

        ns_hlx::host_list_t l_host_list;
        // -------------------------------------------------
        // Host list processing
        // -------------------------------------------------
        // Read from command
        if(!l_execute_line.empty())
        {
                FILE *fp;
                int32_t l_status = STATUS_OK;

                fp = popen(l_execute_line.c_str(), "r");
                // Error executing...
                if (fp == NULL)
                {
                }

                l_status = add_line(fp, l_host_list);
                if(STATUS_OK != l_status)
                {
                        return STATUS_ERROR;
                }

                l_status = pclose(fp);
                // Error reported by pclose()
                if (l_status == -1)
                {
                        printf("Error: performing pclose\n");
                        return STATUS_ERROR;
                }
                // Use macros described under wait() to inspect `status' in order
                // to determine success/failure of command executed by popen()
                else
                {
                }
        }
        // Read from file
        else if(!l_host_file_str.empty())
        {
                FILE * l_file;
                int32_t l_status = STATUS_OK;

                l_file = fopen(l_host_file_str.c_str(),"r");
                if (NULL == l_file)
                {
                        printf("Error opening file: %s.  Reason: %s\n", l_host_file_str.c_str(), strerror(errno));
                        return STATUS_ERROR;
                }

                l_status = add_line(l_file, l_host_list);
                if(STATUS_OK != l_status)
                {
                        return STATUS_ERROR;
                }

                //printf("ADD_FILE: DONE: %s\n", a_url_file.c_str());

                l_status = fclose(l_file);
                if (0 != l_status)
                {
                        printf("Error performing fclose.  Reason: %s\n", strerror(errno));
                        return STATUS_ERROR;
                }
        }
        else if(!l_host_file_json_str.empty())
        {
                // TODO Create a function to do all this mess
                // ---------------------------------------
                // Check is a file
                // TODO
                // ---------------------------------------
                struct stat l_stat;
                int32_t l_status = STATUS_OK;
                l_status = stat(l_host_file_json_str.c_str(), &l_stat);
                if(l_status != 0)
                {
                        printf("Error performing stat on file: %s.  Reason: %s\n", l_host_file_json_str.c_str(), strerror(errno));
                        return STATUS_ERROR;
                }

                // Check if is regular file
                if(!(l_stat.st_mode & S_IFREG))
                {
                        printf("Error opening file: %s.  Reason: is NOT a regular file\n", l_host_file_json_str.c_str());
                        return STATUS_ERROR;
                }

                // ---------------------------------------
                // Open file...
                // ---------------------------------------
                FILE * l_file;
                l_file = fopen(l_host_file_json_str.c_str(),"r");
                if (NULL == l_file)
                {
                        printf("Error opening file: %s.  Reason: %s\n", l_host_file_json_str.c_str(), strerror(errno));
                        return STATUS_ERROR;
                }

                // ---------------------------------------
                // Read in file...
                // ---------------------------------------
                int32_t l_size = l_stat.st_size;
                int32_t l_read_size;
                char *l_buf = (char *)malloc(sizeof(char)*l_size);
                l_read_size = fread(l_buf, 1, l_size, l_file);
                if(l_read_size != l_size)
                {
                        printf("Error performing fread.  Reason: %s [%d:%d]\n",
                                        strerror(errno), l_read_size, l_size);
                        return STATUS_ERROR;
                }
                std::string l_buf_str;
                l_buf_str.assign(l_buf, l_size);

                // NOTE: rapidjson assert's on errors -interestingly
                rapidjson::Document l_doc;
                l_doc.Parse(l_buf_str.c_str());
                if(!l_doc.IsArray())
                {
                        printf("Error reading json from file: %s.  Reason: data is not an array\n",
                                        l_host_file_json_str.c_str());
                        return STATUS_ERROR;
                }

                // rapidjson uses SizeType instead of size_t.
                for(rapidjson::SizeType i_record = 0; i_record < l_doc.Size(); ++i_record)
                {
                        if(!l_doc[i_record].IsObject())
                        {
                                printf("Error reading json from file: %s.  Reason: array member not an object\n",
                                                l_host_file_json_str.c_str());
                                return STATUS_ERROR;
                        }

                        ns_hlx::host_t l_host;

                        // "host" : "coolhost.com:443",
                        // "hostname" : "coolhost.com",
                        // "id" : "DE4D",
                        // "where" : "my_house"

                        if(l_doc[i_record].HasMember("host")) l_host.m_host = l_doc[i_record]["host"].GetString();
                        else l_host.m_host = "NO_HOST";

                        if(l_doc[i_record].HasMember("hostname")) l_host.m_hostname = l_doc[i_record]["hostname"].GetString();
                        else l_host.m_hostname = "NO_HOSTNAME";

                        if(l_doc[i_record].HasMember("id")) l_host.m_id = l_doc[i_record]["id"].GetString();
                        else l_host.m_id = "NO_ID";

                        if(l_doc[i_record].HasMember("where")) l_host.m_hostname = l_doc[i_record]["where"].GetString();
                        else l_host.m_where = "NO_WHERE";

                        l_host_list.push_back(l_host);
                }

                // ---------------------------------------
                // Close file...
                // ---------------------------------------
                l_status = fclose(l_file);
                if (STATUS_OK != l_status)
                {
                        printf("Error performing fclose.  Reason: %s\n", strerror(errno));
                        return STATUS_ERROR;
                }
        }
        // Read from stdin
        else
        {
                int32_t l_status = STATUS_OK;
                l_status = add_line(stdin, l_host_list);
                if(STATUS_OK != l_status)
                {
                        return STATUS_ERROR;
                }
        }

        if(l_settings.m_verbose)
        {
                printf("Showing hostname list:\n");
                for(ns_hlx::host_list_t::iterator i_host = l_host_list.begin(); i_host != l_host_list.end(); ++i_host)
                {
                        printf("%s\n", i_host->m_host.c_str());
                }
        }

        // -------------------------------------------
        // Sigint handler
        // -------------------------------------------
        if (signal(SIGINT, sig_handler) == SIG_ERR)
        {
                printf("Error: can't catch SIGINT\n");
                return -1;
        }
        // TODO???
        signal(SIGPIPE, SIG_IGN);

        // Initializer hlx
        int l_status = 0;

        // Set host list
        l_settings.m_total_reqs = l_host_list.size();
        l_status = l_subreq->set_host_list(l_host_list);
        if(l_status != HLX_SERVER_STATUS_OK)
        {
                printf("Error: performing set_host_list.\n");
                return -1;
        }

#ifdef ENABLE_PROFILER
        // Start Profiler
        if (!l_gprof_file.empty())
        {
                ProfilerStart(l_gprof_file.c_str());
        }
#endif

        // TODO Fix for 0 threads

        // run...
        l_status = l_hlx->run();
        if(HLX_SERVER_STATUS_OK != l_status)
        {
                return -1;
        }

        // -------------------------------------------
        // Run command exec
        // -------------------------------------------
        if(l_settings.m_cli)
        {
                l_status = command_exec_cli(l_settings);
                if(l_status != 0)
                {
                        return -1;
                }
        }
        else
        {
                l_status = command_exec(l_settings, true);
                if(l_status != 0)
                {
                        return -1;
                }
        }

        // Stop hlx
        l_hlx->stop();

        // wait for completion...
        l_hlx->wait_till_stopped();

#ifdef ENABLE_PROFILER
        if (!l_gprof_file.empty())
        {
                ProfilerStop();
        }
#endif

        //uint64_t l_end_time_ms = get_time_ms() - l_start_time_ms;

        // -------------------------------------------
        // Results...
        // -------------------------------------------
        if(!g_cancelled && !l_settings.m_quiet)
        {
                bool l_use_color = l_settings.m_color;
                if(!l_output_file.empty()) l_use_color = false;
                std::string l_responses_str;
                l_responses_str = l_subreq->dump_all_responses(l_use_color, l_output_pretty, l_output_mode, l_output_part);
                if(l_output_file.empty())
                {
                        printf("%s\n", l_responses_str.c_str());
                }
                else
                {
                        int32_t l_num_bytes_written = 0;
                        int32_t l_status = 0;
                        // Open
                        FILE *l_file_ptr = fopen(l_output_file.c_str(), "w+");
                        if(l_file_ptr == NULL)
                        {
                                printf("Error performing fopen. Reason: %s\n", strerror(errno));
                                return STATUS_ERROR;
                        }

                        // Write
                        l_num_bytes_written = fwrite(l_responses_str.c_str(), 1, l_responses_str.length(), l_file_ptr);
                        if(l_num_bytes_written != (int32_t)l_responses_str.length())
                        {
                                printf("Error performing fwrite. Reason: %s\n", strerror(errno));
                                fclose(l_file_ptr);
                                return STATUS_ERROR;
                        }

                        // Close
                        l_status = fclose(l_file_ptr);
                        if(l_status != 0)
                        {
                                printf("Error performing fclose. Reason: %s\n", strerror(errno));
                                return STATUS_ERROR;
                        }
                }
        }

        // -------------------------------------------
        // Summary...
        // -------------------------------------------
        if(l_settings.m_show_summary)
        {
                display_summary(l_settings, l_host_list.size());
        }

        // -------------------------------------------
        // Cleanup...
        // -------------------------------------------
        if(l_hlx)
        {
                delete l_hlx;
                l_hlx = NULL;
        }

        //if(l_settings.m_verbose)
        //{
        //      printf("Cleanup\n");
        //}

        return 0;

}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void display_summary(settings_struct_t &a_settings, uint32_t a_num_hosts)
{
        std::string l_header_str = "";
        std::string l_protocol_str = "";
        std::string l_cipher_str = "";
        std::string l_off_color = "";

        if(a_settings.m_color)
        {
                l_header_str = ANSI_COLOR_FG_CYAN;
                l_protocol_str = ANSI_COLOR_FG_GREEN;
                l_cipher_str = ANSI_COLOR_FG_YELLOW;
                l_off_color = ANSI_COLOR_OFF;
        }

        ns_hlx::summary_info_t l_summary_info;
        a_settings.m_hlx->get_summary_info(l_summary_info);
        printf("****************** %sSUMMARY%s ****************** \n", l_header_str.c_str(), l_off_color.c_str());
        printf("| total hosts:                     %u\n",a_num_hosts);
        printf("| success:                         %u\n",l_summary_info.m_success);
        printf("| error address lookup:            %u\n",l_summary_info.m_error_addr);
        printf("| error connectivity:              %u\n",l_summary_info.m_error_conn);
        printf("| error unknown:                   %u\n",l_summary_info.m_error_unknown);
        printf("| ssl error cert expired           %u\n",l_summary_info.m_ssl_error_expired);
        printf("| ssl error cert self-signed       %u\n",l_summary_info.m_ssl_error_self_signed);
        printf("| ssl error other                  %u\n",l_summary_info.m_ssl_error_other);

        // Sort
        typedef std::map<uint32_t, std::string> _sorted_map_t;
        _sorted_map_t l_sorted_map;
        printf("+--------------- %sSSL PROTOCOLS%s -------------- \n", l_protocol_str.c_str(), l_off_color.c_str());
        l_sorted_map.clear();
        for(ns_hlx::summary_map_t::iterator i_s = l_summary_info.m_ssl_protocols.begin(); i_s != l_summary_info.m_ssl_protocols.end(); ++i_s)
        l_sorted_map[i_s->second] = i_s->first;
        for(_sorted_map_t::reverse_iterator i_s = l_sorted_map.rbegin(); i_s != l_sorted_map.rend(); ++i_s)
        printf("| %-32s %u\n", i_s->second.c_str(), i_s->first);
        printf("+--------------- %sSSL CIPHERS%s ---------------- \n", l_cipher_str.c_str(), l_off_color.c_str());
        l_sorted_map.clear();
        for(ns_hlx::summary_map_t::iterator i_s = l_summary_info.m_ssl_ciphers.begin(); i_s != l_summary_info.m_ssl_ciphers.end(); ++i_s)
        l_sorted_map[i_s->second] = i_s->first;
        for(_sorted_map_t::reverse_iterator i_s = l_sorted_map.rbegin(); i_s != l_sorted_map.rend(); ++i_s)
        printf("| %-32s %u\n", i_s->second.c_str(), i_s->first);
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void display_status_line(settings_struct_t &a_settings)
{
        // -------------------------------------------------
        // Get results from clients
        // -------------------------------------------------

        // Get stats
        ns_hlx::t_stat_t l_total;
        a_settings.m_hlx->get_stats(l_total);
        uint32_t l_num_done = l_total.m_total_reqs;
        uint32_t l_num_resolved = l_total.m_num_resolved;
        uint32_t l_num_get = l_total.m_num_conn_started;
        uint32_t l_num_rx = a_settings.m_total_reqs;
        uint32_t l_num_error = l_total.m_num_errors;
        if(a_settings.m_color)
        {
                printf("Done/Req'd/Resolved/Total/Error %s%8u%s / %s%8u%s / %s%8u%s / %s%8u%s / %s%8u%s\n",
                                ANSI_COLOR_FG_GREEN, l_num_done, ANSI_COLOR_OFF,
                                ANSI_COLOR_FG_YELLOW, l_num_get, ANSI_COLOR_OFF,
                                ANSI_COLOR_FG_MAGENTA, l_num_resolved, ANSI_COLOR_OFF,
                                ANSI_COLOR_FG_BLUE, l_num_rx, ANSI_COLOR_OFF,
                                ANSI_COLOR_FG_RED, l_num_error, ANSI_COLOR_OFF);
        }
        else
        {
                printf("Done/Req'd/Resolved/Total/Error %8u / %8u / %8u / %8u / %8u\n",
                                l_num_done, l_num_get, l_num_resolved, l_num_rx, l_num_error);
        }
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t read_file(const char *a_file, char **a_buf, uint32_t *a_len)
{
        // Check is a file
        struct stat l_stat;
        int32_t l_status = STATUS_OK;
        l_status = stat(a_file, &l_stat);
        if(l_status != 0)
        {
                printf("Error performing stat on file: %s.  Reason: %s\n", a_file, strerror(errno));
                return -1;
        }

        // Check if is regular file
        if(!(l_stat.st_mode & S_IFREG))
        {
                printf("Error opening file: %s.  Reason: is NOT a regular file\n", a_file);
                return -1;
        }

        // Open file...
        FILE * l_file;
        l_file = fopen(a_file,"r");
        if (NULL == l_file)
        {
                printf("Error opening file: %s.  Reason: %s\n", a_file, strerror(errno));
                return -1;
        }

        // Read in file...
        int32_t l_size = l_stat.st_size;
        *a_buf = (char *)malloc(sizeof(char)*l_size);
        *a_len = l_size;
        int32_t l_read_size;
        l_read_size = fread(*a_buf, 1, l_size, l_file);
        if(l_read_size != l_size)
        {
                printf("Error performing fread.  Reason: %s [%d:%d]\n",
                                strerror(errno), l_read_size, l_size);
                return -1;
        }

        // Close file...
        l_status = fclose(l_file);
        if (STATUS_OK != l_status)
        {
                printf("Error performing fclose.  Reason: %s\n", strerror(errno));
                return -1;
        }
        return 0;
}

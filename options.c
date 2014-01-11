#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "twd.h"
#include "options.h"

/* This determines the client defaults */

TWD_Options_t default_client_options =
{

};

/* This determines the server defaults */

TWD_Options_t default_server_options =
{

};

/* This determines what the server will allow to be controlled via remote
   control */

TWD_Options_t default_remote_options =
{
  .verbose        = false;
  .up             = true;
  .dn             = true;
  .bidir          = true;
  .dontfork       = false;
  .help           = false;
  .randomize_data = false;
  .randomize_size = false;
  .passfail       = false;
  .ecn            = false;
  .ipv4           = true;
  .ipv6           = true;
  .server         = true;
  .multicast      = false;
  .test_owd       = true;
  .test_ecn       = true;
  .test_diffserv  = true;
  .test_tos       = true;
  .test_all       = false;
  .test_self      = false;
  .test_fq        = true;
  .test_bw        = false;
  .inetd          = false;	/* no command line option---set automatically */
  .debug          = 0;
  .packet_size    = TWD_DEFAULT_PACKET_SIZE;
  .tests          = 0 ;
  .length         = TWD_DEFAULT_DURATION;
  .interval       = TWD_DEFAULT_INTERVAL;
  .diffserv       = false ;
  .format         = 2; // json
  .logdir         = "/tmp/";
  .filename       = NULL;
  .hosts          = NULL;
  .server_address = NULL;
};

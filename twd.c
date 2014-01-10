/*
	Copyright (C) 2014 Michael D. Täht
 */

#define _GNU_SOURCE

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <locale.h>
#include <assert.h>
#include <getopt.h>
#include <iconv.h>
#include <malloc.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "dscp.h"
#include "twd.h"
#include "parse_addr.h"

sockaddr__u *g_hosts;
size_t       g_numhosts;

/************************************************************************/

int usage (char *err) {
  if(err) fprintf(stderr,"%s\n",err);
  printf("twd [options] addr [addr:port...]\n");
  printf("twd [options] -S [addr:port...]\n");
  printf(
	 "    -D --debug     X   debug output level\n"
	 "    -v --verbose       provide more verbose insight\n"
	 "    -h --help          this message\n"
	 "    -F --dontfork      remain in foreground\n"
	 "    -1 --up            test up direction only\n"
	 "    -2 --down          test down direction only\n"
	 "    -3 --bidir         test bidirectionally\n"
	 "    -4 --ipv4          test ipv4\n"
	 "    -6 --ipv6          test ipv6\n"
	 "    -r --random-data   use a random data size\n" 
	 "    -i --interval      interval [default 10ms]\n"
	 "    -L --logdir        log directory [default .]\n"
	 "    -l --length        length of test [default 300ms]\n"
	 "    -s --size          packet size [default 240 + headers] \n"
	 "    -S --server        run in server mode\n"
	 "    -P --passfail      report pass/fail only\n"
	 "    -e --ecn           enable ecn on all tests\n"
	 "    -d --diffserv      [value] \n"
	 "    -T --test-all      run all tests\n"
	 "    -t --test-all      run bw test\n"
	 "       --test-ecn      test for ecn\n"
	 "       --test-diffserv test diffserv\n"
	 "       --test-tos      test all tos bits\n"
	 "       --test-self     test various bits of self\n"
	 "    -Q --test-fq       test for presence of fair queuing\n"
	 "    -B --test-bw       test bandwidth\n"
	 "    -C --test-call     test call\n"
	 "    -o --output        [type]\n"
	 "    -w --write         [filename]\n"
  );


  exit(-1);
}

static const struct option long_options[] = {
  { "debug"		, required_argument	, NULL , 'D' } ,
  { "help"		, no_argument		, NULL , 'h' } ,
  { "interval"		, required_argument	, NULL , 'i' } ,
  { "output"		, required_argument	, NULL , 'o' } ,
  { "write"		, required_argument	, NULL , 'w' } ,
  { "random-data"	, no_argument		, NULL , 'r' } ,
  { "size"		, required_argument	, NULL , 's' } ,
  { "passfail"		, no_argument		, NULL , 'P' } ,
  { "verbose"		, no_argument		, NULL , 'v' } ,
  { "length"		, required_argument    	, NULL , 'l' } ,
  { "logdir"		, required_argument	, NULL , 'L' } ,
  { "server"		, required_argument	, NULL , 'S' } ,

  { "dontfork"		, no_argument		, NULL , 'F'  } ,
  { "up"		, no_argument		, NULL , '1' } ,
  { "down"		, no_argument		, NULL , '2' } ,
  { "bidir"		, no_argument		, NULL , '3' } ,
  { "ipv4"		, no_argument		, NULL , '4'  } ,
  { "ipv6"		, no_argument		, NULL , '6'  } ,
  { "ecn"		, no_argument		, NULL , 'e' } ,
  { "diffserv"		, required_argument	, NULL , 'd' } ,
  { "test"		, no_argument		, NULL , 't' } ,
  { "test-ecn"		, no_argument		, NULL , 'E' } ,
  { "test-diffserv"	, no_argument		, NULL , 'C' } ,
  { "test-self"		, no_argument		, NULL , '@' } ,
  { "test-fq"		, no_argument		, NULL , 'Q' } ,
  { "test-bw"		, no_argument		, NULL , 'B' } ,
  { "test-all"		, no_argument		, NULL , 'T' } ,

  { NULL		, 0			, NULL ,  0  }
};

#define penabled(a) if(o->a) fprintf(fp,"" # a " ")
#define penabledl(a) fprintf(fp,"" # a ":%llu ",(unsigned long long)o->a)
#define penabledd(a) fprintf(fp,"" # a ":%d ",o->a)
#define penableds(a) if(o->a != NULL) fprintf(fp,"" # a ":%s ",o->a)

int
print_enabled_options(TWD_Options_t *o, FILE *fp) {
  fprintf(fp,"Options: ");
  penabled(debug);
  penabled(server);
  penabled(test_owd);
  penabled(test_self);
  penabled(test_all);
  penabled(test_fq);
  penabled(test_bw);
  penabled(test_ecn);
  penabled(test_diffserv);
  penabled(ecn);
  penabled(up);
  penabled(dn);
  penabled(randomize_data);
  penabled(passfail);
  penabled(dontfork);
  penabled(ipv4);
  penabled(ipv6);
  penabled(inetd);
  fprintf(fp,"\nArgs:    ");
  fprintf(fp,"dscp:%s ",iptos2str(o->diffserv));

  penabledd(packet_size);
  penabledl(interval);
  penabledl(length);
  penabledd(format);

  penableds(filename);
  penableds(logdir);
  fprintf(fp,"\n");
  return 0;
}


#define QSTRING "Si:D:s:l:L:o:m:w:sh?Pv12346FeCTt"

int process_options(int argc, char **argv, TWD_Options_t *o)
{
  int	    option_index;
  int	    opt;
  
  option_index = 0;
  opt	    = 0;
  optind	 = 1;

  while(true)
  {
    opt = getopt_long(argc, argv,
			QSTRING,
			long_options, &option_index);
    if(opt == -1) break;

    switch (opt)
    {
	case 'S': 
	     o->server = 1;
	     break;
	     
	case 'D': o->debug = strtoul(optarg,NULL,10); break;
	case 'F': o->dontfork = 1; break;
	case 'h':
	case '?': usage(NULL); break;
	case 'i': o->interval = strtoul(optarg,NULL,10); break;
	case 's': o->packet_size = strtoul(optarg,NULL,10); break;
	case 'l': o->length = strtoul(optarg,NULL,10); break;
	case 'L': o->logdir = optarg; break;
	case 'o':
	     for(int i = 0; output_type[i].desc != NULL; i++)
	    if(strcmp(output_type[i].desc,optarg) == 0)
		o->format = output_type[i].id;
	     break;
	case 'P': o->passfail = 1; break;
	case 'v': o->verbose = 1; break;

	case '1': o->up = 1; break;
	case '2': o->dn = 1; break;
	case '3': o->up = 1; o->dn = 1; break;
	case '4': o->ipv4 = 1; break;
	case '6': o->ipv6 = 1; break;
	case 'w': o->filename = optarg; break; 
	case '@': o->test_self = 1; break; 
	case 't': o->test_owd = 1; break; 
	case 'E': o->test_ecn = 1; break; 
	case 'C': o->test_diffserv = 1; break; 
	case 'T': o->test_all = 1; break; 
	case 'Q': o->test_fq = 1; break; 
	case 'B': o->test_bw = 1; break; 
	case 'e': o->ecn = 1; break; 
	case 'd': o->diffserv = parse_ipqos(optarg); break; 

	default: fprintf(stderr,"%d",opt); usage("Invalid option"); break;
    }
  }

  return optind;
}

typedef struct tests_hosts TestHosts_t;

int finish_setup(TWD_Options_t *o,int idx,int argc,char **argv __attribute__((unused)))
{
  assert(o    != NULL);
  assert(idx  >  0);
  assert(idx  <= argc);
  assert(argv != NULL);
  
  g_numhosts = argc - idx;
  
  if (g_numhosts == 0)
  {
    socklen_t len;
    
    if (!o->server)
    {
      fprintf(stderr,"need at least one address\n");
      return EINVAL;
    }
    
    g_hosts = calloc(1,sizeof(sockaddr__u));
    if (g_hosts == NULL)
      return ENOMEM;
    
    g_numhosts = 1;
    
    len = sizeof(sockaddr__u);
    if (getsockname(STDIN_FILENO,&g_hosts[0].sa,&len) < 0)
    {
      int err = errno;
      fprintf(stderr,"If you are not running via [x]inetd, then you need at least one address\n");
      return err;
    }
    
    /*---------------------------------------------------------------------
    ; from here, we can check g_hosts[0].sa.sa_family to get the address
    ; family.  The --ipv4 and --ipv6 options really aren't necessary.  The
    ; inetd one probably is, since it flags that we already have a socket
    ; available for use, we don't have to create one.
    ;----------------------------------------------------------------------*/
  
    o->inetd = true;
  }
  else
  {
    g_hosts = calloc(idx,sizeof(sockaddr__u));
  
    if (g_hosts == NULL)
      return ENOMEM;
  
    for (int i = 0 ; idx < argc ; idx++ , i++)
    {
      int rc = to_addr_port(&g_hosts[i],argv[idx],TWDIP_any);
      if (rc != 0)
      {
        fprintf(stderr,"%s: %s\n",argv[idx],strerror(rc));
        return rc;
      }
    }
  }
  
  if(o->debug > 0) print_enabled_options(o, stderr);
  return 0;
}

static void default_options(TWD_Options_t *q) {
	q->interval = TWD_DEFAULT_INTERVAL;
	q->length = TWD_DEFAULT_DURATION;
	q->packet_size = TWD_DEFAULT_PACKET_SIZE;
}

int main(int argc, char **argv) {
  //int result;
  int host_count;
  TWD_Options_t options;

  memset(&options,0,sizeof(options));
  default_options(&options);

  host_count = process_options(argc,argv,&options);

  if (finish_setup(&options,host_count,argc,argv) != 0)
    return EXIT_FAILURE;

  for (size_t i = 0 ; i < g_numhosts ; i++)
  {
    char tipv4[INET_ADDRSTRLEN];
    char tipv6[INET6_ADDRSTRLEN];
    
    switch(g_hosts[i].sa.sa_family)
    {
      case AF_INET:
           printf(
           	"%3d: IPv4 %s:%d\n",
           	i,
           	inet_ntop(
           		AF_INET,
           		&g_hosts[i].sin.sin_addr.s_addr,
           		tipv4,
           		INET_ADDRSTRLEN
           	),
           	ntohs(g_hosts[i].sin.sin_port)
           );
           break;
           
      case AF_INET6:
           printf(
           	"%3d: IPv6 [%s]:%d\n",
           	i,
           	inet_ntop(
           		AF_INET6,
           		&g_hosts[i].sin6.sin6_addr.s6_addr,
           		tipv6,
           		INET6_ADDRSTRLEN
           	),
           	ntohs(g_hosts[i].sin6.sin6_port)
           );
           break;
           
      default:
           assert(0);
           break;
    }
  }
  
  return(0);
}

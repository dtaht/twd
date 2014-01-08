/*
Copyright (C) 2014 Michael D. Täht
 */

#define _GNU_SOURCE

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

#include "twd.h"

int usage (char *err) {
  if(err) fprintf(stderr,"%s\n",err);
  printf("twd [options] addr [addr:port...]\n");
  printf("twd [options] -s [addr:port...]\n");
  printf(
	 "    -D --debug     X   debug output level\n"
	 "    -v --verbose       provide more verbose insight\n"
	 "    -h --help          this message\n"
	 "    -F --dontfork      remain in foreground\n"
	 "    -1 --up            test up direction only\n"
	 "    -2 --down          test down direction only\n"
	 "    -3 --bidir         test bidirectionally\n"
	 "    -4 --ipv4          listen on/use ipv4\n"
	 "    -6 --ipv6          listen on/use ipv6\n"
	 "    -m --multicast     use multicast\n"
	 "    -r --random-data   use a random data size\n" 
	 "    -i --interval      interval [default 10ms]\n"
	 "    -L --logdir        log directory [default .]\n"
	 "    -l --length        length of test [default 300ms]\n"
	 "    -s --size          packet size [default 240 + headers] \n"
	 "    -S --server        server mode\n"
	 "    -P --passfail      report pass/fail only\n"
	 "    -e --ecn           enable ecn on all tests\n"
	 "    -d --diffserv      [value] \n"
	 "    -T --test-all      run all tests\n"
	 "    -t --test-all      run bw test\n"
	 "       --test-ecn      test for ecn\n"
	 "       --test-diffserv test diffserv\n"
	 "       --test-tos      test all tos bits\n"
	 "       --test-self     test various bits of self\n"
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
  { "server"		, no_argument		, NULL , 'S' } ,

  { "dontfork"		, no_argument		, NULL , 'F'  } ,
  { "up"		, no_argument		, NULL , '1' } ,
  { "down"		, no_argument		, NULL , '2' } ,
  { "bidir"		, no_argument		, NULL , '3' } ,
  { "ipv4"		, no_argument		, NULL , '4' } ,
  { "ipv6"		, no_argument		, NULL , '6' } ,
  { "multicast"		, no_argument		, NULL , 'm' } ,
  { "ecn"		, no_argument		, NULL , 'e' } ,
  { "test"		, no_argument		, NULL , 't' } ,
  { "test-ecn"		, no_argument		, NULL , 'E' } ,
  { "test-diffserv"	, no_argument		, NULL , 'C' } ,
  { "test-self"		, no_argument		, NULL , '@' } ,
  { "test-all"		, no_argument		, NULL , 'T' } ,

  { NULL		, 0			, NULL ,  0  }
};

#define penabled(a) if(o->a) fprintf(fp,"" # a " ")
#define penabledd(a) fprintf(fp,"" # a ":%d ",o->a)
#define penableds(a) if(o->a != NULL) fprintf(fp,"" # a ":%s ",o->a)

int
print_enabled_options(TWD_Options_t *o, FILE *fp) {
  fprintf(fp,"Options: ");
  penabled(multicast);
  penabled(ipv4);
  penabled(ipv6);
  penabled(debug);
  penabled(server);
  penabled(test_owd);
  penabled(test_self);
  penabled(test_all);
  penabled(test_ecn);
  penabled(test_diffserv);
  penabled(ecn);
  penabled(up);
  penabled(dn);
  penabled(randomize_data);
  penabled(passfail);
  penabled(dontfork);

  fprintf(fp,"\nArgs:    ");
  fprintf(fp,"dscp:%s ",iptos2str(o->diffserv));

  penabledd(packet_size);
  penabledd(interval);
  penabledd(length);
  penabledd(format);

  penableds(filename);
  penableds(logdir);
  fprintf(fp,"\n");
  return 0;
}

#define QSTRING "i:D:s:l:L:o:m:w:sh?Pv12346FeCTt"

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
	case 'S': o->server = 1;  break;
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
	case 'm': o->multicast = 1; break;
	case 'w': o->filename = optarg; break; 
	case '@': o->test_self = 1; break; 
	case 't': o->test_owd = 1; break; 
	case 'E': o->test_ecn = 1; break; 
	case 'C': o->test_diffserv = 1; break; 
	case 'T': o->test_all = 1; break; 
	case 'e': o->ecn = 1; break; 
	case 'd': o->diffserv = parse_ipqos(optarg); break; 

	default: fprintf(stderr,"%d",opt); usage("Invalid option"); break;
    }
  }

  return optind;
}

struct tests_hosts {
  char hostname[255];
};

typedef struct tests_hosts TestHosts_t;

int finish_setup(TWD_Options_t *o,int idx,int argc,char **argv __attribute__((unused)))
{
  // char    string[MAX_MTU];
  // TestHosts_t *hosts_under_tests = (TestHosts_t *) 
    calloc(1,(idx + 1) * sizeof(TestHosts_t));
  for(int i = idx; i < argc; i++)
  {
     //    size_t  len = strlen(argv[i]);
    
  }
  if(o->debug > 0) print_enabled_options(o, stderr);
  return 0;
}

static void default_options(TWD_Options_t *q) {
	q->interval = 1000*1000*10; // 10ms
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

  return(0);
}

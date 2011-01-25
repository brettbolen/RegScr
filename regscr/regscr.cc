#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>


#define VERSION "v1.1"

// todo:
//   change from scanf to strtok_r for cmd parsing.
//      accept commas and spaces
//   research whole line for "xx" or '..', print it out.


using namespace std;

// data
int                                 reg_fd;
unsigned long long                  vmap_size;
unsigned long long                  vmap_base = 0;
unsigned char                      *vmap_ptr;
map<string, unsigned long long int> registers;

int do_io = 1;
int check = 0;


/***************************
 * read register dictionary
 */
void parse_regs(const char *fname)
{
    //   char line[255];
    string line;
    char reg_name[100];
    char reg_val[100];
    unsigned long long int  val;
    ifstream datfile(fname, ios::in);
    if (datfile.is_open()) {
	while (!datfile.eof()) {
	    getline( datfile, line);
	    const char *s = line.c_str();
	    if (*s != '#') {
		sscanf(line.c_str(), "%s %s", reg_name, reg_val);
		//printf("Adding to dictionary:  "
		//       "%s -> %s \n", reg_name, reg_val);
		registers[ reg_name] =  strtoll( reg_val, NULL, 0);
	    }
	}
    }
}


/*******************************
 * print register dictionary
 */
void print_regs()
{
    char line[255];
    for( map<string,unsigned long long int>::iterator ii=registers.begin();
	 ii!=registers.end();
	 ++ii)
    {
	    printf("0x%llx => register['%s']\n", (*ii).second,
		   (*ii).first.c_str());
    }
}

/*******************************
 *
 */
unsigned long long lookup_register( char * arg) {
    unsigned long long val = 0;
    if ( registers.find(arg) == registers.end()) {
	val  = strtoll(arg, NULL, 0);
	if (!do_io && val==0 && check) {
	    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
		   "Register '%s' not found, using value=0x%llx\n",
		   arg, val);
	}
    }
    else {
	val = registers[arg];
    }
    return val;
}
/*******************************
 *
 */
int open_regs() {
    int rc = 0;
    reg_fd = open("/dev/mem", O_RDWR);
    if (reg_fd == -1) {
	rc = -1;
	perror("open");
	goto exit;
    }

   
    unsigned long long                  psz;
    psz = getpagesize();
    vmap_base = 0;
    vmap_ptr  = NULL;
    vmap_size = 2*psz;   // two pages
   
    //printf("open_regs: pagesize is 0x%llx ( %ld), vmap_size=0x%llx\n",
    //       psz, psz,
    //       vmap_size);
 exit:
    return rc;
   
}

/*******************************
 *
 */
int close_regs() {
    if (vmap_ptr) {
	munmap(vmap_ptr, vmap_size);
    }
    close(reg_fd);
    return 0;
}


/*******************************
 *
 */
void map_regs( unsigned long long reg_addr)
{
    unsigned long long new_vmap_base;

    new_vmap_base = reg_addr & ~(vmap_size -1);
   
    //   printf("!!xx map_regs(0x%llx):  entry vmap_base=0x%llx, vmap_size=0x%llx ==> new_vmap_base=0x%llx\n",
    //	  reg_addr, vmap_base, vmap_size, new_vmap_base);
   
    if ( new_vmap_base != vmap_base) {    // base changed, remap 
      
	if (vmap_ptr) {
	    munmap(vmap_ptr, vmap_size);
	}

      
      
      
	vmap_base = new_vmap_base;
	vmap_ptr = (unsigned char*) mmap64(NULL,   vmap_size,
					   PROT_READ | PROT_WRITE, MAP_SHARED,
					   reg_fd, vmap_base);
	if (vmap_ptr == MAP_FAILED) {
	    //	 printf("!!xx map_regs():  mmap FAILURE\n");
	    perror("mmap");
	}
	else {
	    //printf("!!xx map_regs(0x%llx): remap vmap_base=0x%llx\n",
	    //		reg_addr, vmap_base);
	 
	}
    }
 exit:
    return;
}

      
      
      
      
/*******************************
 *
 */
int read_register(unsigned long long  reg_base,
		  unsigned long long  reg_offset,
		  uint32_t           *val)
{
    int rc = 0;
    unsigned long long reg_addr;
    volatile uint32_t *pu32;
    unsigned long vmap_off;
   
    reg_addr = reg_base + reg_offset;
    map_regs( reg_addr);
    vmap_off = (reg_addr - vmap_base);
    pu32 = (volatile uint32_t *) (vmap_ptr + vmap_off);

   

#ifdef __PPC64__
    asm volatile("sync");
#endif

   
    *val = *pu32;

    //   printf("!! read_register( addr=0x%llx, val=0x%x), base=0x%llx, offset=0x%llx\n",
    //   	  reg_addr, *val, vmap_base, vmap_off);
   
   
}


/*******************************
 *
 */
int   write_register(unsigned long long  reg_base,
		     unsigned long long  reg_offset,
		     uint32_t            val)
{
    int rc = 0;
    unsigned long long reg_addr;
    volatile uint32_t *pu32;
    unsigned long vmap_off;
   
    reg_addr = reg_base + reg_offset;
    map_regs( reg_addr);
    vmap_off = (reg_addr - vmap_base);

    pu32 = (volatile uint32_t *) (vmap_ptr + vmap_off);

    //   printf("!! write_register( addr=0x%llx, val=0x%x), base=0x%llx, offset=0x%llx\n",
    //   	  reg_addr, val, vmap_base, vmap_off);
   
   
    *pu32 = val;

#ifdef __PPC64__
    asm volatile("sync");
#endif
   
	       
 exit:
    return rc;
}



/*******************************
 *
 */
int parse_cmds(char *fname)
{
    string line;
    int    line_no = 0;
    char   cmd[255];
    char   arg0[255];
    char   arg1[255];
    char   arg2[255];
    char   arg3[255];
    char   arg4[255];
    uint32_t val = 0;
    ifstream datfile(fname, ios::in);
    if (datfile.is_open()) {
	while (!datfile.eof()) {
	    getline( datfile, line);
	    line_no ++;
	    cmd[0] = arg0[0] = arg1[0] = arg2[0] = arg3[0] =
		arg4[0] = 0;

	    // todo - fix tokenizer using strtok_r -- great example on man page
	    //char   c_line[255];
	    //char   c_line_cmd[255];
	    //char  *c_line_saveptr;
	    //sprintf(c_line, "%s",line.c_str());
	    //c_line_p = c_line;
	    //c_line_cmd = strtok(c_line_p, " ");
	    

	    //                     c  0  1  2  3
	    sscanf(line.c_str(), "%s %s %s %s %s %s\n",
		   cmd, arg0, arg1, arg2, arg3, arg4 );

	    if(0) {
		printf("parse '%s'\n", line.c_str());
		printf(" to cmd='%s', arg0='%s', arg1='%s',"
		       " arg2='%s', arg3='%s', arg4='%s'\n",
		       cmd, arg0, arg1, arg2, arg3, arg4);
	    }
		    
	    // parse commands
	    //     # or / - comment -- ignore
	    //     read  - read memory  ( base+offset)
	    //     write - write memory ( base+offset)
	    //     print - print text line
	    //     sleep - sleep ( microseconds)
	    //     dump  - dump memory
	    //     table - print register table


	    //	 if ( cmd[0] == '#') {
	    //	    printf("got pound\n");
	    //	 }

	    if ((cmd[0] == '#')  || (cmd[0] == '/') ||
		(cmd[0] == '\\') || (cmd[0] == ' ') || 
		(cmd[0] == 0)    || (cmd[0] == 13)) {   // ignore
	    }
	    else if (( cmd[0] ==  'r') || ( cmd[0] ==  'R')) {  // read memory
		unsigned long long int base = lookup_register(arg0);
		unsigned long long int ofs  = lookup_register(arg1);
		if (base == 0) {
		    fprintf(stderr,
			    "#############1 line: %d : bad base address \n",
			    line_no);
		}
		else {
		    if ( do_io)
			read_register(base, ofs, &val);

		    
		    printf("Read  [0x%012llx + 0x%08llx]     returns"
			   " 0x%04llx_%04llx :",
			   (unsigned long long ) base,
			   (unsigned long long ) ofs,
			   (val >> 16) & 0xffffLL,
			   (val & 0xffffLL));
		    printf("\t%s\n", arg2);
		}
	    }
	    else if (( cmd[0]==  'w') || ( cmd[0]==  'W')) {  // write to memory
		
		unsigned long long int base = lookup_register(arg0);
		unsigned long long int ofs  = lookup_register(arg1);
		unsigned long long int val  = lookup_register(arg2);

		if (base == 0) {
		    fprintf(stderr,
			    "#############2 bad base address at line %d\n",
			    line_no);
		}
		else {
		    if ( do_io) {
			write_register(base, ofs, val);
		    }
			
		    printf("Write [0x%012llx + 0x%08llx] with "
			   "val =  0x%04llx_%04llx :",
			   (unsigned long long ) base,
			   (unsigned long long ) ofs,
			   (val >> 16) & 0xffffLL,
			   (val & 0xffffLL));
		    printf("\t%s\n", arg3);
		}
	    }
	    else if ((( cmd[0]==  's') || ( cmd[0]==  'S')) && // sleep n microseconds
		     (( cmd[1]==  'l') || ( cmd[1]==  'l')))
	    { 
		int val = strtol(arg0, NULL, 0);
		if (val > 5* 1000*1000) val == 5*1000*1000;    // max of 5 seconds
		printf("Sleep %d microseconds\n", val);
		usleep(val);
	    }
	    else if ((( cmd[0]=='s') || ( cmd[0]=='S')) &&     // system cmd
		     (( cmd[1]=='y') || ( cmd[1]=='Y')))
            {
		char sys_cmd[255];
		char read_line[255];
		char *lp = read_line;
		sprintf(read_line, line.c_str());
		lp+=2;  // skip two chars;
		printf("system cmd: '%s'\n", lp);
		system(lp);
	    }
	    else if (( cmd[0]==  'p') || ( cmd[0]==  'P')) {   // print
		printf("##### '%s'  ##### \n", line.c_str());
	    }
	    else if (( cmd[0]==  'd') || ( cmd[0]==  'D')) {  // Dump  base offset num_qwords
		unsigned long long int base = lookup_register(arg0);
		unsigned long long int ofs  = lookup_register(arg1);
		unsigned long long int len  = lookup_register(arg2);
		if (len > 4096) len = 4096;
		if (base == 0) {
		    fprintf(stderr,
			    "#############3 line: %d : bad base address \n",
			    line_no);
		}
		else {

		    printf("dump:  [0x%08llx + 0x%08llx, len=0x%llx ] ------ '%s' \n",
			   base, ofs, len, arg3);


		    // read data
		    int i;
		    unsigned int vals[4096+4] = {0};
		    if ( do_io) {
			for (i = 0; i < len; i+=4) {
			    read_register(base,
					  ofs+i,
					  &vals[i/4]);
			}
		    }
		    else {
			len = 0;
		    }

		    // print data
		    for (i = 0; i < len; i+=4) {
			// start of a new line
			if ((i != 0) && (i % 16) == 0) {
			    printf("\n");
			}
			// addr
			if ((i % 16) == 0) {
			    unsigned int v1 = i+ofs;
			    printf("  0x%04lx_%04lx: ",
				   (v1 >> 16), (v1 & 0xffff));
			}
			// data
			printf("%04lx_%04lx ",
			       (vals[i/4] >> 16),
			       (vals[i/4] & 0xffff));

			// ascii data  todo:
			    
		    }
		    printf("\n\n");
		    
		}
		 
	    }
	    else if (( cmd[0]==  'z') || ( cmd[0]==  'Z')) {   // Zero
	    }
	    else if (( cmd[0]==  'f') || ( cmd[0]==  'F')) {  // Fill
	    }
	    else if (( cmd[0]==  't') || ( cmd[0]==  'T')) {   // print dictionary
		printf("#################### register dictionary #################\n");
		print_regs();
		printf("..........................................................\n");
	    }
	    else if (( cmd[0]==  'q') || ( cmd[0]==  'Q')) {   // quit
		printf("..................... Processing Quit.....................\n");
		close_regs();
		exit(0);
	    }
	    else {
		fprintf(stderr,"!!! bad line %d: cmd=%d/'%c' : '%s'\n", line_no, (int) cmd[0], cmd[0], line.c_str());
	    }
	 
	    fflush(stdout);
	}
    }
}





/*******************************
 *
 */
int main(int argc, char *argv[])
{
    int rc = 0;
    char *cmd_file;

    if ( argc == 1 ) {
	printf("usage:   regscr fname <do_io> <check>\n"
	       "                fname - name of script to run\n"
	       "                do_io - 0 to disable register writing\n"
	       "                check - check register lookups\n\n"
	       "");
	exit(-1);
    }
   

    if (argc > 1) {
	cmd_file = argv[1];
    }

    if ( argc> 2) {
	do_io = atoi( argv[2]);
    }
   
    if ( argc> 3) {
	check  =  atoi( argv[3]);
	if ( check) do_io = 0;
    }
   
   

    fprintf(stderr, "Register script processor ver %s\n",VERSION);
    if ( !do_io) {
	fprintf(stderr, "   diags only -- no Reads or Writes\n");
    }
    if ( check) {
	fprintf(stderr, "   check dictionary lookups\n");
    }

   
    char home_defs[255];
    sprintf(home_defs,"%s/.regscr.dat",getenv("HOME"));
    parse_regs(home_defs);
    parse_regs("regscr.dat");
    parse_regs(".regscr.dat");

    open_regs();
    parse_cmds(cmd_file);
    close_regs();


 exit:
    return rc;
}

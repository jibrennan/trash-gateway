/*
 * trash-display.cxx V1.00
 * 
 * This program is for exploring how to interact with a Matrix Orbital GLK19264-7T-1U display 
 * Intended to be used only as a reference for other projects. Be smart and build a driver for LCDPROC.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

//TODO: implement syslog, and rotate the screen text to prevent burn in, add support for wi-fi adapters

#include <iostream> 
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
//for the serial port
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
//for the ethernet port
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

// serial port
struct termios options;
// functions
static int open_logfile();
static int open_port();
static int setup();
static int printaddress();
static int close_port();
static int beep();
static int check_service();
static void signal_handler(int);
void *threadproc(void *arg);
// IO
int fd;    // Serial port 
int fd2;   // IP interface
FILE *fp;  // Config file
FILE *fp2; // Logfile
// timer threads
pthread_t tid; 
// global
int status, service;
char* address;
char user_command, read_buffer [256];
// GLK19264-7T-1U display commands, number of bytes, description
uint8_t clear         []={0xFE, 0x58};  // 2 clears the screen
uint8_t backlightOff  []={0xFE, 0x46};  // 2 turns off the backlight, but just the display, not the keypad
uint8_t backlightOn   []={0xFE, 0x99, 0xFE};  // 3 where the third byte is the lenght of time in minutes
uint8_t info          []={0xFE, 0x47, 0x16, 0x08, 0x49, 0x4E, 0x46, 0x4F, 0x2D, 0x3E};  // 10 print "back" on the screen
uint8_t back          []={0xFE, 0x47, 0x16, 0x08, 0x42, 0x41, 0x43, 0x4B, 0x2D, 0x3E};  // 10 print "back" on the screen
uint8_t IPstring      []={0xFE, 0x47, 0x01, 0x01, 0x49, 0x50, 0x3A, 0x20};  // 8 print "IP: " on the screen
uint8_t homescreen    []={0xFE, 0x62, 0x01, 0x00, 0x00, 0x00};  // 6 show the homescreen, or bitmap position 1
uint8_t servicescreen []={0xFE, 0x62, 0x01, 0x00, 0x00, 0x00};  // 6 the third byte is the bitmap postion, 1 is loaded by default 
uint8_t clearLEDs     []={0xFE, 0x57, 0x01, 0xFE, 0x57, 0x02, 0xFE, 0x57, 0x03, 0xFE, 0x57, 0x04, 0xFE, 0x57, 0x05, 0xFE, 0x57, 0x06}; //18
uint8_t home          []={0xFE, 0x47, 0x01, 0x01};  // 4  set the cursor position to home
uint8_t animationPlay []={0xFE, 0x36, 0xFE, 0xC2, 0x01, 0x01};  // 6 play an animation on the screen, where the 5th byte is the animation number and the 6th is 1 for start or 0 for stop
uint8_t animationStop []={0xFE, 0x36, 0xFE, 0xC2, 0x01, 0x00};  // 6 play an animation on the screen, where the 5th byte is the animation number and the 6th is 1 for start or 0 for stop
uint8_t beepcommand   []={0xFE, 0xBB, 0x00, 0x03, 0xE8, 0x00};  // 6 
uint8_t okLED         []={0xFE, 0x57, 0x01, 0xFE, 0x56, 0x02};  // 6 LED 1.1 green 
uint8_t okClearLED    []={0xFE, 0x57, 0x01, 0xFE, 0x57, 0x02};  // 6 LED 1.2 off
uint8_t statusLED     []={0xFE, 0x56, 0x03, 0xFE, 0x56, 0x04};  // 6 LED 2.1 amber
uint8_t stsClearLED   []={0xFE, 0x57, 0x03, 0xFE, 0x57, 0x04};  // 6 LED 2.2 off
uint8_t errLED        []={0xFE, 0x56, 0x05, 0xFE, 0x57, 0x06};  // 6 LED 3.1 red
uint8_t errClearLED   []={0xFE, 0x57, 0x05, 0xFE, 0x57, 0x06};  // 6 LED 3.2 off
uint8_t versionInfo   []={0xFE, 0x47, 0x01, 0x08, 0x56, 0x31, 0x2E, 0x30, 0x30};  // 9 print the version info on the screen 1st column, 8th row, Vx.xx

int main(int argc, char **argv)
{
	printf("[display control] starting display control service... \n");
	setup();
	signal(SIGINT, signal_handler);
	usleep(1000);

	while (1)                               
	{
		read(fd, &user_command, 1);      // read from fd.
	    switch (user_command)
	    {
	    case 'A':                        // "Menu" button. 
		     beep();           
		     break;
		case 'G':                        // "BACK" button.  
		     printaddress();               
		     break;
		case 'B':                        // "UP" button.  
		     beep();             
		     break;     
		case 'H':                        // "DOWN" button.  
		     beep();              
		     break;
		case 'D':                        // "LEFT" button.  
		     beep();              
		     break;
		case 'C':                        // "RIGHT" button.  
		     beep();              
		     break;
		case 'E':                        // "CENTER" button.  
		     beep();       
		     break;  
		}
	}
	return 0;
}

//startup
int setup(void) 
{
	open_logfile();
	open_port();
	write(fd, backlightOn,3);
	write(fd, clearLEDs, 18);
	usleep(1000);  
	write(fd, clear, 2); 
	service = check_service();
	servicescreen[2] = uint8_t(service);
	write(fd, servicescreen, 6);
	write(fd, versionInfo, 9);
	write(fd, info, 10);    
	write(fd, okLED, 6);
    pthread_create(&tid, NULL, &threadproc, NULL);
    beep();
	return 0;
}

// SIGINT handler
void signal_handler(int signum)
{
	if (signum == SIGINT)
	{
		printf("\nCaught Ctrl+c, closing serial port.\n");
		if (fd >=0)
		{
			close_port();
		}
		exit(-1);
	}
}

// thread for blinking the status LEDs
void *threadproc(void *arg) 
{
    while(1)
    {
        sleep(5);
        write(fd, statusLED, 6);
        usleep(80000);
        write(fd, stsClearLED, 6);
    }
    return 0;
}

// open the serial port
int open_port(void) 
{
	fd = open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NDELAY);  //ttyS1 is the internal port the displays are connected to / well be reading and writing, is not the controling terminal and w are not usign the DCD signaling lines. 
	if (fd == -1)
	{
			fprintf(fp2, "[display control] Could not open the Port... \n");  // to do later: log event with code and syslog
			fflush(fp2);
	}
	else 
	{
		fcntl(fd, F_SETFL, 0);  // sets the file status flag with blocking enabled "0" - we're talking to a device that just sends single characters, so the VMIN and VTIME may be useful. When talking with anotehr computer or a modem, then use FNDELAY 
	}
	tcgetattr(fd, &options);              // get: the current options 
	cfsetispeed(&options, B19200);        // set: baudrate
	cfsetospeed(&options, B19200);        // set: baudrate
	options.c_cflag |= (CLOCAL | CREAD);  // enable: local control and read capability. 
	options.c_cflag &= ~PARENB;           // disable: parity
	options.c_cflag |= CSTOPB;            // enable: cch/displays requires two stop bits
	options.c_cflag &= ~CSIZE;            // disable: no bit mask
	options.c_cflag |= CS8;               // 8 data bits
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //disable: all of this shit when talking with an embedded device. Need raw data, unprocessed, no extra stuff to worry about.  
	options.c_cc[VMIN] = 1;               // enable: blocking until reading at least one character from the buffer aka: display keyboard.  
	tcsetattr(fd, TCSANOW, &options);     // set: the new options
	return (fd);  // return the status of FD, use it for later. start/stop/retry/whatever.
}

// print the IP address 
// TODO: separate out getting the address/s and printing stuff on the screen. 
int printaddress() 
{
	// ip interface
	struct ifreq ifr;                             // we're going to be using the ifreq strucure for IPV4, and we'll call it "ifr" 
	fd2 = socket(AF_INET, SOCK_DGRAM,0);          // we need a socket for this, so lets give it a name of fd2, and say that our protocol will be IPV4, and that socket will be using UDP, with no flags specified. 
	ifr.ifr_addr.sa_family = AF_INET;             // in our structure set the ip format to IPV4
	strncpy(ifr.ifr_name, "enp2s0", IFNAMSIZ-1);  // in our structure, set the name of the socket to the actual name of the port we want to look at. This is specific to the hardware it's running on which is perfect for our needs. 
	ioctl(fd2, SIOCGIFADDR, &ifr);                // G as in GET the IPV4 info (32bit binary) and stick it in our ifr structure. 
	close(fd2);                                   // we're now done with the port for this task and can close it, if we want to open it again we can just open fd2 without all this mess.  
	address = (inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));  // we have some information in our ifr structure now, so we just want to extract the IP ADDRESS from that information and turn the 32 bytes into a string in dotted decimal notation. 
	// display
	write(fd, clear, 2);                          // clear the screen
	write(fd, home, 4);                           // set the home position of the cursor  
	write(fd, IPstring, 8);
	write(fd, address, strlen(address));          // write the dotted decimal notation address to the screen
	write(fd, back, 10);                          // back button
	//navigation
	while (1) //wait for the back button to be pressed 
	{
		read(fd, &user_command, 1);
		if (user_command == 'G')
		{	
         break;
		}
		else beep();
	}
	//return to home
	write(fd, clear, 2);
	write(fd, servicescreen, 6);  
	write(fd, versionInfo, 9);  
	write(fd, info, 10);   
	return 0;
}

// if the user enters the wrong key, beep and blink the 3rd LED
int beep()  
{
	write(fd, errLED, 6);
	write(fd, beepcommand, 6);
	write(fd, errClearLED, 6);
	return 0;
}
// read the local configuration file 
int check_service() 
{
	int value; 
	char sname[20];  // values will not be over 20 characters. 
	fp = fopen("display.conf", "r");
	if(fp == NULL)
	{
		fprintf(fp2, "[display control] error opening or reading display.conf, please check if it exists. \n");  // log it
		fflush(fp2);
	}
	while(fgets(read_buffer, sizeof read_buffer, fp) != NULL)
	{
		if (read_buffer[0] == ';' || read_buffer[0] == '\n')
		continue;
		sscanf(read_buffer, "%20s%d", sname, &value);   // read up to 20 chars, then read a number
		if (strcmp(sname, "SERVICE") == 0)
		{
			fclose(fp);
			return(value);
	    }
	}
	return 0;
}

 // open the local logfile
int open_logfile()
{
	fp2 = fopen("logfile.log", "a+");
	if(fp2 == NULL)
	{
		printf("[display control] error opening or creatign logile.log, please check permissions. \n");  // log it
		return(0);
	}
	
	fprintf(fp2, "[display control] starting the application. \n");
	fflush(fp2);
	return 0;
}

// close the serial port
int close_port (void) 
{
	close(fd);
	fprintf(fp2, "[display control] port has been closed... \n");  // log it
	printf("Serial port has been closed.");
	fflush(fp2);
	return(fd);
}

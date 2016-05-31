	/*
	 *  bmpServer.c
 *  1917 serve that 3x3 bmp from lab3 Image activity
	 *
	 *  Created by Tim Lambert on 02/04/12.
	 *  Containing code created by Richard Buckland on 28/01/11.
 *  Copyright 2012 Licensed under Creative Commons SA-BY-NC 3.0.
	 *
	 */

	#include <stdlib.h>
	#include <stdio.h>
	#include <netinet/in.h>
	#include <string.h>
	#include <assert.h>
	#include <unistd.h>
	#include <math.h>

	#include "mandelbrot.h"
	#include "pixelColor.h"

	#define BYTES_PER_PIXEL 3
	#define BITS_PER_PIXEL (BYTES_PER_PIXEL*8)
	#define NUMBER_PLANES 1
	#define PIX_PER_METRE 2835
	#define MAGIC_NUMBER 0x4d42
	#define NO_COMPRESSION 0
	#define OFFSET 54
	#define DIB_HEADER_SIZE 40
	#define NUM_COLORS 0
	#define BYTE_SIZE 1
	#define SIZE 512
	#define CENTREPOINT 256.0


	#define BLACK 0x00
	#define WHITE 0xFF

	typedef unsigned char  bits8;
	typedef unsigned short bits16;
	typedef unsigned int   bits32;

	void serveBMP (int socket, double x, double y, int zoom);
	int waitForConnection (int serverSocket);
	int makeServerSocket (int portno);
	void writeHeader (int socket);
	static bits8 blackOrWhite (int pos, double xCentre, double yCentre, int zoom);
	void servePage (int socket);

	#define SIMPLE_SERVER_VERSION 1.0
	#define REQUEST_BUFFER_SIZE 1000
	#define DEFAULT_PORT 3200
	#define NUMBER_OF_PAGES_TO_SERVE 10
	// after serving this many pages the server will halt

	int main (int argc, char *argv[]) {

	   printf ("************************************\n");
	   printf ("Starting simple server %f\n", SIMPLE_SERVER_VERSION);
	   printf ("Serving bmps since 2012\n");

	   int serverSocket = makeServerSocket (DEFAULT_PORT);
   printf ("Access this server at http://localhost:%d/\n", DEFAULT_PORT);
	   printf ("************************************\n");

	   char request[REQUEST_BUFFER_SIZE];

	   int numberServed = 0;
	   while (numberServed < NUMBER_OF_PAGES_TO_SERVE) {
			double x = 0;
			double y = 0;
			double zoom = 0;
			int flag = 0;
			/*printf("Enter x: \n");
	   	scanf("%lf", &x);
	   	printf("Enter y: \n");
	   	scanf("%lf", &y);
	   	printf("Enter zoom level: \n");
	   	scanf("%lf", &zoom);*/
	      printf ("*** So far served %d pages ***\n", numberServed);

	      int connectionSocket = waitForConnection (serverSocket);
	      // wait for a request to be sent from a web browser, open a new
    // connection for this conversation

      // read the first line of the request sent by the browser
	      int bytesRead;
	      bytesRead = read (connectionSocket, request, (sizeof request)-1);
	      assert (bytesRead >= 0);
	      // were we able to read any data from the connection?
			flag = sscanf(request, "GET /tile_x%lf_y%lf_z%lf.bmp", &x, &y, &zoom);
			printf("%d", flag);
	      // print entire request to the console
	      printf (" *** Received http request ***\n %s\n", request);

	      //send the browser a simple html page using http
	      printf (" *** Sending http response ***\n");
	      if (flag == 3){
				serveBMP(connectionSocket, x, y, zoom);
			}else{
					servePage(connectionSocket);
			}
	      // close the connection after sending the page- keep aust beautiful
	      close(connectionSocket);
			numberServed++;
	   }

	   // close the server connection after we are done- keep aust beautiful
	   printf ("** shutting down the server **\n");
	   close(serverSocket);

	   return EXIT_SUCCESS;
	}
	void servePage (int socket){
	   char message[] = "HTTP/1.0 200 OK\r\n"
	                "Content-Type: text/html\r\n"
	                "\r\n";
	   printf ("about to send=> %s\n", message);
	   write (socket, message, strlen (message));

	   char almondBread[]= "<!DOCTYPE html>"
	    "<script src=\"http://almondbread.cse.unsw.edu.au/tiles.js\"></script>";

	   write (socket, almondBread, strlen(almondBread));

	   return;
	}

	void serveBMP (int socket, double x, double y, int zoom) {
	   char* message;
		zoom = zoom +4;
	   // first send the http response header

	   // (if you write strings one after another like this on separate
	   // lines the c compiler kindly joins them togther for you into
	   // one long string)
	   message = "HTTP/1.0 200 OK\r\n"
	                "Content-Type: image/bmp\r\n"
	                "\r\n";
	   printf ("about to send=> %s\n", message);

	   write (socket, message, strlen (message));
		writeHeader (socket);

	   int totalBytes = (SIZE * SIZE * BYTES_PER_PIXEL);
	   int position = 0;



	   while (position < totalBytes) {
	      int steps = blackOrWhite(position, x, y, zoom);
		unsigned char red = stepsToRed (steps);
			unsigned char blue = stepsToBlue (steps);
			unsigned char green = stepsToGreen (steps);
	      write (socket, &red, 1);
			write (socket, &blue, 1);
			write (socket, &green, 1);
			position++;

	   }
	}

	static bits8 blackOrWhite (int position, double x, double y, int z) {

	   bits8 byte = 0;
	   double xPosition = (position % (SIZE));// BYTES_PER_PIXEL)
	   double yPosition = (position / (SIZE));
	   double xPoint = (xPosition - CENTREPOINT + (x / pow(2,-z))) * pow(2, -z);
	   double yPoint = (yPosition - CENTREPOINT + (y / pow(2,-z))) * pow(2, -z);
	   if (escapeSteps(xPoint, yPoint) == 256) {
	      byte = WHITE;
	   } else {
	      byte = escapeSteps(xPoint,yPoint);
	   }

	   return byte;
	}

	int escapeSteps (double x, double y) {
	    int counter;

	    double r = 0;
	    double i = 0;

	    double nextR = 0;
	    double nextI = 0;

	    counter = 0;

	    while (r*r + i*i <= 4 && counter< 256){
	            nextR = r*r - i*i + x;
	            nextI = 2*r*i + y;
	            r = nextR;
	            i = nextI;

	            counter++;

    }

	    return counter;


	}
	// start the server listening on the specified port number
	int makeServerSocket (int portNumber) {

	   // create socket
	   int serverSocket = socket (AF_INET, SOCK_STREAM, 0);
   assert (serverSocket >= 0);
	   // error opening socket

	   // bind socket to listening port
	   struct sockaddr_in serverAddress;
	   memset ((char *) &serverAddress, 0,sizeof (serverAddress));

	   serverAddress.sin_family      = AF_INET;
	   serverAddress.sin_addr.s_addr = INADDR_ANY;
	   serverAddress.sin_port        = htons (portNumber);

	   // let the server start immediately after a previous shutdown
	   int optionValue = 1;
	   setsockopt (
	      serverSocket,
	      SOL_SOCKET,
	      SO_REUSEADDR,
	      &optionValue,
      sizeof(int)
	   );

	   int bindSuccess =
	      bind (
	         serverSocket,
	         (struct sockaddr *) &serverAddress,
	         sizeof (serverAddress)
	      );

	   assert (bindSuccess >= 0);
	   // if this assert fails wait a short while to let the operating
	   // system clear the port before trying again

	   return serverSocket;
	}

	// wait for a browser to request a connection,
	// returns the socket on which the conversation will take place
	int waitForConnection (int serverSocket) {
	   // listen for a connection
	   const int serverMaxBacklog = 10;
	   listen (serverSocket, serverMaxBacklog);

	   // accept the connection
	   struct sockaddr_in clientAddress;
     socklen_t clientLen = sizeof (clientAddress);
	   int connectionSocket =
	      accept (
	         serverSocket,
	         (struct sockaddr *) &clientAddress,
	         &clientLen
	      );

	   assert (connectionSocket >= 0);
	   // error on accept

   return (connectionSocket);
	}


	void writeHeader (int socket) {
	    bits16 magicNumber[1] = {MAGIC_NUMBER};
	    write (socket, magicNumber, sizeof magicNumber);

	    bits32 fileSize[1] = {OFFSET + (SIZE*SIZE* BYTES_PER_PIXEL)};
	    write (socket, fileSize, sizeof fileSize[1]);

	    bits32 reserved[1] = {0};
	    write (socket, reserved, sizeof reserved);

	    bits32 offset[1] = {OFFSET};
	    write (socket, offset, sizeof offset);

	    bits32 dibHeaderSize[1] = {DIB_HEADER_SIZE};
	    write (socket, dibHeaderSize, sizeof dibHeaderSize);

	    bits32 width[1] = {SIZE};
	    write (socket, width, sizeof width);

	    bits32 height[1] = {SIZE};
	    write (socket, height, sizeof height);

	    bits16 planes[1] = {NUMBER_PLANES};
	    write (socket, planes, sizeof planes);

	    bits16 bitsPerPixel[1] = {BITS_PER_PIXEL};
	    write (socket, bitsPerPixel, sizeof bitsPerPixel);

	    bits32 compression[1] = {NO_COMPRESSION};
	    write (socket, compression, sizeof compression);

	    bits32 imageSize[1] = {(SIZE * SIZE * BYTES_PER_PIXEL)};
    write (socket, imageSize, sizeof imageSize);

	    bits32 hResolution[1] = {PIX_PER_METRE};
	    write (socket, hResolution, sizeof hResolution);

	    bits32 vResolution[1] = {PIX_PER_METRE};
	    write (socket, vResolution, sizeof vResolution);

	    bits32 numColors[1] = {NUM_COLORS};
	    write (socket, numColors, sizeof numColors);

	    bits32 importantColors[1] = {NUM_COLORS};
	    write (socket, importantColors, sizeof importantColors);

	}



	/*int escapeSteps(double x, double y) {  //x = real number, y = imaginary

	    int counter;

	    double r = 0;
	    double i = 0;

	    double nextR = 0;
	    double nextI = 0;

	    counter = 0;

	    while( r*r + i*i <= 4 && counter < 256){
	        nextR  = r*r - i*i + x;
	        nextI = 2*r*i + y;
	        r=nextR;
	        i=nextI;
	        counter++;
	    }
   return counter;

	}*/

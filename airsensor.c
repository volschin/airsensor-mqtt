/*
	
	airsensor.c
	
	Original source: Rodric Yates http://code.google.com/p/airsensor-linux-usb/
	Modified from source: Ap15e (MiOS) http://wiki.micasaverde.com/index.php/CO2_Sensor
	Modified by Sebastian Sjoholm, sebastian.sjoholm@gmail.com
  
	This version created by Veit Olschinski, volschin@googlemail.com
	
	requirement:
	
	libusb libpaho-mqtt3c libpthread
	
	compile:
	
	gcc -o airsensor airsensor.c -lusb -lpaho-mqtt3c -lpthread
	
*/

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <asm/byteorder.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include "MQTTClient.h"

#define QOS         1
#define TIMEOUT     10000L
 
MQTTClient client;
struct usb_dev_handle *devh;

void help() {

	printf("AirSensor [options]\n");
	printf("Options:\n");
	printf("-d = debug printout\n");
	printf("-v = Print VOC value only, nothing returns if value out of range (450-2000)\n");
	printf("-o = One value and then exit\n");
	printf("-h = Help, this printout\n");
	exit(0);
	
}

void printout(char *str, int value) {
	
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	
	printf("%04d-%02d-%02d %02d:%02d:%02d, ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (value == 0) {
		printf("%s\n", str); 
	} else {
		printf("%s %d\n", str, value); 
	}
}

void release_usb_device(int dummy) {
	int ret;
	ret = usb_release_interface(devh, 0);
	usb_close(devh);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
	exit(ret);
}
 
struct usb_device* find_device(int vendor, int product) {
	struct usb_bus *bus;
 
	for (bus = usb_get_busses(); bus; bus = bus->next) {
		struct usb_device *dev;
		
		for (dev = bus->devices; dev; dev = dev->next) {
			if (dev->descriptor.idVendor == vendor
				&& dev->descriptor.idProduct == product)
			return dev;
		}
	}
	return NULL;
}
 
int main(int argc, char *argv[])
{
    char *brokername = "127.0.0.1";
    brokername = getenv("MQTT_BROKERNAME");
    char *portnumber = "1883";
    portnumber = getenv("MQTT_PORT");
    char address[80] = "tcp://";
    strcat (address, brokername);
    strcat (address, ":");
    strcat (address, portnumber);
    char *clientid = "airsensor" ;
    clientid = getenv("MQTT_CLIENTID");
    char *topicname = "home/CO2/voc";
    topicname = getenv("MQTT_TOPIC");
	const char *username = getenv("MQTT_USERNAME");
	const char *password = getenv("MQTT_PASSWORD");

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    MQTTClient_create(&client, address, clientid,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 70;
    conn_opts.cleansession = 1;
    conn_opts.username = getenv("MQTT_USERNAME");
    conn_opts.password = getenv("MQTT_PASSWORD");
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }


	int ret, vendor, product, debug, counter, one_read;
	int print_voc_only;
	struct usb_device *dev;
	char buf[1000];
	// char str[5];
    char command[2048];
	
	debug = 0;
	print_voc_only = 0;
	one_read = 0; 	

	vendor = 0x03eb;
	product = 0x2013; 
	dev = NULL;
	
	while ((argc > 1) && (argv[1][0] == '-'))
	{
		switch (argv[1][1])
		{
			case 'd':
				debug = 1;
				break;
			
			case 'v':
				print_voc_only = 1;
				break;
			
			case 'o':
				one_read = 1;
				break;
			
			case 'h':
				help();
				
		}
		
		++argv;
		--argc;
	}
	
	if (debug == 1) {
		printout("DEBUG: Active", 0);
	}
	
	if (debug == 1) {
		printout("DEBUG: Init USB", 0);
	}
	
	usb_init();
	
	counter = 0;
	
	do {
		
		usb_set_debug(0);
		usb_find_busses();
		usb_find_devices();
		dev = find_device(vendor, product);
		sleep(1);
		
		if (dev == NULL)
			if (debug == 1)
				printout("DEBUG: No device found, wait 10sec...", 0);
		
		sleep(10);
		
		++counter;
		
		if (counter == 10) {
			printout("Error: Device not found", 0);
			exit(1);
		}
				
	}  while (dev == NULL);
	
	assert(dev);
	
	if (debug == 1)
		printout("DEBUG: USB device found", 0);
	
	devh = usb_open(dev);
	assert(devh);
	
	signal(SIGTERM, release_usb_device);
	
	ret = usb_get_driver_np(devh, 0, buf, sizeof(buf));
	if (ret == 0) {
		ret = usb_detach_kernel_driver_np(devh, 0);
	}
		
	ret = usb_claim_interface(devh, 0);
	if (ret != 0) {
		printout("Error: claim failed with error: ", ret);
		exit(1);
	}
	
	unsigned short iresult=0;
	unsigned short voc=0;
	
	if (debug == 1)
		printout("DEBUG: Read any remaining data from USB", 0);
		
	ret = usb_interrupt_read(devh, 0x00000081, buf, 0x0000010, 1000);
	
	if (debug == 1)
		printout("DEBUG: Return code from USB read: ", ret);
	
	while(rc==MQTTCLIENT_SUCCESS) {
	
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		
		// USB COMMAND TO REQUEST DATA
 		// @h*TR
		if (debug == 1)
			printout("DEBUG: Write data to device", 0);
		
		memcpy(buf, "\x40\x68\x2a\x54\x52\x0a\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40", 0x0000010);
		ret = usb_interrupt_write(devh, 0x00000002, buf, 0x0000010, 1000);
		
		if (debug == 1)
			printout("DEBUG: Return code from USB write: ", ret);
		
		if (debug == 1)
			printout("DEBUG: Read USB", 0);
		
		ret = usb_interrupt_read(devh, 0x00000081, buf, 0x0000010, 1000);
		
		if (debug == 1)
			printout("DEBUG: Return code from USB read: ", ret);
		
		if ( !((ret == 0) || (ret == 16)))
		{
			if (print_voc_only == 1) {
				printf("0\n");
			} else {
				printout("ERROR: Invalid result code: ", ret);
			}
		}
		
		if (ret == 0) {
			
			if (debug == 1)
				printout("DEBUG: Read USB", 0);
			
			sleep(1);
			ret = usb_interrupt_read(devh, 0x00000081, buf, 0x0000010, 1000);
			
			if (debug == 1)
				printout("DEBUG: Return code from USB read: ", ret);
		}
 		
		memcpy(&iresult,buf+2,2);
		voc = __le16_to_cpu(iresult);
		sleep(1);
		
		if (debug == 1) {
			printout("DEBUG: Read USB [flush]", 0);
		}
		
		ret = usb_interrupt_read(devh, 0x00000081, buf, 0x0000010, 1000);
		
		if (debug == 1)
			printout("DEBUG: Return code from USB read: ", ret);
 		
 		// According to AppliedSensor specifications the output range is between 450 and 2000
 		// So only printout values between this range
 		
		if ( voc >= 450 && voc <= 15001) {
			if (print_voc_only == 1) {
				printf("%d\n", voc);
			} else {
				printf("%04d-%02d-%02d %02d:%02d:%02d, ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
				printf("VOC: %d, RESULT: OK\n", voc); 	
			}

            char svoc[5];
            // convert 123 to string [buf]
            sprintf(svoc, "%d", voc);
            pubmsg.payload = svoc;
            pubmsg.payloadlen = strlen(svoc);
            pubmsg.qos = QOS;
            pubmsg.retained = 0;
            MQTTClient_publishMessage(client, topicname, &pubmsg, &token);
            printf("Waiting for up to %d seconds for publication of %s\n"
               "on topic %s for client with ClientID: %s\n",
               (int)(TIMEOUT/1000), pubmsg.payload, topicname, clientid);
            rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
            printf("Message with delivery token %d delivered\n", token);

		} else {
			if (print_voc_only == 1) {
				printf("0\n");
			} else {
				printf("%04d-%02d-%02d %02d:%02d:%02d, ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
				printf("VOC: %d, RESULT: Error value out of range\n", voc); 	
			}
		}
		
		// If one read, then exit
		if (one_read == 1)
			exit(0);
		
		// Wait for next request for data
		sleep(30);
	
	}
 
}

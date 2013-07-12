#include <string>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>

#include "MSPBSL_Connection5xxUSB.h"
#include "MSPBSL_PhysicalInterfaceUSB.h"
#include "MSPBSL_PacketHandler5xxUSB.h"

#define DefaultAddr 0x0220
#define SleepTime globalArgs.Reset
#define EXIT_SUCCESS 0
#define EXIT_WITH_ERROR -1
#define False 0
#define True 1
#define LastOption -1
MSPBSL_PhysicalInterfaceUSB* Interface;
MSPBSL_PacketHandler5xxUSB* PacketHandler;

static const char *optString = "r:vmo:ebh";
struct globalArgs_t
{
	int Version;
	std::string inFile;
	int MassErase;
	int NewBsl;
	int Reset;
	int Run;
	int Help;
} globalArgs;

int CheckPassword(MSPBSL_Connection5xxUSB* theBSLConnection)
{
	uint8_t databuf[0x20000];
	for (unsigned i = 0; i < 0x20000; i++)
		databuf[i] = 0x00FF;
	int i = (theBSLConnection)->RX_Password(databuf);
	printf("%s: %s \n", __func__, (i == EXIT_SUCCESS ? "OK" : "FAILED"));
	return i;
}
int LoadBSL(MSPBSL_Connection5xxUSB* theBSLConnection)
{
	int i = theBSLConnection->loadRAM_BSL();
	printf("%s: %s \n", __func__, (i == EXIT_SUCCESS ? "OK" : "FAILED"));
	return i;
}
int PrintBSLVersion(MSPBSL_Connection5xxUSB* theBSLConnection)
{
	std::string returnstring;
	int i = theBSLConnection->TX_BSL_Version(returnstring);
	printf("%s:  %s value %s \n", __func__,
			(i == EXIT_SUCCESS ? "OK" : "FAILED"), returnstring.c_str());
	return i;
}
int MassErase(MSPBSL_Connection5xxUSB *theBSLConnection)
{
	int i = theBSLConnection->massErase();
	printf("%s: %s \n", __func__, (i == EXIT_SUCCESS ? "OK" : "FAILED"));
	return i;
}
MSPBSL_Connection5xxUSB* OpenConnection()
{
	MSPBSL_Connection5xxUSB *theBSLConnection = new MSPBSL_Connection5xxUSB("");
	Interface = new MSPBSL_PhysicalInterfaceUSB("");
	PacketHandler = new MSPBSL_PacketHandler5xxUSB("");

	PacketHandler->setPhysicalInterface(Interface);
	theBSLConnection->setPacketHandler(PacketHandler);
	((theBSLConnection->getPacketHandler())->getPhysicalInterface())->invokeBSL();
	return theBSLConnection;
}
void CloseConnetion(MSPBSL_Connection5xxUSB *theBSLConnection)
{
	delete PacketHandler;
	delete Interface;
	delete theBSLConnection;
}
int LoadFirmware(MSPBSL_Connection5xxUSB *theBSLConnection,
		std::string Firmware)
{
	struct stat s;
	if (stat(Firmware.c_str(), &s))
	{
		printf("%s\n", "FIRMWARE NOT FOUND!!!");
		return EXIT_WITH_ERROR;
	}
	if (!globalArgs.NewBsl)
		LoadBSL(theBSLConnection);
	if (!globalArgs.MassErase)
		MassErase(theBSLConnection);
	int i = theBSLConnection->loadFile(Firmware);
	printf("%s: %s \n", __func__, (i == EXIT_SUCCESS ? "OK" : "FAILED"));
	return i;
}

int Run(MSPBSL_Connection5xxUSB *theBSLConnection, uint32_t addr)
{
	int i = theBSLConnection->setPC(addr);
	printf("%s: %s \n", __func__, (i == EXIT_SUCCESS ? "OK" : "FAILED"));
	return i;
}
void Reset(int time)
{
	//system("reset_msp.sh");
	printf("%s\n", "Reseting...");
	FILE* file = fopen("/sys/class/gpio/gpio93/value", "w");
	if (file != NULL)
	{
		char buf1[] = "1\n";
		char buf0[] = "0\n";
		fwrite(buf0, sizeof(buf1), 1, file);
		fflush(file);
		sleep(time);
		fwrite(buf1, sizeof(buf1), 1, file);
		fflush(file);
		sleep(time);
		fclose(file);
	}
	else
		printf("%s\n", "???");
}

void SetCommandLineOptions(int argc, char* argv[])
{
	int opt = 0;

	globalArgs.Version = 0;
	globalArgs.Run = False;
	globalArgs.Reset = 0;
	globalArgs.MassErase = False;
	globalArgs.inFile = "";
	globalArgs.NewBsl = False;
	globalArgs.Help = False;

	opt = getopt(argc, argv, optString);
	while (opt != LastOption)
	{
		switch (opt)
		{
		case 'v':
			globalArgs.Version = True;
			break;

		case 'm':
			globalArgs.MassErase = True;
			break;

		case 'b':
			globalArgs.NewBsl = True;
			break;

		case 'o':
			globalArgs.inFile = optarg;
			break;

		case 'r':
			globalArgs.Reset = atoi(optarg);
			break;

		case 'e':
			globalArgs.Run = True;
			break;

		case 'h':
			globalArgs.Help = True;
			break;

		default:
			break;
		}
		opt = getopt(argc, argv, optString);
	}

}

int main(int argc, char* argv[])
{
	SetCommandLineOptions(argc, argv);

	if (globalArgs.Reset)
	{
		Reset(SleepTime);
		return EXIT_SUCCESS;
	}

	if (globalArgs.Help)
	{
		printf("%s\n", "-m MassErase");
		printf("%s\n", "-r [time] Reset");
		printf("%s\n", "-e Run");
		printf("%s\n", "-o [file] MassErase & LoadFirmware");
		printf("%s\n", "-b LoadBSL");
		printf("%s\n", "-v VersionBSL");
		return EXIT_SUCCESS;
	}

	MSPBSL_Connection5xxUSB *theBSLConnection = OpenConnection();
	CheckPassword(theBSLConnection);
	//---
	if (globalArgs.NewBsl)
		LoadBSL(theBSLConnection);

	if (globalArgs.MassErase)
		MassErase(theBSLConnection);

	if (globalArgs.Version)
		PrintBSLVersion(theBSLConnection);

	if (!(globalArgs.inFile == ""))
		LoadFirmware(theBSLConnection, globalArgs.inFile);

	if (globalArgs.Run)
		Run(theBSLConnection, DefaultAddr);

	//---
	CloseConnetion(theBSLConnection);
	return EXIT_SUCCESS;
}

#include <lcd1602.h>
#include <stdio.h>
#include "string.h"
#include "sleep.h"
#include "platform.h"
#include "xil_printf.h"
#include "xgpio.h"
#include "xintc.h"
#include "xuartlite.h"
#include "xil_exception.h"



/*Definitioner til boolean states*/
#define HIGH 1
#define LOW 0

/*Definitioner til vaegtstatus*/
#define intetGlas 0
#define korrektGlas 1
#define forkertGlas 2
#define fyldtGlas 3
int vaegtStatus;

/*Definitioner til olstorrelse i hoejde(antal lyssensorer), ml og gram*/
#define lilleHoej 7
#define mellemHoej 8
#define storHoej 11
#define lilleOlvol 230
#define mellemOlvol 400
#define storOlvol 400
#define lilleOlGram 386
#define mellemOlGram 205
#define storOlGram 271
#define ingenOel 0
u8 glasHoejde = 0;

/*Definitioner til stationstatus*/
#define ligeBestilt 0
#define manglerGlas 1
#define haelderNu 2
#define fjernOel 3
u8 stationStatus = ligeBestilt;

/*Definitioner til PSOC kommunikation status*/
#define tjekIntet 0
#define tjekAlt 1
#define	altBeregnet 2
u8 statusPSOCKom = tjekIntet;	//variabel til hvor meget der skal hentes fra psoc

/*Opsaetning af Besked laengder til UARTs*/
#define mesLenghtPSOC 30
#define mesLenghtESP 1
XUartLite psocUART;
u8 recBufferPSOC[mesLenghtPSOC];
XUartLite espUART;
u8 recBufferESP[mesLenghtESP];
u8 lillePils = 0;
u8 mellemPils = 0;
u8 storPils = 0;

/*Opsaetning af GPIO enheder der bruges globalt*/
XGpio motorControl;
XGpio ventilControl;
XGpio psocResetPin;


/*Variabler mm til motorstyring*/
#define topPosition 255
#define bundPosition 0
int oensketPos = bundPosition; //en variabel til den oenskede pos for motoren

/*Arrays til sensordata*/
long flowData;
unsigned int vaegtData[3];
unsigned int lysData[11];
unsigned int lysKalib[11];

/*kalibreringsfaktor*/
int vaegtKalibFaktor = 0;
//skal aendres
#define flowKali 1900 // er ganget med 1000

/*Handlers til esp uart */
void RecvHandlerESP(void *CallBackRef, unsigned int EventData) {
	print("ESP\n\r");
	XUartLite_Recv(&espUART, recBufferESP, mesLenghtESP);
	u8 stoerelse = (recBufferESP[0]>>4) & 0x03;
	u8 antalOel = recBufferESP[0] & 0x03;
	if (stoerelse == 1){
		lillePils += antalOel;
	}
	else if (stoerelse == 2){
		mellemPils += antalOel;
	}
	else if (stoerelse == 3){
		storPils += antalOel;
	}
}
void SendHandlerESP(void *CallBackRef, unsigned int EventData) {
	//TotalSentCount = EventData;
}
/*****************************************/

/*Handlers til psoc uart */
void RecvHandlerPSOC(void *CallBackRef, unsigned int EventData) {
	//print("PSOC\n\r");
	if (statusPSOCKom == tjekIntet) {
		XUartLite_ResetFifos(&psocUART); //toem buffer
	}
	else {
		XUartLite_Recv(&psocUART, recBufferPSOC, mesLenghtPSOC);
		u32 check = 0; //checksum variabel

		for (int i = 0; i < 29; i++) { //udregning af checksum
			check += recBufferPSOC[i];
		}
		check = check & 0xFF;

		if (check == recBufferPSOC[29]) { //check checksum
			flowData = recBufferPSOC[0];
			int a = 1;
			for (int i = 0; i < 11; i++) {
				a++;
				lysData[i] = recBufferPSOC[a];
				lysData[i] = lysData[i] << 8;
				lysData[i] += recBufferPSOC[a - 1];
				a++;
			}
			for (int i = 0; i < 3; i++) {
				a++;
				vaegtData[i] = recBufferPSOC[a];
				vaegtData[i] = vaegtData[i] << 8;
				vaegtData[i] += recBufferPSOC[a - 1];
				a++;
				statusPSOCKom = tjekAlt;

			}
		}
		else {
			print("ProblemsCHECKSUM\n\r");
			statusPSOCKom = altBeregnet;
		}
	}
	//

}
void SendHandlerPSOC(void *CallBackRef, unsigned int EventData) {
	//TotalSentCount = EventData;
}
/*****************************************/

/*****************************************/
/*Laver udregningen x^y
 *
 * 
 unsigned int powerU(u8 x, u8 y) {
	 int res = 1;
	 for (u8 i = 0; i < y; i++) {
		 res *= x;
	 }
	 return res;
 }
 */
/*****************************************/


/*****************************************/
/*
 * Funktionen fungerer som digital write til Arduino
 * Fungerer med maks 12 pins
 * Der er tre variabler
 *
 * @ioNavn er en pointer til en GPIO enhed
 *
 * @pinNumber er hvilken pin der skal skrives til - starter fra 0
 *
 * @vaerdi er vaerdi paa valgte pin - 0 er slukket, alt over er taendt
 *  */
void digitalExWrite(XGpio * ioNavn, u8 pinNumber, u8 vaerdi) {
	XGpio_SetDataDirection(ioNavn, 1, 0xfff);
	unsigned int status = XGpio_DiscreteRead(ioNavn, 1);
	XGpio_SetDataDirection(ioNavn, 1, 0x000);
	if (vaerdi >= 1) {
		status |= 1UL << pinNumber;
		XGpio_DiscreteWrite(ioNavn, 1, status);
	} else {
		status &= ~(1UL << pinNumber);
		XGpio_DiscreteWrite(ioNavn, 1, status);
	}
}
/*****************************************/



/*****************************************/
/*
 * Funktionen goer saa motoren koerer til den oenskede position
 * Der er 2 arguments
 *
 * @ioNavn er en pointer til en GPIO enhed
 *
 * @nyOensketPos er den oenskede postion paa enheden
 *  */
void setEnginePos(XGpio *ioNavn, int nyOensketPos) {
	int nuvaerendePos;
	int ackIn = 0;
	int manglendeSteps;
	u8 retning;

	//fortaeller der er ny data klar
	digitalExWrite(ioNavn, 9, 1);
	XGpio_SetDataDirection(ioNavn, 1, 0xfff); 	//laes pins
	while (ackIn == 0){						//venter paa handshake
		ackIn = XGpio_DiscreteRead(ioNavn, 1) & 0x200;
	}

	//henter data fra register
	XGpio_SetDataDirection(ioNavn, 1, 0xfff);
	manglendeSteps = XGpio_DiscreteRead(ioNavn, 1);
	retning = (manglendeSteps>>8) & 0x0001;  //Hvad retning koerte den sidst
	manglendeSteps = manglendeSteps & 0x00ff;
	//xil_printf("Manglende skridt 1: %d\n\r", manglendeSteps);

	//Finder den nuvaerende position
	if (retning == 1){
		nuvaerendePos = oensketPos + manglendeSteps;
	}
	else if (retning == 0){
		nuvaerendePos = oensketPos - manglendeSteps;
	}
	//xil_printf("nuvaerende pos: %d\n\r", nuvaerendePos);
	//xil_printf("Oensket retning: %d\n\r", retning);

	//Regner og skriver til motor
	if(nuvaerendePos > nyOensketPos){
		manglendeSteps = (nuvaerendePos - nyOensketPos) | 0x100;
	}
	else if(nuvaerendePos < nyOensketPos){
		manglendeSteps = nyOensketPos - nuvaerendePos;
	}
	XGpio_SetDataDirection(ioNavn, 1, 0xc00); //saetter 9 pins til outputs
	XGpio_DiscreteWrite(ioNavn, 1, manglendeSteps); //sender det oenskede antal steps + en retning + dataNewPin bliver lav
	//xil_printf("Manglende skridt 2: %d\n\r", manglendeSteps);
	oensketPos = nyOensketPos; //opdaterer den oenskede position
	//xil_printf("oensketPosition NEW: %d\n\r", oensketPos);

}
/*****************************************/



int samletVaegt() {

	long a = -16722 * vaegtData[2];
	a = (a / 10000) + 65884;
	long c = -16521 * vaegtData[1];
	c = (c / 10000) + 64998;
	long e = -16398 * vaegtData[0];
	e = (e / 10000) + 60630;

	int vaegt = 0;
	vaegt = a + c + e;
	return vaegt;
}

int checkGlasPlacering(int a){
	u8 faldetTilRo = 0;
	int tempVaegt = 0;
	int vaegt = 0;

	while (faldetTilRo < 10 ){
		while (statusPSOCKom == altBeregnet) {
				}
		vaegt = samletVaegt();
		statusPSOCKom = altBeregnet;
		if ((tempVaegt != vaegt) && ((tempVaegt-vaegt) < 10) && ((tempVaegt-vaegt) > -10)){
			faldetTilRo++;
		} else {
			tempVaegt = vaegt;
		}
	}

	//xil_printf("vaegtKalibFaktor: %d\n\r", vaegtKalibFaktor);
	//xil_printf("vaegt: %d\n\r", vaegt);

	if (a < (vaegt - vaegtKalibFaktor + 20) && a > (vaegt - vaegtKalibFaktor - 20)) {
		//print("Rigtig glas\n\r");
		return korrektGlas;

	}
	else if (0 < (vaegt - vaegtKalibFaktor + 60) && 0 > (vaegt - vaegtKalibFaktor - 60)) {
		//print("Intet glas\n\r");
		return intetGlas;

	}
	else {
		//print("Forkert glas\n\r");
		return forkertGlas;
	}

}

int kalibrerVaegt() {

	long tempVaegt = 0;
	int kalib = 0;
	statusPSOCKom = altBeregnet; //skal maaske fjernes senere hvis dette bliver en del af main loop
	for (int i = 0; i < 20; i++) {


		tempVaegt += samletVaegt();
		//xil_printf("tempvaegt: %d\r", tempVaegt);
		statusPSOCKom = altBeregnet;
	}
	kalib = tempVaegt / 20;
	//xil_printf("Kalibreringsfaktor: %d\n\r", kalib);
	return kalib;
}

void kalibrerLys(){
	//print("kalibLYS\n\r");
	u32 tempLys[11];
	for(int q =0; q<11;q++){
		tempLys[q] = 0;
	}
	for(int j = 0; j < 20; j++){
		while (statusPSOCKom == altBeregnet) {
		}
		for (int i = 0; i < 11; i++) 
		{	
			tempLys[i] += lysData[i];
			//xil_printf("templys: %d\n\r", tempLys[i]);
			//xil_printf("lys nr %d: %d\n\r", i, lysData[i] );
		}
		statusPSOCKom = altBeregnet;
	}
	for (int p = 0; p < 11; p++){
		tempLys[p] = tempLys[p]/20;
		lysKalib[p] = tempLys[p];
		//xil_printf("lysKalib nr %d: %d\n\r", p, lysKalib[p] );
	}
}


void oelLogik(int StoerelseBestilt) {
	int denPosViVilHave = topPosition;
	digitalExWrite(&psocResetPin, 0, LOW); //stopper med at resette PSOC flow
	while((flowData*flowKali)/1000 <= StoerelseBestilt){

		while (statusPSOCKom == altBeregnet) {
				}
		statusPSOCKom = tjekIntet;

//		//Funktion til escape ved fejlophaeldning
		//int vaegt=samletVaegt();
//		if(((vaegt + 30) > (flowData*flowKali)/1000) || ((vaegt - 30) < (flowData*flowKali)/1000)){
//			//sluk hane og send fejlmelding
//			digitalExWrite(&ventilControl, 0, LOW);
//			lcd1602Shutdown();
//			return;
//		}

//		for(int p = 0; p<11; p++){
//			xil_printf("lys nr %d: %d\n\r", p, lysData[p] );
//		}

		if(lysData[glasHoejde-1] > (lysKalib[glasHoejde-1]-120)){
			//taend hane
			digitalExWrite(&ventilControl, 0, HIGH);
		}
		else {
			//sluk hane
			digitalExWrite(&ventilControl, 0, LOW);
		}

		if(lysData[glasHoejde-3] < (lysKalib[glasHoejde-3]-120)){
			//vip ned
			if(denPosViVilHave > bundPosition){
				denPosViVilHave -=2;
				if(denPosViVilHave < bundPosition){
					denPosViVilHave = bundPosition;
				}
				setEnginePos(&motorControl, denPosViVilHave);
				//xil_printf("enginePos: %d",denPosViVilHave );
			}
		}

		long testFlow = (flowData*flowKali)/1000;
		xil_printf("FlowData: %d\n\r", flowData );
		xil_printf("TestFlow: %d\n\r", testFlow );
		statusPSOCKom = altBeregnet;
	}
	//sluk hane
	digitalExWrite(&ventilControl, 0, LOW);
	digitalExWrite(&psocResetPin, 0, HIGH);
	stationStatus = fjernOel;

}
	

int main(){
	init_platform();
	print("Hello World\n\r");

	/*opsaetning af skaerm*/
	unsigned int skaermAddress = 0x27;
	lcd1602Init(skaermAddress);
	lcd1602Clear();
								// 0 er clear
	char afventOrdre[] = "Afventer ordre"; // 1
	char placerGlas[] = "Placer glas"; // 2
	char klargoer1[] = "Klargoerer"; // 3
	char klargoer2[] = "servering"; // 4
	char serverer[] = "Serverer oel"; // 5
	char faerdig[] = "Hent oel"; // 6
	char wrongGlas[] = "Forkert glas"; // 7
	/* Status til sidste udskrift paa skaerm */
	u8 sidsteIndex = 0;
	/*****************************************/


	/*Opsaetning af GPIO
	 * Alt er opsat til output som standard
	 * */
	XGpio statusLED;
	XGpio_Initialize(&statusLED, XPAR_GPIO_STATUS_DEVICE_ID);
	XGpio_SetDataDirection(&statusLED, 1, 0x00);
		
	XGpio_Initialize(&motorControl, XPAR_GPIO_MOTOR_DEVICE_ID);
	XGpio_SetDataDirection(&motorControl, 1, 0x000);
		
	XGpio_Initialize(&ventilControl, XPAR_GPIO_VENTIL_DEVICE_ID);
	XGpio_SetDataDirection(&ventilControl, 1, 0x00);

	XGpio_Initialize(&psocResetPin, XPAR_GPIO_PSOC_RESET_DEVICE_ID);
	XGpio_SetDataDirection(&psocResetPin, 1, 0x00);

	//sender reset signal til PSOC taeller
	digitalExWrite(&psocResetPin, 0, HIGH);


	/*Opsaetning af interrupt generelt*/
	XIntc intController;
	XIntc_Initialize(&intController, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	/*****************************************/

	/*Opsaetning af uart+interrupt til ESP */
	XUartLite_Initialize(&espUART, XPAR_UART_ESP_DEVICE_ID);
	XIntc_Connect(&intController,
			  XPAR_MICROBLAZE_0_AXI_INTC_UART_ESP_INTERRUPT_INTR,
				  (XInterruptHandler)XUartLite_InterruptHandler, (void *)&espUART);
	XIntc_Enable(&intController,
				 XPAR_MICROBLAZE_0_AXI_INTC_UART_ESP_INTERRUPT_INTR);
	XUartLite_SetSendHandler(&espUART, SendHandlerESP, &espUART);
	XUartLite_SetRecvHandler(&espUART, RecvHandlerESP, &espUART);
	XUartLite_EnableInterrupt(&espUART);
	/*****************************************/

	/*Opsaetning af uart+interrupt til PSOC */
	XUartLite_Initialize(&psocUART, XPAR_UART_PSOC_DEVICE_ID);
	XIntc_Connect(&intController,
				  XPAR_MICROBLAZE_0_AXI_INTC_UART_PSOC_INTERRUPT_INTR,
				  (XInterruptHandler)XUartLite_InterruptHandler, (void *)&psocUART);
	XIntc_Enable(&intController,
				 XPAR_MICROBLAZE_0_AXI_INTC_UART_PSOC_INTERRUPT_INTR);
	XUartLite_SetSendHandler(&psocUART, SendHandlerPSOC, &psocUART);
	XUartLite_SetRecvHandler(&psocUART, RecvHandlerPSOC, &psocUART);
	XUartLite_EnableInterrupt(&psocUART);
	/*****************************************/

	/*Opsaetning af interrupt generelt afslutning*/
	XIntc_Start(&intController, XIN_REAL_MODE);
	Xil_ExceptionInit(); //Exception til interrupts
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				 (Xil_ExceptionHandler)XIntc_InterruptHandler, &intController);
	Xil_ExceptionEnable();
	/*****************************************/

	/*initierer/nulstiller variabler*/
	unsigned int oelStoerelseBestilt = 0;
	unsigned int oelVaegtBestilt = 0;
	statusPSOCKom = tjekAlt;
	/*****************************************/

//	sleep(5);
//	vaegtKalibFaktor = kalibrerVaegt();
//	int vaegtTester = 0;
//	while(1){
//		vaegtTester = (samletVaegt()) - vaegtKalibFaktor;
//		xil_printf("Vaegt: %d\n\r", vaegtTester );
//		sleep(1);
//	}


//	//testBLok
//	setEnginePos(&motorControl, topPosition);
//	int tester = topPosition;
//	sleep(5);
//	print(" \n \r");
//	while(1){
//
//		if(tester > bundPosition){
//			tester -=10;
//			if(tester < bundPosition){
//				tester = bundPosition;
//			}
//			setEnginePos(&motorControl, tester);
//			print(" \n \r");
//		}
//	}


	/*Main Loop af kode*/
	while (1){
		oelStoerelseBestilt = 0;
		oelVaegtBestilt = 0;
		if (sidsteIndex != 1){
			lcd1602Clear();
			lcd1602SetCursor(0, 0);
			lcd1602WriteString(afventOrdre);
			sidsteIndex = 1;
		}

		/*blok til bestillingshaandtering*/
		if (storPils > 0){

			oelStoerelseBestilt = storOlvol;
			oelVaegtBestilt = storOlGram;
			glasHoejde = storHoej;
			stationStatus = ligeBestilt;
			storPils--;
		}
		else if (mellemPils > 0){

			oelStoerelseBestilt = mellemOlvol;
			oelVaegtBestilt = mellemOlGram;
			glasHoejde = mellemHoej;
			stationStatus = ligeBestilt;
			mellemPils--;
		}
		else if (lillePils > 0){

			oelStoerelseBestilt = lilleOlvol;
			oelVaegtBestilt = lilleOlGram;
			glasHoejde = lilleHoej;
			stationStatus = ligeBestilt;
			lillePils--;
		}


		/*blok til skaenkning*/
		while (oelStoerelseBestilt != ingenOel){ //saa hvis bestilling er modtaget

			switch(stationStatus){

			case ligeBestilt :
				vaegtKalibFaktor = kalibrerVaegt();
				if (sidsteIndex != 2){
					lcd1602Clear();
					lcd1602SetCursor(0, 0);
					lcd1602WriteString(placerGlas);
					sidsteIndex = 2;
				}
				stationStatus = manglerGlas;
				break;

			case manglerGlas :
				vaegtStatus = checkGlasPlacering(oelVaegtBestilt);
				if (vaegtStatus == korrektGlas){


					setEnginePos(&motorControl, topPosition);
					kalibrerLys();

					if (sidsteIndex != 3){
						lcd1602Clear();
						lcd1602SetCursor(0, 0);
						lcd1602WriteString(klargoer1);
						lcd1602SetCursor(0, 1);
						lcd1602WriteString(klargoer2);
						sidsteIndex = 3;
					}

					//delay her - skal ikke vaere delay i et system med flere haner
					usleep(1800000); //tid er random lige nu
					if (sidsteIndex != 5){
						lcd1602Clear();
						lcd1602SetCursor(0, 0);
						lcd1602WriteString(serverer);
						sidsteIndex = 5;
					}
					//digitalExWrite(&ventilControl, 0, 1); //vi burde koere det her i oelLogik
					stationStatus = haelderNu;
					statusPSOCKom = tjekAlt;
				}
				else if (vaegtStatus == forkertGlas){
					if (sidsteIndex != 7){
						lcd1602Clear();
						lcd1602SetCursor(0, 0);
						lcd1602WriteString(wrongGlas);
						sidsteIndex = 7;
					}
				}
				else if (vaegtStatus == intetGlas){
					if (sidsteIndex != 2){
						lcd1602Clear();
						lcd1602SetCursor(0, 0);
						lcd1602WriteString(placerGlas);
						sidsteIndex = 2;
					}
				}
			    break;

			case haelderNu :
				if (statusPSOCKom != altBeregnet){
					statusPSOCKom = tjekIntet;
					oelLogik(oelStoerelseBestilt);
					statusPSOCKom = altBeregnet;
				}
				break;

			case fjernOel :
				if (sidsteIndex != 6){
					lcd1602Clear();
					lcd1602SetCursor(0,0);
					lcd1602WriteString(faerdig);
					sidsteIndex = 6;
				}
				if (checkGlasPlacering(oelStoerelseBestilt) == intetGlas){

					oelStoerelseBestilt = ingenOel;
				}
				break;

			}


		}
	}
	//shutdown
	lcd1602Shutdown();
	cleanup_platform();
	return 0;
}

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
#define lilleOlvol 330
#define mellemOlvol 400
#define storOlvol 470
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


/*kalibreringsfaktorer*/
int vaegtKalibFaktor = 0;
#define flowKali 1600 //er ganget med 1000


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
void SendHandlerESP(void *CallBackRef, unsigned int EventData) { //benyttes ikke
	//TotalSentCount = EventData;
}
/*****************************************/



/*Handlers til psoc uart */
void RecvHandlerPSOC(void *CallBackRef, unsigned int EventData) {

	if (statusPSOCKom == tjekIntet) {		//ignorerer ny data under udregninger
		XUartLite_ResetFifos(&psocUART);	//toem buffer
	}
	else {
		XUartLite_Recv(&psocUART, recBufferPSOC, mesLenghtPSOC); //modtager data
		u32 check = 0; //checksum variabel

		for (int i = 0; i < 29; i++) { //udregning af checksum
			check += recBufferPSOC[i];
		}
		check = check & 0xFF; //checksum er kun 8 bit saa der maskes

		if (check == recBufferPSOC[29]) { //check checksum
			flowData = recBufferPSOC[0];


			int a = 1;
			for (int i = 0; i < 11; i++) {
				a++;
				lysData[i] = recBufferPSOC[a];		//gemmer foerste 8 bit af data
				lysData[i] = lysData[i] << 8;		//de er MSB saa shiftes til plads 8-15
				lysData[i] += recBufferPSOC[a - 1];	//laegger sidste 8 bit til
				a++;
			}
			for (int i = 0; i < 3; i++) {
				a++;
				vaegtData[i] = recBufferPSOC[a];
				vaegtData[i] = vaegtData[i] << 8;
				vaegtData[i] += recBufferPSOC[a - 1];
				a++;


			}
			statusPSOCKom = tjekAlt; //Alt data gemt saa klar til ny data
		}
		else {
			statusPSOCKom = altBeregnet; //kaster data vaek hvis checksum er fejl
		}

	}
	//

}
void SendHandlerPSOC(void *CallBackRef, unsigned int EventData) { //bruges ikke
	//TotalSentCount = EventData;
}
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
	XGpio_SetDataDirection(ioNavn, 1, 0xfff); 			//laeser på alle pins
	while (ackIn == 0){									//venter paa handshake
		ackIn = XGpio_DiscreteRead(ioNavn, 1) & 0x200;	//tjekker handshake/ack pin
	}

	//henter data fra register
	XGpio_SetDataDirection(ioNavn, 1, 0xfff);
	manglendeSteps = XGpio_DiscreteRead(ioNavn, 1);
	retning = (manglendeSteps>>8) & 0x0001;  			//Hvad retning koerte den sidst
	manglendeSteps = manglendeSteps & 0x00ff;			//antal steps motor ikke har koert

	//Finder den nuvaerende position
	if (retning == 1){
		nuvaerendePos = oensketPos + manglendeSteps;
	}
	else if (retning == 0){
		nuvaerendePos = oensketPos - manglendeSteps;
	}


	//Regner og skriver til motor
	if(nuvaerendePos > nyOensketPos){
		manglendeSteps = (nuvaerendePos - nyOensketPos) | 0x100; //saetter bit 8/retning pin hoej
	}
	else if(nuvaerendePos < nyOensketPos){
		manglendeSteps = nyOensketPos - nuvaerendePos;
	}
	XGpio_SetDataDirection(ioNavn, 1, 0xc00); 		//saetter 9 pins til outputs
	XGpio_DiscreteWrite(ioNavn, 1, manglendeSteps); //sender det oenskede antal steps + en retning + dataNewPin bliver lav
	oensketPos = nyOensketPos; 						//opdaterer den oenskede position
}
/*****************************************/


//funktion der returnerer hvad vaegt i gram der staar paa platformen
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


//Funktion der returnerer hvorvidt det rigtige, intet eller forkerte glas staar paa platformen
//Input er oeltype vaegt
int checkGlasPlacering(int a){
	u8 faldetTilRo = 0;
	int tempVaegt = 0;
	int vaegt = 0;

	//Der kraeves 10 naesten ens maalinger for at vaegten er verificeret
	while (faldetTilRo < 10 ){

		//venter paa ny data
		while (statusPSOCKom == altBeregnet) {
		}
		statusPSOCKom = altBeregnet;
		vaegt = samletVaegt();

		if ((tempVaegt != vaegt) && ((tempVaegt-vaegt) < 10) && ((tempVaegt-vaegt) > -10)){
			faldetTilRo++;
		}
		else {
			tempVaegt = vaegt;	//nulstiller reference vaegt
			faldetTilRo = 0; 	//nulstiller antal stabile maalinger
		}
	}


	if (a < (vaegt - vaegtKalibFaktor + 20) && a > (vaegt - vaegtKalibFaktor - 20)) {
		return korrektGlas;

	}
	else if (0 < (vaegt - vaegtKalibFaktor + 60) && 0 > (vaegt - vaegtKalibFaktor - 60)) {
		return intetGlas;

	}
	else {
		return forkertGlas;
	}

}

//Funktion der tager tyve maalinger og returnerer vaegten maalt. Benyttes til kalibrering
int kalibrerVaegt() {
	long tempVaegt = 0;
	int kalib = 0;

	statusPSOCKom = altBeregnet; //Sikrer vi arbejder med ny data
	for (int i = 0; i < 20; i++) { //udfoerer 20 maalinger

		//venter paa ny data
		while (statusPSOCKom == altBeregnet) {
		}
		statusPSOCKom = altBeregnet;
		tempVaegt += samletVaegt();
	}
	kalib = tempVaegt / 20;
	return kalib;
}

//funktion til kalibrering af lyssensorer
void kalibrerLys(){
	u32 tempLys[11];
	for(int q =0; q<11;q++){
		tempLys[q] = 0;
	}
	for(int j = 0; j < 20; j++){ //looper 20 maalinger igennem

		//venter paa ny data
		while (statusPSOCKom == altBeregnet) {
		}
		for (int i = 0; i < 11; i++)
		{	
			tempLys[i] += lysData[i];
		}
		statusPSOCKom = altBeregnet;
	}
	for (int p = 0; p < 11; p++){
		tempLys[p] = tempLys[p]/20;
		lysKalib[p] = tempLys[p];
	}
}



//Funktion der styrer ophaeldning af oellen
void oelLogik(int StoerelseBestilt) {
	int denPosViVilHave = topPosition;								//variabel til motorPos
	digitalExWrite(&psocResetPin, 0, LOW); 							//stopper med at resette PSOC flow
	while((flowData*flowKali)/1000 <= StoerelseBestilt){ //tjekker flowsensor om hele oellen er haeldt op


		while (statusPSOCKom == altBeregnet) { //sikrer der ikke regnes paa gammel data
				}
		statusPSOCKom = tjekIntet;

//		//tjek til escape ved fejlophaeldning
		//udkommenteret da flow kali er unreliable
		//int vaegt=samletVaegt();
//		if(((vaegt + 30) > (flowData*flowKali)/1000) || ((vaegt - 30) < (flowData*flowKali)/1000)){
//			//sluk hane og send fejlmelding
//			digitalExWrite(&ventilControl, 0, LOW);
//			lcd1602Shutdown();
//			return;
//		}


		//Hvis der ikke er oel i toppen af glas så taend hane. Sluk hane hvis glas fyldt
		if(lysData[glasHoejde-1] > (lysKalib[glasHoejde-1]-120)){

			digitalExWrite(&ventilControl, 0, HIGH);
		}
		else {

			digitalExWrite(&ventilControl, 0, LOW);
		}

		//justerer vippeposition saafremt at der er oel hoejt oppe i glas
		if(lysData[glasHoejde-3] < (lysKalib[glasHoejde-3]-120)){
			if(denPosViVilHave > bundPosition){
				denPosViVilHave -=2;
				if(denPosViVilHave < bundPosition){
					denPosViVilHave = bundPosition;
				}
				setEnginePos(&motorControl, denPosViVilHave);
			}
		}

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


	/*Main Loop af kode*/
	while (1){
		oelStoerelseBestilt = ingenOel;
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
					usleep(1800000); //tiden kan saenkes hvis det oenskes
					if (sidsteIndex != 5){
						lcd1602Clear();
						lcd1602SetCursor(0, 0);
						lcd1602WriteString(serverer);
						sidsteIndex = 5;
					}

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

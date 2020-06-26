#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "MicroBit.h"
#include "MicroBitFile.h"
#include "MicroBitUARTService.h"
#include "Orologio.h"

MicroBit uBit;
MicroBitUARTService *uart;
Orologio orologio;

const ManagedString NOME_FILE("Logs.csv");
const ManagedString EOM(":");
const int RECORD_LEN_HIRO = 25;
const int RECORD_LEN_ALLY = 25;
const int SEND_LEN = 16;
const char DLM = ',';

bool squadra = true;
bool connesso = false;
bool statoTrasferimento = false;
bool bluetooth = false;

void vaiAvanti() {

}

void giraSinistra() {

}

void giraDestra() {

}

data mStrToData(ManagedString mStrData)
{
    int tmp[6] = {};
    int posTmp = 0;
    int lenNum = 0;
    int lenStr = mStrData.length();

    for(int k = 0; k<lenStr; k++) {
        if(mStrData.charAt(k) == DLM) {
            tmp[posTmp] = atoi(mStrData.substring(k-lenNum, lenNum).toCharArray());
            posTmp++, lenNum = 0;
        } else {
            lenNum++;
        }
    }

    data retData = {tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]};

    return retData;
}

void onConnected(MicroBitEvent)
{
    if(bluetooth) {
        connesso = true;

        //simbolo del bluetooth
        MicroBitImage bluetooth("255,0,255,255,0 \n 0,255,255,0,255 \n 0,0,255,255,0 \n 0,255,255,0,255 \n 255,0,255,255,0 \n");
        uBit.display.print(bluetooth);

        uBit.sleep(2000);

        //ottiene la data corrente
        uart->send("tempo", SYNC_SLEEP);
        ManagedString mStrData = uart->readUntil(EOM, SYNC_SLEEP);
        
        data nuovaData = mStrToData(mStrData);

        //se la data corrente è presente, la aggiorna
        if(orologio.isInit()) {
            orologio.setData(nuovaData);
        } else { //altrimenti la inserisce per la prima volta
            orologio.init(nuovaData);
        }
        
        //controlla se ci sono file nel microbit e li trasferisce nel telefono
        MicroBitFile fileRead(NOME_FILE, READ);
        
        if(fileRead.isValid()) {
            uart->send("fileTr", SYNC_SLEEP);
            uBit.sleep(500);
            ManagedString tmp = fileRead.read(SEND_LEN);

            while(tmp.length() > 0) {
                uart->send(tmp, SYNC_SLEEP);
                tmp = fileRead.read(SEND_LEN);
            }
            uart->send("fileTrStop", SYNC_SLEEP);
            fileRead.close();
            uBit.sleep(500);
        }
        
        uart->send("stop", SYNC_SLEEP);

        MicroBitFileSystem::defaultFileSystem->remove(NOME_FILE.toCharArray());

        statoTrasferimento = true;
        connesso = false;
    } else {
        uart->send("occupato", SYNC_SLEEP);
    }
}

int main()
{
    uBit.init();
    uart = new MicroBitUARTService(*uBit.ble, 32, 32);

    uBit.messageBus.listen(MICROBIT_ID_BLE, MICROBIT_BLE_EVT_CONNECTED, onConnected);

    MicroBitImage clessidra("255,255,255,255,255 \n 0,255,0,255,0 \n 0,0,255,0,0 \n 0,255,0,255,0 \n 255,255,255,255,255 \n");
    MicroBitImage clessidraSkip("255,255,255,255,255 \n 0,255,0,255,0 \n 0,0,255,0,255 \n 0,255,0,255,0 \n 255,255,255,255,255 \n");
    MicroBitImage spunta("0,0,0,0,0 \n 0,0,0,0,255 \n 0,0,0,255,0 \n 255,0,255,0,0 \n 0,255,0,0,0 \n");
    MicroBitImage frecciaSinstra("0,0,255,0,0 \n 0,255,0,0,0 \n 255,255,255,255,255 \n 0,255,0,0,0 \n 0,0,255,0,0 \n");

    while(true) {
        if(uBit.buttonA.isPressed()) {
            break;
        }

        if(uBit.buttonB.isPressed()) {
            squadra = false;
            break;
        }
    }
    
    uBit.sleep(3000);

    while(true) {
        //Stiamo ascoltando per le connessioni bluetooth in entrata
        bluetooth = true;
        
        //Attesa che si prema il bottone B o che finisca il trasferimento dati
        if(orologio.isInit()) {
            uBit.display.print(clessidraSkip);
            while(!statoTrasferimento && !(uBit.buttonB.isPressed() && !connesso)) {
                uBit.sleep(100);
            }
        } else {
            uBit.display.print(clessidra);
            while(!statoTrasferimento) {
                uBit.sleep(100);
            }

            //fa vedere una freccia che insica il tasto da schiacciare
            uBit.display.print(frecciaSinstra);

            while(!uBit.buttonA.isPressed()) {
                uBit.sleep(100);
            }
        }

        statoTrasferimento = false;

        //Fine dell'ascolto
        bluetooth = false;
        
        //fa vedere una spunta
        uBit.display.print(spunta);
        uBit.sleep(3000);
        uBit.display.clear();
        
        int lastMin = -1;

        if(squadra) {
            while(!uBit.buttonB.isPressed()) {
                data attuale = orologio.getData();

                if((attuale.min != lastMin) && (attuale.min == 0)) {
                    
                }

                if((attuale.min != lastMin) && (attuale.min%15 == 0)) {
                    lastMin = attuale.min;



                    //inizia la rilevazione
                    int temp = uBit.thermometer.getTemperature()-1;

                    char recordTmp[RECORD_LEN_HIRO+1];
                    sprintf(recordTmp, "\n%04d,%01d,%02d,%02d,%02d,%02d,%04d,", temp, attuale.ora, attuale.min, attuale.gg, attuale.mm, attuale.aaaa);
                    ManagedString record(recordTmp);

                    //apre il file
                    MicroBitFile fileOpen(NOME_FILE, READ_AND_WRITE);

                    if(fileOpen.isValid()) {
                        //scrive
                        fileOpen.append(record);

                        //chiude il file
                        fileOpen.close();
                    } else {
                        //creo il file
                        MicroBitFile fileCreate(NOME_FILE, CREATE);

                        //scrive
                        ManagedString legenda("temp,p,hs,ms,gg,mm,aaaa,lo");
                        fileCreate.append(legenda);
                        fileCreate.append(record);

                        //chiude il file
                        fileCreate.close();
                    }

                    //giraSinistra()
                    //vaiAvanti()

                    /* PER IL DEBUG
                    MicroBitFile fileRead(NOME_FILE, READ);
                    ManagedString tmp = fileRead.read(RECORD_LEN);
                    while(tmp.length() > 0) {
                        uBit.serial.printf("%s\t", tmp.toCharArray());
                        tmp = fileRead.read(RECORD_LEN);
                    }
                    fileRead.close();
                    */
                }
                uBit.sleep(100);
            }
        } else {
            while(!uBit.buttonB.isPressed()) {
                data attuale = orologio.getData();

                if((attuale.min != lastMin) && (attuale.sec%15 == 0)) {
                    lastMin = attuale.min;

                    //inizia la rilevazione
                    int temp = uBit.thermometer.getTemperature()-3;

                    char recordTmp[RECORD_LEN_ALLY+1];
                    sprintf(recordTmp, "\n%04d,%02d,%02d,%02d,%02d,%04d,ME", temp, attuale.ora, attuale.min, attuale.gg, attuale.mm, attuale.aaaa);
                    ManagedString record(recordTmp);

                    //apre il file
                    MicroBitFile fileOpen(NOME_FILE, READ_AND_WRITE);

                    if(fileOpen.isValid()) {
                        //scrive
                        fileOpen.append(record);

                        //chiude il file
                        fileOpen.close();
                    } else {
                        //creo il file
                        MicroBitFile fileCreate(NOME_FILE, CREATE);

                        //scrive
                        ManagedString legenda("temp,hs,ms,gg,mm,aaaa,lo");
                        fileCreate.append(legenda);
                        fileCreate.append(record);

                        //chiude il file
                        fileCreate.close();
                    }

                    //giraSinistra()
                    //vaiAvanti()

                    /* PER IL DEBUG
                    MicroBitFile fileRead(NOME_FILE, READ);
                    ManagedString tmp = fileRead.read(RECORD_LEN);
                    while(tmp.length() > 0) {
                        uBit.serial.printf("%s\t", tmp.toCharArray());
                        tmp = fileRead.read(RECORD_LEN);
                    }
                    fileRead.close();
                    */
                }
                uBit.sleep(100);
            }
        }
        uBit.sleep(3000);
    }
	release_fiber();
}
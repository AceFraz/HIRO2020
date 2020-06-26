#ifndef OROLOGIO
#define OROLOGIO
#include <stdbool.h>
#include "MicroBitFiber.h"

extern const int GIORNI_MESE[];

struct data {
    int gg, mm, aaaa;
    int ora, min, sec;
};

class Orologio 
{
    private:
    static data Data;
    static bool Inizializzato;
    static void startClock();
    
    public:
    Orologio();
    void init(data dataIniziale);
    bool isInit();
    data getData();
    void setData(data nuovaData);
};

#endif //OROLOGIO
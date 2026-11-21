#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "simuladorSO.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    Config cfg = configurar();
    Control ctl;
    ctl.retardoMs = cfg.retardoMs;

    Simulador sim(cfg, ctl);
    thread entrada(hiloComandos, ref(ctl));  // lee +, -, p, q mientras corre

    sim.correr();
    ctl.salir = true;
    sim.estadisticas();

    entrada.detach();  // el hilo puede seguir bloqueado en cin; el proceso termina igual

#ifdef _WIN32
    system("pause");
#endif
    return 0;
}
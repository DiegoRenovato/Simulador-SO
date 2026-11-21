#ifndef SIMULADORSO_H
#define SIMULADORSO_H

#include <bits/stdc++.h>
#include <windows.h>
using namespace std;


constexpr int SPLIT_MIN = 32;  // Tamaño minimo de split (KB)

struct Bloque {            // Un bloque de memoria: proceso (id>0) o hueco (id==0)
    int id;
    int tamano;            // KB
    int cuanto;            // cuanto RESTANTE del proceso (0 en huecos)
    int creado = 0;        // ciclo en que se genero (para estadisticas)
    bool esHueco() const { return id == 0; }
};

enum class Algoritmo { PRIMER_AJUSTE = 1, MEJOR_AJUSTE = 2, COMBINADO = 3 };

struct Config {
    int memoriaKB = 4096;      // 1024, 4096 u 8192
    int cuantoSistema = 2;     // Cuanto del sistema (Round Robin)
    int tamMaxProceso = 600;   // Tamaño maximo de un proceso (KB)
    int cuantoMaxProceso = 10; // Cuanto maximo que puede pedir un proceso
    Algoritmo alg = Algoritmo::PRIMER_AJUSTE;
    int retardoMs = 1000;      // Velocidad inicial
};

string aTexto(const Bloque& b);

class Memoria {
public:
    using It = list<Bloque>::iterator;
    list<Bloque> bloques;
    int total;

    explicit Memoria(int t);

    It primerAjuste(int tam);              // primer hueco donde cabe
    It mejorAjuste(int tam);               // hueco mas pequeño donde cabe
    It combinado(int tam);                 // primer ajuste + mejor ajuste
    It buscar(int tam, Algoritmo alg);     // despacha al algoritmo elegido

    It asignar(Bloque p, Algoritmo alg);   // coloca (con split); end() si no cabe
    void descargar(It it);                 // bloque -> hueco
    bool condensar();                      // fusiona huecos contiguos
    int ocupado() const;
    void imprimir() const;
};

struct Control {
    atomic<int>  retardoMs{1000};
    atomic<bool> pausa{false};
    atomic<bool> salir{false};
};

void hiloComandos(Control& c);

class Simulador {
    Config cfg;
    Control& ctl;
    Memoria mem;
    deque<Memoria::It> colaRR;   // Cola Round Robin (iteradores a bloques)
    Bloque pendiente;            // Siempre hay un proceso listo para entrar
    int siguienteId = 1;
    int ciclo = 0;
    mt19937 rng{random_device{}()};

    // Estadisticas
    int terminados = 0;
    long long sumaTurnaround = 0;    // ciclos desde creacion hasta terminar
    double sumaOcupacion = 0;        // % de ocupacion acumulado por ciclo
    double ocupacionMax = 0;
    int unidadesCPU = 0;             // unidades de cuanto realmente ejecutadas

    Bloque generarProceso();
    void imprimirCola() const;
    void entrarProceso();            // paso 1 del ciclo: entrada a memoria
    void planificar();               // paso 2 del ciclo: Round Robin

public:
    Simulador(Config c, Control& k);
    void ejecutarCiclo();
    void correr();
    void estadisticas() const;
};

int leerEntero(const string& msg, int lo, int hi);
Config configurar();

#endif
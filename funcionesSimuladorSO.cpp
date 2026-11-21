#include "simuladorSO.h"

// =====================================================================
//Estructuras y definicion de funciones 
// =====================================================================

string aTexto(const Bloque& b) {
    return "[" + to_string(b.id) + "," + to_string(b.tamano) + "," +
           to_string(b.cuanto) + "]";
}

Memoria::Memoria(int t) : total(t) { bloques.push_back({0, t, 0}); }

// ===================== ALGORITMOS DE COLOCACION ======================
Memoria::It Memoria::primerAjuste(int tam) {
    (void)tam;
    return bloques.end();   // pendiente de implementar
}

Memoria::It Memoria::mejorAjuste(int tam) {
    (void)tam;
    return bloques.end();   // pendiente de implementar
}

Memoria::It Memoria::combinado(int tam) {
    (void)tam;
    return bloques.end();   // pendiente de implementar
}

Memoria::It Memoria::buscar(int tam, Algoritmo alg) {
    switch (alg) {
        case Algoritmo::PRIMER_AJUSTE: return primerAjuste(tam);
        case Algoritmo::MEJOR_AJUSTE:  return mejorAjuste(tam);
        default:                       return combinado(tam);
    }
}

// Coloca el proceso. Devuelve end() si no cabe.
Memoria::It Memoria::asignar(Bloque p, Algoritmo alg) {
    It h = buscar(p.tamano, alg);
    if (h == bloques.end()) return h;
    int resto = h->tamano - p.tamano;
    if (resto >= SPLIT_MIN) {              // split: proceso + hueco sobrante
        It nuevo = bloques.insert(h, p);
        h->tamano = resto;
        return nuevo;
    }
    p.tamano = h->tamano;                  // sin split: el proceso ocupa todo el hueco
    *h = p;
    return h;
}

// Convierte el bloque en hueco (sin fusionar todavia)
void Memoria::descargar(It it) { it->id = 0; it->cuanto = 0; }

// Fusiona huecos contiguos. true si hubo al menos una fusion
bool Memoria::condensar() {
    bool hubo = false;
    for (It it = bloques.begin(); it != bloques.end();) {
        It sig = next(it);
        if (sig != bloques.end() && it->esHueco() && sig->esHueco()) {
            it->tamano += sig->tamano;
            bloques.erase(sig);
            hubo = true;
        } else ++it;
    }
    return hubo;
}

int Memoria::ocupado() const {
    int s = 0;
    for (const auto& b : bloques) if (!b.esHueco()) s += b.tamano;
    return s;
}

void Memoria::imprimir() const {
    for (const auto& b : bloques) cout << aTexto(b);
    cout << "\n";
}

void hiloComandos(Control& c) {
    string linea;
    while (!c.salir && getline(cin, linea)) {
        if (linea.empty()) continue;
        switch (linea[0]) {
            case '+': c.retardoMs = max(0, c.retardoMs.load() - 250); break; // mas rapido
            case '-': c.retardoMs = c.retardoMs.load() + 250;         break; // mas lento
            case 'p': case 'P': c.pausa = !c.pausa;                   break;
            case 'q': case 'Q': c.salir = true;                       break;
        }
    }
    c.salir = true; // EOF o 'q'
}

Simulador::Simulador(Config c, Control& k) : cfg(c), ctl(k), mem(c.memoriaKB) {
    pendiente = generarProceso();
}

Bloque Simulador::generarProceso() {
    uniform_int_distribution<int> t(SPLIT_MIN, min(cfg.tamMaxProceso, cfg.memoriaKB));
    uniform_int_distribution<int> q(1, cfg.cuantoMaxProceso);
    return Bloque{siguienteId++, t(rng), q(rng), ciclo};
}

void Simulador::imprimirCola() const {
    cout << "Lista RR";
    if (colaRR.empty()) cout << " (vacía)";
    for (auto it : colaRR) cout << " " << aTexto(*it);
    cout << "\n";
}

// Paso 1: entrada del proceso pendiente a memoria
void Simulador::entrarProceso() {
    auto it = mem.asignar(pendiente, cfg.alg);
    if (it != mem.bloques.end()) {
        cout << "Entra Proceso " << aTexto(*it) << "\n";
        colaRR.push_back(it);          // entra a la cola de listos
        pendiente = generarProceso();
    } else {
        cout << "Proceso " << aTexto(pendiente)
             << " en espera (no hay hueco suficiente)\n";
    }
    mem.imprimir();
}

// ===================== PLANIFICADOR ROUND ROBIN ======================
// TODO (version 2): Round Robin
void Simulador::planificar() {
    cout << "(planificador no implementado)\n";
}

void Simulador::ejecutarCiclo() {
    ciclo++;
    cout << "\nCICLO " << ciclo << "\n";

    entrarProceso();   // 1) memoria
    planificar();      // 2) Round Robin

    double ocup = 100.0 * mem.ocupado() / mem.total;
    sumaOcupacion += ocup;
    ocupacionMax = max(ocup, ocupacionMax);
    cout << fixed << setprecision(1) << "Ocupación de memoria: " << ocup
         << "% | Terminados: " << terminados
         << " | ([+] rápido [-] lento [p] pausa [q] salir) + [Enter]\n";
}

void Simulador::correr() {
    cout << "\nMemoria inicial:\n";
    mem.imprimir();
    while (!ctl.salir) {
        if (ctl.pausa) { this_thread::sleep_for(chrono::milliseconds(100)); continue; }
        ejecutarCiclo();
        int restante = ctl.retardoMs;
        while (restante > 0 && !ctl.salir) {   // espera "cortada" para responder rapido
            int paso = min(restante, 50);
            this_thread::sleep_for(chrono::milliseconds(paso));
            restante -= paso;
        }
    }
}

void Simulador::estadisticas() const {
    cout << "\nESTADÍSTICAS\n";
    cout << fixed << setprecision(2);
    cout << "Ciclos simulados:              " << ciclo << "\n";
    cout << "Procesos atendidos (terminados): " << terminados << "\n";
    cout << "Tiempo medio de atención:      "
         << (terminados ? (double)sumaTurnaround / terminados : 0.0) << " ciclos\n";
    cout << "Ocupación media de memoria:    "
         << (ciclo ? sumaOcupacion / ciclo : 0.0) << " %\n";
    cout << "Ocupación máxima de memoria:   " << ocupacionMax << " %\n";
    cout << "Unidades de CPU ejecutadas:    " << unidadesCPU << "\n";
}


int leerEntero(const string& msg, int lo, int hi) {
    int v;
    while (true) {
        cout << msg << " [" << lo << "-" << hi << "]: ";
        if (cin >> v && v >= lo && v <= hi) return v;
        if (cin.eof()) exit(0);
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "  Valor inválido.\n";
    }
}

Config configurar() {
    Config c;
    int m = leerEntero("Tamaño de memoria en MB (1, 4 u 8)", 1, 8);
    while (m != 1 && m != 4 && m != 8) m = leerEntero("Solo 1, 4 u 8 MB", 1, 8);
    c.memoriaKB = m * 1024;
    c.cuantoSistema    = leerEntero("Cuanto del sistema", 1, 100);
    c.tamMaxProceso    = leerEntero("Tamaño máximo de proceso (KB)", SPLIT_MIN, c.memoriaKB);
    c.cuantoMaxProceso = leerEntero("Cuanto máximo por proceso", 1, 1000);
    cout << "Algoritmo: 1) Primer Ajuste  2) Mejor Ajuste  3) Combinado\n";
    c.alg = (Algoritmo)leerEntero("Elige", 1, 3);
    c.retardoMs = leerEntero("Retardo inicial entre ciclos (ms)", 0, 10000);
    cin.ignore(10000, '\n');
    return c;
}
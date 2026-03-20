//
// Created by juanp on 16/03/2026.
//

#include "scheduler.h"
#include <stdexcept>
#include <exception>
#include <sstream>
#include <iostream>

//==Constructor==
SCHEDULER::SCHEDULER(std::vector<std::string> &arguments) {
    /*
     Este constructor inicializa el simulador de planificación.

     Recibe los argumentos de ejecución del programa:
     arguments[0] = archivo de entrada con los procesos
     arguments[1] = algoritmo de planificación (MLQ, MLFQ o uno simple)
     arguments[2..] = parámetros adicionales (número de colas, quantum, etc.)

     Ejemplo:
     SchedulingSim.exe input.txt MLFQ 3 1 2 SJF
    */

    // Cargar los procesos desde el archivo de entrada
    dataTable.extractDataFromFile(arguments[0]);

    // Inicializar estructura de colas (MLQ) y variables de control
    MLQ.clear();        // vector de colas
    numQ = 0;           // número de colas
    currentTime = 0;    // tiempo actual de la simulación

    // Normalizar el nombre del algoritmo a mayúsculas
    arguments[1] = toUpper(arguments[1]);

    // ============================================================
    // CASO 1: MULTI-LEVEL QUEUE (MLQ)
    // ============================================================
    if (arguments[1] == "MLQ") {
        multiQueueType = 1;

        // Número de colas
        int n = std::stoi(arguments[2]);

        std::string algName;
        std::string algParam;
        std::istringstream algBuffer;

        int i = 0;

        // Crear cada cola con su algoritmo correspondiente
        for (i = 0; i < n && i < arguments.size(); i++) {
            /*
             Cada argumento viene como:
             "ALGORITMO PARAM"
             Ej: "RR 4", "SJF"
            */
            algBuffer.clear();
            algBuffer.str(arguments[3 + i]);

            // Extraer nombre del algoritmo y su parámetro (si existe)
            algBuffer >> algName;
            algBuffer >> algParam;

            // Crear la cola con ese algoritmo
            emplaceAlg(algName, algParam);
        }

        // Validación: verificar que se definieron todas las colas
        if (i != n) {
            throw std::invalid_argument(
                "The provided queue algorithm has missing algorithms\nN = " +
                std::to_string(n) + "; Provided = " + std::to_string(i)
            );
        }
    }

    // ============================================================
    // CASO 2: MULTI-LEVEL FEEDBACK QUEUE (MLFQ)
    // ============================================================
    else if (arguments[1] == "MLFQ") {
        multiQueueType = 2;

        // Número de colas
        int n = std::stoi(arguments[2]);

        std::string algName = "RR";  // Todas las colas iniciales son RR
        std::string algParam;
        std::istringstream algBuffer;

        int i = 0;

        // Crear las primeras n-1 colas como Round Robin
        for (i = 0; i < n - 1 && i < arguments.size(); i++) {
            // arguments[3 + i] contiene el quantum de cada cola
            emplaceAlg(algName, arguments[3 + i]);
        }

        // La última cola puede tener un algoritmo distinto (ej: SJF)
        if (i == (n - 1)) {
            algBuffer.clear();
            algBuffer.str(arguments[3 + i]);

            algBuffer >> algName;
            algBuffer >> algParam;

            emplaceAlg(algName, algParam);
        }
        else {
            throw std::invalid_argument(
                "The provided queue algorithm has missing algorithms\nN = " +
                std::to_string(n) + "; Provided = " + std::to_string(i)
            );
        }
    }

    // ============================================================
    // CASO 3: ALGORITMO SIMPLE (UNA SOLA COLA)
    // ============================================================
    else {
        multiQueueType = 0;

        // Si el algoritmo requiere parámetro (ej: RR 4)
        if (arguments.size() > 2) {
            emplaceAlg(arguments[1], arguments[2]);
        }
        // Si no requiere parámetro (ej: FCFS, SJF)
        else {
            emplaceAlg(arguments[1], arguments[1]);
        }
    }

    // ============================================================
    // ASIGNACIÓN DE PROCESOS A COLAS
    // ============================================================
    /*
     Después de crear las colas, se asigna cada proceso a su cola correspondiente.
     - En MLQ: cada proceso ya tiene una cola definida en el input
     - En otros casos: todos los procesos van a la misma cola
    */
    assignProcesses();

    // ============================================================
    // REGISTRO DE TIEMPOS RELEVANTES
    // ============================================================
    /*
     Se almacenan los tiempos de llegada de los procesos.
     Esto permite simular eventos discretos y saber cuándo
     debe avanzar el tiempo si no hay procesos disponibles.
    */
    setRelevantTimes();
}

//==Métodos privados==
std::string SCHEDULER::toUpper(const std::string& input) {
    std::string result = input;
    for (char& c : result) {
        c = std::toupper(static_cast<unsigned char>(c));
    }
    return result;
}

void SCHEDULER::emplaceAlg(std::string &alg, std::string &param) {
    alg = toUpper(alg);
    if (alg == "FCFS") {
        MLQ.emplace_back(false, false, 0, 0);
    }
    else if (alg == "SJF") {
        MLQ.emplace_back(false, false, 1, 0);
    }
    else if (alg == "PSJF") {
        MLQ.emplace_back(true, false, 2, 0);
    }
    else if (alg == "PRIORITY") {
        param = toUpper(param);
        if (param == "ASC") {
            MLQ.emplace_back(false, true, 3, 0);
        }
        else {
            MLQ.emplace_back(false, false, 3, 0);
        }
    }
    else if (alg == "P-PRIORITY") {
        param = toUpper(param);
        if (param == "ASC") {
            MLQ.emplace_back(true, true, 4, 0);
        }
        else {
            MLQ.emplace_back(true, false, 4, 0);
        }
    }
    else if (alg == "RR") {
        MLQ.emplace_back(true, false, 5, std::stoi(param));
    }
    else {
        throw std::invalid_argument("The algorithm '" + alg + "' is not supported by the simulator.");
    }
    numQ++;
}

void SCHEDULER::assignProcesses() {
    if (multiQueueType == 1) {
        for (int i = 0; i < dataTable.getSize(); i++) {
            MLQ[dataTable.getQueue()[i] - 1].addProcess(i, dataTable.getArrivalTime()[i]);
        }
    }
    else {
        for (int i = 0; i < dataTable.getSize(); i++) {
            MLQ[0].addProcess(i, dataTable.getArrivalTime()[i]);
        }
    }
}

void SCHEDULER::setRelevantTimes() {
    for (int i = 0; i < dataTable.getSize(); i++) {
        relevantTimes.emplace(dataTable.getArrivalTime()[i]);
    }
}

void SCHEDULER::simulation() {
    /*
     Función principal de simulación del scheduler.

     La lógica general es:
     - Mientras existan procesos sin completar
     - Recorrer las colas desde mayor prioridad (cola 0)
     - Ejecutar el primer proceso disponible
     - Si no hay procesos listos, avanzar el tiempo al siguiente evento (arrival)

     Esto implementa una simulación basada en eventos (event-driven simulation).
    */

    int numCola;              // índice de la cola actual
    bool proccessFound = true; // indica si se ejecutó algún proceso en este ciclo
    int p;                    // id del proceso ejecutado

    numPCompleted = 0;        // número de procesos completados

    // Loop principal: continúa hasta que todos los procesos terminen
    while (numPCompleted < dataTable.getSize()) {

        proccessFound = false; // reiniciamos la bandera
        numCola = 0;

        // Recorremos las colas desde la de mayor prioridad (0)
        while (numCola < numQ && !proccessFound) {

            // Si la cola tiene procesos
            if (!MLQ[numCola].isEmpty()) {

                // Intentamos ejecutar un proceso de esta cola
                p = executeProcess(numCola);

                // Si se ejecutó un proceso válido
                if (p != -1) {
                    proccessFound = true; // ya encontramos algo para ejecutar
                }
            }

            numCola++; // pasamos a la siguiente cola (menor prioridad)
        }

        // Si no se encontró ningún proceso listo para ejecutar
        if (!proccessFound) {

            // Avanzamos el tiempo al siguiente evento relevante (arrival de un proceso)
            if (!relevantTimes.empty()) {
                currentTime = *relevantTimes.begin();  // próximo tiempo de llegada
                relevantTimes.erase(relevantTimes.begin());
            }
        }
    }

    // ============================================================
    // CÁLCULO DE TURNAROUND TIME (TAT)
    // ============================================================
    /*
     TAT = Completion Time - Arrival Time

     Se calcula al final para cada proceso una vez terminada la simulación.
    */
    for (int i = 0; i < dataTable.getSize(); i++) {
        dataTable.getTAT()[i] =
            dataTable.getCompletionTime()[i] - dataTable.getArrivalTime()[i];
    }
}

int SCHEDULER::executeProcess(int numCola) {
    /*
     Ejecuta un proceso de la cola 'numCola' según su algoritmo de planificación.

     Esta función:
     - Selecciona el proceso a ejecutar
     - Determina cuánto tiempo se ejecuta (según el algoritmo)
     - Actualiza tiempos (currentTime, waiting, completion, etc.)
     - Maneja preemption (interrupciones)
     - Maneja cambios de cola (en MLFQ)

     Retorna:
     - id del proceso ejecutado
     - -1 si no hay proceso válido en ese momento
    */

    int p;              // proceso seleccionado
    int passedTime;     // tiempo que se ejecuta en esta iteración
    int startTime;      // inicio de ejecución
    int endTime;        // fin de ejecución

    // Eliminar tiempos de llegada que ya pasaron
    while (!relevantTimes.empty() && *relevantTimes.begin() <= currentTime) {
        relevantTimes.erase(relevantTimes.begin());
    }

    // Seleccionar el proceso según el algoritmo de la cola
    p = determineProcess(numCola);

    // ============================================================
    // CASO 0: No hay proceso listo
    // ============================================================
    if (p == -1) {
        // No hay procesos disponibles en este tiempo
    }

    // ============================================================
    // CASO 1: ALGORITMOS NO EXPROPIATIVOS (FCFS, SJF, PRIORITY)
    // ============================================================
    else if (!MLQ[numCola].isPreemp()) {

        // Ejecuta el proceso completamente
        passedTime = dataTable.getRemainingTime()[p];
        dataTable.getRemainingTime()[p] = 0;

        currentTime += passedTime;

        // Guardar tiempo de finalización
        dataTable.getCompletionTime()[p] = currentTime;
        numPCompleted++;

        // Actualizar tiempos de espera de los demás procesos
        startTime = currentTime - passedTime;
        endTime = currentTime;

        for (int i = 0; i < dataTable.getSize(); i++) {
            if (dataTable.getRemainingTime()[i] > 0 &&
                dataTable.getArrivalTime()[i] <= startTime) {

                dataTable.getWaitingTime()[i] += passedTime;
            }
            else if (dataTable.getRemainingTime()[i] > 0 &&
                     dataTable.getArrivalTime()[i] < endTime) {

                dataTable.getWaitingTime()[i] +=
                    (endTime - dataTable.getArrivalTime()[i]);
            }
        }

        // Limpiar eventos de llegada ya procesados
        for (auto it = relevantTimes.begin(); it != relevantTimes.end();) {
            if (*it <= currentTime) {
                it = relevantTimes.erase(it);
            } else {
                break;
            }
        }

        // Remover el proceso de la cola
        MLQ[numCola].removeProcess(p);
    }

    // ============================================================
    // CASO 2: PREEMPTIVOS (PSJF, P-PRIORITY)
    // ============================================================
    else if (MLQ[numCola].get_algID() == 2 || MLQ[numCola].get_algID() == 4) {

        /*
         Ejecuta hasta:
         - terminar el proceso
         - o hasta que llegue un nuevo proceso (posible interrupción)
        */

        if (relevantTimes.empty() ||
            currentTime + dataTable.getRemainingTime()[p] < *(relevantTimes.begin())) {

            // El proceso termina completamente
            passedTime = dataTable.getRemainingTime()[p];

            MLQ[numCola].removeProcess(p);
            dataTable.getCompletionTime()[p] = currentTime + passedTime;
            numPCompleted++;

            if (!relevantTimes.empty() && currentTime + passedTime == *relevantTimes.begin()) {
                relevantTimes.erase(relevantTimes.begin());
            }
        }
        else {
            // Se ejecuta hasta el siguiente evento (preemption)
            passedTime = *(relevantTimes.begin()) - currentTime;
            if (!relevantTimes.empty() && currentTime + passedTime == *relevantTimes.begin()) {
                relevantTimes.erase(relevantTimes.begin());
            }
            if (multiQueueType == 2 && numCola != numQ - 1) {
                MLQ[numCola].removeProcess(p);
                MLQ[numCola + 1].addProcess(p, currentTime + passedTime);
            }
        }

        // Actualizar tiempo restante y reloj
        dataTable.getRemainingTime()[p] -= passedTime;
        currentTime += passedTime;

        // Actualizar waiting time de los demás procesos
        startTime = currentTime - passedTime;
        endTime = currentTime;

        for (int i = 0; i < dataTable.getSize(); i++) {
            if (i != p &&
                dataTable.getRemainingTime()[i] > 0 &&
                dataTable.getArrivalTime()[i] <= startTime) {

                dataTable.getWaitingTime()[i] += passedTime;
            }
            else if (i != p &&
                     dataTable.getRemainingTime()[i] > 0 &&
                     dataTable.getArrivalTime()[i] < endTime) {

                dataTable.getWaitingTime()[i] +=
                    (endTime - dataTable.getArrivalTime()[i]);
            }
        }
    }

    // ============================================================
    // CASO 3: ROUND ROBIN (RR)
    // ============================================================
    else if (MLQ[numCola].get_algID() == 5) {
        //RR
        passedTime = std::min(dataTable.getRemainingTime()[p],MLQ[numCola].get_quantum());

        if (dataTable.getRemainingTime()[p] <= passedTime) {
            // Proceso termina
            MLQ[numCola].removeProcess(p);
            dataTable.getCompletionTime()[p] = currentTime + passedTime;
            numPCompleted++;

            if (!relevantTimes.empty() && currentTime + passedTime == *relevantTimes.begin()) {
                relevantTimes.erase(relevantTimes.begin());
            }
        }
        else if (passedTime == MLQ[numCola].get_quantum()) {
            if (multiQueueType == 2 && numCola != numQ - 1) {
                MLQ[numCola].removeProcess(p);
                MLQ[numCola + 1].addProcess(p, currentTime + passedTime);
            }
            else {
                MLQ[numCola].removeProcess(p);
                MLQ[numCola].addProcess(p, currentTime + passedTime);
            }

            if (!relevantTimes.empty() && currentTime + passedTime == *relevantTimes.begin()) {
                relevantTimes.erase(relevantTimes.begin());
            }
        }

        // Actualizar tiempos
        dataTable.getRemainingTime()[p] -= passedTime;
        currentTime += passedTime;

        startTime = currentTime - passedTime;
        endTime = currentTime;

        for (int i = 0; i < dataTable.getSize(); i++) {
            if (i != p &&
                dataTable.getRemainingTime()[i] > 0 &&
                dataTable.getArrivalTime()[i] <= startTime) {

                dataTable.getWaitingTime()[i] += passedTime;
            }
            else if (i != p &&
                     dataTable.getRemainingTime()[i] > 0 &&
                     dataTable.getArrivalTime()[i] < endTime) {

                dataTable.getWaitingTime()[i] +=
                    (endTime - dataTable.getArrivalTime()[i]);
            }
        }
    }

    // ============================================================
    // RESPONSE TIME (primera vez que ejecuta)
    // ============================================================
    if (p != -1 && dataTable.getResponseTime()[p] == -1) {
        dataTable.getResponseTime()[p] = currentTime - passedTime;
    }

    return p;
}

int SCHEDULER::determineProcess(int numCola) {
    /*
     Selecciona el proceso a ejecutar dentro de una cola específica,
     según el algoritmo de planificación asociado a esa cola.

     Retorna:
     - ID del proceso seleccionado
     - -1 si no hay procesos disponibles en el tiempo actual
    */

    int id = MLQ[numCola].get_algID(); // tipo de algoritmo de la cola
    int process;

    // Variables auxiliares para encontrar el "mejor" candidato
    std::string firstTag = "!"; // desempate lexicográfico
    int firstTime = -1;         // valor de comparación (arrival, burst, priority, etc.)
    int firstProcess = -1;      // proceso seleccionado

    // ============================================================
    // CASO 1: FCFS (First Come First Serve)
    // ============================================================
    if (id == 0) {
        /*
         Selecciona el proceso que llegó primero (menor arrivalTime).
         En caso de empate, usa el identificador (tag).
        */
        for (auto it = MLQ[numCola].getAssociatedProcesses().begin();
             it != MLQ[numCola].getAssociatedProcesses().end(); ++it) {

            process = *it;

            if (dataTable.getArrivalTime()[process] <= currentTime &&
                (firstTime == -1 ||
                 dataTable.getArrivalTime()[process] < firstTime ||
                 (dataTable.getArrivalTime()[process] == firstTime &&
                  (firstTag == "!" ||
                   dataTable.getProcessTag()[process] < firstTag)))) {

                firstTime = dataTable.getArrivalTime()[process];
                firstTag = dataTable.getProcessTag()[process];
                firstProcess = process;
            }
        }
    }

    // ============================================================
    // CASO 2: SJF / PSJF
    // ============================================================
    else if (id == 1 || id == 2) {
        /*
         SJF → menor burst (no expropiativo)
         PSJF → menor tiempo restante (expropiativo)

         Nota: Se unifica lógica porque un proceso puede venir parcialmente ejecutado.
        */

        // Caso especial: primera vez en SJF (usa arrivalTime)
        if (id == 1 && numCola == 0 && MLQ[numCola].isFirstTimeSJF()) {

            for (auto it = MLQ[numCola].getAssociatedProcesses().begin();
                 it != MLQ[numCola].getAssociatedProcesses().end(); ++it) {

                process = *it;

                if (dataTable.getArrivalTime()[process] <= currentTime &&
                    (firstTime == -1 ||
                     dataTable.getArrivalTime()[process] < firstTime ||
                     (dataTable.getArrivalTime()[process] == firstTime &&
                      dataTable.getProcessTag()[process] < firstTag))) {

                    firstTime = dataTable.getArrivalTime()[process];
                    firstTag = dataTable.getProcessTag()[process];
                    firstProcess = process;
                }
            }

            // Después de la primera ejecución ya usa remainingTime
            MLQ[numCola].set_firstTimeSJF(false);
        }
        else {
            // Selecciona el proceso con menor tiempo restante
            for (auto it = MLQ[numCola].getAssociatedProcesses().begin();
                 it != MLQ[numCola].getAssociatedProcesses().end(); ++it) {

                process = *it;

                if (dataTable.getArrivalTime()[process] <= currentTime &&
                    (firstTime == -1 ||
                     dataTable.getRemainingTime()[process] < firstTime ||
                     (dataTable.getRemainingTime()[process] == firstTime &&
                      dataTable.getProcessTag()[process] < firstTag))) {

                    firstTime = dataTable.getRemainingTime()[process];
                    firstTag = dataTable.getProcessTag()[process];
                    firstProcess = process;
                }
            }
        }
    }

    // ============================================================
    // CASO 3: PRIORITY / P-PRIORITY
    // ============================================================
    else if (id == 3 || id == 4) {
        /*
         Selecciona el proceso según prioridad:
         - ASC → menor número = mayor prioridad
         - DESC → mayor número = mayor prioridad
        */

        if (MLQ[numCola].isAscending()) {
            // Prioridad ascendente (menor es mejor)
            for (auto it = MLQ[numCola].getAssociatedProcesses().begin();
                 it != MLQ[numCola].getAssociatedProcesses().end(); ++it) {

                process = *it;

                if (dataTable.getArrivalTime()[process] <= currentTime &&
                    (firstTime == -1 ||
                     dataTable.getPriority()[process] < firstTime ||
                     (dataTable.getPriority()[process] == firstTime &&
                      dataTable.getProcessTag()[process] < firstTag))) {

                    firstTime = dataTable.getPriority()[process];
                    firstTag = dataTable.getProcessTag()[process];
                    firstProcess = process;
                }
            }
        }
        else {
            // Prioridad descendente (mayor es mejor)
            for (auto it = MLQ[numCola].getAssociatedProcesses().begin();
                 it != MLQ[numCola].getAssociatedProcesses().end(); ++it) {

                process = *it;

                if (dataTable.getArrivalTime()[process] <= currentTime &&
                    (firstTime == -1 ||
                     dataTable.getPriority()[process] > firstTime ||
                     (dataTable.getPriority()[process] == firstTime &&
                      dataTable.getProcessTag()[process] < firstTag))) {

                    firstTime = dataTable.getPriority()[process];
                    firstTag = dataTable.getProcessTag()[process];
                    firstProcess = process;
                }
            }
        }
    }

    // ============================================================
    // CASO 4: ROUND ROBIN (RR)
    // ============================================================
    else if (id == 5) {
        /*
         Selecciona el proceso que llegó primero a la cola (FIFO interno).
         Aquí no se usa burst ni prioridad, solo orden de llegada a la cola.
        */

        for (auto it = MLQ[numCola].getAssociatedProcesses().begin();
             it != MLQ[numCola].getAssociatedProcesses().end(); ++it) {

            process = *it;

            if (MLQ[numCola].getArrivalT()[process] <= currentTime &&
                (firstTime == -1 ||
                 MLQ[numCola].getArrivalT()[process] < firstTime ||
                 (MLQ[numCola].getArrivalT()[process] == firstTime &&
                  dataTable.getProcessTag()[process] < firstTag))) {

                firstTime = MLQ[numCola].getArrivalT()[process];
                firstTag = dataTable.getProcessTag()[process];
                firstProcess = process;
            }
        }
    }

    return firstProcess;
}

//==Getters==

std::vector<QUEUE>& SCHEDULER::getMLQ() {
    return MLQ;
}

TABLE& SCHEDULER::getTable() {
    return dataTable;
}
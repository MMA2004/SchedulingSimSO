# MLFQ Scheduler Simulator

Este proyecto implementa un simulador de planificación de procesos utilizando el algoritmo **MLFQ (Multi-Level Feedback Queue)**, desarrollado en C++ bajo el paradigma de programación orientada a objetos.

---

## Descripción

El simulador permite ejecutar diferentes configuraciones de colas de planificación, combinando algoritmos como:

- Round Robin (RR)
- Shortest Job First (SJF)
- Shortest Time to Completion First (STCF)
- Priority

Los procesos son leídos desde archivos de entrada y el sistema calcula métricas como:

- Waiting Time (WT)
- Completion Time (CT)
- Response Time (RT)
- Turnaround Time (TAT)

## Ejemplos

En la carpeta **`testFiles/`** se encuentran archivos de entrada de prueba para cada algoritmo.

En el archivo **`solucionesAMano.xlsx`** se encuentran los casos de prueba resueltos manualmente, incluyendo:

- Orden de ejecución esperado
- Cálculo paso a paso
- Validación de resultados del simulador

Esto permite verificar la correcta implementación del algoritmo.

---

## Ejecución

### Compilar

```bash 
  g++ *.cpp -o SchedulingSimSO
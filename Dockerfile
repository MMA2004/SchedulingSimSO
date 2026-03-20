FROM japeto/so-tools:v64

# Directorio de trabajo dentro del contenedor
WORKDIR /app

# Copiar archivos uno por uno
COPY SchedulingSim.cpp .
COPY queue.cpp .
COPY queue.h .
COPY table.cpp .
COPY table.h .
COPY scheduler.cpp .
COPY scheduler.h .

# Copiar toda la carpeta testFiles al contenedor
COPY testFiles/ ./testFiles/

# Compilar manualmente
RUN g++ -std=c++20 -O2 -static -static-libgcc -static-libstdc++ \
    SchedulingSim.cpp queue.cpp table.cpp scheduler.cpp \
    -o SchedulingSimSO
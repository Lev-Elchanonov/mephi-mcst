## Сборка

### Обычная сборка
```bash
cmake -S . -B build
cmake --build build -j
./build/graph
```

### Сборка с санитайзерами и DEBUG
```bash
cmake -S . -B build-debug -DENABLE_SAN=ON -DENABLE_DEBUG=ON
cmake --build build-debug -j
./build-debug/graph
```
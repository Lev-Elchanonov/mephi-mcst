## Сборка

### Обычная сборка
```bash
cmake -S . -B build
cmake --build build -j
./build/bin/graph
```

### Сборка с санитайзерами и DEBUG
```bash
cmake -S . -B build -DSAN=ON -DDEBUG=ON
cmake --build build -j
./build/bin/graph
```

### Сборка фронтенда
```bash
cmake -S . -B build -DAST=ON
cmake --build build -j
./build/bin/lang <file.lang>
```

### Сборка с визуализацией графа
```bash
cmake -S . -B build -DVIEW=ON
cmake --build build -j
./build/bin/graph
```

### Сборка тестов
```bash
cmake -S . -B build -DAST=ON -DTESTS=ON
cmake --build build -j
./build/bin/tests
```

### Сборка IR (промежуточного представления) и CFG
```bash
cmake -S . -B build -DAST=ON -DIR=ON
cmake --build build -j
```

### Запуск IR и CFG
```bash
# IR в текстовом виде
./build/bin/ir_test --ir <file.lang>
# CFG в текстовом виде
./build/bin/ir_test --cfg <file.lang>
# CFG в PNG (открывается автоматически)
./build/bin/ir_test --cfg-png <file.lang>
```


### Собрать все вместе
```bash
cmake -S . -B build -DAST=ON -DVIEW=ON -DTESTS=ON -DSAN=ON -DDEBUG=ON
cmake --build build -j
```
# Projeto 4 - Sistemas Distribuídos

## Autores
- Rodrigo Antunes - fc57879
- Rodrigo Santos - fc61825
- Teresa Grangeia - fc61869

## Estrutura do Projeto
```
/binary/  - executáveis (list_client, list_server)
/include/ - ficheiros header (.h)
/source/  - ficheiros fonte (.c)
/object/  - ficheiros objeto (.o)
/lib/     - biblioteca estática (liblist.a)
```

## Compilação
```bash
make all         # compila tudo (biblioteca + cliente + servidor)
make proto       # gera ficheiros ProtoBuf
make liblist     # compila biblioteca liblist.a
make list_client # compila cliente
make list_server # compila servidor
make clean       # remove ficheiros gerados
make clean-proto # remove ficheiros gerados pelo ProtoBuf
```

## Execução

**Exemplo:**
```bash
./binary/list_server 8000 127.0.0.1:2181
./binary/list_client 127.0.0.1:2181
```

---

**Âmbito:** Projeto nº4 elaborado no âmbito da cadeira de Sistemas Distribuídos 2025/26, FCUL
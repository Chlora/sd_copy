# Projeto 4 - Sistemas Distribuídos

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
**Servidor:**
```bash
./binary/list_server <port> <zk_host:zk_port>
```
- `<port>`: Porta TCP para o servidor (1024-65535)
- `<zk_host:zk_port>`: Endereço do servidor ZooKeeper

**Cliente:**
```bash
./binary/list_client <zk_host:zk_port>
```
- `<zk_host:zk_port>`: Endereço do servidor ZooKeeper

**Exemplo:**
```bash
./binary/list_server 8000 127.0.0.1:2181
./binary/list_client 127.0.0.1:2181
```

## Gestão de Memória
- A execução com valgrind poderá reportar memória "still reachable" devido a bibliotecas internas do ZooKeeper. Esta memória é gerida pela biblioteca e libertada automaticamente quando o processo termina, não constituindo um memory leak.
---

**Âmbito:** Projeto nº4 elaborado no âmbito da cadeira de Sistemas Distribuídos 2025/26, FCUL
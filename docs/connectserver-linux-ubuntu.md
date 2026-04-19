# ConnectServer Linux Ubuntu

## Objetivo

Esta etapa inicia a conversao do `Source/ConnectServerLinux` para execucao em Linux Ubuntu sem alterar os protocolos de rede do ConnectServer.

## O que foi alterado

- O build em `Visual Studio` foi substituido por `CMake` em `Source/ConnectServerLinux/CMakeLists.txt`.
- A entrada grafica Win32 foi removida e o processo agora roda em console.
- A leitura de configuracao deixou de depender do diretório atual.
- O ConnectServer sempre resolve os arquivos a partir da pasta `Data` ao lado do executavel.
- Os logs continuam sendo gravados localmente ao lado do binario.
- Foram adicionados comandos de console:
  - `reload list`
  - `reload servers`
  - `exit`

## Resolucao de caminhos

O executavel passa a resolver os arquivos usando o diretório real do binario:

- `Data/ConnectServer.ini`
- `Data/ServerList.dat`
- `Data/Update/*`

Isso permite dois cenarios:

1. Saida padrao em `Archives/1.- ConnectServer`.
2. Copia do binario para outra pasta, desde que a pasta `Data` acompanhe o executavel.

## Build com CMake

No Ubuntu, a compilacao esperada e:

```bash
cd Source/ConnectServerLinux
cmake -S . -B build
cmake --build build -j
```

Por padrao, o `CMake` envia o binario para:

```text
Archives/1.- ConnectServer
```

Nome do executavel:

```text
connectserver
```

## Execucao

Executando o binario no Linux:

```bash
./connectserver
```

Comandos de runtime:

- `reload list`: recarrega `Data/ServerList.dat`.
- `reload servers`: recarrega `Data/ConnectServer.ini` para limites e versao de update, depois recarrega `Data/Update`.
- `exit`: encerra o processo.

## Observacoes desta primeira conversao

- Os pacotes `0xF4:03`, `0xF4:04`, `0xF4:06`, `0xF4:08` e `0xF4:09` foram preservados.
- O loop de rede Windows baseado em IOCP foi substituido por sockets POSIX com `accept`, `poll`, `recv`, `send`, `recvfrom` e `sendto`.
- Alteracoes de porta em `ConnectServer.ini` via `reload servers` sao detectadas, mas exigem reinicio para rebind de TCP/UDP.
- A logica de autenticacao protegida em `Protect.cpp` permanece condicionada por `PROTECT_STATE`.
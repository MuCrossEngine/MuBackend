# MuBackend

## Visao geral

Repositorio com os sources e arquivos de runtime do backend do projeto, incluindo a conversao inicial do ConnectServer para execucao em Linux Ubuntu.

## Documentacao

- `docs/connectserver-protocolos-e-criptografia.md`
  - Organizacao dos pacotes, headers, heartbeats, fluxo de update e camada de autenticacao/ofuscacao.

## Changelog

### 2026-04-15

- Iniciada a conversao de `Source/ConnectServerLinux` para Linux Ubuntu.
- Removido o build baseado em `.sln` e adicionado `CMake`.
- Removida a interface Win32 do ConnectServer e substituida por runtime em console.
- Implementada leitura fixa de `Data/ConnectServer.ini` e `Data/ServerList.dat` relativa ao executavel.
- Adicionados os comandos `reload list` e `reload servers`.
- Reestruturados sockets TCP e UDP para base POSIX.
- Organizada a documentacao do ConnectServer em pt-BR dentro de `docs/`.

### 26-04-16

- Testing Discord

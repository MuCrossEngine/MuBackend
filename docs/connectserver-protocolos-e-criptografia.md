# ConnectServer Protocolos e Criptografia

## Visao geral

O ConnectServer continua operando como gateway de descoberta de servidores e distribuicao de update, preservando o formato dos pacotes existentes.

## Headers base

Os headers base continuam definidos em `ConnectServerProtocol.h`.

### Pacotes simples

- `PBMSG_HEAD`
  - Tipo `0xC1`
  - Estrutura: `type`, `size`, `head`

- `PSBMSG_HEAD`
  - Tipo `0xC1`
  - Estrutura: `type`, `size`, `head`, `subh`

### Pacotes com tamanho em word

- `PWMSG_HEAD`
  - Tipo `0xC2`
  - Estrutura: `type`, `size[2]`, `head`

- `PSWMSG_HEAD`
  - Tipo `0xC2`
  - Estrutura: `type`, `size[2]`, `head`, `subh`

### Tipos reservados

- `0xC3`
  - Mantido pelos helpers `setE`
- `0xC4`
  - Mantido pelos helpers `setE`

Nesta fase, a conversao para Linux nao alterou o significado desses headers.

## Fluxos cliente -> ConnectServer

### `C1:F4:03` Solicitar IP e porta do servidor

Estrutura:

- `PMSG_SERVER_INFO_RECV`
  - `ServerCode`

Resposta:

- `PMSG_SERVER_INFO_SEND`
  - `ServerAddress[16]`
  - `ServerPort`

### `C1:F4:04` Validacao de Hardware ID

Estrutura:

- `PMSG_SERVER_HWID_RECV`
  - `ComputerHardwareId[36]`

Comportamento:

- O ConnectServer verifica o limite em memoria via `CGMHardwareId`.
- Se exceder o limite configurado, a conexao e encerrada.

### `C1:F4:06` Solicitar lista de servidores

Estrutura:

- `PMSG_SERVER_LIST_RECV`

Resposta:

- `PMSG_SERVER_LIST_SEND`
- Lista de `PMSG_SERVER_LIST`

Campos da lista:

- `ServerCode`
- `UserTotal`
- `type`
- `ServerName[32]`

### `C1:F4:08` Solicitar versao/update

Estrutura:

- `PMSG_SERVER_VERSION_RECV`
  - `ClientVersion`

Resposta:

- `PMSG_SERVER_VERSION_UPDATE_SEND`
- Sequencia de `PMSG_SERVER_FILE_VERSION_SEND` quando ha update

## Fluxos UDP servidor -> ConnectServer

### `C1:01` Heartbeat do GameServer

Estrutura:

- `SDHP_GAME_SERVER_LIVE_RECV`
  - `ServerCode`
  - `UserTotal`
  - `UserCount`
  - `AccountCount`
  - `PCPointCount`
  - `MaxUserCount`

Efeito:

- Atualiza estado online do GameServer.
- Atualiza contadores exibidos na lista entregue aos clientes.

### `C1:02` Heartbeat do JoinServer

Estrutura:

- `SDHP_JOIN_SERVER_LIVE_RECV`
  - `QueueSize`

Efeito:

- Marca o JoinServer como online.
- Controla se o ConnectServer pode responder com lista de servidores.

## Arquivo `ServerList.dat`

Carregado por `ServerList.cpp`.

Campos lidos por entrada:

- secao
- `ServerCode` relativo a secao
- `ServerClient`
- `ServerName`
- `ServerAddress`
- `ServerPort`
- `SHOW`

O `ServerCode` final e calculado assim:

```text
ServerCodeFinal = ServerCode + (secao * 20)
```

## Distribuicao de updates

Os arquivos de update continuam sendo lidos em:

```text
Data/Update
```

Regra de nome:

- Os tres primeiros caracteres devem ser numericos.
- O arquivo deve seguir o prefixo `NNN - `.

Exemplo:

```text
803 - launcher.bin
```

O ConnectServer divide os arquivos em blocos para envio usando:

- `PMSG_SERVER_FILE_VERSION_SEND`
  - `ChunkSize`
  - `TotalSize`
  - `Offset`
  - `Version`

## Criptografia e ofuscacao

### Pacotes operacionais do ConnectServer

Nesta base analisada, os pacotes normais do ConnectServer nao aplicam uma camada adicional de criptografia propria no envio da lista e da resposta de servidor. O formato e mantido em `0xC1` e `0xC2`.

### Handshake de autenticacao em `Protect.cpp`

A camada de protecao permanece condicionada por:

```text
PROTECT_STATE
```

Quando ativa, a autenticacao usa:

- `SDHP_AUTH_SERVER_DATA_SEND`
- `SDHP_AUTH_SERVER_DATA_RECV`

Campos relevantes:

- `EncKey`
- `ServerType`
- `CustomerName`
- `CustomerHardwareId`

Transformacao de envio:

- Para cada byte a partir do offset `4`:
  - `byte ^= EncKey + 0x1A`
  - `byte -= 0x69`

Transformacao de recebimento:

- Para cada byte a partir do offset `4`:
  - `byte += 0xDA`
  - `byte ^= EncKey + 0x25`

### Criptografia interna de bloco

Ainda em `Protect.cpp`, quando a protecao esta ativa, existem rotinas de:

- `EncryptBlock`
- `DecryptBlock`

Baseadas em:

- `gProtectTable`
- `CustomerHardwareId`
- `m_AuthInfo.CustomerHardwareId`

Essas rotinas nao foram alteradas nesta conversao inicial para Linux.

## Comandos de recarga

Os comandos expostos no console Linux sao:

- `reload list`
  - Recarrega `ServerList.dat`

- `reload servers`
  - Recarrega `ConnectServer.ini`
  - Recarrega a versao do ConnectServer
  - Revarre `Data/Update`

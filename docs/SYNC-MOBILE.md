# Especificação Técnica de Sincronização Mobile-Desktop (SYNC-MOBILE.md)

Este documento descreve detalhadamente o protocolo de comunicação, os formatos de payload, as taxas de amostragem, os cabeçalhos de rede e as regras de arquitetura do **ShareAudioLite**. O objetivo é servir como uma **especificação formal** para garantir a compatibilidade total entre a implementação Android (mobile) e qualquer cliente ou servidor desktop (Windows/Linux/macOS).

---

## 1. Modelo de Comunicação e Rede

A comunicação utiliza uma arquitetura **Cliente-Servidor de Fluxo Contínuo (Streaming)** baseada sobre o protocolo **TCP/IP** (comunicações HTTP de canal aberto).

### Parâmetros de Socket (Essenciais para Latência Ultra-Baixa)
Para garantir latência em milissegundos e evitar atrasos na transmissão, ambos os lados (Mobile e Desktop) devem configurar seus Sockets TCP com os seguintes parâmetros:

* **TCP_NODELAY (Algoritmo de Nagle desabilitado):** `true`. O algoritmo de Nagle agrupa pequenos pacotes antes de enviá-los. Para transmissão em tempo real, isso deve ser desativado para que cada bloco de áudio seja enviado imediatamente à placa de rede física.
* **SO_REUSEADDR (Reutilização de porta):** `true`. Permite que o servidor seja reiniciado instantaneamente sem entrar no estado `TIME_WAIT` do SO.
* **SO_SNDBUF / SO_RCVBUF:** Recomendado em `65536` bytes (64 KB). Isso evita gargalos de buffer do kernel quando a rede flutua.

### Endereço e Porta Padrão
* **Porta do Servidor:** `8080` (padrão, configurável).
* **Esquema de URL:** `http://<SERVER_IP>:8080/`

---

## 2. Especificação dos Endpoints HTTP

O servidor (seja ele rodando no Android ou no Desktop) deve expor três endpoints HTTP/1.1 fundamentais usando conexões TCP persistentes:

### A. Endpoint `/info` (Metadados do Sistema)
* **Método:** `GET`
* **Descrição:** Retorna o estado atual do servidor e os detalhes do codec ativo. Essencial para que o cliente se autoconfigure antes de ler o stream.
* **Cabeçalhos de Resposta Obrigatórios:**
  ```http
  HTTP/1.1 200 OK
  Content-Type: application/json
  Connection: close
  ```
* **Payload JSON:**
  ```json
  {
    "status": "streaming",
    "connectedClients": 1,
    "sampleRate": 48000,
    "channels": 2,
    "codec": "opus",
    "bitrate": 128000,
    "chunkSize": 4096
  }
  ```
  *(O valor de `"codec"` deve ser `"opus"` ou `"pcm"`)*.

### B. Endpoint `/stream` (Canal de Áudio Contínuo)
* **Método:** `GET`
* **Descrição:** O canal aberto que transmite os dados binários do áudio. O servidor nunca encerra esta conexão de forma voluntária.
* **Cabeçalhos de Resposta Obrigatórios:**
  ```http
  HTTP/1.1 200 OK
  Content-Type: application/octet-stream
  Connection: keep-alive
  Cache-Control: no-cache, no-store, must-revalidate
  Pragma: no-cache
  ```
* **Payload:** Fluxo infinito de bytes de áudio estruturado de acordo com o codec ativo (ver Seção 3).

### C. Endpoint `/` (Cliente Web Embutido)
* **Método:** `GET`
* **Descrição:** Retorna uma página HTML5 estática e leve contendo scripts baseados na **Web Audio API** do navegador para fins de reprodução cruzada direta (desktop sem aplicativo instalado).

---

## 3. Formato dos Payloads de Áudio e Estrutura de Pacotes

O ShareAudioLite suporta dois modos de dados no canal `/stream`. O receptor deve verificar o campo `"codec"` obtido via `/info` para saber como processar o fluxo.

```
       Fluxo do Stream TCP (GET /stream)
       │
       ├───> Se Codec = "pcm" ──> [BLOCO PCM BRUTO 1] [BLOCO PCM BRUTO 2] ... (Sem cabeçalho)
       │
       └───> Se Codec = "opus" ─> [Tamanho: 2 bytes] [Payload Opus] [Tamanho: 2 bytes] [Payload Opus] ...
```

---

### MODO A: PCM Linear (Modos "Balanced" e "Ultrafast")

Neste modo, o áudio é transmitido sem nenhuma compressão. É composto puramente por amostras de áudio digital bruto em sequência direta.

#### Especificação de Áudio:
* **Formato de Amostra (Bit Depth):** PCM 16-bit Assinado (`Int16`).
* **Ordenação de Bytes (Endianness):** **Little-Endian** (padrão de sistemas x86 e processamento de som ARM).
* **Taxa de Amostragem (Sample Rate):** `48000 Hz`.
* **Canais:** Estéreo (`2` canais intercalados - Esquerdo/Direito).
  * *Estrutura de Amostra de Canal:* `[L_0][R_0] [L_1][R_1] [L_2][R_2] ...` onde cada `L` ou `R` ocupa exatamente 2 bytes (`Int16`).
  * Portanto, cada amostra estéreo completa ocupa exatamente **4 bytes**.

#### Estrutura de Rede:
Não há cabeçalhos de pacote no nível do aplicativo. O servidor escreve os blocos de bytes diretamente na rede e o cliente lê os blocos continuamente. Os tamanhos de bloco definidos no ShareAudioLite são:

1. **Balanced Mode:**
   * **Tamanho do Bloco de Transmissão (Chunk Size):** `2048 bytes` (equivalente a 512 amostras de som inteiras ou `~10.66 ms` de áudio).
2. **Ultrafast Mode:**
   * **Tamanho do Bloco de Transmissão (Chunk Size):** `1024 bytes` (equivalente a 256 amostras de som inteiras ou `~5.33 ms` de áudio).

**Pseudocódigo de Leitura do Receptor Desktop em PCM:**
```python
# loop de leitura contínua
buffer = bytearray(2048)
while streaming_ativo:
    bytes_lidos = socket.read_into(buffer)
    if bytes_lidos > 0:
        audio_output_stream.write(buffer[:bytes_lidos])
```

---

### MODO B: Opus Comprimido (Modo "Quality")

Para reduzir drasticamente a largura de banda ocupada na rede de Wi-Fi local sem perder qualidade, o áudio PCM bruto de entrada é comprimido usando o codec **Opus**. Como o Opus é um codec orientado a pacotes e o TCP é um protocolo de fluxo contínuo de bytes sem fronteiras, é obrigatório encapsular os frames com um indicador de tamanho de pacote.

#### Especificação de Áudio:
* **Frequência de Amostragem do Codificador:** `48000 Hz` (exigência nativa do Opus).
* **Canais:** Estéreo (`2` canais).
* **Bitrate:** `128000 bps` (128 kbps) com Taxa de Bits Constante (CBR).
* **Complexidade:** Nível `5` (otimizado).
* **Janela de Tempo (Frame Size):** `20 ms` por frame.
  * O codificador Opus recebe `4096 bytes` de PCM bruto (`1024` amostras estéreo com 4 bytes cada) e cospe um frame Opus comprimido de tamanho variável (tipicamente entre 100 e 350 bytes).

#### Estrutura do Pacote na Rede (Framing):
Cada pacote no stream TCP é composto por um cabeçalho curto de comprimento seguido pelo payload Opus comprimido:

```
┌───────────────────────────────┬────────────────────────────────────────┐
│  Comprimento do Frame (2B)   │       Dados Comprimidos Opus           │
│  (Big-Endian / unsigned short)│       (Tamanho = "Comprimento")        │
└───────────────────────────────┴────────────────────────────────────────┘
```

* **Cabeçalho (2 bytes):** Um inteiro de 16 bits sem sinal (`uint16_t` / `unsigned short`) codificado em **Big-Endian** (Network Byte Order). Este valor indica o tamanho em bytes do frame comprimido que o segue diretamente.
* **Payload (Tamanho Variável):** O frame bruto de áudio Opus comprimido.

**Conversão de Bytes do Cabeçalho para Tamanho (Big-Endian):**
$$\text{tamanho} = (\text{byte}[0] \ll 8) \mid \text{byte}[1]$$

**Pseudocódigo de Leitura do Receptor Desktop em OPUS:**
```python
while streaming_ativo:
    # 1. Lê exatamente os 2 bytes do cabeçalho
    header = socket.read_exact(2)
    payload_len = (header[0] << 8) | header[1]
    
    # 2. Lê exatamente "payload_len" bytes do frame Opus comprimido
    opus_frame = socket.read_exact(payload_len)
    
    # 3. Decodifica o frame Opus para PCM estéreo de 48kHz
    pcm_bruto = opus_decoder.decode(opus_frame)
    
    # 4. Envia o PCM decodificado para a placa de som local
    audio_output_stream.write(pcm_bruto)
```

---

## 4. Inicialização de Codecs (Mapeamento do MediaCodec / Opus)

Ao portar o receptor para o Desktop (usando bibliotecas como `gstreamer`, `libopus` nativa, `ffmpeg` ou `PortAudio`), o decodificador Opus deve ser iniciado corretamente.

No Android, o decodificador `MediaCodec` é inicializado usando os metadados de codec `csd-0` (Codec Specific Data), que contêm os bytes mágicos do cabeçalho do Opus. Se você estiver implementando um receptor nativo de desktop usando a **`libopus`**, o processo é simplificado:
1. Chame `opus_decoder_create(48000, 2, &error)`.
2. Configure para processar frames de 20ms (`opus_decode` passando o bloco lido).
3. Caso ocorra perda de pacotes ou interrupção na leitura do socket, use a funcionalidade de ocultação de perda de pacotes (PLC) passando `NULL` para o buffer do decodificador para preencher o silêncio sem estalos de som.

---

## 5. Prevenção de Loop de Feedback (Eco)

Uma regra crítica de segurança de rede implementada no ShareAudioLite para evitar microfonias severas e travamentos de loopback:

* **Validação de IP:** O cliente **NUNCA** deve tentar conectar-se a um servidor rodando sob o mesmo IP local do seu próprio adaptador de rede Wi-Fi ou a endereços de loopback (`127.0.0.1`, `localhost`).
* **Implementação Desktop:** Antes de iniciar a requisição ao socket, o cliente desktop deve listar as suas interfaces de rede locais e rejeitar conexões que possuam IPs idênticos ao do servidor.

---

## 6. Checklist de Implementação de um App de Desktop Sincronizado

Para criar uma versão de desktop perfeitamente compatível (ex: em Go, C++, Rust, Python ou C#):

### Se o Desktop for o SERVIDOR (Transmissor):
- [ ] Capture o áudio de saída do Desktop (Stereo Mix, Loopback WASAPI no Windows, ou monitor de áudio PulseAudio/PipeWire no Linux).
- [ ] Faça o resampling automático do áudio capturado para `48000 Hz`, `16-bit Stereo Signed Little-Endian` se a placa de áudio nativa usar outra amostragem.
- [ ] Abra um servidor TCP na porta `8080`. Desative o algoritmo de Nagle (`TCP_NODELAY = true`).
- [ ] Responda às chamadas de `/info` retornando o JSON correto.
- [ ] Quando um cliente chamar `/stream`, envie o cabeçalho HTTP de stream contínuo e comece a bombear os pacotes de áudio continuamente.
  * Se for PCM: Envie blocos de 2048 bytes diretamente.
  * Se for Opus: Envie 2 bytes Big-Endian (comprimento) + frame comprimido.

### Se o Desktop for o RECEPTOR (Cliente):
- [ ] Conecte a `http://<IP_DO_ANDROID>:8080/info` para consultar o codec e o status.
- [ ] Conecte a `http://<IP_DO_ANDROID>:8080/stream` e comece o loop de leitura conforme o codec especificado.
- [ ] Inicialize a sua saída de áudio nativa do desktop (WASAPI/ALSA/Pulse) configurada estritamente para reprodução em `48000 Hz`, `16-bit Stereo`.
- [ ] Em caso de rede lenta ou timeout temporário, limpe e reinicie o buffer do decodificador para manter a latência o mais baixa possível.

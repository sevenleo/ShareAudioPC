# Prompt de Partida para Portabilidade Windows Nativa (ShareAudioLite)

> **Nota de estado atual:** este arquivo foi preservado como ideia/especificação inicial do projeto. A implementação atual mudou para **C++20 + CMake**, com CLI nativo Windows/Linux, protocolo TCP próprio com cabeçalho `SAL1` e suporte atual apenas aos modos PCM `balanced` e `ultrafast`. Opus/Quality Mode, cliente via browser e compatibilidade Android/Web ainda não estão concluídos. Consulte `README.md`, `docs/CURRENT_STATUS.md`, `docs/PLAN.md` e `CHANGELOG.md` para o estado real do projeto.

Este documento contém a especificação técnica detalhada e o prompt estruturado para criar uma versão nativa do **ShareAudioLite para Windows**. A proposta é que este aplicativo seja totalmente compatível, de forma bidirecional (transmissor e receptor), com as versões Android e Web já existentes no ecossistema do projeto.

Salve esta especificação ou copie e cole o prompt final na sua ferramenta de desenvolvimento ou agente de IA favorito para gerar o código-fonte completo.

---

# 📝 Especificação de Portabilidade (Android ──► Windows)

## 1. Tecnologias Recomendadas
Para garantir alto desempenho de áudio, baixa latência e uma interface integrada ao Windows 11, recomendamos:
*   **Linguagem & Framework:** C# com **WinUI 3** (Windows App SDK) ou **WPF (.NET 8/9)**.
*   **APIs de Áudio Nativo (Essencial):** **WASAPI** (Windows Audio Session API) via biblioteca `NAudio` ou similar. O WASAPI nativo em modo loopback é o único capaz de capturar áudio do sistema com latência sub-10ms no Windows de forma limpa.
*   **Codec Opus:** Utilizar um wrapper gerenciado ou biblioteca nativa (como `Concentus` ou a DLL nativa `libopus`) para compressão e descompressão rápida.

## 2. Compatibilidade de Protocolo e Formatos (Bidirecional)
A comunicação em rede deve espelhar exatamente as especificações de canais e cabeçalhos do Android:
*   **Porta de Rede:** TCP padrão `8080`.
*   **Formato Nativo de Captura:** **48.000 Hz, Estéreo, PCM Linear de 16 bits** (sem sinal ou signed de 16 bits, dependendo da convenção).
*   **Modos de Latência / Codecs:**
    1.  **Quality Mode (Opus):**
        *   Áudio comprimido em tempo real usando o codec **Opus** a 128 kbps constante (CBR).
        *   Cada pacote enviado pela rede deve conter um prefixo de cabeçalho de **2 bytes (Big-Endian)** indicando o tamanho exato em bytes do frame Opus subsequente.
    2.  **Balanced Mode (Raw PCM):**
        *   PCM bruto de 16 bits sem compressão.
        *   Agrupamento de pacotes em blocos de **2048 bytes** (~10.6ms de áudio) para transmissão estável em Wi-Fi local.
    3.  **Ultrafast Mode (Raw PCM):**
        *   PCM bruto de 16 bits sem compressão.
        *   Agrupamento de pacotes em blocos de **1024 bytes** (~5.3ms de áudio) para latência mínima absoluta.

---

# 🤖 Prompt de Partida Completo (Copiar e Colar)

Copie o texto abaixo e envie para o seu agente de codificação para iniciar o desenvolvimento do aplicativo Windows:

```markdown
Você é um desenvolvedor especialista em sistemas Windows (.NET C#, WinUI 3 e WASAPI) e processamento de áudio de baixa latência em tempo real.
Seu objetivo é criar um aplicativo Windows nativo e leve chamado **ShareAudioLite** em C# (.NET 8 ou superior) usando **WinUI 3** (ou WPF moderno se preferir estabilidade de distribuição) que funcione tanto como **Transmissor (Servidor)** quanto como **Receptor (Cliente)** de áudio de alta fidelidade via Wi-Fi local (LAN), sendo 100% compatível de forma bidirecional com nossa aplicação Android e cliente Web.

### 🛡️ Requisitos Principais do Aplicativo Windows:

#### 1. Módulo Transmissor (Server - Compartilhar Áudio do PC)
*   **Captura de Áudio do Sistema (Loopback):** Use **WASAPI** no modo loopback (`AUDCLNT_STREAMFLAGS_LOOPBACK`) para capturar todo o áudio sendo reproduzido no dispositivo de som padrão do Windows. A taxa de captura padrão deve ser configurada para **48.000 Hz, Estéreo, 16-bit PCM** para evitar reamostragem pesada.
*   **Servidor de Rede TCP:** Inicie um `TcpListener` na porta padrão `8080`. Ele deve suportar múltiplos clientes ou conexões simultâneas de forma assíncrona usando Tasks do C#.
*   **Modos de Transmissão Compatíveis:**
    *   **Quality Mode (Opus):** Comprima o PCM capturado em tempo real usando o encoder Opus (`Concentus` ou DLL `libopus` nativa) a 128kbps CBR. Envie cada pacote de rede estruturado com: `[Cabeçalho de 2 bytes indicando tamanho do frame (Big-Endian)] + [Dados do Frame Opus]`.
    *   **Balanced Mode:** Envie o PCM capturado diretamente por rede TCP em blocos de exatamente **2048 bytes** (~10.6ms).
    *   **Ultrafast Mode:** Envie o PCM direto em blocos de exatamente **1024 bytes** (~5.3ms) para latência ultra-baixa.

#### 2. Módulo Receptor (Client - Ouvir Áudio de outro Dispositivo)
*   **Conexão de Rede Cliente:** Conecte via TCP Socket a um IP de transmissor na porta `8080` (que pode ser um PC Windows rodando este app ou um celular Android executando o ShareAudioLite).
*   **Prevenção de Auto-Conexão (Loop):** Verifique se o IP de destino é local (`localhost`, `127.0.0.1`, `::1` ou qualquer IP atribuído às placas de rede físicas/virtuais da própria máquina). Se for, exiba um aviso elegante ao usuário e impeça a conexão para evitar loops de feedback de áudio infinitos, travamento do processador ou explosões de ruído (squealing).
*   **Decodificador & Jitter Buffer:**
    *   Implemente uma fila ou buffer adaptativo contra jitter de rede (Jitter Buffer).
    *   Se o servidor enviar no modo Opus (Quality), leia os cabeçalhos de 2 bytes, isole o frame Opus, decodifique-o em PCM de 16 bits.
    *   Se o servidor enviar PCM bruto, consuma os pacotes e jogue no buffer.
*   **Reprodução de Áudio Nativa:** Use WASAPI em modo de baixa latência para reproduzir as amostras decodificadas sem atrasos perceptíveis.

#### 3. Histórico de Dispositivos Recentes
*   Armazene localmente (usando o registro do Windows, arquivo JSON ou `App.xaml` properties) os IPs de transmissores digitados recentemente para reconexão rápida em um clique. Adicione um botão para "Limpar histórico".

#### 4. Design Visual Moderno (Fluent Design & Marca Soundwave)
*   Crie uma interface moderna seguindo as diretrizes do **Fluent Design** do Windows 11 (cantos arredondados, fundo translúcido estilo Mica/Acrylic se disponível, temas claro e escuro automáticos ou manuais).
*   A interface deve ter uma navegação limpa dividida em três abas ou seções principais:
    1.  **Transmitter (Server):** Painel mostrando o status do servidor, o IP local do PC (fácil de copiar para digitar no celular), a seleção do modo de áudio (Quality, Balanced, Ultrafast) e um botão de ação proeminente "Start Sharing" / "Stop Sharing".
    2.  **Receiver (Client):** Painel de conexão com um campo de texto para IP do Transmissor, botão de conectar/desconectar, lista de dispositivos recentes para conexão instantânea, e dicas visuais de conectividade na rede local.
    3.  **Settings:** Opções extras (como seleção específica do dispositivo de áudio para captura/reprodução caso não queira usar o padrão do sistema).
*   **Identidade Visual Soundwave:** Use a paleta de cores da marca:
    *   Gradiente de marca destacado: Verde-Neon (`#1DF09A`) para Azul-Ciano (`#00A3FF`).
    *   Esquema de cores escuras: Slate Escuro (`#1C253E`) migrando para um fundo espacial profundo (`#0B101D`).

Crie uma estrutura de pastas organizada (Models, ViewModels, Services, Views) baseada no padrão MVVM. Forneça o código limpo, documentado em inglês, com tratamento de exceções robusto para conexões de rede perdidas inesperadamente e interrupções de hardware de áudio.
```

#include "lwip/apps/mqtt.h"       // Biblioteca MQTT do lwIP
#include "include/mqtt_comm.h"    // Header file com as declarações locais
// Base: https://github.com/BitDogLab/BitDogLab-C/blob/main/wifi_button_and_led/lwipopts.h
#include "lwipopts.h"             // Configurações customizadas do lwIP
#include <string.h>               // Para strncpy
#include <stdio.h>                // Para sscanf

/* Variável global estática para armazenar a instância do cliente MQTT
 * 'static' limita o escopo deste arquivo */
static mqtt_client_t *client;

/* Variável para armazenar o callback de recebimento de mensagens */
static message_callback_t user_message_callback = NULL;

/* Variável para armazenar o último timestamp válido - para prevenção de replay attacks */
static uint32_t ultima_timestamp_recebida = 0;

/* Variável para armazenar o tópico entre callbacks */
static char current_topic[128];

/* Função para validar mensagens e prevenir replay attacks */
static void on_message(char* topic, char* msg) {
    // 1. Parse do JSON (exemplo simplificado)
    uint32_t nova_timestamp;
    float valor;
    if (sscanf(msg, "{\"valor\":%f,\"ts\":%lu}", &valor, &nova_timestamp) != 2) {
        printf("Erro no parse da mensagem!\n");
        return;
    }

    // 2. Verificação de replay
    if (nova_timestamp > ultima_timestamp_recebida) {
        ultima_timestamp_recebida = nova_timestamp;
        printf("Timestamp mais recente: %.2f (ts: %lu)\n", valor, nova_timestamp);       
    } else {
        printf("Replay detectado (ts: %lu <= %lu)\n",
              nova_timestamp, ultima_timestamp_recebida);
    }
}

/* Callback de conexão MQTT - chamado quando o status da conexão muda
 * Parâmetros:
 *   - client: instância do cliente MQTT
 *   - arg: argumento opcional (não usado aqui)
 *   - status: resultado da tentativa de conexão */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("Conectado ao broker MQTT com sucesso!\n");
    } else {
        printf("Falha ao conectar ao broker, código: %d\n", status);
    }
}

/* Função para configurar e iniciar a conexão MQTT
 * Parâmetros:
 *   - client_id: identificador único para este cliente
 *   - broker_ip: endereço IP do broker como string (ex: "192.168.1.1")
 *   - user: nome de usuário para autenticação (pode ser NULL)
 *   - pass: senha para autenticação (pode ser NULL) */
void mqtt_setup(const char *client_id, const char *broker_ip, const char *user, const char *pass) {
    ip_addr_t broker_addr;  // Estrutura para armazenar o IP do broker
    
    // Converte o IP de string para formato numérico
    if (!ip4addr_aton(broker_ip, &broker_addr)) {
        printf("Erro no IP\n");
        return;
    }

    // Cria uma nova instância do cliente MQTT
    client = mqtt_client_new();
    if (client == NULL) {
        printf("Falha ao criar o cliente MQTT\n");
        return;
    }

    // Configura as informações de conexão do cliente
    struct mqtt_connect_client_info_t ci = {
        .client_id = client_id,  // ID do cliente
        .client_user = user,     // Usuário (opcional)
        .client_pass = pass      // Senha (opcional)
    };

    // Inicia a conexão com o broker
    // Parâmetros:
    //   - client: instância do cliente
    //   - &broker_addr: endereço do broker
    //   - 1883: porta padrão MQTT
    //   - mqtt_connection_cb: callback de status
    //   - NULL: argumento opcional para o callback
    //   - &ci: informações de conexão
    mqtt_client_connect(client, &broker_addr, 1883, mqtt_connection_cb, NULL, &ci);
}

/* Callback de confirmação de publicação
 * Chamado quando o broker confirma recebimento da mensagem (para QoS > 0)
 * Parâmetros:
 *   - arg: argumento opcional
 *   - result: código de resultado da operação */
static void mqtt_pub_request_cb(void *arg, err_t result) {
    if (result == ERR_OK) {
        printf("Publicação MQTT enviada com sucesso!\n");
    } else {
        printf("Erro ao publicar via MQTT: %d\n", result);
    }
}

/* Função para publicar dados em um tópico MQTT
 * Parâmetros:
 *   - topic: nome do tópico (ex: "sensor/temperatura")
 *   - data: payload da mensagem (bytes)
 *   - len: tamanho do payload */
void mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len) {
    // Envia a mensagem MQTT
    err_t status = mqtt_publish(
        client,              // Instância do cliente
        topic,               // Tópico de publicação
        data,                // Dados a serem enviados
        len,                 // Tamanho dos dados
        0,                   // QoS 0 (nenhuma confirmação)
        0,                   // Não reter mensagem
        mqtt_pub_request_cb, // Callback de confirmação
        NULL                 // Argumento para o callback
    );

    if (status != ERR_OK) {
        printf("mqtt_publish falhou ao ser enviada: %d\n", status);
    }
}

/* Callback chamado quando uma mensagem é recebida em um tópico assinado
 * Parâmetros:
 *   - client: instância do cliente MQTT
 *   - arg: argumento opcional (não usado aqui)
 *   - topic: tópico da mensagem recebida
 *   - tot_len: tamanho total do tópico
 */
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    // Armazenamos o tópico para uso posterior no callback de dados
    if (topic && tot_len < sizeof(current_topic) - 1) {
        strncpy(current_topic, topic, tot_len);
        current_topic[tot_len] = '\0';  // Garantir terminação com nulo
    } else {
        // Tópico vazio ou muito longo para o buffer
        current_topic[0] = '\0';
    }
}

/* Callback chamado para cada parte de dados de uma mensagem recebida
 * Parâmetros:
 *   - arg: argumento opcional (não usado aqui)
 *   - data: dados recebidos
 *   - len: tamanho dos dados
 *   - flags: indica se é o último pacote (MQTT_DATA_FLAG_LAST)
 */
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    // Verificamos se é o último pacote de dados
    if (flags & MQTT_DATA_FLAG_LAST) {
        // Criamos um buffer temporário para armazenar os dados
        // e adicionar um terminador nulo para strings
        char buffer[256];
        uint16_t copy_len = len < 255 ? len : 255;
        
        memcpy(buffer, data, copy_len);
        buffer[copy_len] = 0; // Adiciona terminador nulo
        
        on_message(current_topic, buffer);
        
        // Se um callback do usuário foi definido, chamamos ele
        if (user_message_callback) {
            // Agora podemos passar o tópico armazenado
            user_message_callback(current_topic, (const uint8_t*)buffer, copy_len);
        }
        
        printf("Dados recebidos (%d bytes): %s\n", len, buffer);
    }
}

/* Função para definir o callback de recebimento de mensagens
 * Parâmetros:
 *   - callback: função a ser chamada quando uma mensagem for recebida
 */
void mqtt_set_message_callback(message_callback_t callback) {
    user_message_callback = callback;
}

/* Callback chamado quando uma solicitação de inscrição é concluída
 * Parâmetros:
 *   - arg: argumento opcional (não usado aqui)
 *   - result: código de resultado da operação
 */
static void mqtt_sub_request_cb(void *arg, err_t result) {
    if (result == ERR_OK) {
        printf("Inscrição MQTT realizada com sucesso!\n");
    } else {
        printf("Erro ao se inscrever via MQTT: %d\n", result);
    }
}

/* Função para assinar um tópico MQTT
 * Parâmetros:
 *   - topic: tópico a ser assinado
 *   - qos: qualidade de serviço (0, 1 ou 2)
 */
void mqtt_comm_subscribe(const char *topic, uint8_t qos) {
    // Configura os callbacks para mensagens recebidas
    mqtt_set_inpub_callback(
        client,
        mqtt_incoming_publish_cb,
        mqtt_incoming_data_cb,
        NULL
    );
    
    // Assina o tópico especificado
    err_t status = mqtt_subscribe(
        client,              // Instância do cliente
        topic,               // Tópico a ser assinado
        qos,                 // QoS (0=sem confirmação, 1=confirmação única, 2=confirmação dupla)
        mqtt_sub_request_cb, // Callback de confirmação
        NULL                 // Argumento para o callback
    );
    
    if (status != ERR_OK) {
        printf("mqtt_subscribe falhou: %d\n", status);
    }
}
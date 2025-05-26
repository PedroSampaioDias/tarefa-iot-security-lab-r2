// Bibliotecas necessárias
#include <string.h>                 // Para funções de string como strlen()
#include "pico/stdlib.h"            // Biblioteca padrão do Pico (GPIO, tempo, etc.)
#include "pico/cyw43_arch.h"        // Driver WiFi para Pico W
#include "include/wifi_conn.h"      // Funções personalizadas de conexão WiFi
#include "include/mqtt_comm.h"      // Funções personalizadas para MQTT
#include "include/xor_cipher.h"     // Funções de cifra XOR
#include <time.h>

// Callback para processar mensagens MQTT recebidas
void process_message(const char *topic, const uint8_t *data, uint16_t len) {
    // Cria uma cópia terminada em nulo da mensagem recebida
    char message[256];
    uint16_t copy_len = len < 255 ? len : 255;
    memcpy(message, data, copy_len);
    message[copy_len] = '\0';
    
    printf("Mensagem recebida no tópico %s: %s\n", topic ? topic : "desconhecido", message);
    
    // Aqui você pode adicionar lógica para tratar diferentes tópicos
    // Por exemplo, controlar LEDs, atuar em dispositivos, etc.
}

int main() {
    // Inicializa todas as interfaces de I/O padrão (USB serial, etc.)
    stdio_init_all();
    
    // Conecta à rede WiFi
    // Parâmetros: Nome da rede (SSID) e senha
    connect_to_wifi("Andre", "plk330qw");

    // Configura o cliente MQTT
    // Parâmetros: ID do cliente, IP do broker, usuário, senha
    mqtt_setup("bitdog1", "192.168.190.236", "aluno", "senha123");
    
    // Define o callback para processar mensagens recebidas
    mqtt_set_message_callback(process_message);
    
    // Assina o tópico para receber comandos
    mqtt_comm_subscribe("escola/sala1/temperatura", 0);
    
    // Mensagem original a ser enviada
    const char *mensagem = "26.5";
    // Buffer para mensagem criptografada (16 bytes)
    uint8_t criptografada[16];
    // Criptografa a mensagem usando XOR com chave 42
    xor_encrypt((uint8_t *)mensagem, criptografada, strlen(mensagem), 42);

    // Loop principal do programa
    while (true) {
        char mensagem_tempo[30] = {0};
        sprintf(mensagem_tempo, "{\"valor\":26.5,\"ts\":%lu}", time(NULL));

        //mqtt_comm_publish("escola/sala1/temperatura", mensagem_tempo, strlen(mensagem_tempo));
        
        // Aguarda 5 segundos antes da próxima publicação
        sleep_ms(5000);
    }
    return 0;
}

/* 
 * Comandos de terminal para testar o MQTT:
 * 
 * Inicia o broker MQTT com logs detalhados:
 * mosquitto -c mosquitto.conf -v
 * 
 * Assina o tópico de temperatura (recebe mensagens):
 * mosquitto_sub -h localhost -p 1883 -t "escola/sala1/temperatura" -u "aluno" -P "senha123"
 * 
 * Publica mensagem de teste no tópico de temperatura:
 * mosquitto_pub -h localhost -p 1883 -t "escola/sala1/temperatura" -u "aluno" -P "senha123" -m "26.6"
 *
 * Publica um comando para o dispositivo (agora ele pode receber):
 * mosquitto_pub -h localhost -p 1883 -t "escola/sala1/comandos" -u "aluno" -P "senha123" -m "{\"comando\":\"ligar_led\"}"
 */
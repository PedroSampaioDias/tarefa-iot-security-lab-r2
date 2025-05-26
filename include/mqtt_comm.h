#ifndef MQTT_COMM_H
#define MQTT_COMM_H

// Tipo de callback para processamento de mensagens recebidas
typedef void (*message_callback_t)(const char *topic, const uint8_t *data, uint16_t len);

// Funções existentes
void mqtt_setup(const char *client_id, const char *broker_ip, const char *user, const char *pass);
void mqtt_comm_publish(const char *topic, const uint8_t *data, size_t len);

// Novas funções para subscriber
void mqtt_comm_subscribe(const char *topic, uint8_t qos);
void mqtt_set_message_callback(message_callback_t callback);

#endif
[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/G8V_0Zaq)

# Tarefa: IoT Security Lab - EmbarcaTech 2025

Autores: **Andre de Oliveira Melo e Pedro Sampaio Dias**

Curso: Residência Tecnológica em Sistemas Embarcados

Instituição: EmbarcaTech - HBr

Brasília, maio de 2025

---
**OBS: ESTA BRANCH É UM FORK DA BRANCH PRINCIPAL COM CÓDIGO DE SUBSCRIBER NA BITDOGLAB**
---

# Instalação e configuração do Mosquitto

Este guia mostra, passo a passo, como instalar e configurar o broker MQTT Mosquitto no Ubuntu, tanto em modo aberto (sem autenticação) quanto com usuário e senha e TLS.

---

## 1. Atualizar repositórios e instalar pacotes

Abra um terminal e execute:

```bash
sudo apt update
sudo apt install -y mosquitto mosquitto-clients
```
- mosquitto: o broker MQTT

- mosquitto-clients: traz os clientes mosquitto_pub e mosquitto_sub

## 2. Habilitar e iniciar o serviço

```bash
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
```
Verifique o status:

```bash
sudo systemctl status mosquitto
```

## 3. Escutar teste básico (modo aberto)

Mostra todos os IP's:

```bash
ip addr | grep 'inet ' | awk '{print $2}' | cut -d/ -f1
```

Escutar no localhost ou IP específico:

```bash
mosquitto_sub -h localhost -t "escola/sala1/temperatura"
```

## 4. Habilitar autenticação por usuário e senha

### 4.1 Desabilitar conexões anônimas
Crie (ou edite) um arquivo de configuração em /etc/mosquitto/conf.d/auth.conf:

```bash
allow_anonymous false
password_file /etc/mosquitto/passwd
listener 1883
```

### 4.2 Criar arquivo de senhas
Adicione um usuário:

```bash
sudo mosquitto_passwd -c /etc/mosquitto/passwd meu_usuario
```

### 4.3 Reiniciar o serviço para aplicar mudanças

```bash
sudo systemctl restart mosquitto
```

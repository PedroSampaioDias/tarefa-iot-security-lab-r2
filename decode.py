import paho.mqtt.client as mqtt

# Função XOR para cifrar/decifrar
def xor_cipher(data: bytes, key: int) -> bytes:
    return bytes(b ^ key for b in data)

# Callback de conexão
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Conectado ao broker MQTT")
        client.subscribe("escola/sala1/temperatura")
    else:
        print(f"Falha na conexão: código {rc}")

# Callback de mensagem recebida
def on_message(client, userdata, msg):
    encrypted = msg.payload
    key = userdata['xor_key']
    decrypted = xor_cipher(encrypted, key)
    # Remove possíveis bytes de padding 0x14 no final
    trimmed = decrypted.rstrip(b'\x14')
    # Decodifica como ASCII
    plaintext = trimmed.decode('ascii', errors='ignore')
    print(f"[{msg.topic}] Criptografado: {encrypted!r}")
    print(f"[{msg.topic}] Decifrado:    {plaintext!r}\n")

if __name__ == "__main__":
    BROKER   = "localhost"
    PORT     = 1883
    USER     = "aluno"
    PASSWORD = "senha123"
    XOR_KEY  = 0x2A  # mesma chave usada em C

    client = mqtt.Client(userdata={'xor_key': XOR_KEY})
    client.username_pw_set(USER, PASSWORD)
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(BROKER, PORT, keepalive=60)
    client.loop_forever()

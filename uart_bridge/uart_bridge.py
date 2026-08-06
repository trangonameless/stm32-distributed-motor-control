import serial
import json
import paho.mqtt.client as mqtt

# UART

ser = serial.Serial('/dev/serial0', 115200, timeout=1)

# MQTT callback

def on_connect(client, userdata, flags, rc):
    print("MQTT connected")

    # recive commands for motor
    client.subscribe("motor/cmd")


def on_message(client, userdata, msg):

    try:
        data = json.loads(msg.payload.decode())

        rpm = int(data["rpm"])
        direction = data.get("direction", "P")

        command = f"{direction}{rpm}\r"

        print("UART TX:", command.strip())

        ser.write(command.encode())

    except Exception as e:
        print("Command error", e)



mqttc = mqtt.Client(
    client_id="uart_bridge",
    callback_api_version=mqtt.CallbackAPIVersion.VERSION1
)

mqttc.on_connect = on_connect
mqttc.on_message = on_message

mqttc.connect("localhost", 1883)

mqttc.loop_start()

print("UART ↔ MQTT bridge started")



while True:

    try:

        line = ser.readline().decode(
            'utf-8',
            errors='ignore'
        ).strip()

        if line:

            data = json.loads(line)

            mqttc.publish(
                topic="motor/status",
                payload=json.dumps(data),
                qos=0,
                retain=True
            )

    except json.JSONDecodeError:

        print("Wrong JSON:", line)

    except KeyboardInterrupt:

        break

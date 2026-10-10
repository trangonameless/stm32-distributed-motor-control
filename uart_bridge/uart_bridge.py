import json
import threading
import time

import serial
import paho.mqtt.client as mqtt

# ---- Configuration ----
SERIAL_PORT = "/dev/serial0"
BAUDRATE = 115200

# STM32 declares a communication fault after COMM_TIMEOUT_MS = 1000 ms
# without any received line, so 250 ms tolerates ~3 lost heartbeats.
HEARTBEAT_PERIOD_S = 0.25

MOTOR_MAX_RPM = 240  # motor rated max (datasheet); STM32 clamps as well

# ---- UART ----
ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
ser_lock = threading.Lock()  # MQTT callback thread and heartbeat thread both write
mqtt_connected = threading.Event()


def uart_send(command):
    """Thread-safe write of a single command line to the STM32."""
    with ser_lock:
        ser.write((command + "\r").encode())


def heartbeat_loop():
    """
    Sends 'H' to the STM32 periodically. The STM32 treats any received line
    as proof that the supervisor is alive; if the lines stop (script crash,
    unplugged cable, hung process) it stops the motor on its own.

    The heartbeat is only sent while connected to the MQTT broker: if the
    operator can no longer reach the motor (Grafana/MQTT side), the motor
    is allowed to time out and stop instead of running unattended.
    """
    while True:
        if mqtt_connected.is_set():
            try:
                uart_send("H")
            except serial.SerialException as e:
                print("Heartbeat error:", e)
        time.sleep(HEARTBEAT_PERIOD_S)


# ---- MQTT callbacks ----
def on_connect(client, userdata, flags, rc):
    print("MQTT connected")
    mqtt_connected.set()

    # receive commands for motor
    client.subscribe("motor/cmd")


def on_disconnect(client, userdata, rc):
    print("MQTT disconnected")
    mqtt_connected.clear()


def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())

        if "command" in data:
            command = str(data["command"]).strip()

        else:
            direction = str(data.get("direction", "P")).upper()
            if direction not in ("L", "P"):
                raise ValueError(f"invalid direction: {direction}")

            rpm = max(0, min(int(data["rpm"]), MOTOR_MAX_RPM))

        print("UART TX:", command)
        uart_send(command)
mqttc.on_message = on_message

mqttc.connect("localhost", 1883)
mqttc.loop_start()
threading.Thread(target=heartbeat_loop, daemon=True).start()

print("UART <-> MQTT bridge started")

try:
    while True:
        line = ser.readline().decode("utf-8", errors="ignore").strip()

        if line:
                    topic="motor/status",
                    payload=json.dumps(data),
                    qos=0,
                    retain=True
                )

            except json.JSONDecodeError:
                print("Wrong JSON:", repr(line))

except KeyboardInterrupt:
    pass

finally:
    # Clean shutdown: stop the motor instead of waiting for the timeout.
    try:
        uart_send("S")
    except Exception:
        pass

    mqttc.loop_stop()
    ser.close()            try:
                data = json.loads(line)
                mqttc.publish(

# daemon=True: the heartbeat dies together with the process, so a crashed
# bridge can never keep the motor alive.

mqttc.on_connect = on_connect
mqttc.on_disconnect = on_disconnect

    except Exception as e:
)
mqttc = mqtt.Client(
    client_id="uart_bridge",
    callback_api_version=mqtt.CallbackAPIVersion.VERSION1
        print("Command error:", e)



